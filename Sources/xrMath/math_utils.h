#pragma once

#include "xrMathCommon.h"
#include "math_constants.h"
#include "math_types.h"
#include "math_bitwise.h" // для iFloor, iCeil

XRAY_BEGIN

// -----------------------------------------------------------------------------
// СРАВНЕНИЯ С ЗАДАННОЙ ТОЧНОСТЬЮ
// -----------------------------------------------------------------------------
template <class T> IC bool similar(T a, T b, T eps = static_cast<T>(EPS))
{
	return std::abs(a - b) < eps;
}

IC bool fsimilar(float a, float b, float eps = EPS)
{
	return std::abs(a - b) < eps;
}
IC bool dsimilar(double a, double b, double eps = EPS)
{
	return std::abs(a - b) < eps;
}

template <class T> IC bool is_zero(T val, T eps = static_cast<T>(EPS_S))
{
	return std::abs(val) < eps;
}

IC bool fis_zero(float val, float eps = EPS_S)
{
	return std::abs(val) < eps;
}
IC bool dis_zero(double val, double eps = EPS_S)
{
	return std::abs(val) < eps;
}

// -----------------------------------------------------------------------------
// ПРЕОБРАЗОВАНИЕ ГРАДУСОВ <-> РАДИАНЫ
// -----------------------------------------------------------------------------
template <class T> IC T deg2rad(T deg)
{
	return deg * static_cast<T>(PI) / static_cast<T>(180);
}
template <class T> IC T rad2deg(T rad)
{
	return rad * static_cast<T>(180) / static_cast<T>(PI);
}

// Явные перегрузки для float/double (на всякий случай)
IC float deg2rad(float deg)
{
	return deg * PI / 180.0f;
}
IC double deg2rad(double deg)
{
	return deg * PI / 180.0;
}
IC float rad2deg(float rad)
{
	return rad * 180.0f / PI;
}
IC double rad2deg(double rad)
{
	return rad * 180.0 / PI;
}

// -----------------------------------------------------------------------------
// ОГРАНИЧЕНИЕ (CLAMP) И ОКРУГЛЕНИЕ К БЛИЖАЙШЕМУ ШАГУ (SNAP)
// -----------------------------------------------------------------------------
template <class T> IC void clamp(T& val, const T& low, const T& high)
{
	if (val < low)
		val = low;
	else if (val > high)
		val = high;
}

template <class T> IC T clampr(const T& val, const T& low, const T& high)
{
	if (val < low)
		return low;
	if (val > high)
		return high;
	return val;
}

IC float snapto(float value, float snap)
{
	if (snap <= 0.0f)
		return value;
	return float(iFloor((value + snap * 0.5f) / snap)) * snap;
}

// -----------------------------------------------------------------------------
// НОРМАЛИЗАЦИЯ УГЛОВ (В РАДИАНАХ)
// -----------------------------------------------------------------------------

// Приведение угла к диапазону [0, 2π) (всегда)
IC float angle_normalize_always(float a)
{
	float div = a / PI_MUL_2;
	int rnd = (div > 0) ? iFloor(div) : iCeil(div);
	float frac = div - static_cast<float>(rnd);
	if (frac < 0)
		frac += 1.0f;
	return frac * PI_MUL_2;
}

// Приведение угла к диапазону [0, 2π), если он уже там – без изменений
IC float angle_normalize(float a)
{
	if (a >= 0.0f && a <= PI_MUL_2)
		return a;
	return angle_normalize_always(a);
}

// Приведение угла к диапазону [-π, π]
IC float angle_normalize_signed(float a)
{
	if (a >= -PI && a <= PI)
		return a;
	float angle = angle_normalize_always(a);
	if (angle > PI)
		angle -= PI_MUL_2;
	return angle;
}

// Разность углов со знаком в диапазоне [-π, π]
IC float angle_difference_signed(float a, float b)
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

// Абсолютная разность углов в диапазоне [0, π]
IC float angle_difference(float a, float b)
{
	return std::abs(angle_difference_signed(a, b));
}

// Линейная интерполяция углов с учётом перехода через 2π (результат в [0, 2π))
IC float angle_lerp(float A, float B, float t)
{
	float diff = B - A;
	if (diff > PI)
		diff -= PI_MUL_2;
	else if (diff < -PI)
		diff += PI_MUL_2;
	return A + diff * t;
}

// Интерполяция углов с заданной скоростью (используется для плавного поворота)
IC bool angle_lerp(float& current, float target, float speed, float dt)
{
	float diff = target - current;
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
	float diff_abs = std::abs(diff);

	if (diff_abs < EPS_S)
		return true;

	float mot = speed * dt;
	if (mot > diff_abs)
		mot = diff_abs;
	current += (diff / diff_abs) * mot;

	// Нормализация результата
	if (current < 0)
		current += PI_MUL_2;
	else if (current > PI_MUL_2)
		current -= PI_MUL_2;

	return false;
}

// Функция инерции (плавное следование с ограничением максимального изменения)
IC float angle_inertion(float src, float tgt, float speed, float clamp_max, float dt)
{
	float a = angle_normalize_signed(tgt);
	angle_lerp(src, a, speed, dt);
	src = angle_normalize_signed(src);
	float dH = angle_difference_signed(src, a);
	float dCH = clampr(dH, -clamp_max, clamp_max);
	src -= dH - dCH;
	return src;
}

// Вариант с переменной скоростью (зависит от величины рассогласования)
IC float angle_inertion_var(float src, float tgt, float min_speed, float max_speed, float clamp_max, float dt)
{
	tgt = angle_normalize_signed(tgt);
	src = angle_normalize_signed(src);
	float speed = std::abs((max_speed - min_speed) * angle_difference(tgt, src) / clamp_max) + min_speed;
	angle_lerp(src, tgt, speed, dt);
	src = angle_normalize_signed(src);
	float dH = angle_difference_signed(src, tgt);
	float dCH = clampr(dH, -clamp_max, clamp_max);
	src -= dH - dCH;
	return src;
}

XRAY_END
