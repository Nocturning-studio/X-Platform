/*
 * AfterMath — high‑performance C++ math library (HLSL‑style, SSE‑accelerated)
 *
 * Project:   Presence AfterMath
 * Copyright: 2026 Presence Collaboratory
 * Authors:   NSDeathman (Architecture & Core)
 *            DeepSeek (Mathematics & HLSL Integration)
 *            Gemini 3 (Optimization & Fast Math)
 *			  Nikolay Partas (Half precision data type prototype)
 * License:   MIT License with Attribution — see LICENSE.md for details.
 *
 * https://github.com/Presence-Collaboratory/AfterMath-CPP-Open-Math-Library
 */
#pragma once
#include <vcruntime.h>

#define AFTERMATH_WARNING_PUSH __pragma(warning(push))
#define AFTERMATH_WARNING_POP  __pragma(warning(pop))

#define AFTERMATH_DISABLE_WARNING_4201 __pragma(warning(disable : 4201))
#define AFTERMATH_DISABLE_WARNING_4324 __pragma(warning(disable : 4324))
#define AFTERMATH_DISABLE_WARNING_4820 __pragma(warning(disable : 4820))

#define AFTERMATH_DISABLE_DEFAULT_WARNINGS \
    AFTERMATH_DISABLE_WARNING_4201 \
    AFTERMATH_DISABLE_WARNING_4324 \
    AFTERMATH_DISABLE_WARNING_4820

#define AFTERMATH_BEGIN \
    namespace AfterMath { \
    AFTERMATH_WARNING_PUSH \
    AFTERMATH_DISABLE_DEFAULT_WARNINGS

#define AFTERMATH_END \
    AFTERMATH_WARNING_POP \
    }

#if (defined(__cplusplus) && __cplusplus >= 202002L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#define AFTERMATH_CXX20_OR_LATER 1
#else
#define AFTERMATH_CXX20_OR_LATER 0
#endif

#if (defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define AFTERMATH_CXX17_OR_LATER 1
#else
#define AFTERMATH_CXX17_OR_LATER 0
#endif

#if AFTERMATH_CXX17_OR_LATER
#define AFTERMATH_INLINE_VAR inline
#else
#define AFTERMATH_INLINE_VAR
#endif

#if AFTERMATH_CXX20_OR_LATER
#define AFTERMATH_CONSTEXPR20 constexpr
#else
#define AFTERMATH_CONSTEXPR20 inline
#endif
