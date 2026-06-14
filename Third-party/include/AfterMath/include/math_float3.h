// math_float3.h
// Description: 3-dimensional vector class with HLSL-like syntax and SSE optimization
// Author: NSDeathman, DeepSeek

#pragma once

#include <string>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cassert>
#include <xmmintrin.h>
#include <pmmintrin.h>

#include "math_float2.h"
#include "math_constants.h"
#include "AfterMathInternal.h"

AFTERMATH_BEGIN

// Forward declarations
class float2;
class float3;

// ============================================================================
// 3D Vector Class
// ============================================================================

/**
    * @class float3
    * @brief 3-dimensional vector with HLSL-like syntax
    *
    * Represents a 3D vector (x, y, z) with 12-byte packing for compatibility
    * with standard vertex buffers. Operations use SSE acceleration by
    * loading/storing to registers on the fly.
    *
    * @note Perfect for 3D game development, computer graphics, and physics engines
    * @note Size is exactly 12 bytes
    */
class float3
{
public:
    // Data members (public for direct access)
    float x; ///< X component of the vector
    float y; ///< Y component of the vector  
    float z; ///< Z component of the vector

    // ============================================================================
    // Constructors
    // ============================================================================

    constexpr float3() noexcept : x(0.0f), y(0.0f), z(0.0f) {}

    constexpr float3(float x, float y, float z) noexcept : x(x), y(y), z(z) {}

    explicit constexpr float3(float scalar) noexcept : x(scalar), y(scalar), z(scalar) {}

    // Note: float2 declaration would be needed here
    // float3(const float2& vec, float z = 0.0f) noexcept : x(vec.x), y(vec.y), z(z) {}

    constexpr float3(const float3&) noexcept = default;

    explicit float3(const float* data) noexcept : x(data[0]), y(data[1]), z(data[2]) {}

    explicit float3(__m128 simd_val) noexcept {
        alignas(16) float temp[4];
        _mm_store_ps(temp, simd_val);
        x = temp[0];
        y = temp[1];
        z = temp[2];
    }

    // ============================================================================
    // Assignment Operators
    // ============================================================================

    float3& operator=(const float3&) noexcept = default;

    float3& operator=(float scalar) noexcept {
        x = scalar;
        y = scalar;
        z = scalar;
        return *this;
    }

    // ============================================================================
    // Compound Assignment Operators
    // ============================================================================

    float3& operator+=(const float3& rhs) noexcept {
        __m128 a = _mm_set_ps(0.0f, z, y, x);
        __m128 b = _mm_set_ps(0.0f, rhs.z, rhs.y, rhs.x);
        __m128 result = _mm_add_ps(a, b);

        alignas(16) float temp[4];
        _mm_store_ps(temp, result);
        x = temp[0];
        y = temp[1];
        z = temp[2];
        return *this;
    }

    float3& operator-=(const float3& rhs) noexcept {
        __m128 a = _mm_set_ps(0.0f, z, y, x);
        __m128 b = _mm_set_ps(0.0f, rhs.z, rhs.y, rhs.x);
        __m128 result = _mm_sub_ps(a, b);

        alignas(16) float temp[4];
        _mm_store_ps(temp, result);
        x = temp[0];
        y = temp[1];
        z = temp[2];
        return *this;
    }

    float3& operator*=(const float3& rhs) noexcept {
        __m128 a = _mm_set_ps(0.0f, z, y, x);
        __m128 b = _mm_set_ps(0.0f, rhs.z, rhs.y, rhs.x);
        __m128 result = _mm_mul_ps(a, b);

        alignas(16) float temp[4];
        _mm_store_ps(temp, result);
        x = temp[0];
        y = temp[1];
        z = temp[2];
        return *this;
    }

    float3& operator/=(const float3& rhs) noexcept {
        __m128 a = _mm_set_ps(0.0f, z, y, x);
        __m128 b = _mm_set_ps(0.0f, rhs.z, rhs.y, rhs.x);
        __m128 result = _mm_div_ps(a, b);

        alignas(16) float temp[4];
        _mm_store_ps(temp, result);
        x = temp[0];
        y = temp[1];
        z = temp[2];
        return *this;
    }

