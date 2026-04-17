#pragma once
#include <cstdint>
#include <limits>

using s8  = int8_t;
using u8  = uint8_t;
using s16 = int16_t;
using u16 = uint16_t;
using s32 = int32_t;
using u32 = uint32_t;
using s64 = int64_t;
using u64 = uint64_t;

typedef char* pstr;
typedef const char* pcstr;
typedef char* const cpstr;
typedef const char* const cpcstr;

// windoze stuff
#ifndef BOOL
typedef pstr LPSTR;
typedef pcstr LPCSTR;
using BOOL = int;
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#endif

// Type limits
#define type_max(T) (std::numeric_limits<T>::max())
#define type_min(T) (-std::numeric_limits<T>::max())
#define type_zero(T) (std::numeric_limits<T>::min())
#define type_epsilon(T) (std::numeric_limits<T>::epsilon())

#define int_max type_max(int)
#define int_min type_min(int)
#define int_zero type_zero(int)

#undef FLT_MAX
#undef FLT_MIN

#define flt_max type_max(float)
#define flt_min type_min(float)
#define FLT_MAX flt_max
#define FLT_MIN flt_min

#define flt_zero type_zero(float)
#define flt_eps type_epsilon(float)

#define dbl_max type_max(double)
#define dbl_min type_min(double)
#define dbl_zero type_zero(double)
#define dbl_eps type_epsilon(double)

typedef char* string;
typedef char string16[16];
typedef char string32[32];
typedef char string64[64];
typedef char string128[128];
typedef char string256[256];
typedef char string512[512];
typedef char string1024[1024];
typedef char string2048[2048];
typedef char string4096[4096];

typedef char string_path[2 * _MAX_PATH];
