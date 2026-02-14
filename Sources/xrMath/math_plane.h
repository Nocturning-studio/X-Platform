#pragma once
#include "xrMathCommon.h"
#include "math_float3.h"
#include "math_float4x4.h"

XRAY_BEGIN

template <class T> struct Plane
{
	using Self = Plane<T>;
	using Vector3 = Vector3<T>;

	Vector3 n; // нормаль (единичная после нормализации)
	T d; // расстояние от начала координат до плоскости (со знаком): n·p + d = 0

	Self& set(T a, T b, T c, T _d)
	{
		n.set(a, b, c);
		d = _d;
		return *this;
	}

	// Построение плоскости по точке и нормали (нормаль будет нормализована)
	Self& build(const Vector3& point, const Vector3& normal)
	{
		n = normal;
		n.normalize();
		d = -n.dot(point);
		return *this;
	}

	// Построение по трём точкам (треугольнику)
	Self& build(const Vector3& v1, const Vector3& v2, const Vector3& v3)
	{
		Vector3 edge1 = v1 - v2;
		Vector3 edge2 = v1 - v3;
		n.cross(edge1, edge2);
		n.normalize();
		d = -n.dot(v1);
		return *this;
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

	// Классификация точки относительно плоскости: >0 спереди, <0 сзади, 0 на плоскости
	T classify(const Vector3& v) const
	{
		return n.dot(v) + d;
	}

	// Расстояние до плоскости (абсолютное)
	T distance(const Vector3& v) const
	{
		return std::abs(classify(v));
	}

	// Сравнение двух плоскостей с заданными допусками для нормали и расстояния
	bool similar(const Self& other, T eps_n = static_cast<T>(EPS), T eps_d = static_cast<T>(EPS)) const
	{
		return n.similar(other.n, eps_n) && (std::abs(d - other.d) < eps_d);
	}

	// Построение плоскости по трём точкам с использованием точной нормализации (exact_normalize)
	// Для типа float используется exact_normalize, для других типов — обычная нормализация.
	Self& build_precise(const Vector3& v1, const Vector3& v2, const Vector3& v3)
	{
		Vector3 edge1 = v1 - v2;
		Vector3 edge2 = v1 - v3;
		n.cross(edge1, edge2);
		precise_normalize_impl(n, typename std::is_same<T, float>::type());
		d = -n.dot(v1);
		return *this;
	}

	// Построение плоскости по точке и нормали, предполагая, что нормаль уже единичная
	Self& build_unit_normal(const Vector3& point, const Vector3& unit_normal)
	{
		// Можно добавить проверку в отладочной сборке: assert(unit_normal.magnitude() близка к 1)
		n = unit_normal;
		d = -n.dot(point);
		return *this;
	}

	// Проецирование точки на плоскость (pdest = psrc - classify(psrc) * n)
	Self& project(Vector3& pdest, const Vector3& psrc) const
	{
		pdest = psrc;
		pdest.mad(n, -classify(psrc)); // pdest = psrc - classify(psrc)*n
		return *this;
	}

	// Пересечение луча с плоскостью (луч: P + t*D, t>0). Возвращает true и расстояние dist, если пересечение есть.
	bool intersect_ray_dist(const Vector3& P, const Vector3& D, T& dist) const
	{
		T numer = classify(P);
		T denom = n.dot(D);

		if (std::abs(denom) < std::numeric_limits<T>::epsilon()) // плоскость параллельна лучу
			return false;

		dist = -numer / denom;
		return (dist > 0 || std::abs(dist) <= std::numeric_limits<T>::epsilon());
	}

	// Пересечение луча с плоскостью с вычислением точки пересечения.
	bool intersect_ray_point(const Vector3& P, const Vector3& D, Vector3& dest) const
	{
		T dist;
		if (!intersect_ray_dist(P, D, dist))
			return false;
		dest.mad(P, D, dist);
		return true;
	}

	// Пересечение отрезка [u, v] с плоскостью. Возвращает true и точку пересечения isect.
	bool intersect_segment(const Vector3& u, const Vector3& v, Vector3& isect) const
	{
		T denom, dist;
		Vector3 dir = v - u;
		denom = n.dot(dir);
		if (std::abs(denom) < std::numeric_limits<T>::epsilon()) // отрезок параллелен плоскости
			return false;

		dist = -(n.dot(u) + d) / denom;
		if (dist < -std::numeric_limits<T>::epsilon() || dist > T(1) + std::numeric_limits<T>::epsilon())
			return false; // пересечение вне отрезка

		isect.mad(u, dir, dist);
		return true;
	}

	// Альтернативная версия пересечения отрезка
	bool intersect_segment_2(const Vector3& u, const Vector3& v, Vector3& isect) const
	{
		T dist1 = classify(u);
		T dist2 = classify(v);

		if (dist1 * dist2 >= 0) // оба с одной стороны или на плоскости
			return false;

		Vector3 dir = v - u;
		// t = dist1 / (dist1 - dist2)  (поскольку dist1 и dist2 разных знаков, знаменатель не ноль)
		T t = dist1 / (dist1 - dist2);
		isect.mad(u, dir, t);
		return true;
	}

	//   M.transform_dir(n);   // применяем поворот к нормали (без учёта обратного транспонирования)
	//   d -= M.c.dot(n);      // корректируем d на трансляцию
	// Это соответствует преобразованию плоскости вместе с объектом: если точка X преобразуется в X' = M*X
	Self& transform(const Matrix4x4<T>& M)
	{
		// Поворачиваем нормаль (используем transform_dir, т.е. без учёта переноса)
		Vector3 new_n = n;
		M.transform_dir(new_n);
		n = new_n;
		// Корректируем расстояние: d' = d - translation·n
		d -= M.c.dot(n);
		return *this;
	}
};

// Конкретные типы
using plane = Plane<float>;
using dplane = Plane<double>;

// Проверка на корректность (NaN/Inf)
template <class T> inline bool valid(const Plane<T>& pl)
{
	return valid(pl.n) && std::isfinite(pl.d);
}

XRAY_END
