#pragma once
#include "xrMathCommon.h"
#include "math_constants.h"

#include <cmath>
#include <limits>
#include <type_traits>
#include <algorithm>

XRAY_BEGIN

template <class T> struct Vector4
{
	using Self = Vector4<T>;

	T x, y, z, w;

	Self& set(T _x, T _y, T _z, T _w)
	{
		x = _x;
		y = _y;
		z = _z;
		w = _w;
		return *this;
	}
	Self& set(const Self& v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		w = v.w;
		return *this;
	}

	Self& add(const Self& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		w += v.w;
		return *this;
	}
	Self& sub(const Self& v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		w -= v.w;
		return *this;
	}
	Self& mul(T s)
	{
		x *= s;
		y *= s;
		z *= s;
		w *= s;
		return *this;
	}

	T magnitude() const
	{
		return std::sqrt(x * x + y * y + z * z + w * w);
	}

	Self& normalize()
	{
		T m = magnitude();
		mul(static_cast<T>(1) / m);
		return *this;
	}

	T dot(const Self& v) const
	{
		return x * v.x + y * v.y + z * v.z + w * v.w;
	}

	// --- Доступ по индексу ---
	T& operator[](int i)
	{
		return (&x)[i];
	}
	const T& operator[](int i) const
	{
		return (&x)[i];
	}

	// --- Дополнительные set для массивов ---
	Self& set(const float* p)
	{
		x = static_cast<T>(p[0]);
		y = static_cast<T>(p[1]);
		z = static_cast<T>(p[2]);
		w = static_cast<T>(p[3]);
		return *this;
	}
	Self& set(const double* p)
	{
		x = static_cast<T>(p[0]);
		y = static_cast<T>(p[1]);
		z = static_cast<T>(p[2]);
		w = static_cast<T>(p[3]);
		return *this;
	}

	// --- Перегрузки add ---
	Self& add(T s)
	{
		x += s;
		y += s;
		z += s;
		w += s;
		return *this;
	}
	Self& add(const Self& a, const Self& b)
	{
		x = a.x + b.x;
		y = a.y + b.y;
		z = a.z + b.z;
		w = a.w + b.w;
		return *this;
	}
	Self& add(const Self& a, T s)
	{
		x = a.x + s;
		y = a.y + s;
		z = a.z + s;
		w = a.w + s;
		return *this;
	}

	// --- Перегрузки sub ---
	Self& sub(T _x, T _y, T _z, T _w)
	{
		x -= _x;
		y -= _y;
		z -= _z;
		w -= _w;
		return *this;
	}
	Self& sub(T s)
	{
		x -= s;
		y -= s;
		z -= s;
		w -= s;
		return *this;
	}
	Self& sub(const Self& a, const Self& b)
	{
		x = a.x - b.x;
		y = a.y - b.y;
		z = a.z - b.z;
		w = a.w - b.w;
		return *this;
	}
	Self& sub(const Self& a, T s)
	{
		x = a.x - s;
		y = a.y - s;
		z = a.z - s;
		w = a.w - s;
		return *this;
	}

	// --- Перегрузки mul (покомпонентные и с парой) ---
	Self& mul(T _x, T _y, T _z, T _w)
	{
		x *= _x;
		y *= _y;
		z *= _z;
		w *= _w;
		return *this;
	}
	Self& mul(const Self& v)
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
		w *= v.w;
		return *this;
	}
	Self& mul(const Self& a, const Self& b)
	{
		x = a.x * b.x;
		y = a.y * b.y;
		z = a.z * b.z;
		w = a.w * b.w;
		return *this;
	}
	Self& mul(const Self& a, T s)
	{
		x = a.x * s;
		y = a.y * s;
		z = a.z * s;
		w = a.w * s;
		return *this;
	}

	// --- Перегрузки div ---
	Self& div(const Self& v)
	{
		x /= v.x;
		y /= v.y;
		z /= v.z;
		w /= v.w;
		return *this;
	}
	Self& div(T s)
	{
		x /= s;
		y /= s;
		z /= s;
		w /= s;
		return *this;
	}
	Self& div(const Self& a, const Self& b)
	{
		x = a.x / b.x;
		y = a.y / b.y;
		z = a.z / b.z;
		w = a.w / b.w;
		return *this;
	}
	Self& div(const Self& a, T s)
	{
		x = a.x / s;
		y = a.y / s;
		z = a.z / s;
		w = a.w / s;
		return *this;
	}

	// --- Сравнение с эпсилон ---
	bool similar(const Self& v, T eps = static_cast<T>(EPS_L)) const
	{
		return std::abs(x - v.x) < eps && std::abs(y - v.y) < eps && std::abs(z - v.z) < eps && std::abs(w - v.w) < eps;
	}

	// --- Квадрат длины ---
	T square_magnitude() const
	{
		return x * x + y * y + z * z + w * w;
	}
	// Синоним для совместимости со старой реализацией
	T magnitude_sqr() const
	{
		return square_magnitude();
	}

	// --- Линейная интерполяция ---
	Self& lerp(const Self& p1, const Self& p2, T t)
	{
		T inv = T(1) - t;
		x = p1.x * inv + p2.x * t;
		y = p1.y * inv + p2.y * t;
		z = p1.z * inv + p2.z * t;
		w = p1.w * inv + p2.w * t;
		return *this;
	}
};

