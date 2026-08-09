#include "pch.h"
#include "xrRHI_Debug.h"

RHI_BEGIN
XRRHI_API std::string WinErrorToString(long code)
{
	char buffer[1024];
	DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
	DWORD result = FormatMessageA(flags, nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer,
								  sizeof(buffer), nullptr);

	if (result == 0)
		return "Unknown error";

	while (result > 0 && (buffer[result - 1] == '\n' || buffer[result - 1] == '\r' || buffer[result - 1] == ' '))
	{
		buffer[--result] = '\0';
	}

	return std::string(buffer, result);
}

XRRHI_API void __cdecl Print(const char* format, ...)
{
	va_list mark;
	char buf[1024];
	va_start(mark, format);
	int sz = _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, format, mark);
	buf[sizeof(buf) - 1] = 0;
	va_end(mark);
	if (sz)
	{
		OutputDebugStringA(buf);
		printf("%s\n", buf);
	}
}
RHI_END
