#pragma once

#include <cmath>

#include "xrMath_constants.h"
#include "xrMath_bitwise.h"

// comparisions
inline bool fsimilar(float a, float b, float cmp = EPS)
{
	return std::abs(a - b) < cmp;
}
inline bool dsimilar(double a, double b, double cmp = EPS)
{
	return std::abs(a - b) < cmp;
}

inline bool fis_zero(float val, float cmp = EPS_S)
{
	return std::abs(val) < cmp;
}
inline bool dis_zero(double val, double cmp = EPS_S)
{
	return std::abs(val) < cmp;
}

// clamping/snapping
template <class T> inline void clamp(T& val, const T& _low, const T& _high)
{
	if (val < _low)
		val = _low;
	else if (val > _high)
		val = _high;
};

template <class T> inline T clampr(const T& val, const T& _low, const T& _high)
{
	if (val < _low)
		return _low;
	else if (val > _high)
		return _high;
	else
		return val;
};

inline float snapto(float value, float snap)
{
	if (snap <= 0.f)
		return value;
	return float(iFloor((value + (snap * 0.5f)) / snap)) * snap;
};

// generic
template <class T> IC T _min(T a, T b)
{
	return a < b ? a : b;
}
template <class T> IC T _max(T a, T b)
{
	return a > b ? a : b;
}
template <class T> IC T _sqr(T a)
{
	return a * a;
}

// float
IC float _abs(float x)
{
	return fabsf(x);
}
IC float _sqrt(float x)
{
	return sqrtf(x);
}
IC float _sin(float x)
{
	return sinf(x);
}
IC float _cos(float x)
{
	return cosf(x);
}
IC BOOL _valid(const float x)
{
	// check for: Signaling NaN, Quiet NaN, Negative infinity ( –INF), Positive infinity (+INF), Negative denormalized,
	// Positive denormalized
	int cls = _fpclass(double(x));

	if (cls & (_FPCLASS_SNAN + _FPCLASS_QNAN + _FPCLASS_NINF + _FPCLASS_PINF + _FPCLASS_ND + _FPCLASS_PD))
		return false;

	return true;
}

// double
IC double _abs(double x)
{
	return fabs(x);
}
IC double _sqrt(double x)
{
	return sqrt(x);
}
IC double _sin(double x)
{
	return sin(x);
}
IC double _cos(double x)
{
	return cos(x);
}
IC BOOL _valid(const double x)
{
	// check for: Signaling NaN, Quiet NaN, Negative infinity ( –INF), Positive infinity (+INF), Negative denormalized,
	// Positive denormalized
	int cls = _fpclass(x);

	if (cls & (_FPCLASS_SNAN + _FPCLASS_QNAN + _FPCLASS_NINF + _FPCLASS_PINF + _FPCLASS_ND + _FPCLASS_PD))
		return false;

	return true;
}

// int8
IC s8 _abs(s8 x)
{
	return (x >= 0) ? x : s8(-x);
}
IC s8 _min(s8 x, s8 y)
{
	return y + ((x - y) & ((x - y) >> (sizeof(s8) * 8 - 1)));
};
IC s8 _max(s8 x, s8 y)
{
	return x - ((x - y) & ((x - y) >> (sizeof(s8) * 8 - 1)));
};

// unsigned int8
IC u8 _abs(u8 x)
{
	return x;
}

// int16
IC s16 _abs(s16 x)
{
	return (x >= 0) ? x : s16(-x);
}
IC s16 _min(s16 x, s16 y)
{
	return y + ((x - y) & ((x - y) >> (sizeof(s16) * 8 - 1)));
};
IC s16 _max(s16 x, s16 y)
{
	return x - ((x - y) & ((x - y) >> (sizeof(s16) * 8 - 1)));
};

// unsigned int16
IC u16 _abs(u16 x)
{
	return x;
}

// int32
IC s32 _abs(s32 x)
{
	return (x >= 0) ? x : s32(-x);
}
IC s32 _min(s32 x, s32 y)
{
	return y + ((x - y) & ((x - y) >> (sizeof(s32) * 8 - 1)));
};
IC s32 _max(s32 x, s32 y)
{
	return x - ((x - y) & ((x - y) >> (sizeof(s32) * 8 - 1)));
};

// int64
IC s64 _abs(s64 x)
{
	return (x >= 0) ? x : s64(-x);
}
IC s64 _min(s64 x, s64 y)
{
	return y + ((x - y) & ((x - y) >> (sizeof(s64) * 8 - 1)));
};
IC s64 _max(s64 x, s64 y)
{
	return x - ((x - y) & ((x - y) >> (sizeof(s64) * 8 - 1)));
};
