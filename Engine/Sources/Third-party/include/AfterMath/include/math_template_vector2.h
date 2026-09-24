/*
 * AfterMath — high‑performance C++ math library (HLSL‑style, SSE‑accelerated)
 *
 * Project:   Presence AfterMath
 * Copyright: 2026 Presence Collaboratory
 * Authors:   NSDeathman (Architecture & Core)
 *            DeepSeek (Mathematics & HLSL Integration)
 *            Gemini 3 (Optimization & Fast Math)
 *			  Nikolay Partas (Half precision data type prototype)
 * License:   MIT License with Attribution — see LICENSE.md for details.
 *
 * https://github.com/Presence-Collaboratory/AfterMath-CPP-Open-Math-Library
 */
#pragma once

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

    constexpr TemplateVector2 operator+() const { return *this; }
    constexpr TemplateVector2 operator-() const { return TemplateVector2(-x, -y); }

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

    friend constexpr bool operator==(const TemplateVector2& a, const TemplateVector2& b)
    {
        return a.x == b.x && a.y == b.y;
    }
    friend constexpr bool operator!=(const TemplateVector2& a, const TemplateVector2& b)
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
constexpr TemplateVector2<T> operator+(const TemplateVector2<T>& a, const TemplateVector2<T>& b)
{
    return TemplateVector2<T>(a.x + b.x, a.y + b.y);
}

template<typename T>
constexpr TemplateVector2<T> operator-(const TemplateVector2<T>& a, const TemplateVector2<T>& b)
{
    return TemplateVector2<T>(a.x - b.x, a.y - b.y);
}

template<typename T>
constexpr TemplateVector2<T> operator*(const TemplateVector2<T>& a, const TemplateVector2<T>& b)
{
    return TemplateVector2<T>(a.x * b.x, a.y * b.y);
}

template<typename T>
constexpr TemplateVector2<T> operator/(const TemplateVector2<T>& a, const TemplateVector2<T>& b)
{
    return TemplateVector2<T>(a.x / b.x, a.y / b.y);
}

template<typename T>
constexpr TemplateVector2<T> operator*(const TemplateVector2<T>& v, T scalar)
{
    return TemplateVector2<T>(v.x * scalar, v.y * scalar);
}

template<typename T>
constexpr TemplateVector2<T> operator*(T scalar, const TemplateVector2<T>& v)
{
    return v * scalar;
}

template<typename T>
constexpr TemplateVector2<T> operator/(const TemplateVector2<T>& v, T scalar)
{
    return TemplateVector2<T>(v.x / scalar, v.y / scalar);
}

template<typename T>
constexpr TemplateVector2<T> operator/(T scalar, const TemplateVector2<T>& v)
{
    return TemplateVector2<T>(scalar / v.x, scalar / v.y);
}

template<typename T>
constexpr TemplateVector2<T> operator+(const TemplateVector2<T>& v, T scalar)
{
    return TemplateVector2<T>(v.x + scalar, v.y + scalar);
}

template<typename T>
constexpr TemplateVector2<T> operator+(T scalar, const TemplateVector2<T>& v)
{
    return v + scalar;
}

template<typename T>
constexpr TemplateVector2<T> operator-(const TemplateVector2<T>& v, T scalar)
{
    return TemplateVector2<T>(v.x - scalar, v.y - scalar);
}

template<typename T>
constexpr TemplateVector2<T> operator-(T scalar, const TemplateVector2<T>& v)
{
    return TemplateVector2<T>(scalar - v.x, scalar - v.y);
}

template<typename T>
constexpr TemplateVector2<T> operator*(const TemplateVector2<T>& v, int scalar)
{
    return v * static_cast<T>(scalar);
}

template<typename T>
constexpr TemplateVector2<T> operator*(int scalar, const TemplateVector2<T>& v)
{
    return v * static_cast<T>(scalar);
}

template<typename T>
constexpr TemplateVector2<T> operator/(const TemplateVector2<T>& v, int scalar)
{
    return v / static_cast<T>(scalar);
}

template<typename T>
constexpr TemplateVector2<T> operator/(int scalar, const TemplateVector2<T>& v)
{
    return static_cast<T>(scalar) / v;
}

template<typename T>
constexpr TemplateVector2<T> operator+(const TemplateVector2<T>& v, int scalar)
{
    return v + static_cast<T>(scalar);
}

template<typename T>
constexpr TemplateVector2<T> operator+(int scalar, const TemplateVector2<T>& v)
{
    return static_cast<T>(scalar) + v;
}

template<typename T>
constexpr TemplateVector2<T> operator-(const TemplateVector2<T>& v, int scalar)
{
    return v - static_cast<T>(scalar);
}

template<typename T>
constexpr TemplateVector2<T> operator-(int scalar, const TemplateVector2<T>& v)
{
    return static_cast<T>(scalar) - v;
}

AFTERMATH_END
