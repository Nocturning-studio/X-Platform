#include "pch.h"
#pragma hdrstop

#include "xrDebug.h"
#include <dbghelp.h>
#include <signal.h>
#include <new.h>
#include <malloc.h>
#include <direct.h>
#include <stdio.h>
#include <Psapi.h>
#include <atomic>

extern bool shared_str_initialized;

XRCORE_API xrDebug Debug;

static bool error_after_dialog = false;
static HANDLE g_oom_log_handle = INVALID_HANDLE_VALUE;
static std::atomic<bool> g_oom_entered{ false };


// ---------------------------------------------------------------------------
// Вспомогательные функции
// ---------------------------------------------------------------------------

// Копирование текста в буфер обмена
static void CopyToClipboard(const char* text)
{
	if(OpenClipboard(nullptr))
	{
		EmptyClipboard();
		size_t len = strlen(text) + 1;
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
		if(hMem)
		{
			memcpy(GlobalLock(hMem), text, len);
			GlobalUnlock(hMem);
			SetClipboardData(CF_TEXT, hMem);
		}
		CloseClipboard();
	}
}

// Форматирование стека вызовов с помощью DebugSupport
static std::string GetStackTraceString(EXCEPTION_POINTERS* pExceptionInfo = nullptr)
{
	auto& dbg = DebugSupport::Instance();
	std::vector<std::string> frames = pExceptionInfo
										  ? dbg.GetStackTrace(pExceptionInfo)
										  : dbg.GetStackTrace();

	std::string result;
	for(const auto& f : frames)
		result += f + "\r\n";
	return result;
}

// Запись стека в лог (без EXCEPTION_POINTERS, для текущего потока)
void LogStackTrace(LPCSTR header)
{
	if(!shared_str_initialized)
		return;
	auto& dbg = DebugSupport::Instance();
	auto frames = dbg.GetStackTrace();
	Msg("%s", header);
	for(const auto& f : frames)
		Msg("%s", f.c_str());
}

