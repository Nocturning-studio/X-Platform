#pragma once

#include <string>
#include <sstream>

template <class T> struct template_vector2
{
  public:
	typedef T TYPE;
	typedef template_vector2<T> Self;
	typedef Self& SelfRef;
	typedef const Self& SelfCRef;

  public:
	T x, y;

	IC SelfRef set(float _u, float _v)
	{
		x = T(_u);
		y = T(_v);
		return *this;
	}
	IC SelfRef set(double _u, double _v)
	{
		x = T(_u);
		y = T(_v);
		return *this;
	}
	IC SelfRef set(int _u, int _v)
	{
		x = T(_u);
		y = T(_v);
		return *this;
	}
	IC SelfRef set(const Self& p)
	{
		x = p.x;
		y = p.y;
		return *this;
	}
	IC SelfRef abs(const Self& p)
	{
		x = std::abs(p.x);
		y = std::abs(p.y);
		return *this;
	}
	IC SelfRef min(const Self& p)
	{
		x = std::min(x, p.x);
		y = std::min(y, p.y);
		return *this;
	}
	IC SelfRef min(T _x, T _y)
	{
		x = std::min(x, _x);
		y = std::min(y, _y);
		return *this;
	}
	IC SelfRef max(const Self& p)
	{
		x = std::max(x, p.x);
		y = std::max(y, p.y);
		return *this;
	}
	IC SelfRef max(T _x, T _y)
	{
		x = std::max(x, _x);
		y = std::max(y, _y);
		return *this;
	}
	IC SelfRef sub(T p)
	{
		x -= p;
		y -= p;
		return *this;
	}
	IC SelfRef sub(const Self& p)
	{
		x -= p.x;
		y -= p.y;
		return *this;
	}
	IC SelfRef sub(const Self& p1, const Self& p2)
	{
		x = p1.x - p2.x;
		y = p1.y - p2.y;
		return *this;
	}
	IC SelfRef sub(const Self& p, T d)
	{
		x = p.x - d;
		y = p.y - d;
		return *this;
	}
	IC SelfRef sub(T s, const Self& p)
	{
		x = s - p.x;
		y = s - p.y;
		return *this;
	}
	IC SelfRef add(T p)
	{
		x += p;
		y += p;
		return *this;
	}
	IC SelfRef add(const Self& p)
	{
		x += p.x;
		y += p.y;
		return *this;
	}
	IC SelfRef add(const Self& p1, const Self& p2)
	{
		x = p1.x + p2.x;
		y = p1.y + p2.y;
		return *this;
	}
	IC SelfRef add(const Self& p, T d)
	{
		x = p.x + d;
		y = p.y + d;
		return *this;
	}
	IC SelfRef mul(const T s)
	{
		x *= s;
		y *= s;
		return *this;
	}
	IC SelfRef mul(const Self& p)
	{
		x *= p.x;
		y *= p.y;
		return *this;
	}
	IC SelfRef div(const T s)
	{
		x /= s;
		y /= s;
		return *this;
	}
	IC SelfRef div(const Self& p)
	{
		x /= p.x;
		y /= p.y;
		return *this;
	}
	IC SelfRef invert()
	{
		x = -x;
		y = -y;
		return *this;
	}
	IC SelfRef invert(const Self& a)
	{
		x = -a.x;
		y = -a.y;
		return *this;
	}
	IC SelfRef rot90(void)
	{
		float t = -x;
		x = y;
		y = t;
		return *this;
	}
	IC SelfRef cross(const Self& D)
	{
		x = D.y;
		y = -D.x;
		return *this;
	}
	IC Self cross()
	{
		// vector3 orthogonal to (x,y) is (y,-x)
		Self kCross;
		kCross.x = y;
		kCross.y = -x;
		return kCross;
	}
	IC T dot(const Self& p)
	{
		return x * p.x + y * p.y;
	}
	IC T dot(const Self& p) const
	{
		return x * p.x + y * p.y;
	}
	IC SelfRef norm(void)
	{
		float m = std::sqrt(x * x + y * y);
		x /= m;
		y /= m;
		return *this;
	}
	IC SelfRef norm_safe(void)
	{
		float m = std::sqrt(x * x + y * y);
		if (m)
		{
			x /= m;
			y /= m;
		}
		return *this;
	}
	IC T distance_to(const Self& p) const
	{
		return std::sqrt((x - p.x) * (x - p.x) + (y - p.y) * (y - p.y));
	}
	IC T square_magnitude(void) const
	{
		return x * x + y * y;
	}
	IC T magnitude(void) const
	{
		return std::sqrt(square_magnitude());
	}

	IC SelfRef mad(const Self& p, const Self& d, T r)
	{
		x = p.x + d.x * r;
		y = p.y + d.y * r;
		return *this;
	}

	IC bool similar(const Self& p, T eu, T ev) const
	{
		return std::abs(x - p.x) <= eu && std::abs(y - p.y) <= ev;
	}

	IC bool similar(const Self& p, T E = EPS_L) const
	{
		return std::abs(x - p.x) <= E && std::abs(y - p.y) <= E;
	};

	// average arithmetic
	IC SelfRef averageA(const Self& p1, const Self& p2)
	{
		x = (p1.x + p2.x) * T(0.5);
		y = (p1.y + p2.y) * T(0.5);
		return *this;
	}
	// average geometric
	IC SelfRef averageG(const Self& p1, const Self& p2)
	{
		x = std::sqrt(p1.x * p2.x);
		y = std::sqrt(p1.y * p2.y);
		return *this;
	}

	T& operator[](int i) const
	{
		return (T&)*(&x + i);
	}

	IC SelfRef normalize(void)
	{
		return norm();
	}
	IC SelfRef normalize_safe(void)
	{
		return norm_safe();
	}
	IC SelfRef normalize(const Self& v)
	{
		float m = std::sqrt(v.x * v.x + v.y * v.y);
		x = v.x / m;
		y = v.y / m;
		return *this;
	}
	IC SelfRef normalize_safe(const Self& v)
	{
		float m = std::sqrt(v.x * v.x + v.y * v.y);
		if (m)
		{
			x = v.x / m;
			y = v.y / m;
		}
		return *this;
	}
	IC float dotproduct(const Self& p) const
	{
		return dot(p);
	}
	IC float crossproduct(const Self& p) const
	{
		return x * p.y - y * p.x;
	}
	IC float getH(void) const
	{
		if (fis_zero(y))
			if (fis_zero(x))
				return (0.f);
			else
				return ((x > 0.0f) ? -PI_DIV_2 : PI_DIV_2);
		else if (y < 0.f)
			return (-(atanf(x / y) - PI));
		else
			return (-atanf(x / y));
	}
};

