// Description: 3-dimensional half-precision vector class with 
//              comprehensive mathematical operations and HLSL compatibility
// Author: NSDeathman, DeepSeek
#pragma once

/**
* @file math_half3.h
* @brief 3-dimensional half-precision vector class
* @note Optimized for 3D graphics, normals, colors, and memory-constrained applications
* @note Features comprehensive HLSL compatibility
*/

#include <cmath>
#include <algorithm>
#include <string>
#include <cstdio>

#include "math_config.h"
#include "math_constants.h"
#include "math_functions.h"
#include "math_half.h"
#include "math_half2.h"
#include "math_float3.h"
#include "AfterMathInternal.h"

AFTERMATH_BEGIN

/**
    * @class half3
    * @brief 3-dimensional half-precision vector with comprehensive mathematical operations
    *
    * Represents a 3D vector (x, y, z) using 16-bit half-precision floating point format.
    * Perfect for 3D graphics, normals, colors, and memory-constrained applications
    * where full 32-bit precision is not required.
    *
    * @note Optimized for memory bandwidth and GPU data formats
    * @note Provides seamless interoperability with float3 and comprehensive HLSL-like functions
    * @note Includes advanced color operations and geometric functions
    */
class half3
{
public:
    // ============================================================================
    // Data Members (Public for Direct Access)
    // ============================================================================

    half x; ///< X component of the vector
    half y; ///< Y component of the vector
    half z; ///< Z component of the vector

    // ============================================================================
    // Constructors (основные конструкторы остаются в классе)
    // ============================================================================

    half3() noexcept : x(half::from_bits(0)), y(half::from_bits(0)), z(half::from_bits(0)) {}
    half3(half x, half y, half z) noexcept : x(x), y(y), z(z) {}
    explicit half3(half scalar) noexcept : x(scalar), y(scalar), z(scalar) {}
    half3(float x, float y, float z) noexcept : x(x), y(y), z(z) {}
    explicit half3(float scalar) noexcept : x(scalar), y(scalar), z(scalar) {}
    half3(const half3&) noexcept = default;
    half3(const half2& vec, half z = half::from_bits(0)) noexcept : x(vec.x), y(vec.y), z(z) {}
    half3(const float3& vec) noexcept : x(vec.x), y(vec.y), z(vec.z) {}
    half3(const float2& vec, float z = 0.0f) noexcept : x(vec.x), y(vec.y), z(z) {}

    // ============================================================================
    // Assignment Operators (основные операторы присваивания)
    // ============================================================================

    half3& operator=(const half3&) noexcept = default;
    half3& operator=(const float3& vec) noexcept { x = vec.x; y = vec.y; z = vec.z; return *this; }
    half3& operator=(half scalar) noexcept { x = y = z = scalar; return *this; }
    half3& operator=(float scalar) noexcept { x = y = z = scalar; return *this; }

    // ============================================================================
    // Compound Assignment Operators (составные операторы)
    // ============================================================================

