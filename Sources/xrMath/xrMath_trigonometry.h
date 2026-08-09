#pragma once
#include "xrMath_constants.h"
#include "xrMath_utils.h"

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

