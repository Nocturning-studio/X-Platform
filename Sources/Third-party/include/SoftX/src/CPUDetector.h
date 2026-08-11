/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once

#include "../include/LibInternal.h"
#include <cstdint>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

struct SOFTX_API CPUCapabilities 
{
    bool sse   : 1;   // SSE
    bool sse2  : 1;   // SSE2
    bool sse3  : 1;   // SSE3
    bool ssse3 : 1;   // SSSE3
    bool sse41 : 1;   // SSE4.1
    bool sse42 : 1;   // SSE4.2
    bool avx   : 1;   // AVX
    bool avx2  : 1;   // AVX2
    bool fma   : 1;   // FMA
};

class SOFTX_API CPUDetector 
{
public:
    static const CPUCapabilities& GetCapabilities();

private:
    static void Initialize();
    static CPUCapabilities s_caps;
    static bool s_initialized;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