    half3& operator+=(const half3& rhs) noexcept { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
    half3& operator-=(const half3& rhs) noexcept { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
    half3& operator*=(const half3& rhs) noexcept { x *= rhs.x; y *= rhs.y; z *= rhs.z; return *this; }
    half3& operator/=(const half3& rhs) noexcept { x /= rhs.x; y /= rhs.y; z /= rhs.z; return *this; }
    half3& operator*=(half scalar) noexcept { x *= scalar; y *= scalar; z *= scalar; return *this; }
    half3& operator*=(float scalar) noexcept { x *= scalar; y *= scalar; z *= scalar; return *this; }
    half3& operator/=(half scalar) noexcept { x /= scalar; y /= scalar; z /= scalar; return *this; }
    half3& operator/=(float scalar) noexcept { x /= scalar; y /= scalar; z /= scalar; return *this; }

    // ============================================================================
    // Unary Operators (унарные операторы)
    // ============================================================================

    half3 operator+() const noexcept { return *this; }
    half3 operator-() const noexcept { return half3(-x, -y, -z); }

    // ============================================================================
    // Access Operators (операторы доступа)
    // ============================================================================

    half& operator[](int index) noexcept { return (&x)[index]; }
    const half& operator[](int index) const noexcept { return (&x)[index]; }

    // ============================================================================
    // Conversion Operators (операторы преобразования)
    // ============================================================================

    explicit operator float3() const noexcept { return float3(float(x), float(y), float(z)); }

    // ============================================================================
    // Static Constructors (статические конструкторы)
    // ============================================================================

    static half3 zero() noexcept { return half3(half::from_bits(0), half::from_bits(0), half::from_bits(0)); }
    static half3 one() noexcept { return half3(half::from_bits(0x3C00), half::from_bits(0x3C00), half::from_bits(0x3C00)); }
    static half3 unit_x() noexcept { return half3(half::from_bits(0x3C00), half::from_bits(0), half::from_bits(0)); }
    static half3 unit_y() noexcept { return half3(half::from_bits(0), half::from_bits(0x3C00), half::from_bits(0)); }
    static half3 unit_z() noexcept { return half3(half::from_bits(0), half::from_bits(0), half::from_bits(0x3C00)); }
    static half3 forward() noexcept { return unit_z(); }
    static half3 up() noexcept { return unit_y(); }
    static half3 right() noexcept { return unit_x(); }

    // ============================================================================
    // Swizzle Operations (swizzle-операции остаются в классе)
    // ============================================================================

    half2 xy() const noexcept { return half2(x, y); }
    half2 xz() const noexcept { return half2(x, z); }
    half2 yz() const noexcept { return half2(y, z); }
    half2 yx() const noexcept { return half2(y, x); }
    half2 zx() const noexcept { return half2(z, x); }
    half2 zy() const noexcept { return half2(z, y); }

    half3 yxz() const noexcept { return half3(y, x, z); }
    half3 zxy() const noexcept { return half3(z, x, y); }
    half3 zyx() const noexcept { return half3(z, y, x); }
    half3 xzy() const noexcept { return half3(x, z, y); }

    // Color swizzles (цветовые swizzle)
    half r() const noexcept { return x; }
    half g() const noexcept { return y; }
    half b() const noexcept { return z; }
    half2 rg() const noexcept { return half2(x, y); }
    half2 rb() const noexcept { return half2(x, z); }
    half2 gb() const noexcept { return half2(y, z); }
    half3 rgb() const noexcept { return *this; }
    half3 bgr() const noexcept { return half3(z, y, x); }
    half3 gbr() const noexcept { return half3(y, z, x); }

    // ============================================================================
    // Basic Checks (базовые проверки)
    // ============================================================================

    bool is_valid() const noexcept { return x.is_finite() && y.is_finite() && z.is_finite(); }
    bool is_inf() const noexcept { return x.is_inf() || y.is_inf() || z.is_inf(); }
    bool is_negative_inf() const noexcept { return x.is_negative_inf() || y.is_negative_inf() || z.is_negative_inf(); }
    bool is_positive_inf() const noexcept { return x.is_positive_inf() || y.is_positive_inf() || z.is_positive_inf(); }
    bool is_negative() const noexcept { return x.is_negative() || y.is_negative() || z.is_negative(); }
    bool is_all_negative() const noexcept { return x.is_negative() && y.is_negative() && z.is_negative(); }
    bool is_positive() const noexcept { return x.is_positive() || y.is_positive() || z.is_positive(); }
    bool is_all_positive() const noexcept { return x.is_positive() && y.is_positive() && z.is_positive(); }
    bool is_nan() const noexcept { return x.is_nan() || y.is_nan() || z.is_nan(); }
    bool is_all_nan() const noexcept { return x.is_nan() && y.is_nan() && z.is_nan(); }
    bool is_finite() const noexcept { return x.is_finite() || y.is_finite() || z.is_finite(); }
    bool is_all_finite() const noexcept { return x.is_finite() && y.is_finite() && z.is_finite(); }
    bool is_zero() const noexcept { return x.is_zero() || y.is_zero() || z.is_zero(); }
    bool is_all_zero() const noexcept { return x.is_zero() && y.is_zero() && z.is_zero(); }
    bool is_positive_zero() const noexcept { return x.is_positive_zero() || y.is_positive_zero() || z.is_positive_zero(); }
    bool is_negative_zero() const noexcept { return x.is_negative_zero() || y.is_negative_zero() || z.is_negative_zero(); }

    // ============================================================================
    // Utility Methods (утилиты)
    // ============================================================================

    std::string to_string() const {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "(%.3f, %.3f, %.3f)", float(x), float(y), float(z));
        return std::string(buffer);
    }

    const half* data() const noexcept { return &x; }
    half* data() noexcept { return &x; }
    void set_xy(const half2& xy) noexcept { x = xy.x; y = xy.y; }

    // ============================================================================
    // Comparison Operators
    // ============================================================================

    bool operator==(const half3& rhs) const noexcept {
        return approximately(x, rhs.x) && approximately(y, rhs.y) && approximately(z, rhs.z);
    }

    bool operator!=(const half3& rhs) const noexcept { return !(*this == rhs); }
};

// ============================================================================
// Binary Operators (бинарные операторы)
// ============================================================================

inline half3 operator+(half3 lhs, const half3& rhs) noexcept { return lhs += rhs; }
inline half3 operator-(half3 lhs, const half3& rhs) noexcept { return lhs -= rhs; }
inline half3 operator*(half3 lhs, const half3& rhs) noexcept { return lhs *= rhs; }
inline half3 operator/(half3 lhs, const half3& rhs) noexcept { return lhs /= rhs; }

inline half3 operator*(half3 vec, half scalar) noexcept { return vec *= scalar; }
inline half3 operator*(half scalar, half3 vec) noexcept { return vec *= scalar; }
inline half3 operator/(half3 vec, half scalar) noexcept { return vec /= scalar; }

inline half3 operator*(half3 vec, float scalar) noexcept { return vec *= scalar; }
inline half3 operator*(float scalar, half3 vec) noexcept { return vec *= scalar; }
inline half3 operator/(half3 vec, float scalar) noexcept { return vec /= scalar; }

inline half3 operator+(half3 vec, half scalar) noexcept { return half3(vec.x + scalar, vec.y + scalar, vec.z + scalar); }
inline half3 operator+(half scalar, half3 vec) noexcept { return half3(scalar + vec.x, scalar + vec.y, scalar + vec.z); }
inline half3 operator-(half3 vec, half scalar) noexcept { return half3(vec.x - scalar, vec.y - scalar, vec.z - scalar); }
inline half3 operator-(half scalar, half3 vec) noexcept { return half3(scalar - vec.x, scalar - vec.y, scalar - vec.z); }

inline half3 operator+(half3 vec, float scalar) noexcept { return half3(vec.x + scalar, vec.y + scalar, vec.z + scalar); }
inline half3 operator+(float scalar, half3 vec) noexcept { return half3(scalar + vec.x, scalar + vec.y, scalar + vec.z); }
inline half3 operator-(half3 vec, float scalar) noexcept { return half3(vec.x - scalar, vec.y - scalar, vec.z - scalar); }
inline half3 operator-(float scalar, half3 vec) noexcept { return half3(scalar - vec.x, scalar - vec.y, scalar - vec.z); }

// ============================================================================
// Mixed Type Operators (half3 <-> float3)
// ============================================================================

inline half3 operator+(const half3& lhs, const float3& rhs) noexcept { return half3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z); }
inline half3 operator-(const half3& lhs, const float3& rhs) noexcept { return half3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z); }
inline half3 operator*(const half3& lhs, const float3& rhs) noexcept { return half3(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z); }
inline half3 operator/(const half3& lhs, const float3& rhs) noexcept { return half3(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z); }

