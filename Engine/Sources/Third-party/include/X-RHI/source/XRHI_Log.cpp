////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-RHI
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include <include/XRHI_Log.h>
////////////////////////////////////////////////////////////////////////////////
namespace xRHI 
{
    namespace detail 
    {
        void DefaultVPrint(LogChannel ch, const char* fmt, va_list args)
        {
            char buf[2048];
            vsnprintf(buf, sizeof(buf), fmt, args);
            buf[sizeof(buf) - 1] = 0;

            const char* prefix = (ch == XRHI_LOG_ERROR) ? "[xrRHI][ERR] " : (ch == XRHI_LOG_WARNING) ? "[xrRHI][WRN] " : "[xrRHI]      ";

            FILE* f = (ch == XRHI_LOG_ERROR) ? stderr : stdout;
            fprintf(f, "%s%s\n", prefix, buf);
            fflush(f);

            OutputDebugStringA(prefix);
            OutputDebugStringA(buf);
            OutputDebugStringA("\n");
        }

        void DefaultPrint(LogChannel ch, const char* fmt, ...)
        {
            va_list args;
            va_start(args, fmt);
            DefaultVPrint(ch, fmt, args);
            va_end(args);
        }

        void DefaultAbort()
        {
#ifdef _DEBUG
            __debugbreak();
#endif
            std::abort();
        }
    }
} // namespace xRHI::detail
////////////////////////////////////////////////////////////////////////////////
