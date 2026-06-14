#include <iostream>
#include <cassert>

#include "AfterMathInternal.h"

AFTERMATH_BEGIN

template<typename T>
class TemplateVector3
{
public:
    T x, y, z;

    constexpr TemplateVector3() : x(T()), y(T()), z(T()) {}
    constexpr TemplateVector3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}
    constexpr TemplateVector3(T scalar) : x(scalar), y(scalar), z(scalar) {}
    TemplateVector3(const TemplateVector3&) = default;

    TemplateVector3& operator=(T value)
    {
        x = value;
        y = value;
        z = value;
        return *this;
    }

    T& operator[](int idx)
    {
        assert(idx >= 0 && idx <= 2);
        if (idx == 0) return x;
        if (idx == 1) return y;
        return z;
    }
    const T& operator[](int idx) const
    {
        assert(idx >= 0 && idx <= 2);
        if (idx == 0) return x;
        if (idx == 1) return y;
        return z;
    }

    TemplateVector3 operator+() const { return *this; }
    TemplateVector3 operator-() const { return TemplateVector3(-x, -y, -z); }

    TemplateVector3& operator++() { ++x; ++y; ++z; return *this; }
    TemplateVector3& operator--() { --x; --y; --z; return *this; }

    TemplateVector3 operator++(int) { TemplateVector3 tmp = *this; ++(*this); return tmp; }
    TemplateVector3 operator--(int) { TemplateVector3 tmp = *this; --(*this); return tmp; }

    TemplateVector3& operator+=(const TemplateVector3& other)
    {
        x += other.x; y += other.y; z += other.z;
        return *this;
    }
    TemplateVector3& operator-=(const TemplateVector3& other)
    {
        x -= other.x; y -= other.y; z -= other.z;
        return *this;
    }
    TemplateVector3& operator*=(const TemplateVector3& other)
    {
        x *= other.x; y *= other.y; z *= other.z;
        return *this;
    }
    TemplateVector3& operator/=(const TemplateVector3& other)
    {
        x /= other.x; y /= other.y; z /= other.z;
        return *this;
    }

    TemplateVector3& operator*=(T scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
    TemplateVector3& operator/=(T scalar) { x /= scalar; y /= scalar; z /= scalar; return *this; }

    friend bool operator==(const TemplateVector3& a, const TemplateVector3& b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
    friend bool operator!=(const TemplateVector3& a, const TemplateVector3& b)
    {
        return !(a == b);
    }
    friend std::ostream& operator<<(std::ostream& os, const TemplateVector3& v)
    {
        os << '(' << v.x << ", " << v.y << ", " << v.z << ')';
        return os;
    }
};

template<typename T>
TemplateVector3<T> operator*(const TemplateVector3<T>& v, T scalar)
{
    return TemplateVector3<T>(v.x * scalar, v.y * scalar, v.z * scalar);
}

template<typename T>
TemplateVector3<T> operator*(T scalar, const TemplateVector3<T>& v)
{
    return v * scalar;
}

template<typename T>
TemplateVector3<T> operator/(const TemplateVector3<T>& v, T scalar)
{
    return TemplateVector3<T>(v.x / scalar, v.y / scalar, v.z / scalar);
}

AFTERMATH_END
