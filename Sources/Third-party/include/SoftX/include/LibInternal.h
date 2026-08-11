/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#define SOFTX_API
#define SOFTX_BEGIN namespace SoftX { using namespace AfterMath;
#define SOFTX_END }

#if (defined(__cplusplus) && __cplusplus >= 202002L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#define SOFTX_CXX20_OR_LATER 1
#else
#define SOFTX_CXX20_OR_LATER 0
#endif

#if (defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define SOFTX_CXX17_OR_LATER 1
#else
#define SOFTX_CXX17_OR_LATER 0
#endif

#if SOFTX_CXX20_OR_LATER
#define SOFTX_LIKELY [[likely]]
#define SOFTX_UNLIKELY [[unlikely]]
#else
#define SOFTX_LIKELY
#define SOFTX_UNLIKELY
#endif

#if defined(_MSC_VER)
#define SOFTX_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define SOFTX_FORCE_INLINE __attribute__((always_inline)) inline
#else
#define SOFTX_FORCE_INLINE inline
#endif

#if defined(_MSC_VER)
#define SOFTX_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define SOFTX_NOINLINE __attribute__((noinline))
#else
#define SOFTX_NOINLINE
#endif

#define SOFTX_INLINE_HINT inline

#ifdef NDEBUG
#define SOFTX_VERIFY(x)
#else
#define SOFTX_VERIFY(x) assert(x)
#endif
/////////////////////////////////////////////////////////////////
