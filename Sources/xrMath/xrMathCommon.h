#pragma once

#ifdef XRMATH_EXPORTS
#define XRMATH_API __declspec(dllexport)
#else
#define XRMATH_API __declspec(dllimport)
#endif

#define IC inline
#define ICF __forceinline

#define XRAY_BEGIN namespace xray {
#define XRAY_END }

#define NOMINMAX
