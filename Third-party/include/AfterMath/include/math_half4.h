// Description: 4-dimensional half-precision vector class with 
//              comprehensive mathematical operations and HLSL compatibility
// Author: NSDeathman, DeepSeek
#pragma once

/**
* @file math_half4.h
* @brief 4-dimensional half-precision vector class
* @note Optimized for 4D graphics, homogeneous coordinates, RGBA colors
* @note Features comprehensive HLSL compatibility and color space operations
*/

#include <cmath>
#include <algorithm>
#include <string>
#include <cstdio>
#include <cstdint>

#include "math_config.h"
#include "math_constants.h"
#include "math_functions.h"
#include "math_half.h"
#include "math_half2.h"
#include "math_half3.h"
#include "math_float4.h"
#include "AfterMathInternal.h"

AFTERMATH_BEGIN

/**
    * @class half4
    * @brief 4-dimensional half-precision vector with comprehensive mathematical operations
    *
    * Represents a 4D vector (x, y, z, w) using 16-bit half-precision floating point format.
    * Perfect for 4D graphics, homogeneous coordinates, RGBA colors, and memory-constrained
    * applications where full 32-bit precision is not required.
    *
    * @note Optimized for memory bandwidth and GPU data formats
    * @note Provides seamless interoperability with float4 and comprehensive HLSL-like functions
    * @note Includes advanced color operations, geometric functions, and homogeneous coordinate support
    */
class half4
{
public:
    // ============================================================================
    // Data Members (Public for Direct Access)
    // ============================================================================

    half x; ///< X component of the vector
    half y; ///< Y component of the vector
    half z; ///< Z component of the vector
    half w; ///< W component of the vector (homogeneous coordinate or alpha)

    // ============================================================================
    // Constructors (basic constructors remain in class)
    // ============================================================================

    half4() noexcept : x(half::from_bits(0)), y(half::from_bits(0)), z(half::from_bits(0)), w(half::from_bits(0)) {}
    half4(half x, half y, half z, half w) noexcept : x(x), y(y), z(z), w(w) {}
    explicit half4(half scalar) noexcept : x(scalar), y(scalar), z(scalar), w(scalar) {}
    half4(float x, float y, float z, float w) noexcept : x(x), y(y), z(z), w(w) {}
    explicit half4(float scalar) noexcept : x(scalar), y(scalar), z(scalar), w(scalar) {}
    half4(const half4&) noexcept = default;
    half4(const half2& vec, half z = half::from_bits(0), half w = half::from_bits(0)) noexcept
        : x(vec.x), y(vec.y), z(z), w(w) {}
    half4(const half3& vec, half w = half::from_bits(0)) noexcept
        : x(vec.x), y(vec.y), z(vec.z), w(w) {}
    half4(const float4& vec) noexcept : x(vec.x), y(vec.y), z(vec.z), w(vec.w) {}
    half4(const float2& vec, float z = 0.0f, float w = 0.0f) noexcept
        : x(vec.x), y(vec.y), z(z), w(w) {}
    half4(const float3& vec, float w = 0.0f) noexcept
        : x(vec.x), y(vec.y), z(vec.z), w(w) {}

    // ============================================================================
    // Assignment Operators
    // ============================================================================

    half4& operator=(const half4&) noexcept = default;
    half4& operator=(const float4& vec) noexcept { x = vec.x; y = vec.y; z = vec.z; w = vec.w; return *this; }
    half4& operator=(const half3& xyz) noexcept { x = xyz.x; y = xyz.y; z = xyz.z; return *this; } // w unchanged
    half4& operator=(half scalar) noexcept { x = y = z = w = scalar; return *this; }
    half4& operator=(float scalar) noexcept { x = y = z = w = scalar; return *this; }

    // ============================================================================
    // Compound Assignment Operators
    // ============================================================================

