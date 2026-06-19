#pragma once
#include <vcruntime.h>

#define AFTERMATH_BEGIN namespace AfterMath {
#define AFTERMATH_END }

#if _HAS_CXX17
#define AFTERMATH_INLINE_VAR inline
#else
#define AFTERMATH_INLINE_VAR
#endif

#if _HAS_CXX20
#define AFTERMATH_CONSTEXPR20 constexpr
#else
#define AFTERMATH_CONSTEXPR20 inline
#endif

#ifndef AFTERMATH_SHOW_ALL_WARNINGS
#define AFTERMATH_DISABLED_WARNING_4324 4324
#define AFTERMATH_DISABLED_WARNING_4201 4201
#else
#define AFTERMATH_DISABLED_WARNING_4324
#define AFTERMATH_DISABLED_WARNING_4201
#endif

#ifndef AFTERMATH_DISABLED_WARNINGS
#define AFTERMATH_DISABLED_WARNINGS                                                                                   \
    AFTERMATH_DISABLED_WARNING_4324 AFTERMATH_DISABLED_WARNING_4201
#endif
