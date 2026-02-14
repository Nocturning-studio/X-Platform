#pragma once
#include "xrMathCommon.h"
#include "math_float2.h"
#include "math_constants.h"

XRAY_BEGIN

template <class T> struct Plane2
{
	using Self = Plane2<T>;
	using Vector2 = Vector2<T>;

	Vector2 n; // нормаль (единичная после нормализации)
	T d;	   // расстояние от начала координат до прямой: n·p + d = 0

	// Установка из другой плоскости
	Self& set(const Self& P)
	{
		n = P.n;
		d = P.d;
		return *this;
	}

	// Сравнение с допусками
	bool similar(const Self& P, T eps_n = static_cast<T>(EPS), T eps_d = static_cast<T>(EPS)) const
	{
		return n.similar(P.n, eps_n) && (std::abs(d - P.d) < eps_d);
	}

	// Построение плоскости по точке и нормали (нормаль будет нормализована)
	Self& build(const Vector2& point, const Vector2& normal)
	{
		n = normal;
		n.normalize();
		d = -n.dot(point);
		return *this;
	}

	// Проецирование точки на прямую
	Self& project(Vector2& pdest, const Vector2& psrc) const
	{
		pdest = psrc;
		pdest.mad(n, -classify(psrc)); // pdest = psrc - classify(psrc)*n
		return *this;
	}

	// Классификация точки: >0 спереди, <0 сзади, 0 на прямой
	T classify(const Vector2& v) const
	{
		return n.dot(v) + d;
	}

	// Нормализация плоскости (приведение |n| к 1)
	Self& normalize()
	{
		T mag = n.magnitude();
		if (mag > std::numeric_limits<T>::epsilon())
		{
			T inv = T(1) / mag;
			n.mul(inv);
			d *= inv;
		}
		return *this;
	}

	// Расстояние от точки до прямой (абсолютное)
	T distance(const Vector2& v) const
	{
		return std::abs(classify(v));
	}

	// Пересечение луча с прямой (луч: P + t*D, t>0). Возвращает true и расстояние dist.
	bool intersect_ray_dist(const Vector2& P, const Vector2& D, T& dist) const
	{
		T numer = classify(P);
		T denom = n.dot(D);

		if (std::abs(denom) < static_cast<T>(EPS_S)) // нормаль перпендикулярна направлению луча
			return false;

		dist = -numer / denom;
		return (dist > 0 || std::abs(dist) <= static_cast<T>(EPS));
	}

	// Пересечение луча с прямой с вычислением точки пересечения.
	bool intersect_ray_point(const Vector2& P, const Vector2& D, Vector2& dest) const
	{
		T dist;
		if (!intersect_ray_dist(P, D, dist))
			return false;
		dest.mad(P, D, dist);
		return true;
	}

	// Пересечение отрезка [u, v] с прямой. Возвращает true и точку пересечения isect.
	bool intersect_segment(const Vector2& u, const Vector2& v, Vector2& isect) const
	{
		Vector2 dir = v - u;
		T denom = n.dot(dir);
		if (std::abs(denom) < static_cast<T>(EPS)) // отрезок параллелен прямой
			return false;

		T t = -(n.dot(u) + d) / denom;
		if (t < -static_cast<T>(EPS) || t > T(1) + static_cast<T>(EPS))
			return false; // пересечение вне отрезка

		isect.mad(u, dir, t);
		return true;
	}

	// Альтернативная версия пересечения отрезка (из старой реализации intersect_2)
	bool intersect_segment_2(const Vector2& u, const Vector2& v, Vector2& isect) const
	{
		T dist1 = classify(u);
		T dist2 = classify(v);

		if (dist1 * dist2 >= 0) // оба с одной стороны или на прямой
			return false;

		Vector2 dir = v - u;
		// t = dist1 / (dist1 - dist2)  (знаменатель не ноль, так как знаки разные)
		T t = dist1 / (dist1 - dist2);
		isect.mad(u, dir, t);
		return true;
	}
};

// Конкретные типы
using plane2 = Plane2<float>;
using dplane2 = Plane2<double>;

// Проверка на корректность (NaN/Inf)
template <class T> inline bool valid(const Plane2<T>& pl)
{
	return valid(pl.n) && std::isfinite(pl.d);
}

XRAY_END
