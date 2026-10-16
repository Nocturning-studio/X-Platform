////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-RHI
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <windows.h>
////////////////////////////////////////////////////////////////////////////////
namespace xRHI
{
    namespace detail
    {
        enum LogChannel : int { XRHI_LOG_INFO = 0, XRHI_LOG_WARNING, XRHI_LOG_ERROR };

        XRHI_API void DefaultPrint(LogChannel ch, const char* fmt, ...);
        XRHI_API void DefaultVPrint(LogChannel ch, const char* fmt, va_list args);
        XRHI_API void DefaultAbort();
    } // namespace detail
} // namespace xRHI
////////////////////////////////////////////////////////////////////////////////
#ifndef XRHI_PRINT
#  define XRHI_PRINT(fmt, ...)                                                  \
        ::xRHI::detail::DefaultPrint(xRHI::detail::LogChannel::XRHI_LOG_INFO,   \
                                      fmt, ##__VA_ARGS__)
#endif

#ifndef XRHI_ERROR
#  define XRHI_ERROR(fmt, ...)                                                      \
        do {                                                                        \
            ::xRHI::detail::DefaultPrint(xRHI::detail::LogChannel::XRHI_LOG_ERROR,  \
                                          fmt, ##__VA_ARGS__);                      \
            ::xRHI::detail::DefaultAbort();                                         \
        } while (0)
#endif

////////////////////////////////////////////////////////////////////////////////
