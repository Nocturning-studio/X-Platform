#pragma once

#include "xrMathCommon.h"
#include "math_types.h"
#include "math_constants.h"
#include "math_utils.h"	  // clamp, similar, is_zero
#include "math_bitwise.h" // iFloor, iCeil
#include <cmath>
#include <cassert>
#include <algorithm>

XRAY_BEGIN

// -----------------------------------------------------------------------------
// Упаковка/распаковка цветов в 32-битное целое (формат A8R8G8B8)
// -----------------------------------------------------------------------------

// Сборка цвета из компонент (0..255)
ICF u32 color_argb(u32 a, u32 r, u32 g, u32 b)
{
	return ((a & 0xff) << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
}
ICF u32 color_rgba(u32 r, u32 g, u32 b, u32 a)
{
	return color_argb(a, r, g, b);
}
ICF u32 color_xrgb(u32 r, u32 g, u32 b)
{
	return color_argb(0xff, r, g, b);
}

// Сборка цвета из float-компонент (0..1)
ICF u32 color_argb_f(float a, float r, float g, float b)
{
	s32 _r = clampr(iFloor(r * 255.f), 0, 255);
	s32 _g = clampr(iFloor(g * 255.f), 0, 255);
	s32 _b = clampr(iFloor(b * 255.f), 0, 255);
	s32 _a = clampr(iFloor(a * 255.f), 0, 255);
	return color_argb(_a, _r, _g, _b);
}
ICF u32 color_rgba_f(float r, float g, float b, float a)
{
	return color_argb_f(a, r, g, b);
}

// Извлечение компонент
ICF u32 color_get_A(u32 rgba)
{
	return ((rgba) >> 24) & 0xff;
}
ICF u32 color_get_R(u32 rgba)
{
	return ((rgba) >> 16) & 0xff;
}
ICF u32 color_get_G(u32 rgba)
{
	return ((rgba) >> 8) & 0xff;
}
ICF u32 color_get_B(u32 rgba)
{
	return (rgba)&0xff;
}

// Замена альфа-канала
ICF u32 color_set_alpha(u32 rgba, u32 a)
{
	return (rgba & 0x00ffffff) | ((a & 0xff) << 24);
}

// Преобразование BGR <-> RGB (без альфы)
ICF u32 bgr2rgb(u32 bgr)
{
	return color_rgba(color_get_B(bgr), color_get_G(bgr), color_get_R(bgr), 0);
}
ICF u32 rgb2bgr(u32 rgb)
{
	return bgr2rgb(rgb);
}

// -----------------------------------------------------------------------------
// Класс цвета с плавающими компонентами (0..1)
// -----------------------------------------------------------------------------
template <class T> class Color
{
  public:
	using Self = Color<T>;

	T r, g, b, a;

	// Конструкторы
	Color() = default;
	Color(T _r, T _g, T _b, T _a = T(1)) : r(_r), g(_g), b(_b), a(_a)
	{
	}
	explicit Color(u32 rgba)
	{
		set(rgba);
	}

	// Установка из 32-битного цвета (A8R8G8B8)
	Self& set(u32 rgba)
	{
		const T f = T(1) / T(255);
		a = f * T(color_get_A(rgba));
		r = f * T(color_get_R(rgba));
		g = f * T(color_get_G(rgba));
		b = f * T(color_get_B(rgba));
		return *this;
	}

	// Установка из компонент
	Self& set(T _r, T _g, T _b, T _a = T(1))
	{
		r = _r;
		g = _g;
		b = _b;
		a = _a;
		return *this;
	}

	// Копирование
	Self& set(const Self& other)
	{
		r = other.r;
		g = other.g;
		b = other.b;
		a = other.a;
		return *this;
	}

	// Получение 32-битного цвета (A8R8G8B8)
	u32 get() const
	{
		return color_rgba_f(r, g, b, a);
	}

	// Получение цвета в формате Windows (A8B8G8R8)
	u32 get_windows() const
	{
		unsigned char _a = (unsigned char)(clampr(a * T(255), T(0), T(255)));
		unsigned char _r = (unsigned char)(clampr(r * T(255), T(0), T(255)));
		unsigned char _g = (unsigned char)(clampr(g * T(255), T(0), T(255)));
		unsigned char _b = (unsigned char)(clampr(b * T(255), T(0), T(255)));
		return ((u32(_a) << 24) | (u32(_b) << 16) | (u32(_g) << 8) | u32(_r));
	}

	// Установка из Windows-формата (A8B8G8R8)
	Self& set_windows(u32 dw)
	{
		const T f = T(1) / T(255);
		a = f * T((dw >> 24) & 0xff);
		b = f * T((dw >> 16) & 0xff);
		g = f * T((dw >> 8) & 0xff);
		r = f * T((dw >> 0) & 0xff);
		return *this;
	}

	// Коррекция контраста (factor > 1 увеличивает контраст)
	Self& adjust_contrast(T factor)
	{
		r = T(0.5) + factor * (r - T(0.5));
		g = T(0.5) + factor * (g - T(0.5));
		b = T(0.5) + factor * (b - T(0.5));
		return *this;
	}
	Self& adjust_contrast(const Self& src, T factor)
	{
		r = T(0.5) + factor * (src.r - T(0.5));
		g = T(0.5) + factor * (src.g - T(0.5));
		b = T(0.5) + factor * (src.b - T(0.5));
		return *this;
	}

	// Коррекция насыщенности
	Self& adjust_saturation(T s)
	{
		// Коэффициенты яркости (ITU-R BT.709)
		T grey = r * T(0.2125) + g * T(0.7154) + b * T(0.0721);
		r = grey + s * (r - grey);
		g = grey + s * (g - grey);
		b = grey + s * (b - grey);
		return *this;
	}
	Self& adjust_saturation(const Self& src, T s)
	{
		T grey = src.r * T(0.2125) + src.g * T(0.7154) + src.b * T(0.0721);
		r = grey + s * (src.r - grey);
		g = grey + s * (src.g - grey);
		b = grey + s * (src.b - grey);
		return *this;
	}

	// Покомпонентное умножение (модуляция)
	Self& modulate(const Self& other)
	{
		r *= other.r;
		g *= other.g;
		b *= other.b;
		a *= other.a;
		return *this;
	}
	Self& modulate(const Self& a, const Self& b)
	{
		r = a.r * b.r;
		g = a.g * b.g;
		this->b = a.b * b.b;
		this->a = a.a * b.a;
		return *this;
	}

	// Инвертирование (негатив)
	Self& negative()
	{
		r = T(1) - r;
		g = T(1) - g;
		b = T(1) - b;
		a = T(1) - a;
		return *this;
	}
	Self& negative(const Self& src)
	{
		r = T(1) - src.r;
		g = T(1) - src.g;
		b = T(1) - src.b;
		a = T(1) - src.a;
		return *this;
	}

	// Сложение/вычитание константы
	Self& sub_rgb(T s)
	{
		r -= s;
		g -= s;
		b -= s;
		return *this;
	}
	Self& add_rgb(T s)
	{
		r += s;
		g += s;
		b += s;
		return *this;
	}
	Self& add_rgba(T s)
	{
		r += s;
		g += s;
		b += s;
		a += s;
		return *this;
	}

	// Умножение на скаляр
	Self& mul_rgb(T s)
	{
		r *= s;
		g *= s;
		b *= s;
		return *this;
	}
	Self& mul_rgba(T s)
	{
		r *= s;
		g *= s;
		b *= s;
		a *= s;
		return *this;
	}
	Self& mul_rgb(const Self& src, T s)
	{
		r = src.r * s;
		g = src.g * s;
		b = src.b * s;
		return *this;
	}
	Self& mul_rgba(const Self& src, T s)
	{
		r = src.r * s;
		g = src.g * s;
		b = src.b * s;
		a = src.a * s;
		return *this;
	}

	// Длина RGB (квадрат и корень)
	T magnitude_sqr_rgb() const
	{
		return r * r + g * g + b * b;
	}
	T magnitude_rgb() const
	{
		return std::sqrt(magnitude_sqr_rgb());
	}

	// Интенсивность (среднее)
	T intensity() const
	{
		return (r + g + b) / T(3);
	}

	// Нормализация RGB (приведение к единичной длине)
	Self& normalize_rgb()
	{
		T len = magnitude_rgb();
		assert(len > std::numeric_limits<T>::epsilon());
		T inv = T(1) / len;
		r *= inv;
		g *= inv;
		b *= inv;
		return *this;
	}
	Self& normalize_rgb(const Self& src)
	{
		T len = src.magnitude_rgb();
		assert(len > std::numeric_limits<T>::epsilon());
		T inv = T(1) / len;
		r = src.r * inv;
		g = src.g * inv;
		b = src.b * inv;
		return *this;
	}

	// Линейная интерполяция
	Self& lerp(const Self& c1, const Self& c2, T t)
	{
		T inv = T(1) - t;
		r = c1.r * inv + c2.r * t;
		g = c1.g * inv + c2.g * t;
		b = c1.b * inv + c2.b * t;
		a = c1.a * inv + c2.a * t;
		return *this;
	}
	Self& lerp(const Self& c1, const Self& c2, const Self& c3, T t)
	{
		if (t > T(0.5))
			return lerp(c2, c3, t * T(2) - T(1));
		else
			return lerp(c1, c2, t * T(2));
	}

	// Сравнение с эпсилон
	bool similar_rgba(const Self& other, T eps = static_cast<T>(EPS_L)) const
	{
		return std::abs(r - other.r) < eps && std::abs(g - other.g) < eps && std::abs(b - other.b) < eps &&
			   std::abs(a - other.a) < eps;
	}
	bool similar_rgb(const Self& other, T eps = static_cast<T>(EPS_L)) const
	{
		return std::abs(r - other.r) < eps && std::abs(g - other.g) < eps && std::abs(b - other.b) < eps;
	}
};

// Типовые определения
using color = Color<float>;
using fcolor = Color<float>;
using dcolor = Color<double>;

// Проверка на корректность (NaN/Inf)
template <class T> inline bool valid(const Color<T>& c)
{
	return std::isfinite(c.r) && std::isfinite(c.g) && std::isfinite(c.b) && std::isfinite(c.a);
}

XRAY_END
