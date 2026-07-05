#pragma once

#include <windows.h>
#include <dbghelp.h>
#include <string>
#include <vector>

#include "xrCoreCommon.h"

class XRCORE_API DebugSupport
{
public:
    static DebugSupport& Instance();

    bool Initialize(HANDLE hProcess = GetCurrentProcess());
    void Cleanup();
    bool IsInitialized() const { return m_bInitialized; }

    // Получение стека вызовов текущего потока
    std::vector<std::string> GetStackTrace();
    std::vector<std::string> GetStackTrace(CONTEXT* context, HANDLE hThread = GetCurrentThread());
    std::vector<std::string> GetStackTrace(EXCEPTION_POINTERS* pExceptionInfo);

    // Форматирование одного адреса в строку "адрес [модуль] символ + смещение (файл:строка)"
    std::string FormatFrame(DWORD64 address);

private:
    DebugSupport() = default;
    ~DebugSupport() { Cleanup(); }
    DebugSupport(const DebugSupport&) = delete;
    DebugSupport& operator=(const DebugSupport&) = delete;

    HANDLE m_hProcess = nullptr;
    bool m_bInitialized = false;
    CRITICAL_SECTION m_cs;   // защита при многопоточном использовании
};

// Вспомогательная функция для перечисления модулей (аналог удалённой GetLoadedModules)
XRCORE_API bool GetProcessModules(DWORD processId, std::vector<HMODULE>& outModules);
// Устаревшая сигнатура для совместимости, если где-то ещё вызывается
XRCORE_API BOOL GetLoadedModules(DWORD dwPID, UINT uiCount, HMODULE* paModArray, LPDWORD pdwRealCount);
