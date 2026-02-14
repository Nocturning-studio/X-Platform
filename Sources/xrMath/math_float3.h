#pragma once
#include "xrMathCommon.h"
#include "math_constants.h" // PI, EPS и т.д.

#include <cmath>
#include <limits>
#include <type_traits>
#include <algorithm> // std::min, std::max
#include <cassert>	 // assert

XRAY_BEGIN

template <class T> struct Vector3
{
	using Self = Vector3<T>;

	T x, y, z;

	Self& set(T _x, T _y, T _z)
	{
		x = _x;
		y = _y;
		z = _z;
		return *this;
	}
	Self& set(const Self& v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	Self& add(const Self& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}
	Self& sub(const Self& v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}
	Self& mul(T s)
	{
		x *= s;
		y *= s;
		z *= s;
		return *this;
	}

	T magnitude() const
	{
		return std::sqrt(x * x + y * y + z * z);
	}

	Self& normalize()
	{
		T m = magnitude();
		mul(static_cast<T>(1) / m);
		return *this;
	}

	T dot(const Self& v) const
	{
		return x * v.x + y * v.y + z * v.z;
	}

	Self& cross(const Self& v1, const Self& v2)
	{
		x = v1.y * v2.z - v1.z * v2.y;
		y = v1.z * v2.x - v1.x * v2.z;
		z = v1.x * v2.y - v1.y * v2.x;
		return *this;
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

	// --- Перегрузки set для разных типов указателей ---
	Self& set(const float* p)
	{
		x = static_cast<T>(p[0]);
		y = static_cast<T>(p[1]);
		z = static_cast<T>(p[2]);
		return *this;
	}
	Self& set(const double* p)
	{
		x = static_cast<T>(p[0]);
		y = static_cast<T>(p[1]);
		z = static_cast<T>(p[2]);
		return *this;
	}

	// --- Дополнительные перегрузки add/sub/mul/div ---

	// add со скаляром
	Self& add(T s)
	{
		x += s;
		y += s;
		z += s;
		return *this;
	}
	// add из двух векторов (this = a + b)
	Self& add(const Self& a, const Self& b)
	{
		x = a.x + b.x;
		y = a.y + b.y;
		z = a.z + b.z;
		return *this;
	}
	// add из вектора и скаляра (this = a + s)
	Self& add(const Self& a, T s)
	{
		x = a.x + s;
		y = a.y + s;
		z = a.z + s;
		return *this;
	}

	// sub со скаляром
	Self& sub(T s)
	{
		x -= s;
		y -= s;
		z -= s;
		return *this;
	}
	// sub из двух векторов (this = a - b)
	Self& sub(const Self& a, const Self& b)
	{
		x = a.x - b.x;
		y = a.y - b.y;
		z = a.z - b.z;
		return *this;
	}
	// sub из вектора и скаляра (this = a - s)
	Self& sub(const Self& a, T s)
	{
		x = a.x - s;
		y = a.y - s;
		z = a.z - s;
		return *this;
	}

	// mul покомпонентно (this *= v)
	Self& mul(const Self& v)
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}
	// mul покомпонентно (this = a * b)
	Self& mul(const Self& a, const Self& b)
	{
		x = a.x * b.x;
		y = a.y * b.y;
		z = a.z * b.z;
		return *this;
	}
	// mul вектора на скаляр (this = a * s)
	Self& mul(const Self& a, T s)
	{
		x = a.x * s;
		y = a.y * s;
		z = a.z * s;
		return *this;
	}

	// div покомпонентно (this /= v)
	Self& div(const Self& v)
	{
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}
	// div на скаляр (this /= s)
	Self& div(T s)
	{
		x /= s;
		y /= s;
		z /= s;
		return *this;
	}
	// div покомпонентно (this = a / b)
	Self& div(const Self& a, const Self& b)
	{
		x = a.x / b.x;
		y = a.y / b.y;
		z = a.z / b.z;
		return *this;
	}
	// div вектора на скаляр (this = a / s)
	Self& div(const Self& a, T s)
	{
		x = a.x / s;
		y = a.y / s;
		z = a.z / s;
		return *this;
	}

	// --- Инвертирование (отрицание) ---
	Self& invert()
	{
		x = -x;
		y = -y;
		z = -z;
		return *this;
	}
	Self& invert(const Self& a)
	{
		x = -a.x;
		y = -a.y;
		z = -a.z;
		return *this;
	}

	// --- Min, Max, Abs ---
	Self& min(const Self& a, const Self& b)
	{
		x = (std::min)(a.x, b.x);
		y = (std::min)(a.y, b.y);
		z = (std::min)(a.z, b.z);
		return *this;
	}
	Self& min(const Self& v)
	{
		x = (std::min)(x, v.x);
		y = (std::min)(y, v.y);
		z = (std::min)(z, v.z);
		return *this;
	}
	Self& max(const Self& a, const Self& b)
	{
		x = (std::max)(a.x, b.x);
		y = (std::max)(a.y, b.y);
		z = (std::max)(a.z, b.z);
		return *this;
	}
	Self& max(const Self& v)
	{
		x = (std::max)(x, v.x);
		y = (std::max)(y, v.y);
		z = (std::max)(z, v.z);
		return *this;
	}
	Self& abs(const Self& v)
	{
		x = std::abs(v.x);
		y = std::abs(v.y);
		z = std::abs(v.z);
		return *this;
	}
	Self& abs()
	{
		x = std::abs(x);
		y = std::abs(y);
		z = std::abs(z);
		return *this;
	}

	// --- Сравнение с эпсилон ---
	bool similar(const Self& v, T eps = static_cast<T>(EPS_L)) const
	{
		return std::abs(x - v.x) < eps && std::abs(y - v.y) < eps && std::abs(z - v.z) < eps;
	}

	// --- Установка длины ---
	Self& set_length(T len)
	{
		T m = magnitude();
		if (m > std::numeric_limits<T>::epsilon())
			mul(len / m);
		return *this;
	}

	// --- Выравнивание и чистка ---
	Self& align()
	{ // Убирает компоненту Y и выравнивает по оси X или Z
		y = 0;
		if (std::abs(z) >= std::abs(x))
		{
			z /= (z != T(0)) ? std::abs(z) : T(1);
			x = 0;
		}
		else
		{
			x /= (x != T(0)) ? std::abs(x) : T(1);
			z = 0;
		}
		return *this;
	}
	Self& squeeze(T eps)
	{
		if (std::abs(x) < eps)
			x = 0;
		if (std::abs(y) < eps)
			y = 0;
		if (std::abs(z) < eps)
			z = 0;
		return *this;
	}

	// --- Ограничение (clamp) ---
	Self& clamp(const Self& minVal, const Self& maxVal)
	{
		x = (std::max)(minVal.x, (std::min)(maxVal.x, x));
		y = (std::max)(minVal.y, (std::min)(maxVal.y, y));
		z = (std::max)(minVal.z, (std::min)(maxVal.z, z));
		return *this;
	}
	Self& clamp(const Self& v)
	{
		// Ограничение относительно симметричного интервала [-|v|, |v|]
		T ax = std::abs(v.x), ay = std::abs(v.y), az = std::abs(v.z);
		x = (std::max)(-ax, (std::min)(ax, x));
		y = (std::max)(-ay, (std::min)(ay, y));
		z = (std::max)(-az, (std::min)(az, z));
		return *this;
	}

	// --- Интерполяция ---
	// Инерция: this = this * v + p * (1-v)
	Self& inertion(const Self& p, T factor)
	{
		T inv = T(1) - factor;
		x = factor * x + inv * p.x;
		y = factor * y + inv * p.y;
		z = factor * z + inv * p.z;
		return *this;
	}
	// Среднее арифметическое this и p
	Self& average(const Self& p)
	{
		x = (x + p.x) * T(0.5);
		y = (y + p.y) * T(0.5);
		z = (z + p.z) * T(0.5);
		return *this;
	}
	// Среднее арифметическое p1 и p2
	Self& average(const Self& p1, const Self& p2)
	{
		x = (p1.x + p2.x) * T(0.5);
		y = (p1.y + p2.y) * T(0.5);
		z = (p1.z + p2.z) * T(0.5);
		return *this;
	}
	// Линейная интерполяция: this = p1 * (1-t) + p2 * t
	Self& lerp(const Self& p1, const Self& p2, T t)
	{
		T inv = T(1) - t;
		x = p1.x * inv + p2.x * t;
		y = p1.y * inv + p2.y * t;
		z = p1.z * inv + p2.z * t;
		return *this;
	}

	// --- MAD (multiply-add) ---
	// this += dir * length
	Self& mad(const Self& dir, T length)
	{
		x += dir.x * length;
		y += dir.y * length;
		z += dir.z * length;
		return *this;
	}
	// this = point + dir * length
	Self& mad(const Self& point, const Self& dir, T length)
	{
		x = point.x + dir.x * length;
		y = point.y + dir.y * length;
		z = point.z + dir.z * length;
		return *this;
	}
	// this += dir * s (покомпонентно)
	Self& mad(const Self& dir, const Self& s)
	{
		x += dir.x * s.x;
		y += dir.y * s.y;
		z += dir.z * s.z;
		return *this;
	}
	// this = point + dir * s (покомпонентно)
	Self& mad(const Self& point, const Self& dir, const Self& s)
	{
		x = point.x + dir.x * s.x;
		y = point.y + dir.y * s.y;
		z = point.z + dir.z * s.z;
		return *this;
	}

	// --- Квадрат длины ---
	T square_magnitude() const
	{
		return x * x + y * y + z * z;
	}

	// --- Нормализация с возвратом исходной длины ---
	T normalize_magn()
	{
		T sq = square_magnitude();
		T len = std::sqrt(sq);
		T inv_len = T(1) / len;
		x *= inv_len;
		y *= inv_len;
		z *= inv_len;
		return len;
	}

	// --- Безопасная нормализация (с проверкой на ноль) ---
	Self& normalize_safe()
	{
		T sq = square_magnitude();
		if (sq > std::numeric_limits<T>::epsilon())
		{
			T inv = T(1) / std::sqrt(sq);
			x *= inv;
			y *= inv;
			z *= inv;
		}
		return *this;
	}
	// Нормализация вектора v, запись в this
	Self& normalize(const Self& v)
	{
		T sq = v.square_magnitude();
		T inv = T(1) / std::sqrt(sq);
		x = v.x * inv;
		y = v.y * inv;
		z = v.z * inv;
		return *this;
	}
	// Безопасная нормализация вектора v
	Self& normalize_safe(const Self& v)
	{
		T sq = v.square_magnitude();
		if (sq > std::numeric_limits<T>::epsilon())
		{
			T inv = T(1) / std::sqrt(sq);
			x = v.x * inv;
			y = v.y * inv;
			z = v.z * inv;
		}
		return *this;
	}

	// --- Синонимы для dot и cross (совместимость) ---
	T dotproduct(const Self& v) const
	{
		return dot(v);
	}
	Self& crossproduct(const Self& v1, const Self& v2)
	{
		return cross(v1, v2);
	}

	// --- Расстояния ---
	T distance_to_xz(const Self& v) const
	{
		T dx = x - v.x;
		T dz = z - v.z;
		return std::sqrt(dx * dx + dz * dz);
	}
	T distance_to_xz_sqr(const Self& v) const
	{
		T dx = x - v.x;
		T dz = z - v.z;
		return dx * dx + dz * dz;
	}
	T distance_to_sqr(const Self& v) const
	{
		T dx = x - v.x;
		T dy = y - v.y;
		T dz = z - v.z;
		return dx * dx + dy * dy + dz * dz;
	}
	T distance_to(const Self& v) const
	{
		return std::sqrt(distance_to_sqr(v));
	}

	// --- Барицентрические координаты ---
	Self& from_bary(const Self& v1, const Self& v2, const Self& v3, T u, T v, T w)
	{
		x = v1.x * u + v2.x * v + v3.x * w;
		y = v1.y * u + v2.y * v + v3.y * w;
		z = v1.z * u + v2.z * v + v3.z * w;
		return *this;
	}
	Self& from_bary(const Self& v1, const Self& v2, const Self& v3, const Self& bary)
	{
		return from_bary(v1, v2, v3, bary.x, bary.y, bary.z);
	}
	Self& from_bary4(const Self& v1, const Self& v2, const Self& v3, const Self& v4, T u, T v, T w, T t)
	{
		x = v1.x * u + v2.x * v + v3.x * w + v4.x * t;
		y = v1.y * u + v2.y * v + v3.y * w + v4.y * t;
		z = v1.z * u + v2.z * v + v3.z * w + v4.z * t;
		return *this;
	}

	// --- Построение нормали треугольника ---
	Self& mknormal_non_normalized(const Self& p0, const Self& p1, const Self& p2)
	{
		Self e01, e12;
		e01.sub(p1, p0);
		e12.sub(p2, p1);
		return cross(e01, e12);
	}
	Self& mknormal(const Self& p0, const Self& p1, const Self& p2)
	{
		mknormal_non_normalized(p0, p1, p2);
		normalize_safe();
		return *this;
	}

	// --- Установка по горизонтальному и вертикальному углам (heading/pitch) ---
	Self& setHP(T heading, T pitch)
	{
		T ch = std::cos(heading);
		T cp = std::cos(pitch);
		T sh = std::sin(heading);
		T sp = std::sin(pitch);
		x = -cp * sh;
		y = sp;
		z = cp * ch;
		return *this;
	}
	void getHP(T& heading, T& pitch) const
	{
		const T eps = static_cast<T>(EPS);
		if (std::abs(x) <= eps && std::abs(z) <= eps)
		{
			heading = T(0);
			pitch = (std::abs(y) <= eps) ? T(0) : ((y > T(0)) ? static_cast<T>(PI_DIV_2) : -static_cast<T>(PI_DIV_2));
		}
		else
		{
			if (std::abs(z) <= eps)
				heading = (x > T(0)) ? -static_cast<T>(PI_DIV_2) : static_cast<T>(PI_DIV_2);
			else if (z < T(0))
				heading = -(std::atan(x / z) - static_cast<T>(PI));
			else
				heading = -std::atan(x / z);

			T hyp = std::sqrt(x * x + z * z);
			if (std::abs(hyp) <= eps)
				pitch = (y > T(0)) ? static_cast<T>(PI_DIV_2) : -static_cast<T>(PI_DIV_2);
			else
				pitch = std::atan(y / hyp);
		}
	}
	T getH() const
	{
		const T eps = static_cast<T>(EPS);
		if (std::abs(x) <= eps && std::abs(z) <= eps)
			return T(0);
		if (std::abs(z) <= eps)
			return (x > T(0)) ? -static_cast<T>(PI_DIV_2) : static_cast<T>(PI_DIV_2);
		if (z < T(0))
			return -(std::atan(x / z) - static_cast<T>(PI));
		return -std::atan(x / z);
	}
	T getP() const
	{
		const T eps = static_cast<T>(EPS);
		if (std::abs(x) <= eps && std::abs(z) <= eps)
		{
			return (std::abs(y) <= eps) ? T(0) : ((y > T(0)) ? static_cast<T>(PI_DIV_2) : -static_cast<T>(PI_DIV_2));
		}
		T hyp = std::sqrt(x * x + z * z);
		if (std::abs(hyp) <= eps)
			return (y > T(0)) ? static_cast<T>(PI_DIV_2) : -static_cast<T>(PI_DIV_2);
		return std::atan(y / hyp);
	}

	// --- Отражение и скольжение ---
	Self& reflect(const Self& dir, const Self& norm)
	{
		// dir отражается относительно нормированной нормали
		T two_dot = T(2) * dir.dot(norm);
		x = dir.x - two_dot * norm.x;
		y = dir.y - two_dot * norm.y;
		z = dir.z - two_dot * norm.z;
		return *this;
	}
	Self& slide(const Self& dir, const Self& norm)
	{
		// Проекция dir на плоскость с нормалью norm (norm не обязательно единичная)
		T dot = dir.dot(norm);
		x = dir.x - dot * norm.x;
		y = dir.y - dot * norm.y;
		z = dir.z - dot * norm.z;
		return *this;
	}

	// --- Статические методы для построения ортонормированного базиса ---
	static void generate_orthonormal_basis(const Self& dir, Self& up, Self& right)
	{
		// dir может быть не нормирован
		T invLen;
		if (std::abs(dir.x) >= std::abs(dir.y))
		{
			invLen = T(1) / std::sqrt(dir.x * dir.x + dir.z * dir.z);
			up.x = -dir.z * invLen;
			up.y = T(0);
			up.z = +dir.x * invLen;
		}
		else
		{
			invLen = T(1) / std::sqrt(dir.y * dir.y + dir.z * dir.z);
			up.x = T(0);
			up.y = +dir.z * invLen;
			up.z = -dir.y * invLen;
		}
		right.cross(up, dir);
	}
	static void generate_orthonormal_basis_normalized(Self& dir, Self& up, Self& right)
	{
		// dir нормализуется на выходе
		dir.normalize();
		const T eps = static_cast<T>(EPS);
		if (std::abs(dir.y - T(1)) <= eps)
		{
			up.set(T(0), T(0), T(1));
			T invLen = T(1) / std::sqrt(dir.x * dir.x + dir.y * dir.y);
			right.x = -dir.y * invLen;
			right.y = dir.x * invLen;
			right.z = T(0);
			// up = dir x right
			up.x = -dir.z * right.y;
			up.y = dir.z * right.x;
			up.z = dir.x * right.y - dir.y * right.x;
		}
		else
		{
			up.set(T(0), T(1), T(0));
			T invLen = T(1) / std::sqrt(dir.x * dir.x + dir.z * dir.z);
			right.x = dir.z * invLen;
			right.y = T(0);
			right.z = -dir.x * invLen;
			// up = dir x right
			up.x = dir.y * right.z;
			up.y = dir.z * right.x - dir.x * right.z;
			up.z = -dir.y * right.x;
		}
	}
};

using float3 = Vector3<float>;
using int3 = Vector3<int>;

// Проверка на NaN/Inf
template <class T> inline bool valid(const Vector3<T>& v)
{
	return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

// Точная нормализация с защитой от вырожденных случаев (из старой реализации)
inline bool exact_normalize(float* a)
{
	double sqr_magnitude = double(a[0]) * a[0] + double(a[1]) * a[1] + double(a[2]) * a[2];
	const double epsilon = 1.192092896e-05; // 2^-23
	if (sqr_magnitude > epsilon)
	{
		double l = 1.0 / std::sqrt(sqr_magnitude);
		a[0] *= float(l);
		a[1] *= float(l);
		a[2] *= float(l);
		return true;
	}
	// Выбираем компоненту с максимальным абсолютным значением
	double a0 = a[0], a1 = a[1], a2 = a[2];
	double aa0 = std::abs(a0), aa1 = std::abs(a1), aa2 = std::abs(a2);
	double l;
	if (aa1 > aa0)
	{
		if (aa2 > aa1)
		{
			// aa2 наибольшая
			a0 /= aa2;
			a1 /= aa2;
			l = 1.0 / std::sqrt(a0 * a0 + a1 * a1 + 1.0);
			a[0] = float(a0 * l);
			a[1] = float(a1 * l);
			a[2] = float((a2 > 0) ? l : -l);
		}
		else
		{
			// aa1 наибольшая
			a0 /= aa1;
			a2 /= aa1;
			l = 1.0 / std::sqrt(a0 * a0 + a2 * a2 + 1.0);
			a[0] = float(a0 * l);
			a[1] = float((a1 > 0) ? l : -l);
			a[2] = float(a2 * l);
		}
	}
	else
	{
		if (aa2 > aa0)
		{
			// aa2 наибольшая
			a0 /= aa2;
			a1 /= aa2;
			l = 1.0 / std::sqrt(a0 * a0 + a1 * a1 + 1.0);
			a[0] = float(a0 * l);
			a[1] = float(a1 * l);
			a[2] = float((a2 > 0) ? l : -l);
		}
		else
		{
			// aa0 наибольшая (или все равны нулю)
			if (aa0 <= 0.0)
			{
				// Вектор нулевой – возвращаем (0,1,0)
				a[0] = 0.0f;
				a[1] = 1.0f;
				a[2] = 0.0f;
				return false;
			}
			a1 /= aa0;
			a2 /= aa0;
			l = 1.0 / std::sqrt(a1 * a1 + a2 * a2 + 1.0);
			a[0] = float((a0 > 0) ? l : -l);
			a[1] = float(a1 * l);
			a[2] = float(a2 * l);
		}
	}
	return true;
}

inline bool exact_normalize(float3& v)
{
	return exact_normalize(&v.x);
}

// ---------- Операторы для Vector3 ----------
// Унарный минус
template <class T> IC Vector3<T> operator-(const Vector3<T>& v)
{
	return Vector3<T>(-v.x, -v.y, -v.z);
}

// Сравнение
template <class T> IC bool operator==(const Vector3<T>& a, const Vector3<T>& b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}
template <class T> IC bool operator!=(const Vector3<T>& a, const Vector3<T>& b)
{
	return !(a == b);
}

// Вектор + вектор
template <class T> IC Vector3<T> operator+(const Vector3<T>& a, const Vector3<T>& b)
{
	Vector3<T> r(a);
	return r.add(b);
}
// Вектор + скаляр
template <class T> IC Vector3<T> operator+(const Vector3<T>& a, T s)
{
	Vector3<T> r(a);
	return r.add(s);
}
// Скаляр + вектор
template <class T> IC Vector3<T> operator+(T s, const Vector3<T>& a)
{
	return a + s;
}

// Вектор - вектор
template <class T> IC Vector3<T> operator-(const Vector3<T>& a, const Vector3<T>& b)
{
	Vector3<T> r(a);
	return r.sub(b);
}
// Вектор - скаляр
template <class T> IC Vector3<T> operator-(const Vector3<T>& a, T s)
{
	Vector3<T> r(a);
	return r.sub(s);
}
// Скаляр - вектор
template <class T> IC Vector3<T> operator-(T s, const Vector3<T>& a)
{
	return Vector3<T>(s, s, s) - a;
}

// Вектор * вектор (покомпонентно)
template <class T> IC Vector3<T> operator*(const Vector3<T>& a, const Vector3<T>& b)
{
	Vector3<T> r(a);
	return r.mul(b);
}
// Вектор * скаляр
template <class T> IC Vector3<T> operator*(const Vector3<T>& a, T s)
{
	Vector3<T> r(a);
	return r.mul(s);
}
// Скаляр * вектор
template <class T> IC Vector3<T> operator*(T s, const Vector3<T>& a)
{
	return a * s;
}

// Вектор / вектор (покомпонентно)
template <class T> IC Vector3<T> operator/(const Vector3<T>& a, const Vector3<T>& b)
{
	Vector3<T> r(a);
	return r.div(b);
}
// Вектор / скаляр
template <class T> IC Vector3<T> operator/(const Vector3<T>& a, T s)
{
	Vector3<T> r(a);
	return r.div(s);
}
// Скаляр / вектор (редко, но можно)
template <class T> IC Vector3<T> operator/(T s, const Vector3<T>& a)
{
	return Vector3<T>(s, s, s) / a;
}

// Составные операторы присваивания
template <class T> IC Vector3<T>& operator+=(Vector3<T>& a, const Vector3<T>& b)
{
	return a.add(b);
}
template <class T> IC Vector3<T>& operator+=(Vector3<T>& a, T s)
{
	return a.add(s);
}
template <class T> IC Vector3<T>& operator-=(Vector3<T>& a, const Vector3<T>& b)
{
	return a.sub(b);
}
template <class T> IC Vector3<T>& operator-=(Vector3<T>& a, T s)
{
	return a.sub(s);
}
template <class T> IC Vector3<T>& operator*=(Vector3<T>& a, const Vector3<T>& b)
{
	return a.mul(b);
}
template <class T> IC Vector3<T>& operator*=(Vector3<T>& a, T s)
{
	return a.mul(s);
}
template <class T> IC Vector3<T>& operator/=(Vector3<T>& a, const Vector3<T>& b)
{
	return a.div(b);
}
template <class T> IC Vector3<T>& operator/=(Vector3<T>& a, T s)
{
	return a.div(s);
}

XRAY_END
