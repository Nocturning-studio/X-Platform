#pragma once

#include <math.h>
#include <cmath>
#include <algorithm>

#ifdef _MSC_VER
#define MSVC_COMPILER
#endif

#ifdef XRMATH_EXPORTS
#define XRMATH_API __declspec(dllexport)
#else
#define XRMATH_API __declspec(dllimport)
#endif

#ifndef IC
#define IC inline
#endif

#ifndef ICF
#define ICF __forceinline
#endif

#ifndef ICN
#define ICN __declspec(noinline)
#endif

#define NOMINMAX