    float3& operator*=(float scalar) noexcept {
        __m128 a = _mm_set_ps(0.0f, z, y, x);
        __m128 s = _mm_set1_ps(scalar);
        __m128 result = _mm_mul_ps(a, s);

        alignas(16) float temp[4];
        _mm_store_ps(temp, result);
        x = temp[0];
        y = temp[1];
        z = temp[2];
        return *this;
    }

    float3& operator/=(float scalar) noexcept {
        __m128 a = _mm_set_ps(0.0f, z, y, x);
        __m128 s = _mm_set1_ps(scalar);
        __m128 result = _mm_div_ps(a, s);

        alignas(16) float temp[4];
        _mm_store_ps(temp, result);
        x = temp[0];
        y = temp[1];
        z = temp[2];
        return *this;
    }

    // ============================================================================
    // Binary Operators
    // ============================================================================

    float3 operator+(const float3& rhs) const noexcept {
        return float3(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    float3 operator-(const float3& rhs) const noexcept {
        return float3(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    float3 operator+(float rhs) const noexcept {
        return float3(x + rhs, y + rhs, z + rhs);
    }

    float3 operator-(float rhs) const noexcept {
        return float3(x - rhs, y - rhs, z - rhs);
    }

    // ============================================================================
    // Unary Operators
    // ============================================================================

    constexpr float3 operator+() const noexcept { return *this; }

    constexpr float3 operator-() const noexcept { return float3(-x, -y, -z); }

    // ============================================================================
    // Access Operators
    // ============================================================================

    float& operator[](int index) noexcept {
        assert(index >= 0 && index < 3);
        return (&x)[index];
    }

    const float& operator[](int index) const noexcept {
        assert(index >= 0 && index < 3);
        return (&x)[index];
    }

    // ============================================================================
    // Conversion Operators
    // ============================================================================

    operator const float* () const noexcept { return &x; }
    operator float* () noexcept { return &x; }

    operator __m128() const noexcept {
        return _mm_set_ps(0.0f, z, y, x);
    }

    // ============================================================================
    // Static Constructors
    // ============================================================================

    static constexpr float3 zero() noexcept { return float3(0.0f, 0.0f, 0.0f); }
    static constexpr float3 one() noexcept { return float3(1.0f, 1.0f, 1.0f); }
    static constexpr float3 unit_x() noexcept { return float3(1.0f, 0.0f, 0.0f); }
    static constexpr float3 unit_y() noexcept { return float3(0.0f, 1.0f, 0.0f); }
    static constexpr float3 unit_z() noexcept { return float3(0.0f, 0.0f, 1.0f); }
    static constexpr float3 forward() noexcept { return float3(0.0f, 0.0f, 1.0f); }
    static constexpr float3 up() noexcept { return float3(0.0f, 1.0f, 0.0f); }
    static constexpr float3 right() noexcept { return float3(1.0f, 0.0f, 0.0f); }

    // ============================================================================
    // Basic Properties
    // ============================================================================

    constexpr float length_sq() const noexcept { return x * x + y * y + z * z; }

    constexpr float distance_sq(const float3& other) const noexcept {
        float dx = x - other.x;
        float dy = y - other.y;
        float dz = z - other.z;
        return dx * dx + dy * dy + dz * dz;
    }

    // ============================================================================
    // Swizzle Operations (Complete Set)
    // ============================================================================

    // 2-component swizzles (all permutations)
    constexpr float2 xx() const noexcept { return float2(x, x); }
    constexpr float2 xy() const noexcept { return float2(x, y); }
    constexpr float2 xz() const noexcept { return float2(x, z); }
    constexpr float2 yx() const noexcept { return float2(y, x); }
    constexpr float2 yy() const noexcept { return float2(y, y); }
    constexpr float2 yz() const noexcept { return float2(y, z); }
    constexpr float2 zx() const noexcept { return float2(z, x); }
    constexpr float2 zy() const noexcept { return float2(z, y); }
    constexpr float2 zz() const noexcept { return float2(z, z); }

    // 3-component swizzles (all permutations)
    constexpr float3 xxx() const noexcept { return float3(x, x, x); }
    constexpr float3 xxy() const noexcept { return float3(x, x, y); }
    constexpr float3 xxz() const noexcept { return float3(x, x, z); }
    constexpr float3 xyx() const noexcept { return float3(x, y, x); }
    constexpr float3 xyy() const noexcept { return float3(x, y, y); }
    constexpr float3 xyz() const noexcept { return float3(x, y, z); }
    constexpr float3 xzx() const noexcept { return float3(x, z, x); }
    constexpr float3 xzy() const noexcept { return float3(x, z, y); }
    constexpr float3 xzz() const noexcept { return float3(x, z, z); }

    constexpr float3 yxx() const noexcept { return float3(y, x, x); }
    constexpr float3 yxy() const noexcept { return float3(y, x, y); }
    constexpr float3 yxz() const noexcept { return float3(y, x, z); }
    constexpr float3 yyx() const noexcept { return float3(y, y, x); }
    constexpr float3 yyy() const noexcept { return float3(y, y, y); }
    constexpr float3 yyz() const noexcept { return float3(y, y, z); }
    constexpr float3 yzx() const noexcept { return float3(y, z, x); }
    constexpr float3 yzy() const noexcept { return float3(y, z, y); }
    constexpr float3 yzz() const noexcept { return float3(y, z, z); }

    constexpr float3 zxx() const noexcept { return float3(z, x, x); }
    constexpr float3 zxy() const noexcept { return float3(z, x, y); }
    constexpr float3 zxz() const noexcept { return float3(z, x, z); }
    constexpr float3 zyx() const noexcept { return float3(z, y, x); }
    constexpr float3 zyy() const noexcept { return float3(z, y, y); }
    constexpr float3 zyz() const noexcept { return float3(z, y, z); }
    constexpr float3 zzx() const noexcept { return float3(z, z, x); }
    constexpr float3 zzy() const noexcept { return float3(z, z, y); }
    constexpr float3 zzz() const noexcept { return float3(z, z, z); }

    // Color swizzles (RGB notation)
    // 1-component
    constexpr float r() const noexcept { return x; }
    constexpr float g() const noexcept { return y; }
    constexpr float b() const noexcept { return z; }

    // 2-component color swizzles
    constexpr float2 rr() const noexcept { return float2(x, x); }
    constexpr float2 rg() const noexcept { return float2(x, y); }
    constexpr float2 rb() const noexcept { return float2(x, z); }
    constexpr float2 gr() const noexcept { return float2(y, x); }
    constexpr float2 gg() const noexcept { return float2(y, y); }
    constexpr float2 gb() const noexcept { return float2(y, z); }
    constexpr float2 br() const noexcept { return float2(z, x); }
    constexpr float2 bg() const noexcept { return float2(z, y); }
    constexpr float2 bb() const noexcept { return float2(z, z); }

    // 3-component color swizzles (complete set)
    constexpr float3 rrr() const noexcept { return float3(x, x, x); }
    constexpr float3 rrg() const noexcept { return float3(x, x, y); }
    constexpr float3 rrb() const noexcept { return float3(x, x, z); }
    constexpr float3 rgr() const noexcept { return float3(x, y, x); }
    constexpr float3 rgg() const noexcept { return float3(x, y, y); }
    constexpr float3 rgb() const noexcept { return float3(x, y, z); }
    constexpr float3 rbr() const noexcept { return float3(x, z, x); }
    constexpr float3 rbg() const noexcept { return float3(x, z, y); }
    constexpr float3 rbb() const noexcept { return float3(x, z, z); }

    constexpr float3 grr() const noexcept { return float3(y, x, x); }
    constexpr float3 grg() const noexcept { return float3(y, x, y); }
    constexpr float3 grb() const noexcept { return float3(y, x, z); }
    constexpr float3 ggr() const noexcept { return float3(y, y, x); }
    constexpr float3 ggg() const noexcept { return float3(y, y, y); }
    constexpr float3 ggb() const noexcept { return float3(y, y, z); }
    constexpr float3 gbr() const noexcept { return float3(y, z, x); }
    constexpr float3 gbg() const noexcept { return float3(y, z, y); }
    constexpr float3 gbb() const noexcept { return float3(y, z, z); }

    constexpr float3 brr() const noexcept { return float3(z, x, x); }
    constexpr float3 brg() const noexcept { return float3(z, x, y); }
    constexpr float3 brb() const noexcept { return float3(z, x, z); }
    constexpr float3 bgr() const noexcept { return float3(z, y, x); }
    constexpr float3 bgg() const noexcept { return float3(z, y, y); }
    constexpr float3 bgb() const noexcept { return float3(z, y, z); }
    constexpr float3 bbr() const noexcept { return float3(z, z, x); }
    constexpr float3 bbg() const noexcept { return float3(z, z, y); }
    constexpr float3 bbb() const noexcept { return float3(z, z, z); }

    // Swizzles that return float2 (common graphics operations)
    constexpr float2 st() const noexcept { return float2(x, y); } // Texture coordinates
    constexpr float2 ts() const noexcept { return float2(y, x); } // Swapped texture coordinates
    constexpr float2 sp() const noexcept { return float2(x, z); } // Special coordinates
    constexpr float2 ps() const noexcept { return float2(z, x); } // Special coordinates swapped

    // ============================================================================
    // Utility swizzle functions for common operations
    // ============================================================================

    /**
        * @brief Get sum of all components
        * @return x + y + z
        */
    constexpr float sum() const noexcept {
        return x + y + z;
    }

    /**
        * @brief Get product of all components
        * @return x * y * z
        */
    constexpr float product() const noexcept {
        return x * y * z;
    }

    // ============================================================================
    // Utility Methods
    // ============================================================================

    std::string to_string() const {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "(%.3f, %.3f, %.3f)", x, y, z);
        return std::string(buffer);
    }

    const float* data() const noexcept { return &x; }
    float* data() noexcept { return &x; }

    // ============================================================================
    // Component Operations
    // ============================================================================

    float min_component() const noexcept {
        return std::min({ x, y, z });
    }

    float max_component() const noexcept {
        return std::max({ x, y, z });
    }

    int min_component_index() const noexcept {
        if (x <= y && x <= z) return 0;
        if (y <= z) return 1;
        return 2;
    }

    int max_component_index() const noexcept {
        if (x >= y && x >= z) return 0;
        if (y >= z) return 1;
        return 2;
    }

    float sum_components() const noexcept {
        return x + y + z;
    }

    float product_components() const noexcept {
        return x * y * z;
    }

    float average() const noexcept {
        return (x + y + z) / 3.0f;
    }

    bool has_nan() const noexcept {
        return std::isnan(x) || std::isnan(y) || std::isnan(z);
    }

    bool has_infinite() const noexcept {
        return std::isinf(x) || std::isinf(y) || std::isinf(z);
    }

    bool all_finite() const noexcept {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
    }
};

// ============================================================================
// Global Mathematical Functions (HLSL Style)
// ============================================================================

// Vector operations
inline float3 operator*(float3 lhs, const float3& rhs) noexcept {
    return float3(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
}

inline float3 operator/(float3 lhs, const float3& rhs) noexcept {
    return float3(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z);
}

inline float3 operator*(float3 vec, float scalar) noexcept {
    return float3(vec.x * scalar, vec.y * scalar, vec.z * scalar);
}

inline float3 operator*(float scalar, float3 vec) noexcept {
    return vec * scalar;
}

inline float3 operator/(float3 vec, float scalar) noexcept {
    return vec * (1.0f / scalar);
}

inline float3 operator/(float scalar, float3 vec) noexcept {
    return float3(scalar / vec.x, scalar / vec.y, scalar / vec.z);
}

// Length and distance
inline float length(const float3& v) noexcept {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline float length_sq(const float3& v) noexcept {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

inline float distance(const float3& a, const float3& b) noexcept {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

inline float distance_sq(const float3& a, const float3& b) noexcept {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

// Normalization
inline float3 normalize(const float3& v) noexcept {
    float len = length(v);
    if (len < EPSILON) {
        return float3::zero();
    }
    return v / len;
}

// Dot and cross products
inline float dot(const float3& a, const float3& b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float3 cross(const float3& a, const float3& b) noexcept {
    return float3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

// Comparison
inline bool approximately(const float3& a, const float3& b, float epsilon = EPSILON) noexcept {
    return std::abs(a.x - b.x) <= epsilon &&
        std::abs(a.y - b.y) <= epsilon &&
        std::abs(a.z - b.z) <= epsilon;
}

inline bool approximately_zero(const float3& v, float epsilon = 1e-6f) noexcept {
    return std::abs(v.x) <= epsilon &&
        std::abs(v.y) <= epsilon &&
        std::abs(v.z) <= epsilon;
}

inline bool isValid(const float3& v) noexcept {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

inline bool is_normalized(const float3& v, float epsilon = EPSILON) noexcept {
    return std::abs(length_sq(v) - 1.0f) <= epsilon;
}

// Interpolation
inline float3 lerp(const float3& a, const float3& b, float t) noexcept {
    return a + (b - a) * t;
}

inline float3 slerp(const float3& a, const float3& b, float t) noexcept {
    if (t <= 0.0f) return a;
    if (t >= 1.0f) return b;

    float3 an = normalize(a);
    float3 bn = normalize(b);
    float d = dot(an, bn);
    d = std::max(-1.0f, std::min(1.0f, d));

    if (d > 0.9995f) return normalize(lerp(an, bn, t));

    if (d < -0.9995f) {
        // Find an orthogonal vector
        float3 ortho;
        if (std::abs(an.x) > 0.1f) {
            ortho = float3(-an.y, an.x, 0.0f);
        }
        else {
            ortho = float3(0.0f, -an.z, an.y);
        }
        ortho = normalize(ortho);
        float angle = PI * t;
        return an * std::cos(angle) + ortho * std::sin(angle);
    }

    float theta = std::acos(d);
    float sinTheta = std::sin(theta);
    if (sinTheta < 1e-8f) return normalize(lerp(an, bn, t));

    float fa = std::sin((1.0f - t) * theta) / sinTheta;
    float fb = std::sin(t * theta) / sinTheta;
    return an * fa + bn * fb;
}

// Geometric operations
inline float3 reflect(const float3& incident, const float3& normal) noexcept {
    return incident - normal * (2.0f * dot(incident, normal));
}

inline float3 refract(const float3& incident, const float3& normal, float eta) noexcept {
    float cos_theta_i = -dot(incident, normal);
    float sin_theta_i_sq = 1.0f - cos_theta_i * cos_theta_i;
    float sin_theta_t_sq = (eta * eta) * sin_theta_i_sq;

    // Total internal reflection
    if (sin_theta_t_sq > 1.0f) {
        return float3::zero();
    }

    float cos_theta_t = std::sqrt(1.0f - sin_theta_t_sq);
    return incident * eta + normal * (eta * cos_theta_i - cos_theta_t);
}

inline float3 project(const float3& v, const float3& onto) noexcept {
    float len_sq = length_sq(onto);
    if (len_sq < EPSILON) {
        return float3::zero();
    }
    return onto * (dot(v, onto) / len_sq);
}

inline float3 reject(const float3& v, const float3& from) noexcept {
    return v - project(v, from);
}

// Angle operations
inline float angle_between(const float3& a, const float3& b) noexcept {
    float3 na = normalize(a);
    float3 nb = normalize(b);
    float dot_val = dot(na, nb);
    dot_val = std::max(-1.0f, std::min(1.0f, dot_val));
    return std::acos(dot_val);
}

// Orthogonality checks
inline bool are_orthogonal(const float3& a, const float3& b, float epsilon = EPSILON) noexcept {
    return std::abs(dot(a, b)) <= epsilon;
}

inline bool is_orthonormal_basis(const float3& x, const float3& y, const float3& z, float epsilon = EPSILON) noexcept {
    return is_normalized(x, epsilon) && is_normalized(y, epsilon) && is_normalized(z, epsilon) &&
        are_orthogonal(x, y, epsilon) && are_orthogonal(x, z, epsilon) && are_orthogonal(y, z, epsilon);
}

// ============================================================================
// HLSL-like Global Functions
// ============================================================================

inline float3 abs(const float3& v) noexcept {
    return float3(std::abs(v.x), std::abs(v.y), std::abs(v.z));
}

inline float3 sign(const float3& v) noexcept {
    return float3(
        (v.x > 0.0f) ? 1.0f : ((v.x < 0.0f) ? -1.0f : 0.0f),
        (v.y > 0.0f) ? 1.0f : ((v.y < 0.0f) ? -1.0f : 0.0f),
        (v.z > 0.0f) ? 1.0f : ((v.z < 0.0f) ? -1.0f : 0.0f)
    );
}

inline float3 floor(const float3& v) noexcept {
    return float3(std::floor(v.x), std::floor(v.y), std::floor(v.z));
}

inline float3 ceil(const float3& v) noexcept {
    return float3(std::ceil(v.x), std::ceil(v.y), std::ceil(v.z));
}

inline float3 round(const float3& v) noexcept {
    return float3(std::round(v.x), std::round(v.y), std::round(v.z));
}

inline float3 frac(const float3& v) noexcept {
    return float3(
        v.x - std::floor(v.x),
        v.y - std::floor(v.y),
        v.z - std::floor(v.z)
    );
}

inline float3 saturate(const float3& v) noexcept {
    return float3(
        std::max(0.0f, std::min(1.0f, v.x)),
        std::max(0.0f, std::min(1.0f, v.y)),
        std::max(0.0f, std::min(1.0f, v.z))
    );
}

inline float3 step(float edge, const float3& v) noexcept {
    return float3(
        (v.x >= edge) ? 1.0f : 0.0f,
        (v.y >= edge) ? 1.0f : 0.0f,
        (v.z >= edge) ? 1.0f : 0.0f
    );
}

inline float3 smoothstep(float edge0, float edge1, const float3& v) noexcept {
    auto smooth = [edge0, edge1](float t) {
        t = std::max(0.0f, std::min(1.0f, (t - edge0) / (edge1 - edge0)));
        return t * t * (3.0f - 2.0f * t);
    };
    return float3(smooth(v.x), smooth(v.y), smooth(v.z));
}

inline float3 min(const float3& a, const float3& b) noexcept {
    return float3(
        std::min(a.x, b.x),
        std::min(a.y, b.y),
        std::min(a.z, b.z)
    );
}

inline float3 max(const float3& a, const float3& b) noexcept {
    return float3(
        std::max(a.x, b.x),
        std::max(a.y, b.y),
        std::max(a.z, b.z)
    );
}

inline float3 clamp(const float3& v, const float3& min_val, const float3& max_val) noexcept {
    return float3(
        std::max(min_val.x, std::min(max_val.x, v.x)),
        std::max(min_val.y, std::min(max_val.y, v.y)),
        std::max(min_val.z, std::min(max_val.z, v.z))
    );
}

inline float3 clamp(const float3& v, float min_val, float max_val) noexcept {
    return float3(
        std::max(min_val, std::min(max_val, v.x)),
        std::max(min_val, std::min(max_val, v.y)),
        std::max(min_val, std::min(max_val, v.z))
    );
}

// ============================================================================
// Global Component Operations
// ============================================================================

inline float min_component(const float3& v) noexcept {
    return v.min_component();
}

inline float max_component(const float3& v) noexcept {
    return v.max_component();
}

inline float sum_components(const float3& v) noexcept {
    return v.sum_components();
}

inline float product_components(const float3& v) noexcept {
    return v.product_components();
}

inline float average(const float3& v) noexcept {
    return v.average();
}

// ============================================================================
// Comparison Operators
// ============================================================================

inline bool operator==(const float3& a, const float3& b) noexcept {
    return approximately(a, b);
}

inline bool operator!=(const float3& a, const float3& b) noexcept {
    return !approximately(a, b);
}

// ============================================================================
// Useful Constants
// ============================================================================

AFTERMATH_INLINE_VAR const float3 float3_Zero(0.0f, 0.0f, 0.0f);
AFTERMATH_INLINE_VAR const float3 float3_One(1.0f, 1.0f, 1.0f);
AFTERMATH_INLINE_VAR const float3 float3_UnitX(1.0f, 0.0f, 0.0f);
AFTERMATH_INLINE_VAR const float3 float3_UnitY(0.0f, 1.0f, 0.0f);
AFTERMATH_INLINE_VAR const float3 float3_UnitZ(0.0f, 0.0f, 1.0f);
AFTERMATH_INLINE_VAR const float3 float3_Forward(0.0f, 0.0f, 1.0f);
AFTERMATH_INLINE_VAR const float3 float3_Up(0.0f, 1.0f, 0.0f);
AFTERMATH_INLINE_VAR const float3 float3_Right(1.0f, 0.0f, 0.0f);

AFTERMATH_END
