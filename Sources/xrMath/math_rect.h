#pragma once

#include "xrMathCommon.h"
#include "math_float2.h"
#include "math_utils.h" // для fsimilar, is_zero, clamp и т.п.
#include <algorithm>
#include <limits>

XRAY_BEGIN

// -----------------------------------------------------------------------------
// 2D прямоугольник (axis-aligned bounding box)
// -----------------------------------------------------------------------------
template <class T> struct Rect
{
	using Self = Rect<T>;
	using Vector2 = Vector2<T>;

	union {
		struct
		{
			T x1, y1, x2, y2;
		};
		struct
		{
			T left, top, right, bottom;
		};
		struct
		{
			Vector2 lt;
			Vector2 rb;
		}; // left-top, right-bottom (аналог min, max)
		T m[4];
	};

	// Конструкторы
	Rect() = default;
	Rect(const Vector2& _lt, const Vector2& _rb) : lt(_lt), rb(_rb)
	{
	}
	Rect(T _x1, T _y1, T _x2, T _y2) : x1(_x1), y1(_y1), x2(_x2), y2(_y2)
	{
	}

	// ---------- Базовые установки (из _rect) ----------
	Self& set(T _x1, T _y1, T _x2, T _y2)
	{
		x1 = _x1;
		y1 = _y1;
		x2 = _x2;
		y2 = _y2;
		return *this;
	}
	Self& set(const Vector2& _lt, const Vector2& _rb)
	{
		lt = _lt;
		rb = _rb;
		return *this;
	}
	Self& set(const Self& other)
	{
		lt = other.lt;
		rb = other.rb;
		return *this;
	}
	Self& null()
	{
		lt.set(0, 0);
		rb.set(0, 0);
		return *this;
	}

	// ---------- Методы из _box2 ----------
	// Единичный прямоугольник с центром в 0 (полуразмер 0.5)
	Self& identity()
	{
		lt.set(T(-0.5), T(-0.5));
		rb.set(T(0.5), T(0.5));
		return *this;
	}

	// Сброс в "невалидное" состояние (для накопления)
	Self& invalidate()
	{
		lt.set(std::numeric_limits<T>::max(), std::numeric_limits<T>::max());
		rb.set(std::numeric_limits<T>::lowest(), std::numeric_limits<T>::lowest());
		return *this;
	}

	// Сжатие/расширение
	Self& shrink(T s)
	{
		lt.x += s;
		lt.y += s;
		rb.x -= s;
		rb.y -= s;
		return *this;
	}
	Self& shrink(const Vector2& s)
	{
		lt += s;
		rb -= s;
		return *this;
	}
	Self& grow(T s)
	{
		lt.x -= s;
		lt.y -= s;
		rb.x += s;
		rb.y += s;
		return *this;
	}
	Self& grow(const Vector2& s)
	{
		lt -= s;
		rb += s;
		return *this;
	}

	// Смещение всего прямоугольника
	Self& add(const Vector2& v)
	{
		lt += v;
		rb += v;
		return *this;
	}
	Self& offset(const Vector2& v)
	{
		return add(v);
	} // синоним
	Self& add(const Self& b, const Vector2& v)
	{
		lt = b.lt + v;
		rb = b.rb + v;
		return *this;
	}

	// Проверка принадлежности точки
	bool in(T x, T y) const
	{
		return (x >= x1) && (x <= x2) && (y >= y1) && (y <= y2);
	}
	bool in(const Vector2& p) const
	{
		return in(p.x, p.y);
	}

	// Проверка содержания другого прямоугольника
	bool contains(const Self& b) const
	{
		return in(b.lt) && in(b.rb);
	}

	// Сравнение с эпсилон
	bool similar(const Self& b, T eps = static_cast<T>(EPS)) const
	{
		return lt.similar(b.lt, eps) && rb.similar(b.rb, eps);
	}

	// Расширение прямоугольника, чтобы включить точку
	Self& modify(const Vector2& p)
	{
		lt.x = (std::min)(lt.x, p.x);
		lt.y = (std::min)(lt.y, p.y);
		rb.x = (std::max)(rb.x, p.x);
		rb.y = (std::max)(rb.y, p.y);
		return *this;
	}
	Self& merge(const Self& b)
	{
		modify(b.lt);
		modify(b.rb);
		return *this;
	}
	Self& merge(const Self& b1, const Self& b2)
	{
		invalidate();
		merge(b1);
		merge(b2);
		return *this;
	}

	// Размеры и центр
	void getsize(Vector2& sz) const
	{
		sz = rb - lt;
	}
	Vector2 getsize() const
	{
		return rb - lt;
	}
	void getradius(Vector2& rad) const
	{
		rad = (rb - lt) * T(0.5);
	}
	Vector2 getradius() const
	{
		return (rb - lt) * T(0.5);
	}
	T getradius_scalar() const
	{
		return getradius().magnitude();
	}

	void getcenter(Vector2& center) const
	{
		center = (lt + rb) * T(0.5);
	}
	Vector2 getcenter() const
	{
		return (lt + rb) * T(0.5);
	}

	// Описанная окружность
	void getsphere(Vector2& center, T& radius) const
	{
		center = getcenter();
		radius = center.distance_to(rb);
	}

	// Проверка пересечения с другим прямоугольником
	bool intersect(const Self& other) const
	{
		if (rb.x < other.lt.x || lt.x > other.rb.x)
			return false;
		if (rb.y < other.lt.y || lt.y > other.rb.y)
			return false;
		return true;
	}

	// Упорядочивание min/max (делает прямоугольник валидным)
	Self& sort()
	{
		if (lt.x > rb.x)
			std::swap(lt.x, rb.x);
		if (lt.y > rb.y)
			std::swap(lt.y, rb.y);
		return *this;
	}

	// ---------- Пересечение с лучом ----------
	// Простой тест пересечения луча (start + t*dir) с прямоугольником
	bool pick_ray(const Vector2& start, const Vector2& dir) const
	{
		// Алгоритм slab для 2D
		T tmin = (lt.x - start.x) / dir.x;
		T tmax = (rb.x - start.x) / dir.x;
		if (tmin > tmax)
			std::swap(tmin, tmax);

		T tymin = (lt.y - start.y) / dir.y;
		T tymax = (rb.y - start.y) / dir.y;
		if (tymin > tymax)
			std::swap(tymin, tymax);

		if ((tmin > tymax) || (tymin > tmax))
			return false;
		return true;
	}

	// Более точный тест с учётом эпсилон (pick_exact)
	bool pick_ray_exact(const Vector2& start, const Vector2& dir, T eps = static_cast<T>(EPS)) const
	{
		Vector2 rvmin = lt - start;
		Vector2 rvmax = rb - start;

		if (!is_zero(dir.x, eps))
		{
			T alpha = rvmin.x / dir.x;
			T yt = alpha * dir.y;
			if (yt >= rvmin.y - eps && yt <= rvmax.y + eps)
				return true;
			alpha = rvmax.x / dir.x;
			yt = alpha * dir.y;
			if (yt >= rvmin.y - eps && yt <= rvmax.y + eps)
				return true;
		}
		if (!is_zero(dir.y, eps))
		{
			T alpha = rvmin.y / dir.y;
			T xt = alpha * dir.x;
			if (xt >= rvmin.x - eps && xt <= rvmax.x + eps)
				return true;
			alpha = rvmax.y / dir.y;
			xt = alpha * dir.x;
			if (xt >= rvmin.x - eps && xt <= rvmax.x + eps)
				return true;
		}
		return false;
	}

	// Пересечение с лучом с возвратом точки (аналог Pick2)
	bool pick_ray(const Vector2& origin, const Vector2& dir, Vector2& coord) const
	{
		bool inside = true;
		Vector2 maxT(-1, -1); // расстояния до плоскостей

		// Для каждой оси определяем, с какой стороны начало
		for (int axis = 0; axis < 2; ++axis)
		{
			T o = origin[axis];
			T lo = lt[axis];
			T hi = rb[axis];

			if (o < lo)
			{
				coord[axis] = lo;
				inside = false;
				if (dir[axis] > 0)
					maxT[axis] = (lo - o) / dir[axis];
			}
			else if (o > hi)
			{
				coord[axis] = hi;
				inside = false;
				if (dir[axis] < 0)
					maxT[axis] = (hi - o) / dir[axis];
			}
		}

		if (inside)
		{
			coord = origin;
			return true;
		}

		// Выбираем ось с максимальным расстоянием
		int which = (maxT.y > maxT.x) ? 1 : 0;
		if (maxT[which] < 0)
			return false;

		// Проверяем, что точка пересечения лежит в пределах другой оси
		if (which == 0)
		{
			T y = origin.y + maxT.x * dir.y;
			if (y < lt.y || y > rb.y)
				return false;
			coord.y = y;
		}
		else
		{
			T x = origin.x + maxT.y * dir.x;
			if (x < lt.x || x > rb.x)
				return false;
			coord.x = x;
		}
		return true;
	}

	// Получение вершины по индексу (0..3)
	void get_point(int idx, Vector2& pt) const
	{
		switch (idx)
		{
		case 0:
			pt.set(lt.x, lt.y);
			break; // min, min
		case 1:
			pt.set(lt.x, rb.y);
			break; // min, max
		case 2:
			pt.set(rb.x, rb.y);
			break; // max, max
		case 3:
			pt.set(rb.x, lt.y);
			break; // max, min
		default:
			pt.set(0, 0);
			break;
		}
	}
	Vector2 get_point(int idx) const
	{
		Vector2 pt;
		get_point(idx, pt);
		return pt;
	}

	// Получение всех 4 вершин
	void get_points(Vector2* pts) const
	{
		pts[0].set(lt.x, lt.y);
		pts[1].set(lt.x, rb.y);
		pts[2].set(rb.x, rb.y);
		pts[3].set(rb.x, lt.y);
	}
};

// Типовые определения
using rect = Rect<float>;
using frect = Rect<float>;
using drect = Rect<double>;
using irect = Rect<int>;

// Синонимы для совместимости с _box2
using box2 = Rect<float>;
using fbox2 = Rect<float>;
using dbox2 = Rect<double>;

// Проверка на корректность (NaN/Inf)
template <class T> inline bool valid(const Rect<T>& r)
{
	return valid(r.lt) && valid(r.rb);
}

XRAY_END
