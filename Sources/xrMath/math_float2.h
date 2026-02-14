#pragma once
#include "xrMathCommon.h"
#include "math_constants.h" // PI, EPS и т.д.

#include <cmath>
#include <limits>
#include <type_traits>
#include <algorithm> // std::min, std::max

XRAY_BEGIN

template <class T> struct Vector2
{
	using Self = Vector2<T>;

	T x, y;

	// Конструкторы
	Self& set(T _x, T _y)
	{
		x = _x;
		y = _y;
		return *this;
	}
	Self& set(const Self& v)
	{
		x = v.x;
		y = v.y;
		return *this;
	}

	// Базовая арифметика
	Self& add(const Self& v)
	{
		x += v.x;
		y += v.y;
		return *this;
	}
	Self& sub(const Self& v)
	{
		x -= v.x;
		y -= v.y;
		return *this;
	}
	Self& mul(T s)
	{
		x *= s;
		y *= s;
		return *this;
	}

	// Длина
	T magnitude() const
	{
		return std::sqrt(x * x + y * y);
	}

	Self& normalize()
	{
		T m = magnitude();
		if (m > std::numeric_limits<T>::epsilon())
			mul(static_cast<T>(1) / m);
		return *this;
	}

	// Скалярное произведение (Dot Product)
	T dot(const Self& v) const
	{
		return x * v.x + y * v.y;
	}

	// "Векторное" произведение в 2D (возвращает скаляр, Z-компоненту 3D аналога)
	T cross(const Self& v) const
	{
		return x * v.y - y * v.x;
	}

	// Abs
	Self& abs()
	{
		x = std::abs(x);
		y = std::abs(y);
		return *this;
	}
	Self& abs(const Self& p)
	{
		x = std::abs(p.x);
		y = std::abs(p.y);
		return *this;
	}

	// Min/Max
	Self& min(const Self& p)
	{
		x = (std::min)(x, p.x);
		y = (std::min)(y, p.y);
		return *this;
	}
	Self& min(T _x, T _y)
	{
		x = (std::min)(x, _x);
		y = (std::min)(y, _y);
		return *this;
	}
	Self& max(const Self& p)
	{
		x = (std::max)(x, p.x);
		y = (std::max)(y, p.y);
		return *this;
	}
	Self& max(T _x, T _y)
	{
		x = (std::max)(x, _x);
		y = (std::max)(y, _y);
		return *this;
	}

	// Sub overloads
	Self& sub(T p)
	{
		x -= p;
		y -= p;
		return *this;
	}
	Self& sub(const Self& p1, const Self& p2)
	{
		x = p1.x - p2.x;
		y = p1.y - p2.y;
		return *this;
	}
	Self& sub(const Self& p, T d)
	{
		x = p.x - d;
		y = p.y - d;
		return *this;
	}

	// Add overloads
	Self& add(T p)
	{
		x += p;
		y += p;
		return *this;
	}
	Self& add(const Self& p1, const Self& p2)
	{
		x = p1.x + p2.x;
		y = p1.y + p2.y;
		return *this;
	}
	Self& add(const Self& p, T d)
	{
		x = p.x + d;
		y = p.y + d;
		return *this;
	}

	// Mul overload (component‑wise)
	Self& mul(const Self& v)
	{
		x *= v.x;
		y *= v.y;
		return *this;
	}

	// Div (scalar)
	Self& div(T s)
	{
		x /= s;
		y /= s;
		return *this;
	}

	// Rotate 90° clockwise: (x,y) -> (y, -x)
	Self& rot90()
	{
		T t = -x;
		x = y;
		y = t;
		return *this;
	}

	// Set to a vector perpendicular to v (v.y, -v.x)
	Self& set_perpendicular(const Self& v)
	{
		x = v.y;
		y = -v.x;
		return *this;
	}
	// Return a new vector perpendicular to this one
	Self get_perpendicular() const
	{
		Self r;
		r.x = y;
		r.y = -x;
		return r;
	}

	// Distance and squared magnitude
	T distance_to(const Self& p) const
	{
		T dx = x - p.x;
		T dy = y - p.y;
		return std::sqrt(dx * dx + dy * dy);
	}
	T square_magnitude() const
	{
		return x * x + y * y;
	}

	// MAD (multiply‑add): this = p + d * r
	Self& mad(const Self& p, const Self& d, T r)
	{
		x = p.x + d.x * r;
		y = p.y + d.y * r;
		return *this;
	}

	// Similarity check with epsilon
	bool similar(const Self& p, T eps = static_cast<T>(EPS_L)) const
	{
		return std::abs(x - p.x) < eps && std::abs(y - p.y) < eps;
	}

	// Averages
	Self& average_arithmetic(const Self& p1, const Self& p2)
	{
		x = (p1.x + p2.x) * static_cast<T>(0.5);
		y = (p1.y + p2.y) * static_cast<T>(0.5);
		return *this;
	}
	Self& average_geometric(const Self& p1, const Self& p2)
	{
		x = std::sqrt(p1.x * p2.x);
		y = std::sqrt(p1.y * p2.y);
		return *this;
	}

	// Array subscript
	T& operator[](int i)
	{
		return (&x)[i];
	}
	const T& operator[](int i) const
	{
		return (&x)[i];
	}

	// Normalize a source vector into this one
	Self& normalize(const Self& v)
	{
		T m = std::sqrt(v.x * v.x + v.y * v.y);
		if (m > std::numeric_limits<T>::epsilon())
		{
			x = v.x / m;
			y = v.y / m;
		}
		return *this;
	}
	Self& normalize_safe(const Self& v)
	{
		T m = std::sqrt(v.x * v.x + v.y * v.y);
		if (m > std::numeric_limits<T>::epsilon())
		{
			x = v.x / m;
			y = v.y / m;
		}
		// if m == 0, keep current values (as in original norm_safe)
		return *this;
	}

	// Get heading angle (in radians) – equivalent to legacy getH()
	T get_heading() const
	{
		using std::abs;
		using std::atan;
		const T eps = static_cast<T>(EPS);
		if (abs(y) <= eps)
		{
			if (abs(x) <= eps)
				return static_cast<T>(0);
			else
				return (x > static_cast<T>(0)) ? -static_cast<T>(PI_DIV_2) : static_cast<T>(PI_DIV_2);
		}
		else if (y < static_cast<T>(0))
		{
			return -(atan(x / y) - static_cast<T>(PI));
		}
		else
		{
			return -atan(x / y);
		}
	}
};