    half4& operator+=(const half4& rhs) noexcept { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
    half4& operator+=(half scalar) noexcept { x += scalar; y += scalar; z += scalar; w += scalar; return *this; }
    half4& operator-=(const half4& rhs) noexcept { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }
    half4& operator-=(half scalar) noexcept { x -= scalar; y -= scalar; z -= scalar; w -= scalar; return *this; }
    half4& operator*=(const half4& rhs) noexcept { x *= rhs.x; y *= rhs.y; z *= rhs.z; w *= rhs.w; return *this; }
    half4& operator/=(const half4& rhs) noexcept { x /= rhs.x; y /= rhs.y; z /= rhs.z; w /= rhs.w; return *this; }
    half4& operator*=(half scalar) noexcept { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
    half4& operator*=(float scalar) noexcept { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
    half4& operator/=(half scalar) noexcept { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }
    half4& operator/=(float scalar) noexcept { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }

    // ============================================================================
    // Unary Operators
    // ============================================================================

    half4 operator+() const noexcept { return *this; }
    half4 operator-() const noexcept { return half4(-x, -y, -z, -w); }

    // ============================================================================
    // Access Operators
    // ============================================================================

    half& operator[](int index) noexcept { return (&x)[index]; }
    const half& operator[](int index) const noexcept { return (&x)[index]; }

    // ============================================================================
    // Conversion Operators
    // ============================================================================

    explicit operator float4() const noexcept { return float4(float(x), float(y), float(z), float(w)); }

    // ============================================================================
    // Static Constructors
    // ============================================================================

    static half4 zero() noexcept { return half4(half::from_bits(0), half::from_bits(0), half::from_bits(0), half::from_bits(0)); }
    static half4 one() noexcept { return half4(half::from_bits(0x3C00), half::from_bits(0x3C00), half::from_bits(0x3C00), half::from_bits(0x3C00)); }
    static half4 unit_x() noexcept { return half4(half::from_bits(0x3C00), half::from_bits(0), half::from_bits(0), half::from_bits(0)); }
    static half4 unit_y() noexcept { return half4(half::from_bits(0), half::from_bits(0x3C00), half::from_bits(0), half::from_bits(0)); }
    static half4 unit_z() noexcept { return half4(half::from_bits(0), half::from_bits(0), half::from_bits(0x3C00), half::from_bits(0)); }
    static half4 unit_w() noexcept { return half4(half::from_bits(0), half::from_bits(0), half::from_bits(0), half::from_bits(0x3C00)); }
    static half4 from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) noexcept {
        return half4(
            half(static_cast<float>(r) / 255.0f),
            half(static_cast<float>(g) / 255.0f),
            half(static_cast<float>(b) / 255.0f),
            half(static_cast<float>(a) / 255.0f)
        );
    }

    // ============================================================================
    // Swizzle Operations (HLSL style)
    // ============================================================================

    half2 xy() const noexcept { return half2(x, y); }
    half2 xz() const noexcept { return half2(x, z); }
    half2 xw() const noexcept { return half2(x, w); }
    half2 yz() const noexcept { return half2(y, z); }
    half2 yw() const noexcept { return half2(y, w); }
    half2 zw() const noexcept { return half2(z, w); }

    half3 xyz() const noexcept { return half3(x, y, z); }
    half3 xyw() const noexcept { return half3(x, y, w); }
    half3 xzw() const noexcept { return half3(x, z, w); }
    half3 yzw() const noexcept { return half3(y, z, w); }

    half4 yxzw() const noexcept { return half4(y, x, z, w); }
    half4 zxyw() const noexcept { return half4(z, x, y, w); }
    half4 zyxw() const noexcept { return half4(z, y, x, w); }
    half4 wzyx() const noexcept { return half4(w, z, y, x); }

    // Color swizzles
    half r() const noexcept { return x; }
    half g() const noexcept { return y; }
    half b() const noexcept { return z; }
    half a() const noexcept { return w; }
    half2 rg() const noexcept { return half2(x, y); }
    half2 rb() const noexcept { return half2(x, z); }
    half2 ra() const noexcept { return half2(x, w); }
    half2 gb() const noexcept { return half2(y, z); }
    half2 ga() const noexcept { return half2(y, w); }
    half2 ba() const noexcept { return half2(z, w); }

    half3 rgb() const noexcept { return half3(x, y, z); }
    half3 rga() const noexcept { return half3(x, y, w); }
    half3 rba() const noexcept { return half3(x, z, w); }
    half3 gba() const noexcept { return half3(y, z, w); }

    half4 grba() const noexcept { return half4(y, x, z, w); }
    half4 brga() const noexcept { return half4(z, x, y, w); }
    half4 bgra() const noexcept { return half4(z, y, x, w); }
    half4 abgr() const noexcept { return half4(w, z, y, x); }

    // ============================================================================
    // Basic Checks
    // ============================================================================

    bool is_valid() const noexcept { return !is_nan() && !is_inf(); }
    bool is_inf() const noexcept { return x.is_inf() || y.is_inf() || z.is_inf() || w.is_inf(); }
    bool is_all_inf() const noexcept { return x.is_inf() && y.is_inf() && z.is_inf() && w.is_inf(); }
    bool is_negative_inf() const noexcept { return x.is_negative_inf() || y.is_negative_inf() || z.is_negative_inf() || w.is_negative_inf(); }
    bool is_all_negative_inf() const noexcept { return x.is_negative_inf() && y.is_negative_inf() && z.is_negative_inf() && w.is_negative_inf(); }
    bool is_positive_inf() const noexcept { return x.is_positive_inf() || y.is_positive_inf() || z.is_positive_inf() || w.is_positive_inf(); }
    bool is_all_positive_inf() const noexcept { return x.is_positive_inf() && y.is_positive_inf() && z.is_positive_inf() && w.is_positive_inf(); }
    bool is_negative() const noexcept { return x.is_negative() || y.is_negative() || z.is_negative() || w.is_negative(); }
    bool is_all_negative() const noexcept { return x.is_negative() && y.is_negative() && z.is_negative() && w.is_negative(); }
    bool is_positive() const noexcept { return x.is_positive() || y.is_positive() || z.is_positive() || w.is_positive(); }
    bool is_all_positive() const noexcept { return x.is_positive() && y.is_positive() && z.is_positive() && w.is_positive(); }
    bool is_nan() const noexcept { return x.is_nan() || y.is_nan() || z.is_nan() || w.is_nan(); }
    bool is_all_nan() const noexcept { return x.is_nan() && y.is_nan() && z.is_nan() && w.is_nan(); }
    bool is_finite() const noexcept { return !is_nan() && !is_inf(); }
    bool is_all_finite() const noexcept { return x.is_finite() && y.is_finite() && z.is_finite() && w.is_finite(); }
    bool is_zero() const noexcept { return x.is_zero() || y.is_zero() || z.is_zero() || w.is_zero(); }
    bool is_all_zero() const noexcept { return x.is_zero() && y.is_zero() && z.is_zero() && w.is_zero(); }
    bool is_positive_zero() const noexcept { return x.is_positive_zero() || y.is_positive_zero() || z.is_positive_zero() || w.is_positive_zero(); }
    bool is_all_positive_zero() const noexcept { return x.is_positive_zero() && y.is_positive_zero() && z.is_positive_zero() && w.is_positive_zero(); }
    bool is_negative_zero() const noexcept { return x.is_negative_zero() || y.is_negative_zero() || z.is_negative_zero() || w.is_negative_zero(); }
    bool is_all_negative_zero() const noexcept { return x.is_negative_zero() && y.is_negative_zero() && z.is_negative_zero() && w.is_negative_zero(); }
    bool is_normal() const noexcept { return x.is_normal() || y.is_normal() || z.is_normal() || w.is_normal(); }
    bool is_all_normal() const noexcept { return x.is_normal() && y.is_normal() && z.is_normal() && w.is_normal(); }

    // ============================================================================
    // Utility Methods
    // ============================================================================

    std::string to_string() const {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "(%.3f, %.3f, %.3f, %.3f)", float(x), float(y), float(z), float(w));
        return std::string(buffer);
    }

    const half* data() const noexcept { return &x; }
    half* data() noexcept { return &x; }
    void set_xyz(const half3& xyz) noexcept { x = xyz.x; y = xyz.y; z = xyz.z; }
    void set_xy(const half2& xy) noexcept { x = xy.x; y = xy.y; }
    void set_zw(const half2& zw) noexcept { z = zw.x; w = zw.y; }

    // ============================================================================
    // Comparison Operators
    // ============================================================================

    bool operator==(const half4& rhs) const noexcept {
        return approximately(x, rhs.x) && approximately(y, rhs.y) && approximately(z, rhs.z) && approximately(w, rhs.w);
    }

    bool operator!=(const half4& rhs) const noexcept {
        return !(*this == rhs);
    }
};

// ============================================================================
// Binary Operators
// ============================================================================

inline half4 operator+(half4 lhs, const half4& rhs) noexcept { return lhs += rhs; }
inline half4 operator-(half4 lhs, const half4& rhs) noexcept { return lhs -= rhs; }
inline half4 operator*(half4 lhs, const half4& rhs) noexcept { return lhs *= rhs; }
inline half4 operator/(half4 lhs, const half4& rhs) noexcept { return lhs /= rhs; }

inline half4 operator+(half4 vec, half scalar) noexcept { return vec += scalar; }
inline half4 operator+(half scalar, half4 vec) noexcept { return vec += scalar; }
inline half4 operator-(half4 vec, half scalar) noexcept { return vec -= scalar; }

inline half4 operator*(half4 vec, half scalar) noexcept { return vec *= scalar; }
inline half4 operator*(half scalar, half4 vec) noexcept { return vec *= scalar; }
inline half4 operator/(half4 vec, half scalar) noexcept { return vec /= scalar; }

inline half4 operator*(half4 vec, float scalar) noexcept { return vec *= scalar; }
inline half4 operator*(float scalar, half4 vec) noexcept { return vec *= scalar; }
inline half4 operator/(half4 vec, float scalar) noexcept { return vec /= scalar; }

inline half4 operator+(half4 vec, float scalar) noexcept {
    return half4(vec.x + scalar, vec.y + scalar, vec.z + scalar, vec.w + scalar);
}
inline half4 operator+(float scalar, half4 vec) noexcept {
    return half4(scalar + vec.x, scalar + vec.y, scalar + vec.z, scalar + vec.w);
}
inline half4 operator-(half4 vec, float scalar) noexcept {
    return half4(vec.x - scalar, vec.y - scalar, vec.z - scalar, vec.w - scalar);
}
inline half4 operator-(float scalar, half4 vec) noexcept {
    return half4(scalar - vec.x, scalar - vec.y, scalar - vec.z, scalar - vec.w);
}

// ============================================================================
// Mixed Type Operators (half4 <-> float4)
// ============================================================================

inline half4 operator+(const half4& lhs, const float4& rhs) noexcept {
    return half4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
}
inline half4 operator-(const half4& lhs, const float4& rhs) noexcept {
    return half4(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
}
inline half4 operator*(const half4& lhs, const float4& rhs) noexcept {
    return half4(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
}
inline half4 operator/(const half4& lhs, const float4& rhs) noexcept {
    return half4(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w);
}

inline half4 operator+(const float4& lhs, const half4& rhs) noexcept {
    return half4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
}
inline half4 operator-(const float4& lhs, const half4& rhs) noexcept {
    return half4(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
}
inline half4 operator*(const float4& lhs, const half4& rhs) noexcept {
    return half4(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
}
inline half4 operator/(const float4& lhs, const half4& rhs) noexcept {
    return half4(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w);
}

// ============================================================================
// GLOBAL MATHEMATICAL FUNCTIONS (HLSL-style)
// ============================================================================

// ============================================================================
// Basic vector operations
// ============================================================================
inline half length(const half4& vec) noexcept {
    float fx = float(vec.x);
    float fy = float(vec.y);
    float fz = float(vec.z);
    float fw = float(vec.w);
    return half(std::sqrt(fx * fx + fy * fy + fz * fz + fw * fw));
}

inline half length_sq(const half4& vec) noexcept {
    return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w;
}

inline half4 normalize(const half4& vec) noexcept {
    float fx = float(vec.x);
    float fy = float(vec.y);
    float fz = float(vec.z);
    float fw = float(vec.w);
    float len = std::sqrt(fx * fx + fy * fy + fz * fz + fw * fw);

    if (len == 0.0f || !std::isfinite(len)) {
        return half4::zero();
    }

    return half4(fx / len, fy / len, fz / len, fw / len);
}

inline half dot(const half4& a, const half4& b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline half dot3(const half4& a, const half4& b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline half4 cross(const half4& a, const half4& b) noexcept {
    return half4(
        half(float(a.y) * float(b.z) - float(a.z) * float(b.y)),
        half(float(a.z) * float(b.x) - float(a.x) * float(b.z)),
        half(float(a.x) * float(b.y) - float(a.y) * float(b.x)),
        half(0.0f)
    );
}

inline half distance(const half4& a, const half4& b) noexcept {
    return length(b - a);
}

inline half distance_sq(const half4& a, const half4& b) noexcept {
    return length_sq(b - a);
}

// ============================================================================
// Geometric operations
// ============================================================================
inline half4 reflect(const half4& incident, const half4& normal) noexcept {
    half dot_val = dot(incident, normal);
    return incident - half(2.0f) * dot_val * normal;
}

inline half4 refract(const half4& incident, const half4& normal, half eta) noexcept {
    half dot_ni = dot(normal, incident);
    half k = half(1.0f) - eta * eta * (half(1.0f) - dot_ni * dot_ni);

    if (k < half(0.0f))
        return half4::zero(); // total internal reflection

    return incident * eta - normal * (eta * dot_ni + sqrt(k));
}

inline half3 project(const half4& vec) noexcept {
    float fw = float(vec.w);
    if (std::abs(fw) > 1e-12f) {
        float inv_w = 1.0f / fw;
        return half3(
            half(float(vec.x) * inv_w),
            half(float(vec.y) * inv_w),
            half(float(vec.z) * inv_w)
        );
    }
    return half3::zero();
}

inline half4 to_homogeneous(const half4& vec) noexcept {
    return half4(vec.x, vec.y, vec.z, half_One);
}

inline half angle_between(const half4& a, const half4& b) noexcept {
    half4 a_norm = normalize(a);
    half4 b_norm = normalize(b);
    half dot_val = dot(a_norm, b_norm);
    dot_val = clamp(dot_val, -half_One, half_One);
    return half(std::acos(float(dot_val)));
}

// ============================================================================
// HLSL-like functions (component-wise)
// ============================================================================
inline half4 abs(const half4& vec) noexcept {
    return half4(abs(vec.x), abs(vec.y), abs(vec.z), abs(vec.w));
}

inline half4 sign(const half4& vec) noexcept {
    return half4(sign(vec.x), sign(vec.y), sign(vec.z), sign(vec.w));
}

inline half4 floor(const half4& vec) noexcept {
    return half4(floor(vec.x), floor(vec.y), floor(vec.z), floor(vec.w));
}

inline half4 ceil(const half4& vec) noexcept {
    return half4(ceil(vec.x), ceil(vec.y), ceil(vec.z), ceil(vec.w));
}

inline half4 round(const half4& vec) noexcept {
    return half4(round(vec.x), round(vec.y), round(vec.z), round(vec.w));
}

inline half4 frac(const half4& vec) noexcept {
    auto frac_channel = [](half h) -> half {
        float f = float(h);
        return half(f - std::floor(f));
    };
    return half4(frac_channel(vec.x), frac_channel(vec.y),
        frac_channel(vec.z), frac_channel(vec.w));
}

inline half4 saturate(const half4& vec) noexcept {
    return half4(saturate(vec.x), saturate(vec.y),
        saturate(vec.z), saturate(vec.w));
}

inline half4 step(half edge, const half4& vec) noexcept {
    return half4(step(edge, vec.x), step(edge, vec.y),
        step(edge, vec.z), step(edge, vec.w));
}

inline half4 smoothstep(half edge0, half edge1, const half4& vec) noexcept {
    return half4(smoothstep(edge0, edge1, vec.x),
        smoothstep(edge0, edge1, vec.y),
        smoothstep(edge0, edge1, vec.z),
        smoothstep(edge0, edge1, vec.w));
}

// ============================================================================
// Min/max/clamp functions
// ============================================================================
inline half4 min(const half4& a, const half4& b) noexcept {
    return half4(min(a.x, b.x), min(a.y, b.y),
        min(a.z, b.z), min(a.w, b.w));
}

inline half4 max(const half4& a, const half4& b) noexcept {
    return half4(max(a.x, b.x), max(a.y, b.y),
        max(a.z, b.z), max(a.w, b.w));
}

inline half4 clamp(const half4& vec, const half4& min_val, const half4& max_val) noexcept {
    return half4(clamp(vec.x, min_val.x, max_val.x),
        clamp(vec.y, min_val.y, max_val.y),
        clamp(vec.z, min_val.z, max_val.z),
        clamp(vec.w, min_val.w, max_val.w));
}

inline half4 clamp(const half4& vec, float min_val, float max_val) noexcept {
    return half4(clamp(vec.x, min_val, max_val),
        clamp(vec.y, min_val, max_val),
        clamp(vec.z, min_val, max_val),
        clamp(vec.w, min_val, max_val));
}

// ============================================================================
// Interpolation
// ============================================================================
inline half4 lerp(const half4& a, const half4& b, half t) noexcept {
    return half4(a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t);
}

inline half4 lerp(const half4& a, const half4& b, float t) noexcept {
    return half4(a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t);
}

// ============================================================================
// Check and comparison functions
// ============================================================================
inline bool approximately(const half4& a, const half4& b, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
    return approximately(a.x, b.x, epsilon) &&
        approximately(a.y, b.y, epsilon) &&
        approximately(a.z, b.z, epsilon) &&
        approximately(a.w, b.w, epsilon);
}

inline bool approximately_zero(const half4& vec, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
    return approximately_zero(vec.x, epsilon) &&
        approximately_zero(vec.y, epsilon) &&
        approximately_zero(vec.z, epsilon) &&
        approximately_zero(vec.w, epsilon);
}

inline bool is_normalized(const half4& vec, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
    half len_sq = length_sq(vec);
    float adjusted_epsilon = std::max(epsilon, 0.01f);
    return approximately(float(len_sq), 1.0f, adjusted_epsilon);
}

// ============================================================================
// Color Operations
// ============================================================================
inline half luminance(const half4& rgb) noexcept {
    return half(0.2126f) * rgb.x + half(0.7152f) * rgb.y + half(0.0722f) * rgb.z;
}

inline half brightness(const half4& rgb) noexcept {
    return (rgb.x + rgb.y + rgb.z) / half(3.0f);
}

inline half4 rgb_to_grayscale(const half4& rgb) noexcept {
    half luma = luminance(rgb);
    return half4(luma, luma, luma, rgb.w);
}

inline half4 premultiply_alpha(const half4& color) noexcept {
    return half4(color.x * color.w, color.y * color.w, color.z * color.w, color.w);
}

inline half4 unpremultiply_alpha(const half4& color) noexcept {
    float fw = float(color.w);
    if (fw == 0.0f) {
        return color;
    }
    if (std::abs(fw) < 1e-6f) {
        float safe_alpha = std::max(std::abs(fw), 1e-6f);
        if (fw < 0.0f) safe_alpha = -safe_alpha;
        float inv_alpha = 1.0f / safe_alpha;
        return half4(
            half(clamp(float(color.x) * inv_alpha, -1000.0f, 1000.0f)),
            half(clamp(float(color.y) * inv_alpha, -1000.0f, 1000.0f)),
            half(clamp(float(color.z) * inv_alpha, -1000.0f, 1000.0f)),
            color.w
        );
    }
    float inv_alpha = 1.0f / fw;
    return half4(
        half(float(color.x) * inv_alpha),
        half(float(color.y) * inv_alpha),
        half(float(color.z) * inv_alpha),
        color.w
    );
}

inline half4 srgb_to_linear(const half4& srgb) noexcept {
    auto srgb_to_linear_channel = [](half channel) -> half {
        float c = float(channel);
        return (c <= 0.04045f) ? half(c / 12.92f) : half(std::pow((c + 0.055f) / 1.055f, 2.4f));
    };
    return half4(srgb_to_linear_channel(srgb.x),
        srgb_to_linear_channel(srgb.y),
        srgb_to_linear_channel(srgb.z),
        srgb.w); // alpha remains linear
}

inline half4 linear_to_srgb(const half4& linear) noexcept {
    auto linear_to_srgb_channel = [](half channel) -> half {
        float c = float(channel);
        return (c <= 0.0031308f) ? half(c * 12.92f) : half(1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f);
    };
    return half4(linear_to_srgb_channel(linear.x),
        linear_to_srgb_channel(linear.y),
        linear_to_srgb_channel(linear.z),
        linear.w); // alpha remains linear
}

// ============================================================================
// Global check functions (for compatibility)
// ============================================================================
inline bool is_valid(const half4& vec) noexcept { return vec.is_valid(); }
inline bool is_inf(const half4& vec) noexcept { return vec.is_inf(); }
inline bool is_all_inf(const half4& vec) noexcept { return vec.is_all_inf(); }
inline bool is_negative_inf(const half4& vec) noexcept { return vec.is_negative_inf(); }
inline bool is_all_negative_inf(const half4& vec) noexcept { return vec.is_all_negative_inf(); }
inline bool is_positive_inf(const half4& vec) noexcept { return vec.is_positive_inf(); }
inline bool is_all_positive_inf(const half4& vec) noexcept { return vec.is_all_positive_inf(); }
inline bool is_negative(const half4& vec) noexcept { return vec.is_negative(); }
inline bool is_all_negative(const half4& vec) noexcept { return vec.is_all_negative(); }
inline bool is_positive(const half4& vec) noexcept { return vec.is_positive(); }
inline bool is_all_positive(const half4& vec) noexcept { return vec.is_all_positive(); }
inline bool is_nan(const half4& vec) noexcept { return vec.is_nan(); }
inline bool is_all_nan(const half4& vec) noexcept { return vec.is_all_nan(); }
inline bool is_finite(const half4& vec) noexcept { return vec.is_finite(); }
inline bool is_all_finite(const half4& vec) noexcept { return vec.is_all_finite(); }
inline bool is_zero(const half4& vec) noexcept { return vec.is_zero(); }
inline bool is_all_zero(const half4& vec) noexcept { return vec.is_all_zero(); }
inline bool is_positive_zero(const half4& vec) noexcept { return vec.is_positive_zero(); }
inline bool is_all_positive_zero(const half4& vec) noexcept { return vec.is_all_positive_zero(); }
inline bool is_negative_zero(const half4& vec) noexcept { return vec.is_negative_zero(); }
inline bool is_all_negative_zero(const half4& vec) noexcept { return vec.is_all_negative_zero(); }
inline bool is_normal(const half4& vec) noexcept { return vec.is_normal(); }
inline bool is_all_normal(const half4& vec) noexcept { return vec.is_all_normal(); }

// ============================================================================
// Type conversion functions
// ============================================================================
inline float4 to_float4(const half4& vec) noexcept {
    return float4(float(vec.x), float(vec.y), float(vec.z), float(vec.w));
}

inline half4 to_half4(const float4& vec) noexcept {
    return half4(vec.x, vec.y, vec.z, vec.w);
}

// ============================================================================
// Utility functions
// ============================================================================
inline half4 ensure_normalized(const half4& normal, const half4& fallback = half4::unit_w()) noexcept {
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
// Useful constants
// ============================================================================
AFTERMATH_INLINE_VAR const half4 half4_Zero(half_Zero);
AFTERMATH_INLINE_VAR const half4 half4_One(half_One);
AFTERMATH_INLINE_VAR const half4 half4_UnitX(half_One, half_Zero, half_Zero, half_Zero);
AFTERMATH_INLINE_VAR const half4 half4_UnitY(half_Zero, half_One, half_Zero, half_Zero);
AFTERMATH_INLINE_VAR const half4 half4_UnitZ(half_Zero, half_Zero, half_One, half_Zero);
AFTERMATH_INLINE_VAR const half4 half4_UnitW(half_Zero, half_Zero, half_Zero, half_One);
AFTERMATH_INLINE_VAR const half4 half4_Red(half_One, half_Zero, half_Zero, half_One);
AFTERMATH_INLINE_VAR const half4 half4_Green(half_Zero, half_One, half_Zero, half_One);
AFTERMATH_INLINE_VAR const half4 half4_Blue(half_Zero, half_Zero, half_One, half_One);
AFTERMATH_INLINE_VAR const half4 half4_White(half_One);
AFTERMATH_INLINE_VAR const half4 half4_Black(half_Zero, half_Zero, half_Zero, half_One);
AFTERMATH_INLINE_VAR const half4 half4_Transparent(half_Zero, half_Zero, half_Zero, half_Zero);
AFTERMATH_INLINE_VAR const half4 half4_Yellow(half_One, half_One, half_Zero, half_One);
AFTERMATH_INLINE_VAR const half4 half4_Cyan(half_Zero, half_One, half_One, half_One);
AFTERMATH_INLINE_VAR const half4 half4_Magenta(half_One, half_Zero, half_One, half_One);

AFTERMATH_END
