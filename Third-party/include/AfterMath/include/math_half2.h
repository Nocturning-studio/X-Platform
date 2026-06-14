// Description: 2-dimensional half-precision vector class with 
//              comprehensive mathematical operations and HLSL compatibility
//              Optimized for memory bandwidth and GPU data formats
// Author: NSDeathman, DeepSeek
#pragma once

/**
 * @file math_half2.h
 * @brief 2-dimensional half-precision vector class
 * @note Optimized for memory bandwidth, GPU data formats, and texture coordinates
 * @note Provides seamless interoperability with float2 and comprehensive HLSL-like functions
 */

#include <cmath>
#include <algorithm>
#include <string>
#include <cstdio>

#include "math_config.h"
#include "math_constants.h"
#include "math_functions.h"
#include "math_half.h"
#include "math_float2.h"
#include "AfterMathInternal.h"

AFTERMATH_BEGIN

/**
    * @class half2
    * @brief 2-dimensional half-precision vector with comprehensive mathematical operations
    *
    * Represents a 2D vector (x, y) using 16-bit half-precision floating point format.
    * Optimized for memory bandwidth, GPU data formats, and scenarios where full 32-bit
    * precision is not required (texture coordinates, normals, colors, etc.).
    *
    * @note Perfect for texture coordinates, 2D graphics, UI systems, and memory-constrained applications
    * @note Provides seamless interoperability with float2 and comprehensive HLSL-like functions
    * @note All operations maintain half-precision accuracy while providing performance benefits
    */
class half2
{
public:
    // ============================================================================
    // Data Members (Public for Direct Access)
    // ============================================================================

    half x; ///< X component of the vector
    half y; ///< Y component of the vector

    // ============================================================================
    // Constructors (остаются как фундаментальные)
    // ============================================================================

    half2() noexcept : x(half::from_bits(0)), y(half::from_bits(0)) {}
    half2(half x, half y) noexcept : x(x), y(y) {}
    explicit half2(half scalar) noexcept : x(scalar), y(scalar) {}
    half2(float x, float y) noexcept : x(x), y(y) {}
    explicit half2(float scalar) noexcept : x(scalar), y(scalar) {}
    half2(const float2& vec) noexcept : x(vec.x), y(vec.y) {}
    half2(const half2&) noexcept = default;

    // ============================================================================
    // Assignment Operators (остаются как фундаментальные)
    // ============================================================================

    half2& operator=(const half2&) noexcept = default;
    half2& operator=(const float2& vec) noexcept { x = vec.x; y = vec.y; return *this; }
    half2& operator=(half scalar) noexcept { x = y = scalar; return *this; }
    half2& operator=(float scalar) noexcept { x = y = scalar; return *this; }

    // ============================================================================
    // Compound Assignment Operators (остаются как фундаментальные)
    // ============================================================================

    half2& operator+=(const half2& rhs) noexcept { x += rhs.x; y += rhs.y; return *this; }
    half2& operator-=(const half2& rhs) noexcept { x -= rhs.x; y -= rhs.y; return *this; }
    half2& operator*=(const half2& rhs) noexcept { x *= rhs.x; y *= rhs.y; return *this; }
    half2& operator/=(const half2& rhs) noexcept { x /= rhs.x; y /= rhs.y; return *this; }
    half2& operator*=(half scalar) noexcept { x *= scalar; y *= scalar; return *this; }
    half2& operator*=(float scalar) noexcept { x *= scalar; y *= scalar; return *this; }
    half2& operator/=(half scalar) noexcept { x /= scalar; y /= scalar; return *this; }
    half2& operator/=(float scalar) noexcept { x /= scalar; y /= scalar; return *this; }

    // ============================================================================
    // Unary Operators (остаются как фундаментальные)
    // ============================================================================

    half2 operator+() const noexcept { return *this; }
    half2 operator-() const noexcept { return half2(-x, -y); }

    // ============================================================================
    // Access Operators (остаются как фундаментальные)
    // ============================================================================

    half& operator[](int index) noexcept { return (&x)[index]; }
    const half& operator[](int index) const noexcept { return (&x)[index]; }

    // ============================================================================
    // Conversion Operators (остаются как фундаментальные)
    // ============================================================================

    explicit operator float2() const noexcept { return float2(float(x), float(y)); }

    // ============================================================================
    // Static Constructors (остаются как удобные утилиты)
    // ============================================================================