using float2 = Vector2<float>;
using int2 = Vector2<int>;

// External validity check (NaN / Inf)
template <class T> inline bool valid(const Vector2<T>& v)
{
	return std::isfinite(v.x) && std::isfinite(v.y);
}

// Операторы сравнения
template <class T> IC bool operator==(const Vector2<T>& a, const Vector2<T>& b)
{
	return a.x == b.x && a.y == b.y;
}
template <class T> IC bool operator!=(const Vector2<T>& a, const Vector2<T>& b)
{
	return !(a == b);
}

// Унарный минус
template <class T> IC Vector2<T> operator-(const Vector2<T>& v)
{
	return Vector2<T>(-v.x, -v.y);
}

// Вектор + вектор
template <class T> IC Vector2<T> operator+(const Vector2<T>& a, const Vector2<T>& b)
{
	Vector2<T> r(a);
	return r.add(b);
}
// Вектор + скаляр
template <class T> IC Vector2<T> operator+(const Vector2<T>& a, T s)
{
	Vector2<T> r(a);
	return r.add(s);
}
// Скаляр + вектор
template <class T> IC Vector2<T> operator+(T s, const Vector2<T>& a)
{
	return a + s;
}

// Вектор - вектор
template <class T> IC Vector2<T> operator-(const Vector2<T>& a, const Vector2<T>& b)
{
	Vector2<T> r(a);
	return r.sub(b);
}
// Вектор - скаляр
template <class T> IC Vector2<T> operator-(const Vector2<T>& a, T s)
{
	Vector2<T> r(a);
	return r.sub(s);
}
// Скаляр - вектор
template <class T> IC Vector2<T> operator-(T s, const Vector2<T>& a)
{
	return Vector2<T>(s, s) - a;
}

// Вектор * вектор (покомпонентно)
template <class T> IC Vector2<T> operator*(const Vector2<T>& a, const Vector2<T>& b)
{
	Vector2<T> r(a);
	return r.mul(b);
}
// Вектор * скаляр
template <class T> IC Vector2<T> operator*(const Vector2<T>& a, T s)
{
	Vector2<T> r(a);
	return r.mul(s);
}
// Скаляр * вектор
template <class T> IC Vector2<T> operator*(T s, const Vector2<T>& a)
{
	return a * s;
}

// Вектор / вектор (покомпонентно)
template <class T> IC Vector2<T> operator/(const Vector2<T>& a, const Vector2<T>& b)
{
	Vector2<T> r(a);
	return r.div(b);
}
// Вектор / скаляр
template <class T> IC Vector2<T> operator/(const Vector2<T>& a, T s)
{
	Vector2<T> r(a);
	return r.div(s);
}
// Скаляр / вектор (редкий случай, но можно определить)
template <class T> IC Vector2<T> operator/(T s, const Vector2<T>& a)
{
	return Vector2<T>(s, s) / a;
}

// Составные операторы присваивания
template <class T> IC Vector2<T>& operator+=(Vector2<T>& a, const Vector2<T>& b)
{
	return a.add(b);
}
template <class T> IC Vector2<T>& operator+=(Vector2<T>& a, T s)
{
	return a.add(s);
}
template <class T> IC Vector2<T>& operator-=(Vector2<T>& a, const Vector2<T>& b)
{
	return a.sub(b);
}
template <class T> IC Vector2<T>& operator-=(Vector2<T>& a, T s)
{
	return a.sub(s);
}
template <class T> IC Vector2<T>& operator*=(Vector2<T>& a, const Vector2<T>& b)
{
	return a.mul(b);
}
template <class T> IC Vector2<T>& operator*=(Vector2<T>& a, T s)
{
	return a.mul(s);
}
template <class T> IC Vector2<T>& operator/=(Vector2<T>& a, const Vector2<T>& b)
{
	return a.div(b);
}
template <class T> IC Vector2<T>& operator/=(Vector2<T>& a, T s)
{
	return a.div(s);
}

XRAY_END
