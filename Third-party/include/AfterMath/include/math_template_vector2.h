#include <iostream>
#include <cassert>

#include "AfterMathInternal.h"

AFTERMATH_BEGIN

template<typename T>
class TemplateVector2
{
public:
    T x, y;

    constexpr TemplateVector2() : x(T()), y(T()) {}
    constexpr TemplateVector2(T x_, T y_) : x(x_), y(y_) {}
    constexpr TemplateVector2(T scalar) : x(scalar), y(scalar) {}
    TemplateVector2(const TemplateVector2&) = default;

    TemplateVector2& operator=(T value)
    {
        x = value;
        y = value;
        return *this;
    }

    T& operator[](int idx)
    {
        assert(idx >= 0 && idx <= 1);
        return (idx == 0) ? x : y;
    }
    const T& operator[](int idx) const
    {
        assert(idx >= 0 && idx <= 1);
        return (idx == 0) ? x : y;
    }

    TemplateVector2 operator+() const { return *this; }
    TemplateVector2 operator-() const { return TemplateVector2(-x, -y); }

    TemplateVector2& operator++() { ++x; ++y; return *this; }
    TemplateVector2& operator--() { --x; --y; return *this; }

    TemplateVector2 operator++(int) { TemplateVector2 tmp = *this; ++(*this); return tmp; }
    TemplateVector2 operator--(int) { TemplateVector2 tmp = *this; --(*this); return tmp; }

    TemplateVector2& operator+=(const TemplateVector2& other) { x += other.x; y += other.y; return *this; }
    TemplateVector2& operator-=(const TemplateVector2& other) { x -= other.x; y -= other.y; return *this; }
    TemplateVector2& operator*=(const TemplateVector2& other) { x *= other.x; y *= other.y; return *this; }
    TemplateVector2& operator/=(const TemplateVector2& other) { x /= other.x; y /= other.y; return *this; }

    TemplateVector2& operator*=(T scalar) { x *= scalar; y *= scalar; return *this; }
    TemplateVector2& operator/=(T scalar) { x /= scalar; y /= scalar; return *this; }

    friend bool operator==(const TemplateVector2& a, const TemplateVector2& b)
    {
        return a.x == b.x && a.y == b.y;
    }
    friend bool operator!=(const TemplateVector2& a, const TemplateVector2& b)
    {
        return !(a == b);
    }
    friend std::ostream& operator<<(std::ostream& os, const TemplateVector2& v)
    {
        os << '(' << v.x << ", " << v.y << ')';
        return os;
    }
};

template<typename T>
TemplateVector2<T> operator*(const TemplateVector2<T>& v, T scalar)
{
    return TemplateVector2<T>(v.x * scalar, v.y * scalar);
}

template<typename T>
TemplateVector2<T> operator*(T scalar, const TemplateVector2<T>& v)
{
    return v * scalar;
}

template<typename T>
TemplateVector2<T> operator/(const TemplateVector2<T>& v, T scalar)
{
    return TemplateVector2<T>(v.x / scalar, v.y / scalar);
}

AFTERMATH_END
