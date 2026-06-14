#include <iostream>
#include <cassert>

#include "AfterMathInternal.h"

AFTERMATH_BEGIN

template<typename T>
class TemplateVector4
{
public:
    T x, y, z, w;

    constexpr TemplateVector4() : x(T()), y(T()), z(T()), w(T()) {}
    constexpr TemplateVector4(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_) {}
    constexpr TemplateVector4(T scalar) : x(scalar), y(scalar), z(scalar), w(scalar) {}
    TemplateVector4(const TemplateVector4&) = default;

    TemplateVector4& operator=(T value)
    {
        x = value;
        y = value;
        z = value;
        w = value;
        return *this;
    }

    T& operator[](int idx)
    {
        assert(idx >= 0 && idx <= 3);
        if (idx == 0) return x;
        if (idx == 1) return y;
        if (idx == 2) return z;
        return w;
    }
    const T& operator[](int idx) const
    {
        assert(idx >= 0 && idx <= 3);
        if (idx == 0) return x;
        if (idx == 1) return y;
        if (idx == 2) return z;
        return w;
    }

    TemplateVector4 operator+() const { return *this; }
    TemplateVector4 operator-() const { return TemplateVector4(-x, -y, -z, -w); }

    TemplateVector4& operator++() { ++x; ++y; ++z; ++w; return *this; }
    TemplateVector4& operator--() { --x; --y; --z; --w; return *this; }

    TemplateVector4 operator++(int) { TemplateVector4 tmp = *this; ++(*this); return tmp; }
    TemplateVector4 operator--(int) { TemplateVector4 tmp = *this; --(*this); return tmp; }

    TemplateVector4& operator+=(const TemplateVector4& other)
    {
        x += other.x; y += other.y; z += other.z; w += other.w;
        return *this;
    }
    TemplateVector4& operator-=(const TemplateVector4& other)
    {
        x -= other.x; y -= other.y; z -= other.z; w -= other.w;
        return *this;
    }
    TemplateVector4& operator*=(const TemplateVector4& other)
    {
        x *= other.x; y *= other.y; z *= other.z; w *= other.w;
        return *this;
    }
    TemplateVector4& operator/=(const TemplateVector4& other)
    {
        x /= other.x; y /= other.y; z /= other.z; w /= other.w;
        return *this;
    }

    TemplateVector4& operator*=(T scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
    TemplateVector4& operator/=(T scalar) { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }

    friend bool operator==(const TemplateVector4& a, const TemplateVector4& b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
    }
    friend bool operator!=(const TemplateVector4& a, const TemplateVector4& b)
    {
        return !(a == b);
    }
    friend std::ostream& operator<<(std::ostream& os, const TemplateVector4& v)
    {
        os << '(' << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ')';
        return os;
    }
};

template<typename T>
TemplateVector4<T> operator*(const TemplateVector4<T>& v, T scalar)
{
    return TemplateVector4<T>(v.x * scalar, v.y * scalar, v.z * scalar, v.w * scalar);
}

template<typename T>
TemplateVector4<T> operator*(T scalar, const TemplateVector4<T>& v)
{
    return v * scalar;
}

template<typename T>
TemplateVector4<T> operator/(const TemplateVector4<T>& v, T scalar)
{
    return TemplateVector4<T>(v.x / scalar, v.y / scalar, v.z / scalar, v.w / scalar);
}

AFTERMATH_END