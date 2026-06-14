#pragma once

// Math lib
#include <AfterMath.h>
using namespace AfterMath;

//#define ENABLE_PROFILER

// Your custom profiler
#ifdef ENABLE_PROFILER
#include "../../../Optick/Include/optick.h"
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