    static half2 zero() noexcept { return half2(half::from_bits(0), half::from_bits(0)); }
    static half2 one() noexcept { return half2(half::from_bits(0x3C00), half::from_bits(0x3C00)); }
    static half2 unit_x() noexcept { return half2(half::from_bits(0x3C00), half::from_bits(0)); }
    static half2 unit_y() noexcept { return half2(half::from_bits(0), half::from_bits(0x3C00)); }
    static half2 uv(half u, half v) noexcept { return half2(u, v); }

    // ============================================================================
    // Swizzle Operations (остаются как удобные утилиты)
    // ============================================================================

    half2 yx() const noexcept { return half2(y, x); }
    half2 xx() const noexcept { return half2(x, x); }
    half2 yy() const noexcept { return half2(y, y); }

    // ============================================================================
    // Texture Coordinate Accessors (остаются как удобные утилиты)
    // ============================================================================

    half u() const noexcept { return x; }
    half v() const noexcept { return y; }
    void set_u(half u) noexcept { x = u; }
    void set_v(half v) noexcept { y = v; }

    // ============================================================================
    // Basic Checks (остаются как фундаментальные проверки)
    // ============================================================================

    bool is_valid() const noexcept { return x.is_finite() && y.is_finite(); }

    bool is_inf() const noexcept { return x.is_inf() || y.is_inf(); }
    bool is_negative_inf() const noexcept { return x.is_negative_inf() || y.is_negative_inf(); }
    bool is_positive_inf() const noexcept { return x.is_positive_inf() || y.is_positive_inf(); }

    bool is_negative() const noexcept { return x.is_negative() || y.is_negative(); }
    bool is_all_negative() const noexcept { return x.is_negative() && y.is_negative(); }

    bool is_positive() const noexcept { return x.is_positive() || y.is_positive(); }
    bool is_all_positive() const noexcept { return x.is_positive() && y.is_positive(); }

    bool is_nan() const noexcept { return x.is_nan() || y.is_nan(); }
    bool is_all_nan() const noexcept { return x.is_nan() && y.is_nan(); }

    bool is_finite() const noexcept { return x.is_finite() || y.is_finite(); }
    bool is_all_finite() const noexcept { return x.is_finite() && y.is_finite(); }

    bool is_zero() const noexcept { return x.is_zero() || y.is_zero(); }
    bool is_all_zero() const noexcept { return x.is_zero() && y.is_zero(); }

    bool is_positive_zero() const noexcept { return x.is_positive_zero() || y.is_positive_zero(); }
    bool is_negative_zero() const noexcept { return x.is_negative_zero() || y.is_negative_zero(); }

    // ============================================================================
    // Comparison Operators
    // ============================================================================

    bool operator==(const half2& rhs) const noexcept {
        return approximately(x, rhs.x) && approximately(y, rhs.y);
    }

    bool operator!=(const half2& rhs) const noexcept { return !(*this == rhs); }

    // ============================================================================
    // Utility Methods
    // ============================================================================

    std::string to_string() const {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "(%.3f, %.3f)", float(x), float(y));
        return std::string(buffer);
    }

    const half* data() const noexcept { return &x; }
    half* data() noexcept { return &x; }
};

// ============================================================================
// Binary Operators
// ============================================================================

inline half2 operator+(half2 lhs, const half2& rhs) noexcept { return lhs += rhs; }
inline half2 operator-(half2 lhs, const half2& rhs) noexcept { return lhs -= rhs; }
inline half2 operator*(half2 lhs, const half2& rhs) noexcept { return lhs *= rhs; }
inline half2 operator/(half2 lhs, const half2& rhs) noexcept { return lhs /= rhs; }

inline half2 operator*(half2 vec, half scalar) noexcept { return vec *= scalar; }
inline half2 operator*(half scalar, half2 vec) noexcept { return vec *= scalar; }
inline half2 operator/(half2 vec, half scalar) noexcept { return vec /= scalar; }

inline half2 operator*(half2 vec, float scalar) noexcept { return vec *= scalar; }
inline half2 operator*(float scalar, half2 vec) noexcept { return vec *= scalar; }
inline half2 operator/(half2 vec, float scalar) noexcept { return vec /= scalar; }

inline half2 operator+(half2 vec, half scalar) noexcept { return half2(vec.x + scalar, vec.y + scalar); }
inline half2 operator+(half scalar, half2 vec) noexcept { return half2(scalar + vec.x, scalar + vec.y); }
inline half2 operator-(half2 vec, half scalar) noexcept { return half2(vec.x - scalar, vec.y - scalar); }
inline half2 operator-(half scalar, half2 vec) noexcept { return half2(scalar - vec.x, scalar - vec.y); }