inline half3 operator+(const float3& lhs, const half3& rhs) noexcept { return half3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z); }
inline half3 operator-(const float3& lhs, const half3& rhs) noexcept { return half3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z); }
inline half3 operator*(const float3& lhs, const half3& rhs) noexcept { return half3(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z); }
inline half3 operator/(const float3& lhs, const half3& rhs) noexcept { return half3(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z); }

// ============================================================================
// ГЛОБАЛЬНЫЕ МАТЕМАТИЧЕСКИЕ ФУНКЦИИ (HLSL-стиль)
// ============================================================================

// ============================================================================
// Базовые векторные операции
// ============================================================================
inline half length(const half3& vec) noexcept {
    float fx = float(vec.x);
    float fy = float(vec.y);
    float fz = float(vec.z);
    return half(std::sqrt(fx * fx + fy * fy + fz * fz));
}

inline half length_sq(const half3& vec) noexcept {
    return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
}

inline half3 normalize(const half3& vec) noexcept {
    float fx = float(vec.x);
    float fy = float(vec.y);
    float fz = float(vec.z);
    float len = std::sqrt(fx * fx + fy * fy + fz * fz);

    if (len == 0.0f || !std::isfinite(len)) {
        return half3::zero();
    }

    return half3(fx / len, fy / len, fz / len);
}