// Определения конкретных типов
using float4 = Vector4<float>;
using int4 = Vector4<int>;

// --- Внешняя функция проверки на NaN/Inf ---
template <class T> inline bool valid(const Vector4<T>& v)
{
	return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) && std::isfinite(v.w);
}

// Унарный минус
template <class T> IC Vector4<T> operator-(const Vector4<T>& v)
{
	return Vector4<T>(-v.x, -v.y, -v.z, -v.w);
}

// Сравнение
template <class T> IC bool operator==(const Vector4<T>& a, const Vector4<T>& b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}
template <class T> IC bool operator!=(const Vector4<T>& a, const Vector4<T>& b)
{
	return !(a == b);
}

// Вектор + вектор
template <class T> IC Vector4<T> operator+(const Vector4<T>& a, const Vector4<T>& b)
{
	Vector4<T> r(a);
	return r.add(b);
}
// Вектор + скаляр
template <class T> IC Vector4<T> operator+(const Vector4<T>& a, T s)
{
	Vector4<T> r(a);
	return r.add(s);
}
// Скаляр + вектор
template <class T> IC Vector4<T> operator+(T s, const Vector4<T>& a)
{
	return a + s;
}

// Вектор - вектор
template <class T> IC Vector4<T> operator-(const Vector4<T>& a, const Vector4<T>& b)
{
	Vector4<T> r(a);
	return r.sub(b);
}
// Вектор - скаляр
template <class T> IC Vector4<T> operator-(const Vector4<T>& a, T s)
{
	Vector4<T> r(a);
	return r.sub(s);
}
// Скаляр - вектор
template <class T> IC Vector4<T> operator-(T s, const Vector4<T>& a)
{
	return Vector4<T>(s, s, s, s) - a;
}

// Вектор * вектор (покомпонентно)
template <class T> IC Vector4<T> operator*(const Vector4<T>& a, const Vector4<T>& b)
{
	Vector4<T> r(a);
	return r.mul(b);
}
// Вектор * скаляр
template <class T> IC Vector4<T> operator*(const Vector4<T>& a, T s)
{
	Vector4<T> r(a);
	return r.mul(s);
}
// Скаляр * вектор
template <class T> IC Vector4<T> operator*(T s, const Vector4<T>& a)
{
	return a * s;
}

// Вектор / вектор (покомпонентно)
template <class T> IC Vector4<T> operator/(const Vector4<T>& a, const Vector4<T>& b)
{
	Vector4<T> r(a);
	return r.div(b);
}
// Вектор / скаляр
template <class T> IC Vector4<T> operator/(const Vector4<T>& a, T s)
{
	Vector4<T> r(a);
	return r.div(s);
}
// Скаляр / вектор
template <class T> IC Vector4<T> operator/(T s, const Vector4<T>& a)
{
	return Vector4<T>(s, s, s, s) / a;
}

// Составные операторы присваивания
template <class T> IC Vector4<T>& operator+=(Vector4<T>& a, const Vector4<T>& b)
{
	return a.add(b);
}
template <class T> IC Vector4<T>& operator+=(Vector4<T>& a, T s)
{
	return a.add(s);
}
template <class T> IC Vector4<T>& operator-=(Vector4<T>& a, const Vector4<T>& b)
{
	return a.sub(b);
}
template <class T> IC Vector4<T>& operator-=(Vector4<T>& a, T s)
{
	return a.sub(s);
}
template <class T> IC Vector4<T>& operator*=(Vector4<T>& a, const Vector4<T>& b)
{
	return a.mul(b);
}
template <class T> IC Vector4<T>& operator*=(Vector4<T>& a, T s)
{
	return a.mul(s);
}
template <class T> IC Vector4<T>& operator/=(Vector4<T>& a, const Vector4<T>& b)
{
	return a.div(b);
}
template <class T> IC Vector4<T>& operator/=(Vector4<T>& a, T s)
{
	return a.div(s);
}

XRAY_END