inline half2 operator+(half2 vec, float scalar) noexcept { return half2(vec.x + scalar, vec.y + scalar); }
inline half2 operator+(float scalar, half2 vec) noexcept { return half2(scalar + vec.x, scalar + vec.y); }
inline half2 operator-(half2 vec, float scalar) noexcept { return half2(vec.x - scalar, vec.y - scalar); }
inline half2 operator-(float scalar, half2 vec) noexcept { return half2(scalar - vec.x, scalar - vec.y); }

// ============================================================================
// Mixed Type Operators (half2 <-> float2)
// ============================================================================

inline half2 operator+(const half2& lhs, const float2& rhs) noexcept { return half2(lhs.x + rhs.x, lhs.y + rhs.y); }
inline half2 operator-(const half2& lhs, const float2& rhs) noexcept { return half2(lhs.x - rhs.x, lhs.y - rhs.y); }
inline half2 operator*(const half2& lhs, const float2& rhs) noexcept { return half2(lhs.x * rhs.x, lhs.y * rhs.y); }
inline half2 operator/(const half2& lhs, const float2& rhs) noexcept { return half2(lhs.x / rhs.x, lhs.y / rhs.y); }

inline half2 operator+(const float2& lhs, const half2& rhs) noexcept { return half2(lhs.x + rhs.x, lhs.y + rhs.y); }
inline half2 operator-(const float2& lhs, const half2& rhs) noexcept { return half2(lhs.x - rhs.x, lhs.y - rhs.y); }
inline half2 operator*(const float2& lhs, const half2& rhs) noexcept { return half2(lhs.x * rhs.x, lhs.y * rhs.y); }
inline half2 operator/(const float2& lhs, const half2& rhs) noexcept { return half2(lhs.x / rhs.x, lhs.y / rhs.y); }

// ============================================================================
// ГЛОБАЛЬНЫЕ МАТЕМАТИЧЕСКИЕ ФУНКЦИИ (HLSL-стиль)
// ============================================================================

// ============================================================================
// Базовые векторные операции
// ============================================================================
inline half length(const half2& vec) noexcept {
    float fx = float(vec.x);
    float fy = float(vec.y);
    return half(std::sqrt(fx * fx + fy * fy));
}

inline half length_sq(const half2& vec) noexcept {
    return vec.x * vec.x + vec.y * vec.y;
}

inline half2 normalize(const half2& vec) noexcept {
    float fx = float(vec.x);
    float fy = float(vec.y);
    float len = std::sqrt(fx * fx + fy * fy);

    if (len == 0.0f || !std::isfinite(len)) {
        return half2::zero();
    }

    return half2(fx / len, fy / len);
}

inline half dot(const half2& a, const half2& b) noexcept {
    return a.x * b.x + a.y * b.y;
}

inline half distance(const half2& a, const half2& b) noexcept {
    return length(b - a);
}

inline half distance_sq(const half2& a, const half2& b) noexcept {
    return length_sq(b - a);
}

// ============================================================================
// Геометрические операции
// ============================================================================
inline half2 perpendicular(const half2& vec) noexcept {
    return half2(-vec.y, vec.x);
}

inline half cross(const half2& a, const half2& b) noexcept {
    return a.x * b.y - a.y * b.x;
}

inline half angle(const half2& vec) noexcept {
    return half(std::atan2(float(vec.y), float(vec.x)));
}

inline half angle_between(const half2& a, const half2& b) noexcept {
    half2 a_norm = normalize(a);
    half2 b_norm = normalize(b);
    half dot_val = dot(a_norm, b_norm);
    dot_val = clamp(dot_val, -half_One, half_One);
    return half(std::acos(float(dot_val)));
}

// ============================================================================
// HLSL-подобные функции (компонентные)
// ============================================================================
inline half2 abs(const half2& vec) noexcept {
    return half2(abs(vec.x), abs(vec.y));
}

inline half2 sign(const half2& vec) noexcept {
    return half2(sign(vec.x), sign(vec.y));
}

inline half2 floor(const half2& vec) noexcept {
    return half2(floor(vec.x), floor(vec.y));
}

inline half2 ceil(const half2& vec) noexcept {
    return half2(ceil(vec.x), ceil(vec.y));
}

inline half2 round(const half2& vec) noexcept {
    return half2(round(vec.x), round(vec.y));
}

inline half2 frac(const half2& vec) noexcept {
    return half2(frac(vec.x), frac(vec.y));
}

inline half2 saturate(const half2& vec) noexcept {
    return half2(saturate(vec.x), saturate(vec.y));
}

inline half2 step(half edge, const half2& vec) noexcept {
    return half2(step(edge, vec.x), step(edge, vec.y));
}

