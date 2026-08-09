#pragma once
#include <limits>

#define type_max(T)         (std::numeric_limits<T>::max())
#define type_lowest(T)      (std::numeric_limits<T>::lowest())
#define type_min(T)         (std::numeric_limits<T>::min())
#define type_epsilon(T)     (std::numeric_limits<T>::epsilon())
#define type_zero(T)		(std::numeric_limits<T>::min())

#define int_max             type_max(int)
#define int_lowest          type_lowest(int)
#define int_min				type_min(int)
#define int_zero			type_zero(int)

#define flt_max             type_max(float)
#define flt_lowest          type_lowest(float)
#define flt_min				type_min(float)
#define flt_eps             type_epsilon(float)
#define flt_zero			type_zero(float)

#define dbl_max             type_max(double)
#define dbl_lowest          type_lowest(double)
#define dbl_min				type_min(double)
#define dbl_eps             type_epsilon(double)
#define dbl_zero			type_zero(double)

#undef  FLT_MAX
#define FLT_MAX             flt_max
#undef  FLT_MIN
#define FLT_MIN             flt_lowest