template <class T>
IC template_vector2<T> operator-(const template_vector2<T>& a) {
	template_vector2<T> r(a); r.invert(); return r;
}

template <class T>
IC template_vector2<T> operator+(const template_vector2<T>& a, const template_vector2<T>& b) {
	template_vector2<T> r(a); r.add(b); return r;
}
template <class T>
IC template_vector2<T> operator+(const template_vector2<T>& a, T s) {
	template_vector2<T> r(a); r.add(s); return r;
}
template <class T>
IC template_vector2<T> operator+(T s, const template_vector2<T>& a) {
	template_vector2<T> r(a); r.add(s); return r;
}

template <class T>
IC template_vector2<T> operator-(const template_vector2<T>& a, const template_vector2<T>& b) {
	template_vector2<T> r(a); r.sub(b); return r;
}
template <class T>
IC template_vector2<T> operator-(const template_vector2<T>& a, T s) {
	template_vector2<T> r(a); r.sub(s); return r;
}
template <class T>
IC template_vector2<T> operator-(T s, const template_vector2<T>& a) {
	template_vector2<T> r; r.sub(s, a); return r;
}

template <class T>
IC template_vector2<T> operator*(const template_vector2<T>& a, const template_vector2<T>& b) {
	template_vector2<T> r(a); r.mul(b); return r;
}
template <class T>
IC template_vector2<T> operator*(const template_vector2<T>& a, T s) {
	template_vector2<T> r(a); r.mul(s); return r;
}
template <class T>
IC template_vector2<T> operator*(T s, const template_vector2<T>& a) {
	template_vector2<T> r(a); r.mul(s); return r;
}

template <class T>
IC template_vector2<T> operator/(const template_vector2<T>& a, const template_vector2<T>& b) {
	template_vector2<T> r(a); r.div(b); return r;
}
template <class T>
IC template_vector2<T> operator/(const template_vector2<T>& a, T s) {
	template_vector2<T> r(a); r.div(s); return r;
}
template <class T>
IC template_vector2<T> operator/(T s, const template_vector2<T>& a) {
	template_vector2<T> r; r.x = s / a.x; r.y = s / a.y; return r;
}

template <class T>
std::string to_string(const template_vector2<T>& v)
{
	std::ostringstream oss;
	oss << "(" << v.x << ", " << v.y << ")";
	return oss.str();
}

template <class T> BOOL _valid(const template_vector2<T>& v)
{
	return _valid((T)v.x) && _valid((T)v.y);
}

typedef template_vector2<float> fvec2;
typedef template_vector2<double> dvec2;
typedef template_vector2<int> ivec2;
