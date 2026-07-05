#include "stdafx.h"
#include "xrDebugSupport.h"
#include <tlhelp32.h>
#include <algorithm>

#pragma comment(lib, "dbghelp.lib")

// ============================================================
// Реализация DebugSupport
// ============================================================

DebugSupport& DebugSupport::Instance()
{
    static DebugSupport s_instance;
    return s_instance;
}

bool DebugSupport::Initialize(HANDLE hProcess)
{
    if (m_bInitialized)
        return true;

    m_hProcess = hProcess;

    // Настройка опций символьного движка
    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

    if (!SymInitialize(hProcess, nullptr, TRUE))
        return false;

    m_bInitialized = true;
    InitializeCriticalSection(&m_cs);
    return true;
}

void DebugSupport::Cleanup()
{
    if (m_bInitialized)
    {
        EnterCriticalSection(&m_cs);
        SymCleanup(m_hProcess);
        m_bInitialized = false;
        LeaveCriticalSection(&m_cs);
        DeleteCriticalSection(&m_cs);
    }
}

std::vector<std::string> DebugSupport::GetStackTrace()
{
    CONTEXT ctx;
    RtlCaptureContext(&ctx);
    return GetStackTrace(&ctx, GetCurrentThread());
}

std::vector<std::string> DebugSupport::GetStackTrace(CONTEXT* context, HANDLE hThread)
{
    std::vector<std::string> result;
    if (!Initialize() || !context)
        return result;

    EnterCriticalSection(&m_cs);

    STACKFRAME64 stackFrame = {};
    DWORD machineType;

#ifdef _M_AMD64
    machineType = IMAGE_FILE_MACHINE_AMD64;
    stackFrame.AddrPC.Offset = context->Rip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context->Rsp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context->Rbp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
#elif defined(_M_IX86)
    machineType = IMAGE_FILE_MACHINE_I386;
    stackFrame.AddrPC.Offset = context->Eip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context->Esp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context->Ebp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
#else
    LeaveCriticalSection(&m_cs);
    return result; // Неподдерживаемая архитектура
#endif

    // Идём по стеку
    while (StackWalk64(machineType, m_hProcess, hThread, &stackFrame, context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
    {
        if (stackFrame.AddrPC.Offset == 0)
            break;

        result.push_back(FormatFrame(stackFrame.AddrPC.Offset));
    }

    LeaveCriticalSection(&m_cs);
    return result;
}

std::vector<std::string> DebugSupport::GetStackTrace(EXCEPTION_POINTERS* pExceptionInfo)
{
    if (!pExceptionInfo || !pExceptionInfo->ContextRecord)
        return {};
    return GetStackTrace(pExceptionInfo->ContextRecord, GetCurrentThread());
}

std::string DebugSupport::FormatFrame(DWORD64 address)
{
    char buffer[2048] = {};
    int len = 0;

    // 1. Адрес
    len += sprintf_s(buffer + len, sizeof(buffer) - len, "0x%p", (void*)address);

    // 2. Имя символа
    char symbolInfoBuf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)] = {};
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)symbolInfoBuf;
    pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymbol->MaxNameLen = MAX_SYM_NAME;

    DWORD64 displacement64 = 0;
    if (SymFromAddr(m_hProcess, address, &displacement64, pSymbol))
    {
        len += sprintf_s(buffer + len, sizeof(buffer) - len, " %s", pSymbol->Name);
        if (displacement64)
            len += sprintf_s(buffer + len, sizeof(buffer) - len, "+0x%I64X", displacement64);
    }
    else
    {
        len += sprintf_s(buffer + len, sizeof(buffer) - len, " <unknown_symbol>");
    }

    // 3. Информация о модуле (имя)
    IMAGEHLP_MODULE64 modInfo = { sizeof(IMAGEHLP_MODULE64) };
    if (SymGetModuleInfo64(m_hProcess, address, &modInfo))
    {
        const char* modName = modInfo.ModuleName;
        len += sprintf_s(buffer + len, sizeof(buffer) - len, " [%s]", modName);
    }

    // 4. Исходный файл и строка
    IMAGEHLP_LINE64 lineInfo = { sizeof(IMAGEHLP_LINE64) };
    DWORD lineDisplacement = 0;
    if (SymGetLineFromAddr64(m_hProcess, address, &lineDisplacement, &lineInfo))
    {
        len += sprintf_s(buffer + len, sizeof(buffer) - len, " (%s:%d", lineInfo.FileName, lineInfo.LineNumber);
        if (lineDisplacement)
            len += sprintf_s(buffer + len, sizeof(buffer) - len, "+0x%X", lineDisplacement);
        len += sprintf_s(buffer + len, sizeof(buffer) - len, ")");
    }

    return buffer;
}

// ============================================================
// Вспомогательные функции для модулей
// ============================================================

bool GetProcessModules(DWORD processId, std::vector<HMODULE>& outModules)
{
    outModules.clear();
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return false;

    MODULEENTRY32 me = { sizeof(MODULEENTRY32) };
    if (Module32First(hSnapshot, &me))
    {
        do
        {
            outModules.push_back(me.hModule);
        } while (Module32Next(hSnapshot, &me));
    }

    CloseHandle(hSnapshot);
    return true;
}

BOOL GetLoadedModules(DWORD dwPID, UINT uiCount, HMODULE* paModArray, LPDWORD pdwRealCount)
{
    if (!pdwRealCount)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    std::vector<HMODULE> modules;
    if (!GetProcessModules(dwPID, modules))
        return FALSE;

    *pdwRealCount = static_cast<DWORD>(modules.size());

    if (uiCount == 0 || paModArray == nullptr)
        return TRUE;

    if (uiCount < modules.size())
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    std::copy(modules.begin(), modules.end(), paModArray);
    return TRUE;
}
