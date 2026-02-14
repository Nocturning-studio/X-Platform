#pragma once

#include "xrMathCommon.h"
#include "math_float3.h"
#include "math_float4x4.h"	// для transform
#include "math_constants.h" // для EPS
#include <cmath>
#include <limits>
#include <algorithm> // для std::min, std::max

XRAY_BEGIN

template <class T> struct AABB
{
	using Self = AABB<T>;
	using Vector3 = Vector3<T>;

	union {
		struct
		{
			Vector3 min;
			Vector3 max;
		};
		struct
		{
			T x1, y1, z1;
			T x2, y2, z2;
		};
	};

	// Сброс в "невалидное" состояние (для начала накопления точек)
	Self& invalidate()
	{
		min.set(std::numeric_limits<T>::max(), std::numeric_limits<T>::max(), std::numeric_limits<T>::max());
		max.set(std::numeric_limits<T>::lowest(), std::numeric_limits<T>::lowest(), std::numeric_limits<T>::lowest());
		return *this;
	}

	// Расширение бокса, чтобы включить точку
	Self& extend(const Vector3& p)
	{
		if (p.x < min.x)
			min.x = p.x;
		if (p.x > max.x)
			max.x = p.x;
		if (p.y < min.y)
			min.y = p.y;
		if (p.y > max.y)
			max.y = p.y;
		if (p.z < min.z)
			min.z = p.z;
		if (p.z > max.z)
			max.z = p.z;
		return *this;
	}

	// Расширение, чтобы включить другой бокс
	Self& extend(const Self& box)
	{
		if (box.min.x < min.x)
			min.x = box.min.x;
		if (box.max.x > max.x)
			max.x = box.max.x;
		if (box.min.y < min.y)
			min.y = box.min.y;
		if (box.max.y > max.y)
			max.y = box.max.y;
		if (box.min.z < min.z)
			min.z = box.min.z;
		if (box.max.z > max.z)
			max.z = box.max.z;
		return *this;
	}

	// Центр бокса
	void get_center(Vector3& center) const
	{
		center = (min + max) * T(0.5);
	}
	Vector3 get_center() const
	{
		return (min + max) * T(0.5);
	}

	// Размер бокса (диагональ)
	void get_size(Vector3& size) const
	{
		size = max - min;
	}
	Vector3 get_size() const
	{
		return max - min;
	}

	// Проверка пересечения с другим AABB
	bool intersect(const Self& other) const
	{
		if (max.x < other.min.x || min.x > other.max.x)
			return false;
		if (max.y < other.min.y || min.y > other.max.y)
			return false;
		if (max.z < other.min.z || min.z > other.max.z)
			return false;
		return true;
	}

	// Проверка содержания точки внутри
	bool contains(const Vector3& p) const
	{
		return (p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y && p.z >= min.z && p.z <= max.z);
	}

	// Проверка, корректен ли бокс (min <= max)
	bool is_valid() const
	{
		return (x2 >= x1) && (y2 >= y1) && (z2 >= z1);
	}

	// Указатель на первый элемент (для передачи в API)
	const T* data() const
	{
		return &x1;
	}

	// Установка значений
	Self& set(const Vector3& _min, const Vector3& _max)
	{
		min = _min;
		max = _max;
		return *this;
	}
	Self& set(T _x1, T _y1, T _z1, T _x2, T _y2, T _z2)
	{
		x1 = _x1;
		y1 = _y1;
		z1 = _z1;
		x2 = _x2;
		y2 = _y2;
		z2 = _z2;
		return *this;
	}
	Self& set(const Self& b)
	{
		min = b.min;
		max = b.max;
		return *this;
	}
	Self& set_center_dim(const Vector3& center, const Vector3& dim)
	{
		min = center - dim;
		max = center + dim;
		return *this;
	}

	// Нулевой бокс (все координаты нули)
	Self& null()
	{
		min.set(0, 0, 0);
		max.set(0, 0, 0);
		return *this;
	}

	// Единичный бокс с центром в 0 (полуразмер 0.5)
	Self& identity()
	{
		min.set(T(-0.5), T(-0.5), T(-0.5));
		max.set(T(0.5), T(0.5), T(0.5));
		return *this;
	}

	// Сжатие/расширение
	Self& shrink(T s)
	{
		min.x += s;
		min.y += s;
		min.z += s;
		max.x -= s;
		max.y -= s;
		max.z -= s;
		return *this;
	}
	Self& shrink(const Vector3& s)
	{
		min += s;
		max -= s;
		return *this;
	}
	Self& grow(T s)
	{
		min.x -= s;
		min.y -= s;
		min.z -= s;
		max.x += s;
		max.y += s;
		max.z += s;
		return *this;
	}
	Self& grow(const Vector3& s)
	{
		min -= s;
		max += s;
		return *this;
	}

	// Смещение всего бокса
	Self& add(const Vector3& v)
	{
		min += v;
		max += v;
		return *this;
	}
	Self& sub(const Vector3& v)
	{
		min -= v;
		max -= v;
		return *this;
	}
	Self& offset(const Vector3& v)
	{
		return add(v);
	}
	Self& add(const Self& b, const Vector3& v)
	{
		min = b.min + v;
		max = b.max + v;
		return *this;
	}

	// Проверка содержания другого бокса
	bool contains(const Self& b) const
	{
		return contains(b.min) && contains(b.max);
	}

	// Сравнение с эпсилон
	bool similar(const Self& b, T eps = static_cast<T>(EPS)) const
	{
		return min.similar(b.min, eps) && max.similar(b.max, eps);
	}

	// Синонимы extend
	Self& modify(const Vector3& p)
	{
		return extend(p);
	}
	Self& modify(T x, T y, T z)
	{
		return extend(Vector3(x, y, z));
	}
	Self& merge(const Self& b)
	{
		return extend(b);
	}

	// Установка в объединение двух боксов
	Self& merge(const Self& b1, const Self& b2)
	{
		invalidate();
		extend(b1);
		extend(b2);
		return *this;
	}

	// Преобразование бокса матрицей (AABB -> OBB, но результат остаётся AABB)
	Self& transform(const Self& src, const Matrix4x4<T>& m)
	{
		// Три ребра исходного бокса, преобразованные через матрицу (без учёта трансляции)
		Vector3 vx = m.transform_dir(Vector3(src.max.x - src.min.x, 0, 0));
		Vector3 vy = m.transform_dir(Vector3(0, src.max.y - src.min.y, 0));
		Vector3 vz = m.transform_dir(Vector3(0, 0, src.max.z - src.min.z));

		// Преобразуем точку минимума
		Vector3 new_min = m.transform(src.min);
		Vector3 new_max = new_min; // начинаем с неё

		// Функция для добавления вектора с учётом знака (расширение AABB)
		auto add_axis = [&](const Vector3& v) {
			if (v.x < 0)
				new_min.x += v.x;
			else
				new_max.x += v.x;
			if (v.y < 0)
				new_min.y += v.y;
			else
				new_max.y += v.y;
			if (v.z < 0)
				new_min.z += v.z;
			else
				new_max.z += v.z;
		};

		add_axis(vx);
		add_axis(vy);
		add_axis(vz);

		min = new_min;
		max = new_max;
		return *this;
	}

	// Преобразование текущего бокса (this = transform(*this, m))
	Self& transform(const Matrix4x4<T>& m)
	{
		Self tmp = *this;
		return transform(tmp, m);
	}

	// Получение размера (синоним get_size)
	void getsize(Vector3& sz) const
	{
		sz = max - min;
	}
	Vector3 getsize() const
	{
		return max - min;
	}

	// Получение половины размера (радиус по каждой оси)
	void getradius(Vector3& r) const
	{
		r = (max - min) * T(0.5);
	}
	Vector3 getradius() const
	{
		return (max - min) * T(0.5);
	}
	T getradius_scalar() const
	{
		return getradius().magnitude();
	} // длина полудиагонали

	// Объём
	T getvolume() const
	{
		Vector3 sz = getsize();
		return sz.x * sz.y * sz.z;
	}

	// Центр и половинные размеры (half-dimensions)
	void get_CD(Vector3& center, Vector3& half_dim) const
	{
		half_dim = (max - min) * T(0.5);
		center = min + half_dim;
	}

	// Масштабирование относительно центра: s – коэффициент изменения половинных размеров
	Self& scale(T s)
	{
		Vector3 half = getradius();
		half.mul(s);
		Vector3 center = get_center();
		min = center - half;
		max = center + half;
		return *this;
	}

	// Описанная сфера
	void get_sphere(Vector3& center, T& radius) const
	{
		center = get_center();
		radius = center.distance_to(max);
	}

	// Проверка пересечения луча (быстрый тест)
	bool pick_ray(const Vector3& start, const Vector3& dir) const
	{
		// Алгоритм slab
		T tmin = (min.x - start.x) / dir.x;
		T tmax = (max.x - start.x) / dir.x;
		if (tmin > tmax)
			std::swap(tmin, tmax);

		T tymin = (min.y - start.y) / dir.y;
		T tymax = (max.y - start.y) / dir.y;
		if (tymin > tymax)
			std::swap(tymin, tymax);

		if ((tmin > tymax) || (tymin > tmax))
			return false;
		if (tymin > tmin)
			tmin = tymin;
		if (tymax < tmax)
			tmax = tymax;

		T tzmin = (min.z - start.z) / dir.z;
		T tzmax = (max.z - start.z) / dir.z;
		if (tzmin > tzmax)
			std::swap(tzmin, tzmax);

		if ((tmin > tzmax) || (tzmin > tmax))
			return false;
		return true;
	}

	// Более полный тест пересечения луча с возвратом точки и классификацией
	enum ERP_Result
	{
		rpNone = 0,
		rpOriginInside = 1,
		rpOriginOutside = 2
	};

	ERP_Result pick_ray(const Vector3& origin, const Vector3& dir, Vector3& coord) const
	{
		bool inside = true;
		Vector3 maxT(-1, -1, -1); // расстояния до плоскостей, если луч идёт снаружи

		// Для каждой оси определяем, с какой стороны начало
		for (int axis = 0; axis < 3; ++axis)
		{
			T o = origin[axis];
			T lo = min[axis];
			T hi = max[axis];

			if (o < lo)
			{
				coord[axis] = lo;
				inside = false;
				if (dir[axis] > 0) // луч направлен в сторону бокса
					maxT[axis] = (lo - o) / dir[axis];
			}
			else if (o > hi)
			{
				coord[axis] = hi;
				inside = false;
				if (dir[axis] < 0) // луч направлен в сторону бокса
					maxT[axis] = (hi - o) / dir[axis];
			}
		}

		if (inside)
		{
			coord = origin;
			return rpOriginInside;
		}

		// Находим ось с максимальным расстоянием (кандидат на пересечение)
		int which = 0;
		if (maxT.y > maxT.x)
			which = 1;
		if (maxT.z > maxT[which])
			which = 2;

		if (maxT[which] < 0)
			return rpNone; // все расстояния отрицательные (луч идёт в обратную сторону)

		// Проверяем, что точка пересечения лежит в пределах двух других осей
		if (which == 0)
		{
			T y = origin.y + maxT.x * dir.y;
			if (y < min.y || y > max.y)
				return rpNone;
			T z = origin.z + maxT.x * dir.z;
			if (z < min.z || z > max.z)
				return rpNone;
			coord.y = y;
			coord.z = z;
		}
		else if (which == 1)
		{
			T x = origin.x + maxT.y * dir.x;
			if (x < min.x || x > max.x)
				return rpNone;
			T z = origin.z + maxT.y * dir.z;
			if (z < min.z || z > max.z)
				return rpNone;
			coord.x = x;
			coord.z = z;
		}
		else // which == 2
		{
			T x = origin.x + maxT.z * dir.x;
			if (x < min.x || x > max.x)
				return rpNone;
			T y = origin.y + maxT.z * dir.y;
			if (y < min.y || y > max.y)
				return rpNone;
			coord.x = x;
			coord.y = y;
		}

		return rpOriginOutside;
	}

	// Получение вершины по индексу (0..7)
	void get_point(int idx, Vector3& pt) const
	{
		switch (idx)
		{
		case 0:
			pt.set(x1, y1, z1);
			break;
		case 1:
			pt.set(x1, y1, z2);
			break;
		case 2:
			pt.set(x2, y1, z2);
			break;
		case 3:
			pt.set(x2, y1, z1);
			break;
		case 4:
			pt.set(x1, y2, z1);
			break;
		case 5:
			pt.set(x1, y2, z2);
			break;
		case 6:
			pt.set(x2, y2, z2);
			break;
		case 7:
			pt.set(x2, y2, z1);
			break;
		default:
			pt.set(0, 0, 0);
			break;
		}
	}
	Vector3 get_point(int idx) const
	{
		Vector3 pt;
		get_point(idx, pt);
		return pt;
	}

	// Получение всех 8 вершин
	void get_points(Vector3* pts) const
	{
		pts[0].set(x1, y1, z1);
		pts[1].set(x1, y1, z2);
		pts[2].set(x2, y1, z2);
		pts[3].set(x2, y1, z1);
		pts[4].set(x1, y2, z1);
		pts[5].set(x1, y2, z2);
		pts[6].set(x2, y2, z2);
		pts[7].set(x2, y2, z1);
	}

	// Расширение текущего бокса всеми вершинами исходного после преобразования матрицей
	Self& modify(const Self& src, const Matrix4x4<T>& m)
	{
		for (int i = 0; i < 8; ++i)
		{
			Vector3 pt = src.get_point(i);
			m.transform(pt);
			extend(pt);
		}
		return *this;
	}
};

// Типовые определения
using aabb = AABB<float>;
using box = AABB<float>;
using box3 = AABB<float>;
using fbox = AABB<float>;
using dbox = AABB<double>;

// Проверка на корректность (NaN/Inf)
template <class T> inline bool valid(const AABB<T>& b)
{
	return valid(b.min) && valid(b.max);
}

XRAY_END
