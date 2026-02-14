#pragma once

#include "xrMathCommon.h"
#include "math_types.h"
#include "math_constants.h"
#include <cmath>
#include <cstdint>

XRAY_BEGIN

// -----------------------------------------------------------------------------
// БИТОВЫЕ МАСКИ ДЛЯ FLOAT (для x86/x64 совместимости)
// -----------------------------------------------------------------------------
#define fdSGN 0x080000000	// маска знакового бита
#define fdMABS 0x07FFFFFFF	// маска для абсолютного значения (~sgn)
#define fdMANT 0x0007FFFFF	// маска мантиссы
#define fdEXPO 0x07F800000	// маска экспоненты
#define fdONE 0x03F800000	// 1.0f
#define fdHALF 0x03F000000	// 0.5f
#define fdTWO 0x040000000	// 2.0
#define fdOOB 0x000000000	// "out of bounds"
#define fdNAN 0x07fffffff	// "Not a number"
#define fdMAX 0x07F7FFFFF	// FLT_MAX
#define fdRLE10 0x03ede5bdb // 1/ln10

// -----------------------------------------------------------------------------
// БЫСТРЫЕ ПРОВЕРКИ ЗНАКА ЧИСЛА (с оптимизацией для разных архитектур)
// -----------------------------------------------------------------------------
#ifdef _M_AMD64
// На AMD64 используем стандартные сравнения (компилятор генерирует эффективный код)
IC bool negative(float f)
{
	return f < 0;
}
IC bool positive(float f)
{
	return f >= 0;
}
IC void set_negative(float& f)
{
	f = -std::abs(f);
}
IC void set_positive(float& f)
{
	f = std::abs(f);
}
#else
// Для x86 используем битовые трюки (быстрее)
IC bool negative(const float& f)
{
	return (*(const u32*)&f) & fdSGN;
}
IC bool positive(const float& f)
{
	return ((*(const u32*)&f) & fdSGN) == 0;
}
IC void set_negative(float& f)
{
	(*(u32*)&f) |= fdSGN;
}
IC void set_positive(float& f)
{
	(*(u32*)&f) &= ~fdSGN;
}
#endif

// -----------------------------------------------------------------------------
// БИТОВЫЕ ТРЮКИ ДЛЯ ЦЕЛЫХ ЧИСЕЛ
// -----------------------------------------------------------------------------

// Маска младшего установленного бита
IC u32 btwLowestBitMask(s32 v)
{
	return u32(v & -v);
}
IC u32 btwLowestBitMask(u32 x)
{
	return x & ~(x - 1);
}

// Проверка, является ли число степенью двойки
IC bool btwIsPow2(s32 v)
{
	return btwLowestBitMask(v) == u32(v);
}
IC bool btwIsPow2(u32 v)
{
	return btwLowestBitMask(v) == v;
}

// Округление вверх до ближайшей степени двойки
IC u32 btwPow2_Ceil(s32 v)
{
	u32 i = btwLowestBitMask(v);
	while (i < u32(v))
		i <<= 1;
	return i;
}
IC u32 btwPow2_Ceil(u32 v)
{
	u32 i = btwLowestBitMask(v);
	while (i < v)
		i <<= 1;
	return i;
}

// -----------------------------------------------------------------------------
// ПОДСЧЁТ КОЛИЧЕСТВА УСТАНОВЛЕННЫХ БИТОВ (population count)
// -----------------------------------------------------------------------------
IC u8 btwCount1(u8 v)
{
	v = (v & 0x55) + ((v >> 1) & 0x55);
	v = (v & 0x33) + ((v >> 2) & 0x33);
	return (v & 0x0f) + ((v >> 4) & 0x0f);
}

