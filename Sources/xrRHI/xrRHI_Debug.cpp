#include "pch.h"
#include "xrRHI_Debug.h"

RHI_BEGIN
void __cdecl Print(const char* format, ...)
{
	va_list mark;
	string1024 buf;
	va_start(mark, format);
	int sz = _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, format, mark);
	buf[sizeof(buf) - 1] = 0;
	va_end(mark);
	if (sz)
		printf("%s\n", buf);
}
RHI_END