inline half2 smoothstep(half edge0, half edge1, const half2& vec) noexcept {
    return half2(smoothstep(edge0, edge1, vec.x), smoothstep(edge0, edge1, vec.y));
}

// ============================================================================
// Функции минимума/максимума/ограничения
// ============================================================================
inline half2 min(const half2& a, const half2& b) noexcept {
    return half2(min(a.x, b.x), min(a.y, b.y));
}

inline half2 max(const half2& a, const half2& b) noexcept {
    return half2(max(a.x, b.x), max(a.y, b.y));
}

inline half2 clamp(const half2& vec, const half2& min_val, const half2& max_val) noexcept {
    return half2(clamp(vec.x, min_val.x, max_val.x), clamp(vec.y, min_val.y, max_val.y));
}

inline half2 clamp(const half2& vec, float min_val, float max_val) noexcept {
    return half2(clamp(vec.x, min_val, max_val), clamp(vec.y, min_val, max_val));
}

// ============================================================================
// Интерполяция
// ============================================================================
inline half2 lerp(const half2& a, const half2& b, half t) noexcept {
    return half2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}

inline half2 lerp(const half2& a, const half2& b, float t) noexcept {
    return half2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}

// ============================================================================
// Функции проверки и сравнения
// ============================================================================
inline bool approximately(const half2& a, const half2& b, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
    return approximately(a.x, b.x, epsilon) && approximately(a.y, b.y, epsilon);
}

inline bool approximately_zero(const half2& vec, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
    return approximately_zero(vec.x, epsilon) && approximately_zero(vec.y, epsilon);
}

inline bool is_normalized(const half2& vec, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
    half len_sq = length_sq(vec);
    float adjusted_epsilon = std::max(epsilon, 0.01f);
    return approximately(float(len_sq), 1.0f, adjusted_epsilon);
}

// ============================================================================
// Глобальные функции проверки (для совместимости)
// ============================================================================
inline bool is_valid(const half2& vec) noexcept { return vec.is_valid(); }
inline bool is_inf(const half2& vec) noexcept { return vec.is_inf(); }
inline bool is_negative_inf(const half2& vec) noexcept { return vec.is_negative_inf(); }
inline bool is_positive_inf(const half2& vec) noexcept { return vec.is_positive_inf(); }
inline bool is_negative(const half2& vec) noexcept { return vec.is_negative(); }
inline bool is_all_negative(const half2& vec) noexcept { return vec.is_all_negative(); }
inline bool is_positive(const half2& vec) noexcept { return vec.is_positive(); }
inline bool is_all_positive(const half2& vec) noexcept { return vec.is_all_positive(); }
inline bool is_nan(const half2& vec) noexcept { return vec.is_nan(); }
inline bool is_all_nan(const half2& vec) noexcept { return vec.is_all_nan(); }
inline bool is_finite(const half2& vec) noexcept { return vec.is_finite(); }
inline bool is_all_finite(const half2& vec) noexcept { return vec.is_all_finite(); }
inline bool is_zero(const half2& vec) noexcept { return vec.is_zero(); }
inline bool is_all_zero(const half2& vec) noexcept { return vec.is_all_zero(); }

// ============================================================================
// Функции преобразования типов
// ============================================================================
inline float2 to_float2(const half2& vec) noexcept {
    return float2(float(vec.x), float(vec.y));
}

inline half2 to_half2(const float2& vec) noexcept {
    return half2(vec.x, vec.y);
}

// ============================================================================
// Полезные константы
// ============================================================================
AFTERMATH_INLINE_VAR const half2 half2_Zero(half_Zero);
AFTERMATH_INLINE_VAR const half2 half2_One(half_One);
AFTERMATH_INLINE_VAR const half2 half2_UnitX(half_One, half_Zero);
AFTERMATH_INLINE_VAR const half2 half2_UnitY(half_Zero, half_One);
AFTERMATH_INLINE_VAR const half2 half2_UV_Zero(half_Zero, half_Zero);
AFTERMATH_INLINE_VAR const half2 half2_UV_One(half_One, half_One);
AFTERMATH_INLINE_VAR const half2 half2_UV_Half(half(0.5f), half(0.5f));
AFTERMATH_INLINE_VAR const half2 half2_Right(half_One, half_Zero);
AFTERMATH_INLINE_VAR const half2 half2_Left(-half_One, half_Zero);
AFTERMATH_INLINE_VAR const half2 half2_Up(half_Zero, half_One);
AFTERMATH_INLINE_VAR const half2 half2_Down(half_Zero, -half_One);

AFTERMATH_END