IC u32 btwCount1(u32 v)
{
	const u32 g31 = 0x49249249ul; // 0100 1001 0010 0100 1001 0010 0100 1001
	const u32 g32 = 0x381c0e07ul; // 0011 1000 0001 1100 0000 1110 0000 0111
	v = (v & g31) + ((v >> 1) & g31) + ((v >> 2) & g31);
	v = ((v + (v >> 3)) & g32) + ((v >> 6) & g32);
	return (v + (v >> 9) + (v >> 18) + (v >> 27)) & 0x3f;
}

IC u64 btwCount1(u64 v)
{
	return btwCount1(u32(v & 0xFFFFFFFF)) + btwCount1(u32(v >> 32));
}

// -----------------------------------------------------------------------------
// БЫСТРОЕ ЦЕЛОЧИСЛЕННОЕ ОКРУГЛЕНИЕ (используем стандартные функции)
// -----------------------------------------------------------------------------
ICF int iFloor(float x)
{
	return int(std::floor(x));
}
ICF int iFloor(double x)
{
	return int(std::floor(x));
}
ICF int iCeil(float x)
{
	return int(std::ceil(x));
}
ICF int iCeil(double x)
{
	return int(std::ceil(x));
}

// -----------------------------------------------------------------------------
// ПРОВЕРКА ЧИСЕЛ НА «СТРАННЫЕ» ЗНАЧЕНИЯ (денормализованные, NaN, Inf)
// -----------------------------------------------------------------------------
IC bool fis_gremlin(const float& f)
{
	// Проверка на экспоненту вне нормального диапазона
	u8 value = u8(((*(const u32*)&f & 0x7f800000) >> 23) - 0x20);
	return value > 0xc0;
}

IC bool fis_gremlin(const double& f)
{
	// ВНИМАНИЕ: Для double используется приведение к u32*, что НЕКОРРЕКТНО.
	// Здесь оставлено для совместимости, но лучше не использовать.
	u8 value = u8(((*(const u32*)&f & 0x7f800000) >> 23) - 0x20);
	return value > 0xc0;
}

IC bool fis_denormal(const float& f)
{
	return (*(const u32*)&f & 0x7f800000) == 0;
}

IC bool fis_denormal(const double& f)
{
	// Аналогично, некорректно для double
	return (*(const u32*)&f & 0x7f800000) == 0;
}

// -----------------------------------------------------------------------------
// АППРОКСИМАЦИИ (быстрые, но менее точные)
// -----------------------------------------------------------------------------

// Быстрый обратный квадратный корень (известный трюк Quake)
IC float apx_InvSqrt(float n)
{
	s32 tmp = (s32(0xBE800000) - *(s32*)&n) >> 1;
	float y = *(float*)&tmp;
	return y * (1.47f - 0.47f * n * y * y);
}

IC double apx_InvSqrt(double n)
{
	// Для double используем ту же формулу, но с double.
	// ВНИМАНИЕ: Это приближение не точно для double, но оставлено для совместимости.
	s64 tmp = (s64(0xBE800000) - *(s64*)&n) >> 1;
	double y = *(double*)&tmp;
	return y * (1.47 - 0.47 * n * y * y);
}

// Аппроксимация asin для аргументов [0..1]
IC float apx_asin(float x)
{
	const float c1 = 0.892399f;
	const float c3 = 1.693204f;
	const float c5 = -3.853735f;
	const float c7 = 2.838933f;
	float x2 = x * x;
	return x * (c1 + x2 * (c3 + x2 * (c5 + x2 * c7)));
}

IC double apx_asin(double x)
{
	const double c1 = 0.892399;
	const double c3 = 1.693204;
	const double c5 = -3.853735;
	const double c7 = 2.838933;
	double x2 = x * x;
	return x * (c1 + x2 * (c3 + x2 * (c5 + x2 * c7)));
}

// Аппроксимация acos через asin
IC float apx_acos(float x)
{
	return float(PI_DIV_2) - apx_asin(x);
}
IC double apx_acos(double x)
{
	return double(PI_DIV_2) - apx_asin(x);
}

XRAY_END
