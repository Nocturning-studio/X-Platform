/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
// Math lib
#include "../../AfterMath/include/AfterMath.h"
/////////////////////////////////////////////////////////////////
#define ENABLE_PROFILER

// Your custom profiler
#ifdef ENABLE_PROFILER
#define USE_OPTICK
#include "../../Optick/Include/optick.h"
#define PROFILE_FRAME(x) OPTICK_FRAME(x)
#define PROFILE_SCOPE(x) OPTICK_EVENT(x)
#define PROFILE_THREAD(x) OPTICK_THREAD(x)
#define PROFILE_TAG(NAME, STATE) OPTICK_TAG(NAME, STATE)
#else
#define PROFILE_FRAME(x)
#define PROFILE_SCOPE(x)
#define PROFILE_THREAD(x)
#define PROFILE_TAG(NAME, STATE)
#endif
/////////////////////////////////////////////////////////////////