inline half dot(const half3& a, const half3& b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline half3 cross(const half3& a, const half3& b) noexcept {
    float x = float(a.y) * float(b.z) - float(a.z) * float(b.y);
    float y = float(a.z) * float(b.x) - float(a.x) * float(b.z);
    float z = float(a.x) * float(b.y) - float(a.y) * float(b.x);
    return half3(half(x), half(y), half(z));
}

inline half distance(const half3& a, const half3& b) noexcept {
    return length(b - a);
}

inline half distance_sq(const half3& a, const half3& b) noexcept {
    return length_sq(b - a);
}

// ============================================================================
// Геометрические операции
// ============================================================================
inline half3 reflect(const half3& incident, const half3& normal) noexcept {
    half dot_val = dot(incident, normal);
    return incident - half(2.0f) * dot_val * normal;
}

inline half3 refract(const half3& incident, const half3& normal, half eta) noexcept {
    half dot_ni = dot(normal, incident);
    half k = half(1.0f) - eta * eta * (half(1.0f) - dot_ni * dot_ni);

    if (k < half(0.0f))
        return half3::zero(); // total internal reflection

    return incident * eta - normal * (eta * dot_ni + sqrt(k));
}

inline half3 project(const half3& vec, const half3& onto) noexcept {
    half onto_length_sq = length_sq(onto);

    if (approximately_zero(onto_length_sq, Constants::Constants<float>::Epsilon * 10.0f))
        return half3::zero();

    half dot_val = dot(vec, onto);
    return onto * (dot_val / onto_length_sq);
}

inline half3 reject(const half3& vec, const half3& from) noexcept {
    half3 projected = project(vec, from);
    return vec - projected;
}

inline half angle_between(const half3& a, const half3& b) noexcept {
    if (a.is_all_zero() || b.is_all_zero()) {
        return half(0.0f);
    }

    half3 a_norm = normalize(a);
    half3 b_norm = normalize(b);

    if (a_norm.is_all_zero() || b_norm.is_all_zero()) {
        return half(0.0f);
    }

    half dot_val = dot(a_norm, b_norm);
    dot_val = clamp(dot_val, -half_One, half_One);

    if (dot_val >= half_One) return half(0.0f);
    if (dot_val <= -half_One) return half(Constants::Constants<float>::Pi);

    return half(std::acos(float(dot_val)));
}

// ============================================================================
// HLSL-подобные функции (компонентные)
// ============================================================================
inline half3 abs(const half3& vec) noexcept {
    return half3(abs(vec.x), abs(vec.y), abs(vec.z));
}

inline half3 sign(const half3& vec) noexcept {
    return half3(sign(vec.x), sign(vec.y), sign(vec.z));
}

inline half3 floor(const half3& vec) noexcept {
    return half3(floor(vec.x), floor(vec.y), floor(vec.z));
}

inline half3 ceil(const half3& vec) noexcept {
    return half3(ceil(vec.x), ceil(vec.y), ceil(vec.z));
}

inline half3 round(const half3& vec) noexcept {
    return half3(round(vec.x), round(vec.y), round(vec.z));
}

inline half3 frac(const half3& vec) noexcept {
    return half3(frac(vec.x), frac(vec.y), frac(vec.z));
}

inline half3 saturate(const half3& vec) noexcept {
    return half3(saturate(vec.x), saturate(vec.y), saturate(vec.z));
}

inline half3 step(half edge, const half3& vec) noexcept {
    return half3(step(edge, vec.x), step(edge, vec.y), step(edge, vec.z));
}

inline half3 smoothstep(half edge0, half edge1, const half3& vec) noexcept {
    return half3(smoothstep(edge0, edge1, vec.x),
        smoothstep(edge0, edge1, vec.y),
        smoothstep(edge0, edge1, vec.z));
}

// ============================================================================
// Функции минимума/максимума/ограничения
// ============================================================================
inline half3 min(const half3& a, const half3& b) noexcept {
    return half3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
}

inline half3 max(const half3& a, const half3& b) noexcept {
    return half3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}

inline half3 clamp(const half3& vec, const half3& min_val, const half3& max_val) noexcept {
    return half3(clamp(vec.x, min_val.x, max_val.x),
        clamp(vec.y, min_val.y, max_val.y),
        clamp(vec.z, min_val.z, max_val.z));
}

inline half3 clamp(const half3& vec, float min_val, float max_val) noexcept {
    return half3(clamp(vec.x, min_val, max_val),
        clamp(vec.y, min_val, max_val),
        clamp(vec.z, min_val, max_val));
}

// ============================================================================
// Интерполяция
// ============================================================================
inline half3 lerp(const half3& a, const half3& b, half t) noexcept {
    return half3(a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t);
}

inline half3 lerp(const half3& a, const half3& b, float t) noexcept {
    return half3(a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t);
}

// ============================================================================
// Функции проверки и сравнения
// ============================================================================
inline bool approximately(const half3& a, const half3& b, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
    return approximately(a.x, b.x, epsilon) &&
        approximately(a.y, b.y, epsilon) &&
        approximately(a.z, b.z, epsilon);
}

inline bool approximately_zero(const half3& vec, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
    return approximately_zero(vec.x, epsilon) &&
        approximately_zero(vec.y, epsilon) &&
        approximately_zero(vec.z, epsilon);
}

inline bool is_normalized(const half3& vec, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
    half len_sq = length_sq(vec);
    float adjusted_epsilon = std::max(epsilon, 0.01f);
    return approximately(float(len_sq), 1.0f, adjusted_epsilon);
}

// ============================================================================
// Color Operations
// ============================================================================
inline half luminance(const half3& rgb) noexcept {
    return half(0.2126f) * rgb.x + half(0.7152f) * rgb.y + half(0.0722f) * rgb.z;
}

inline half3 rgb_to_grayscale(const half3& rgb) noexcept {
    half luma = luminance(rgb);
    return half3(luma, luma, luma);
}

inline half3 gamma_correct(const half3& color, half gamma) noexcept {
    return half3(pow(color.x, gamma), pow(color.y, gamma), pow(color.z, gamma));
}

inline half3 srgb_to_linear(const half3& srgb) noexcept {
    auto srgb_to_linear_channel = [](half channel) -> half {
        float c = float(channel);
        return (c <= 0.04045f) ? half(c / 12.92f) : half(std::pow((c + 0.055f) / 1.055f, 2.4f));
    };

    return half3(srgb_to_linear_channel(srgb.x),
        srgb_to_linear_channel(srgb.y),
        srgb_to_linear_channel(srgb.z));
}

inline half3 linear_to_srgb(const half3& linear) noexcept {
    auto linear_to_srgb_channel = [](half channel) -> half {
        float c = float(channel);
        return (c <= 0.0031308f) ? half(c * 12.92f) : half(1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f);
    };

    return half3(linear_to_srgb_channel(linear.x),
        linear_to_srgb_channel(linear.y),
        linear_to_srgb_channel(linear.z));
}

// ============================================================================
// Глобальные функции проверки (для совместимости)
// ============================================================================
inline bool is_valid(const half3& vec) noexcept { return vec.is_valid(); }
inline bool is_inf(const half3& vec) noexcept { return vec.is_inf(); }
inline bool is_negative_inf(const half3& vec) noexcept { return vec.is_negative_inf(); }
inline bool is_positive_inf(const half3& vec) noexcept { return vec.is_positive_inf(); }
inline bool is_negative(const half3& vec) noexcept { return vec.is_negative(); }
inline bool is_all_negative(const half3& vec) noexcept { return vec.is_all_negative(); }
inline bool is_positive(const half3& vec) noexcept { return vec.is_positive(); }
inline bool is_all_positive(const half3& vec) noexcept { return vec.is_all_positive(); }
inline bool is_nan(const half3& vec) noexcept { return vec.is_nan(); }
inline bool is_all_nan(const half3& vec) noexcept { return vec.is_all_nan(); }
inline bool is_finite(const half3& vec) noexcept { return vec.is_finite(); }
inline bool is_all_finite(const half3& vec) noexcept { return vec.is_all_finite(); }
inline bool is_zero(const half3& vec) noexcept { return vec.is_zero(); }
inline bool is_all_zero(const half3& vec) noexcept { return vec.is_all_zero(); }
inline bool is_positive_zero(const half3& vec) noexcept { return vec.is_positive_zero(); }
inline bool is_negative_zero(const half3& vec) noexcept { return vec.is_negative_zero(); }

// ============================================================================
// Функции преобразования типов
// ============================================================================
inline float3 to_float3(const half3& vec) noexcept {
    return float3(float(vec.x), float(vec.y), float(vec.z));
}

inline half3 to_half3(const float3& vec) noexcept {
    return half3(vec.x, vec.y, vec.z);
}

// ============================================================================
// Утилитные функции
// ============================================================================
inline half3 ensure_normalized(const half3& normal, const half3& fallback = half3::unit_z()) noexcept {
    half len_sq = length_sq(normal);
    if (len_sq > half::epsilon()) {
        half len = sqrt(len_sq);
        if (abs(len - half_One) > half(0.01f)) {
            return normal / len;
        }
        return normal;
    }
    return fallback;
}

// ============================================================================
// Полезные константы
// ============================================================================
AFTERMATH_INLINE_VAR const half3 half3_Zero(half_Zero);
AFTERMATH_INLINE_VAR const half3 half3_One(half_One);
AFTERMATH_INLINE_VAR const half3 half3_UnitX(half_One, half_Zero, half_Zero);
AFTERMATH_INLINE_VAR const half3 half3_UnitY(half_Zero, half_One, half_Zero);
AFTERMATH_INLINE_VAR const half3 half3_UnitZ(half_Zero, half_Zero, half_One);
AFTERMATH_INLINE_VAR const half3 half3_Forward(half_Zero, half_Zero, half_One);
AFTERMATH_INLINE_VAR const half3 half3_Up(half_Zero, half_One, half_Zero);
AFTERMATH_INLINE_VAR const half3 half3_Right(half_One, half_Zero, half_Zero);
AFTERMATH_INLINE_VAR const half3 half3_Red(half_One, half_Zero, half_Zero);
AFTERMATH_INLINE_VAR const half3 half3_Green(half_Zero, half_One, half_Zero);
AFTERMATH_INLINE_VAR const half3 half3_Blue(half_Zero, half_Zero, half_One);
AFTERMATH_INLINE_VAR const half3 half3_White(half_One);
AFTERMATH_INLINE_VAR const half3 half3_Black(half_Zero);
AFTERMATH_INLINE_VAR const half3 half3_Yellow(half_One, half_One, half_Zero);
AFTERMATH_INLINE_VAR const half3 half3_Cyan(half_Zero, half_One, half_One);
AFTERMATH_INLINE_VAR const half3 half3_Magenta(half_One, half_Zero, half_One);

AFTERMATH_END
