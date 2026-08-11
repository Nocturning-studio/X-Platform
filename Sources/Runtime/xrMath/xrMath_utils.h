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

// degree 2 radians and vice-versa
namespace implement
{
template <class T> inline T deg2rad(T val)
{
	return (val * T(::PI) / T(180));
};
template <class T> inline T rad2deg(T val)
{
	return (val * T(180) / T(::PI));
};
}; // namespace implement
inline float deg2rad(float val)
{
	return implement::deg2rad(val);
}
inline double deg2rad(double val)
{
	return implement::deg2rad(val);
}
inline float rad2deg(float val)
{
	return implement::rad2deg(val);
}
inline double rad2deg(double val)
{
	return implement::rad2deg(val);
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

// normalize angle (0..2PI)
inline float angle_normalize_always(float a)
{
	float div = a / PI_MUL_2;
	int rnd = (div > 0) ? iFloor(div) : iCeil(div);
	float frac = div - rnd;
	if (frac < 0)
		frac += 1.f;
	return frac * PI_MUL_2;
}

// normalize angle (0..2PI)
inline float angle_normalize(float a)
{
	if (a >= 0 && a <= PI_MUL_2)
		return a;
	else
		return angle_normalize_always(a);
}

// -PI .. +PI
inline float angle_normalize_signed(float a)
{
	if (a >= (-PI) && a <= PI)
		return a;
	float angle = angle_normalize_always(a);
	if (angle > PI)
		angle -= PI_MUL_2;
	return angle;
}

// -PI..PI
inline float angle_difference_signed(float a, float b)
{
	float diff = angle_normalize_signed(a) - angle_normalize_signed(b);
	if (diff > 0)
	{
		if (diff > PI)
			diff -= PI_MUL_2;
	}
	else
	{
		if (diff < -PI)
			diff += PI_MUL_2;
	}
	return diff;
}

// 0..PI
inline float angle_difference(float a, float b)
{
	return std::abs(angle_difference_signed(a, b));
}

// c=current, t=target, s=speed, dt=dt
inline bool angle_lerp(float& c, float t, float s, float dt)
{
	float diff = t - c;
	if (diff > 0)
	{
		if (diff > PI)
			diff -= PI_MUL_2;
	}
	else
	{
		if (diff < -PI)
			diff += PI_MUL_2;
	}
	float diff_a = std::abs(diff);

	if (diff_a < EPS_S)
		return true;

	float mot = s * dt;
	if (mot > diff_a)
		mot = diff_a;
	c += (diff / diff_a) * mot;

	if (c < 0)
		c += PI_MUL_2;
	else if (c > PI_MUL_2)
		c -= PI_MUL_2;

	return false;
}

// Just lerp :)	expects normalized angles in range [0..2PI)
inline float angle_lerp(float A, float B, float f)
{
	float diff = B - A;
	if (diff > PI)
		diff -= PI_MUL_2;
	else if (diff < -PI)
		diff += PI_MUL_2;

	return A + diff * f;
}

inline float angle_inertion(float src, float tgt, float speed, float clmp, float dt)
{
	float a = angle_normalize_signed(tgt);
	angle_lerp(src, a, speed, dt);
	src = angle_normalize_signed(src);
	float dH = angle_difference_signed(src, a);
	float dCH = clampr(dH, -clmp, clmp);
	src -= dH - dCH;
	return src;
}

inline float angle_inertion_var(float src, float tgt, float min_speed, float max_speed, float clmp, float dt)
{
	tgt = angle_normalize_signed(tgt);
	src = angle_normalize_signed(src);
	float speed = std::abs((max_speed - min_speed) * angle_difference(tgt, src) / clmp) + min_speed;
	angle_lerp(src, tgt, speed, dt);
	src = angle_normalize_signed(src);
	float dH = angle_difference_signed(src, tgt);
	float dCH = clampr(dH, -clmp, clmp);
	src -= dH - dCH;
	return src;
}

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
