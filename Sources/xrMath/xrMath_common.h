#pragma once
#include <cmath>
#include <algorithm>

#ifdef _MSC_VER
#   define MSVC_COMPILER
#endif

#ifndef IC
#   define IC inline
#endif
#ifndef ICF
#   define ICF __forceinline
#endif
#ifndef ICN
#   define ICN __declspec(noinline)
#endif

#define NOMINMAX