// Сбор строки с информацией об ошибке (включая стек, если нужно)
static void gather_info(const char* expression, const char* description,
						const char* argument0, const char* argument1,
						const char* file, int line, const char* function,
						bool includeStack, LPSTR outBuffer, size_t bufferSize)
{
	LPSTR buf = outBuffer;
	size_t remain = bufferSize;
	auto append = [&](const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		int n = _vsnprintf(buf, remain, fmt, args);
		va_end(args);
		if(n > 0)
		{
			buf += n;
			remain -= n;
		}
	};

	LPCSTR endline = "\n";
	LPCSTR prefix = "[error]";
	bool extended = (description && !argument0 && strchr(description, '\n'));

	for(int pass = 0; pass < 2; ++pass)
	{
		if(pass == 0)
			append("%sFATAL ERROR%s%s", endline, endline, endline);
		append("%sExpression    : %s%s", prefix, expression, endline);
		append("%sFunction      : %s%s", prefix, function, endline);
		append("%sFile          : %s%s", prefix, file, endline);
		append("%sLine          : %d%s", prefix, line, endline);

		if(extended)
		{
			append("%s%s%s", endline, description, endline);
			if(argument0)
			{
				append("%s%s", argument0, endline);
				if(argument1)
					append("%s%s", argument1, endline);
			}
		}
		else
		{
			append("%sDescription   : %s%s", prefix, description, endline);
			if(argument0)
			{
				if(argument1)
				{
					append("%sArgument 0    : %s%s", prefix, argument0, endline);
					append("%sArgument 1    : %s%s", prefix, argument1, endline);
				}
				else
					append("%sArguments     : %s%s", prefix, argument0, endline);
			}
		}
		append("%s", endline);

		if(pass == 0)
		{
			if(shared_str_initialized)
			{
				Msg("%s", outBuffer);
				FlushLog();
			}
			// переходим ко второму проходу для GUI (CR+LF, без префикса)
			buf = outBuffer;
			remain = bufferSize;
			endline = "\r\n";
			prefix = "";
		}
	}

	if(includeStack && !g_oom_entered.load())
	{
		if(!IsDebuggerPresent() && !strstr(GetCommandLine(), "-no_call_stack_assert"))
		{
			append("stack trace:%s%s", endline, endline);
			std::string stack = GetStackTraceString();
			append("%s", stack.c_str());

			if(shared_str_initialized)
			{
				Msg("stack trace:\n");
				auto lines = GetStackTraceString();
				// разбить на строки и вывести через Msg
				size_t pos = 0;
				while(pos < stack.size())
				{
					size_t nl = stack.find('\n', pos);
					if(nl == std::string::npos)
						nl = stack.size();
					std::string result_line = stack.substr(pos, nl - pos);
					if(!result_line.empty())
						Msg("%s", result_line.c_str());
					pos = nl + 1;
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Методы xrDebug
// ---------------------------------------------------------------------------

void xrDebug::backend(const char* expression, const char* description,
					  const char* argument0, const char* argument1,
					  const char* file, int line, const char* function,
					  bool& ignore_always)
{
	static xrCriticalSection CS
#ifdef PROFILE_CRITICAL_SECTIONS
		(MUTEX_PROFILE_ID(xrDebug::backend))
#endif
			;
	if (!CS.TryEnter())
	{
		// Другой поток уже показывает ошибку. Просто выходим.
		// Не блокируем систему.
		return;
	}
	error_after_dialog = true;

	string4096 assertion_info;
	gather_info(expression, description, argument0, argument1,
				file, line, function, true,
				assertion_info, sizeof(assertion_info));

	// Дописываем кнопки
	LPCSTR endline = "\r\n";
	size_t len = strlen(assertion_info);
	_snprintf(assertion_info + len, sizeof(assertion_info) - len,
			  "%sPress CANCEL to abort execution%s"
			  "Press TRY AGAIN to continue execution%s"
			  "Press CONTINUE to continue execution and ignore all the errors of this type%s%s",
			  endline, endline, endline, endline, endline);

	if(handler)
		handler();
	if(get_on_dialog())
		get_on_dialog()(true);

	HANDLE hWatchdog = CreateThread(nullptr, 0,
		[](LPVOID) -> DWORD {
			Sleep(60000);
			TerminateProcess(GetCurrentProcess(), 1);
			return 0;
		}, nullptr, 0, nullptr);
	if (hWatchdog) CloseHandle(hWatchdog);

	int result = MessageBox(nullptr, assertion_info, "Fatal Error", MB_CANCELTRYCONTINUE | MB_ICONERROR | MB_TOPMOST);

	switch(result)
	{
	case IDCANCEL:
		__debugbreak();
		break;
	case IDTRYAGAIN:
		error_after_dialog = false;
		break;
	case IDCONTINUE:
		error_after_dialog = false;
		ignore_always = true;
		break;
	default:
		__debugbreak();
	}

	if(get_on_dialog())
		get_on_dialog()(false);
	CS.Leave();
}

LPCSTR xrDebug::error2string(long code)
{
	static string1024 desc_storage;
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, code, 0, desc_storage, sizeof(desc_storage) - 1, 0);
	return desc_storage;
}

void xrDebug::error(long hr, const char* expr, const char* file, int line, const char* function, bool& ignore_always)
{
	backend(error2string(hr), expr, nullptr, nullptr, file, line, function, ignore_always);
}

void xrDebug::error(long hr, const char* expr, const char* e2, const char* file, int line, const char* function, bool& ignore_always)
{
	backend(error2string(hr), expr, e2, nullptr, file, line, function, ignore_always);
}

void xrDebug::fail(const char* e1, const char* file, int line, const char* function, bool& ignore_always)
{
	backend("assertion failed", e1, nullptr, nullptr, file, line, function, ignore_always);
}

void xrDebug::fail(const char* e1, const std::string& e2, const char* file, int line, const char* function, bool& ignore_always)
{
	backend(e1, e2.c_str(), nullptr, nullptr, file, line, function, ignore_always);
}

void xrDebug::fail(const char* e1, const char* e2, const char* file, int line, const char* function, bool& ignore_always)
{
	backend(e1, e2, nullptr, nullptr, file, line, function, ignore_always);
}

void xrDebug::fail(const char* e1, const char* e2, const char* e3, const char* file, int line, const char* function, bool& ignore_always)
{
	backend(e1, e2, e3, nullptr, file, line, function, ignore_always);
}

void xrDebug::fail(const char* e1, const char* e2, const char* e3, const char* e4, const char* file, int line, const char* function, bool& ignore_always)
{
	backend(e1, e2, e3, e4, file, line, function, ignore_always);
}

void __cdecl xrDebug::fatal(const char* file, int line, const char* function, const char* F, ...)
{
	string1024 buffer;
	va_list p;
	va_start(p, F);
	vsprintf(buffer, F, p);
	va_end(p);
	bool ignore_always = true;
	backend("fatal error", "<no expression>", buffer, nullptr, file, line, function, ignore_always);
}

void xrDebug::do_exit(const std::string& message)
{
	FlushLog();
	MessageBox(nullptr, message.c_str(), "Error", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
	TerminateProcess(GetCurrentProcess(), 1);
}

// ---------------------------------------------------------------------------
// Обработчики ошибок / исключений
// ---------------------------------------------------------------------------

static void format_message(LPSTR buffer, size_t size)
{
	DWORD code = GetLastError();
	if(!code)
	{
		buffer[0] = 0;
		return;
	}
	LPVOID msg;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				  nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				  (LPSTR)&msg, 0, nullptr);
	_snprintf(buffer, size, "[error][%8d]    : %s", code, (char*)msg);
	LocalFree(msg);
}

static void handler_base(LPCSTR reason)
{
	bool ignore_always = false;
	Debug.backend("error handler is invoked!", reason, nullptr, nullptr, DEBUG_INFO, ignore_always);
}

static void invalid_parameter_handler(const wchar_t* expression, const wchar_t* function, const wchar_t* file,
									  unsigned int line, uintptr_t)
{
	char expr[1024] = "", func[1024] = "", fil[1024] = "";
	if(expression)
		wcstombs_s(nullptr, expr, sizeof(expr), expression, _TRUNCATE);
	if(function)
		wcstombs_s(nullptr, func, sizeof(func), function, _TRUNCATE);
	if(file)
		wcstombs_s(nullptr, fil, sizeof(fil), file, _TRUNCATE);
	bool ignore_always = false;
	Debug.backend(expr[0] ? expr : "invalid parameter",
				  "error handler is invoked!", nullptr, nullptr,
				  fil[0] ? fil : __FILE__, line ? line : __LINE__,
				  func[0] ? func : __FUNCTION__, ignore_always);
}

static void pure_call_handler() { handler_base("pure virtual function call"); }

#ifdef CS_USE_EXCEPTIONS
static void unexpected_handler() { handler_base("unexpected program termination"); }
#endif

static void abort_handler(int) { handler_base("application is aborting"); }
static void floating_point_handler(int) { handler_base("floating point error"); }
static void illegal_instruction_handler(int) { handler_base("illegal instruction"); }
static void termination_handler(int) { handler_base("termination with exit code 3"); }

void _terminate()
{
	if(strstr(GetCommandLine(), "-silent_error_mode"))
		exit(-1);
	string4096 info;
	gather_info("<no expression>", "Unexpected application termination", nullptr, nullptr,
				__FILE__, __LINE__, __FUNCTION__, true, info, sizeof(info));
	strcat_s(info, "\r\nPress OK to abort execution\r\n");
	MessageBox(GetTopWindow(nullptr), info, "Fatal Error", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
	exit(-1);
}

void debug_on_thread_spawn()
{
	std::set_terminate(_terminate);
}

// ============================================================================
// Emergency OOM handler — allocation-free
// ============================================================================
void xrDebug::InitializeOOMEmergency()
{
    char path[MAX_PATH] = {};
    DWORD n = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
        return;

    // Заменяем .exe → .oom.log, или приписываем
    char* dot = strrchr(path, '.');
    if (dot && (dot - path) < (MAX_PATH - 9))
        strcpy_s(dot, MAX_PATH - (dot - path), ".oom.log");
    else
        strcat_s(path, ".oom.log");

    g_oom_log_handle = CreateFileA(
        path, GENERIC_WRITE, FILE_SHARE_READ,
        nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
}

static void OOM_WriteRaw(const char* s, size_t n)
{
    if (g_oom_log_handle == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    WriteFile(g_oom_log_handle, s, (DWORD)n, &written, nullptr);
}
static void OOM_WriteStr(const char* s) { OOM_WriteRaw(s, strlen(s)); }

static void OOM_WriteFmt(const char* fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    int n = _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
    va_end(args);
    if (n > 0) OOM_WriteRaw(buf, (size_t)n);
}

static void OOM_FlushAndClose()
{
    if (g_oom_log_handle != INVALID_HANDLE_VALUE)
    {
        FlushFileBuffers(g_oom_log_handle);
        CloseHandle(g_oom_log_handle);
        g_oom_log_handle = INVALID_HANDLE_VALUE;
    }
}

// Walk stack — никакой символизации, только raw return addresses.
// Symbolization offline (WinDbg + PDB).
static void OOM_DumpRawStack()
{
    void* frames[96];
    USHORT n = CaptureStackBackTrace(0, 96, frames, nullptr);
    for (USHORT i = 0; i < n; ++i)
        OOM_WriteFmt("  #%02u  0x%08p\n", i, frames[i]);
}

static void OOM_DumpMemoryInfo(size_t request)
{
    OOM_WriteFmt("Request:    %zu bytes (%zu KB)\n", request, request / 1024);

    MEMORYSTATUSEX msex = { sizeof(msex) };
    if (GlobalMemoryStatusEx(&msex))
    {
        OOM_WriteFmt("Phys total: %llu MB\n", msex.ullTotalPhys     / (1024ull * 1024));
        OOM_WriteFmt("Phys avail: %llu MB\n", msex.ullAvailPhys     / (1024ull * 1024));
        OOM_WriteFmt("Virt total: %llu MB\n", msex.ullTotalVirtual  / (1024ull * 1024));
        OOM_WriteFmt("Virt avail: %llu MB\n", msex.ullAvailVirtual  / (1024ull * 1024));
    }

    PROCESS_MEMORY_COUNTERS_EX pmc = { sizeof(pmc) };
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
    {
        OOM_WriteFmt("WorkingSet: %llu MB\n", pmc.WorkingSetSize     / (1024ull * 1024));
        OOM_WriteFmt("Peak WS:    %llu MB\n", pmc.PeakWorkingSetSize / (1024ull * 1024));
        OOM_WriteFmt("Pagefile:   %llu MB\n", pmc.PagefileUsage      / (1024ull * 1024));
    }

    OOM_WriteFmt("xrMemory: stat_calls=%u counter=%d\n",
                 Memory.stat_calls, Memory.stat_counter);
}

[[noreturn]] static void OOM_Fatal(size_t request)
{
    // Recursion guard — если уже в OOM, terminate NOW
    bool expected = false;
    if (!g_oom_entered.compare_exchange_strong(expected, true))
    {
        TerminateProcess(GetCurrentProcess(), 0xC0000017); // STATUS_NO_MEMORY
    }

    OOM_WriteStr("\n\n===========================================\n");
    OOM_WriteStr("         OUT OF MEMORY\n");
    OOM_WriteStr("===========================================\n");

    __try
    {
        OOM_DumpMemoryInfo(request);
        OOM_WriteStr("\nStack trace (raw addresses — symbolize offline):\n");
        OOM_DumpRawStack();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        OOM_WriteStr("(exception during OOM diagnostics)\n");
    }

    OOM_WriteStr("===========================================\n");
    OOM_FlushAndClose();

    const char* cmd = GetCommandLineA();
    const bool  debugger_attached = IsDebuggerPresent() != FALSE;
    const bool  show_dialog       = strstr(cmd, "-oom_show_dialog") != nullptr;

    // Если под отладчиком — даём шанс посмотреть
    if (debugger_attached)
    {
        __debugbreak();   // VS catches as exception
        // Если продолжили — умираем
        TerminateProcess(GetCurrentProcess(), 0xC0000017);
    }

    if (!show_dialog)
    {
        // По умолчанию — тихий выход. Лог лежит рядом с exe.
        TerminateProcess(GetCurrentProcess(), 0xC0000017);
    }

    // Показать диалог, но с watchdog
    HANDLE hWatchdog = CreateThread(nullptr, 0,
        [](LPVOID) -> DWORD
        {
            Sleep(15000);
            TerminateProcess(GetCurrentProcess(), 0xC0000017);
            return 0;
        }, nullptr, 0, nullptr);
    if (hWatchdog) CloseHandle(hWatchdog);

    MessageBoxA(nullptr,
        "Out of memory.\n\n"
        "Details written to <exe>.oom.log.\n\n"
        "Process will terminate automatically in 15 seconds.",
        "Fatal: Out Of Memory",
        MB_OK | MB_ICONERROR | MB_TOPMOST);

    TerminateProcess(GetCurrentProcess(), 0xC0000017);
}

// CRT new handler
int out_of_memory_handler(size_t size)
{
    OOM_Fatal(size);
}

// std::new_handler
static void std_out_of_memory_handler()
{
    OOM_Fatal(0);
}

// ---------------------------------------------------------------------------
// Необработанное исключение (использует DebugSupport)
// ---------------------------------------------------------------------------
static LPTOP_LEVEL_EXCEPTION_FILTER previous_filter = nullptr;

LONG WINAPI UnhandledFilter(_EXCEPTION_POINTERS* pExceptionInfo)
{
	if (g_oom_entered.load())
		TerminateProcess(GetCurrentProcess(), 0xC0000017);

	string256 errorMsg;
	format_message(errorMsg, sizeof(errorMsg));

	if(!error_after_dialog && !strstr(GetCommandLine(), "-no_call_stack_assert"))
	{
		auto& dbg = DebugSupport::Instance();
		auto frames = dbg.GetStackTrace(pExceptionInfo);
		std::string stackText;
		for(const auto& f : frames)
			stackText += f + "\r\n";

		if(shared_str_initialized)
		{
			Msg("stack trace:\n");
			for(const auto& f : frames)
				Msg("%s", f.c_str());
		}
		CopyToClipboard(("stack trace:\r\n\r\n" + stackText).c_str());
		if(*errorMsg)
		{
			if(shared_str_initialized)
				Msg("\n%s", errorMsg);
			CopyToClipboard((stackText + errorMsg + "\r\n").c_str());
		}
	}

	if(shared_str_initialized)
		FlushLog();

	if(!error_after_dialog)
	{
		if(Debug.get_on_dialog())
			Debug.get_on_dialog()(true);
		MessageBox(nullptr, "Fatal error occured\n\nPress OK to abort program execution",
				   "Fatal error", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
	}

	if(!previous_filter)
	{
		if(Debug.get_on_dialog())
			Debug.get_on_dialog()(false);
		return EXCEPTION_CONTINUE_SEARCH;
	}
	previous_filter(pExceptionInfo);
	if(Debug.get_on_dialog())
		Debug.get_on_dialog()(false);
	return EXCEPTION_CONTINUE_SEARCH;
}

// ---------------------------------------------------------------------------
// Инициализация / завершение
// ---------------------------------------------------------------------------
void xrDebug::Initialize(const bool& dedicated)
{
	DebugSupport::Instance().Initialize();

	InitializeOOMEmergency();

	debug_on_thread_spawn();
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
	signal(SIGABRT, abort_handler);
	signal(SIGABRT_COMPAT, abort_handler);
	signal(SIGFPE, floating_point_handler);
	signal(SIGILL, illegal_instruction_handler);
	signal(SIGINT, 0);
	signal(SIGTERM, termination_handler);
	_set_invalid_parameter_handler(&invalid_parameter_handler);
	_set_new_mode(1);
	_set_new_handler(&out_of_memory_handler);
	std::set_new_handler(&std_out_of_memory_handler);
	_set_purecall_handler(&pure_call_handler);
	previous_filter = ::SetUnhandledExceptionFilter(UnhandledFilter);
}

void xrDebug::Destroy()
{
	DebugSupport::Instance().Cleanup();
}
