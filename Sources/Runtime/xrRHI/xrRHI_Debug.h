#pragma once

#include "xrRHI_Internal.h"

RHI_BEGIN

XRRHI_API const char* WinErrorToString(long code);
XRRHI_API void __cdecl Print(const char* format, ...);

RHI_END
