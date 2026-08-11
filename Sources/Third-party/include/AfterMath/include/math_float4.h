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

#include <string>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <xmmintrin.h>
#include <pmmintrin.h>
#include <smmintrin.h>

#include "AfterMathInternal.h"
#include "math_float2.h"
#include "math_float3.h"

AFTERMATH_BEGIN

// Forward declarations
class float4;

// ============================================================================
// 4D Vector Class
// ============================================================================

/**
    * @class float4
    * @brief 4-dimensional vector with HLSL-like syntax
    *
    * Represents a 4D vector (x, y, z, w) with SSE optimization for
    * 4D graphics, homogeneous coordinates, colors with alpha, and 4D mathematics.
    *
    * @note Perfect for 4D graphics, homogeneous coordinates, RGBA colors, and quaternions
    * @note Includes comprehensive HLSL-like function set and color operations
    */
class float4
{
public:
    // Union for SSE optimization
    union {
        struct {
            float x; ///< X component of the vector
            float y; ///< Y component of the vector  
            float z; ///< Z component of the vector
            float w; ///< W component of the vector
        };
        __m128 simd_; ///< SSE register for optimized operations
    };

    // ============================================================================
    // Constructors
    // ============================================================================

    float4() noexcept : simd_(_mm_setzero_ps()) {}

    float4(float x, float y, float z, float w) noexcept :
        simd_(_mm_set_ps(w, z, y, x)) {}

    explicit float4(float scalar) noexcept :
        simd_(_mm_set1_ps(scalar)) {}

    float4(const float2& xy, float z = 0.0f, float w = 0.0f) noexcept
        : simd_(_mm_set_ps(w, z, xy.y, xy.x)) {}

    float4(const float3& xyz, float w = 0.0f) noexcept
        : simd_(_mm_set_ps(w, xyz.z, xyz.y, xyz.x)) {}

    float4(const float4&) noexcept = default;

    explicit float4(const float* data) noexcept :
        simd_(_mm_loadu_ps(data)) {}

    explicit float4(__m128 simd_val) noexcept : simd_(simd_val) {}

    // ============================================================================
    // Assignment Operators
    // ============================================================================

    float4& operator=(const float4&) noexcept = default;

    float4& operator=(float scalar) noexcept {
        simd_ = _mm_set1_ps(scalar);
        return *this;
    }

    float4& operator=(const float3& xyz) noexcept {
        simd_ = _mm_set_ps(0.0f, xyz.z, xyz.y, xyz.x);  // w = 0
        return *this;
    }

    float4& operator=(const float2& xy) noexcept {
        simd_ = _mm_set_ps(0.0f, 0.0f, xy.y, xy.x);     // z = 0, w = 0
        return *this;
    }

    // ============================================================================
    // Compound Assignment Operators
    // ============================================================================

    float4& operator+=(const float4& rhs) noexcept {
        simd_ = _mm_add_ps(simd_, rhs.simd_);
        return *this;
    }

    float4& operator-=(const float4& rhs) noexcept {
        simd_ = _mm_sub_ps(simd_, rhs.simd_);
        return *this;
    }

    float4& operator*=(const float4& rhs) noexcept {
        simd_ = _mm_mul_ps(simd_, rhs.simd_);
        return *this;
    }

    float4& operator/=(const float4& rhs) noexcept {
        simd_ = _mm_div_ps(simd_, rhs.simd_);
        return *this;
    }

    float4& operator*=(float scalar) noexcept {
        __m128 scalar_vec = _mm_set1_ps(scalar);
        simd_ = _mm_mul_ps(simd_, scalar_vec);
        return *this;
    }

    float4& operator/=(float scalar) noexcept {
        __m128 inv_scalar = _mm_set1_ps(1.0f / scalar);
        simd_ = _mm_mul_ps(simd_, inv_scalar);
        return *this;
    }

    // ============================================================================
    // Binary Operators
    // ============================================================================

    float4 operator+(const float4& rhs) const noexcept {
        return float4(_mm_add_ps(simd_, rhs.simd_));
    }

    float4 operator-(const float4& rhs) const noexcept {
        return float4(_mm_sub_ps(simd_, rhs.simd_));
    }

    // ============================================================================
    // Unary Operators
    // ============================================================================

    float4 operator+() const noexcept { return *this; }

    float4 operator-() const noexcept {
        __m128 neg = _mm_set1_ps(-1.0f);
        return float4(_mm_mul_ps(simd_, neg));
    }

    // ============================================================================
    // Access Operators
    // ============================================================================

    float& operator[](int index) noexcept {
        assert(index >= 0 && index < 4);
        return (&x)[index];
    }

    const float& operator[](int index) const noexcept {
        assert(index >= 0 && index < 4);
        return (&x)[index];
    }

    // ============================================================================
    // Conversion Operators
    // ============================================================================

    operator const float* () const noexcept { return &x; }
    operator float* () noexcept { return &x; }
    operator __m128() const noexcept { return simd_; }

    // ============================================================================
    // Static Constructors
    // ============================================================================

    static float4 zero() noexcept { return float4(0.0f, 0.0f, 0.0f, 0.0f); }
    static float4 one() noexcept { return float4(1.0f, 1.0f, 1.0f, 1.0f); }
    static float4 unit_x() noexcept { return float4(1.0f, 0.0f, 0.0f, 0.0f); }
    static float4 unit_y() noexcept { return float4(0.0f, 1.0f, 0.0f, 0.0f); }
    static float4 unit_z() noexcept { return float4(0.0f, 0.0f, 1.0f, 0.0f); }
    static float4 unit_w() noexcept { return float4(0.0f, 0.0f, 0.0f, 1.0f); }

    static float4 from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) noexcept {
        return float4(
            static_cast<float>(r) / 255.0f,
            static_cast<float>(g) / 255.0f,
            static_cast<float>(b) / 255.0f,
            static_cast<float>(a) / 255.0f
        );
    }

    static float4 from_color(float r, float g, float b, float a = 1.0f) noexcept {
        return float4(r, g, b, a);
    }

    // ============================================================================
    // Basic Properties
    // ============================================================================

    float length_sq() const noexcept {
        __m128 squared = _mm_mul_ps(simd_, simd_);
        __m128 sum = _mm_hadd_ps(squared, squared);
        sum = _mm_hadd_ps(sum, sum);
        return _mm_cvtss_f32(sum);
    }

    float distance_sq(const float4& other) const noexcept {
        float4 diff = *this - other;
        return diff.length_sq();
    }

    // ============================================================================
    // Swizzle Operations (HLSL style) - float4 returns
    // ============================================================================

    float4 xyzw() const noexcept { return float4(x, y, z, w); }
    float4 yxzw() const noexcept { return float4(y, x, z, w); }
    float4 zxyw() const noexcept { return float4(z, x, y, w); }
    float4 zyxw() const noexcept { return float4(z, y, x, w); }
    float4 wzyx() const noexcept { return float4(w, z, y, x); }
    float4 xyxw() const noexcept { return float4(x, y, x, w); }
    float4 xzxw() const noexcept { return float4(x, z, x, w); }
    float4 yxyw() const noexcept { return float4(y, x, y, w); }
    float4 yzyw() const noexcept { return float4(y, z, y, w); }
    float4 zxzw() const noexcept { return float4(z, x, z, w); }
    float4 zyzw() const noexcept { return float4(z, y, z, w); }

    float3 xyz() const noexcept { return float3(x, y, z); }
    float3 yxz() const noexcept { return float3(y, x, z); }
    float3 zxy() const noexcept { return float3(z, x, y); }
    float3 zyx() const noexcept { return float3(z, y, x); }
    float3 xyx() const noexcept { return float3(x, y, x); }
    float3 xzx() const noexcept { return float3(x, z, x); }
    float3 yxy() const noexcept { return float3(y, x, y); }
    float3 yzy() const noexcept { return float3(y, z, y); }
    float3 zxz() const noexcept { return float3(z, x, z); }
    float3 zyz() const noexcept { return float3(z, y, z); }

    // Color swizzles (as float4)
    float r() const noexcept { return x; }
    float g() const noexcept { return y; }
    float b() const noexcept { return z; }
    float a() const noexcept { return w; }

    float4 grba() const noexcept { return float4(y, x, z, w); }
    float4 brga() const noexcept { return float4(z, x, y, w); }
    float4 bgra() const noexcept { return float4(z, y, x, w); }
    float4 abgr() const noexcept { return float4(w, z, y, x); }
    float4 argb() const noexcept { return float4(w, x, y, z); }
    float4 rbga() const noexcept { return float4(x, z, y, w); }
    float4 gbra() const noexcept { return float4(y, z, x, w); }

    // ============================================================================
    // Utility Methods
    // ============================================================================

    std::string to_string() const {
        char buffer[80];
        std::snprintf(buffer, sizeof(buffer), "(%.3f, %.3f, %.3f, %.3f)", x, y, z, w);
        return std::string(buffer);
    }

    const float* data() const noexcept { return &x; }
    float* data() noexcept { return &x; }

    // SSE-specific methods
    __m128 get_simd() const noexcept { return simd_; }
    void set_simd(__m128 new_simd) noexcept { simd_ = new_simd; }

    static float4 load_unaligned(const float* data) noexcept {
        return float4(_mm_loadu_ps(data));
    }

    static float4 load_aligned(const float* data) noexcept {
        return float4(_mm_load_ps(data));
    }

    void store_unaligned(float* data) const noexcept {
        _mm_storeu_ps(data, simd_);
    }

    void store_aligned(float* data) const noexcept {
        _mm_store_ps(data, simd_);
    }

    // ============================================================================
    // Color Operations
    // ============================================================================

    float luminance() const noexcept {
        // Rec. 709 weights: 0.2126*R + 0.7152*G + 0.0722*B
        return x * 0.2126f + y * 0.7152f + z * 0.0722f;
    }

    float brightness() const noexcept {
        return (x + y + z) / 3.0f;
    }

    float4 premultiply_alpha() const noexcept {
        return float4(x * w, y * w, z * w, w);
    }

    float4 unpremultiply_alpha() const noexcept {
        if (std::abs(w) < EPSILON) return *this;
        return float4(x / w, y / w, z / w, w);
    }

    float4 grayscale() const noexcept {
        float lum = luminance();
        return float4(lum, lum, lum, w);
    }

    // ============================================================================
    // Geometric Operations
    // ============================================================================

    float4 to_homogeneous() const noexcept {
        return float4(x, y, z, 1.0f);
    }
};

// ============================================================================
// Global Mathematical Functions (HLSL Style)
// ============================================================================

// Vector operations
inline float4 operator*(const float4& lhs, const float4& rhs) noexcept {
    return float4(_mm_mul_ps(lhs.get_simd(), rhs.get_simd()));
}

inline float4 operator/(const float4& lhs, const float4& rhs) noexcept {
    return float4(_mm_div_ps(lhs.get_simd(), rhs.get_simd()));
}

inline float4 operator*(const float4& vec, float scalar) noexcept {
    __m128 scalar_vec = _mm_set1_ps(scalar);
    return float4(_mm_mul_ps(vec.get_simd(), scalar_vec));
}

inline float4 operator*(float scalar, const float4& vec) noexcept {
    return vec * scalar;
}

inline float4 operator/(const float4& vec, float scalar) noexcept {
    __m128 inv_scalar = _mm_set1_ps(1.0f / scalar);
    return float4(_mm_mul_ps(vec.get_simd(), inv_scalar));
}

inline float4 operator/(float scalar, const float4& vec) noexcept {
    return float4(scalar / vec.x, scalar / vec.y, scalar / vec.z, scalar / vec.w);
}

// Length and distance
inline float length(const float4& v) noexcept {
    __m128 squared = _mm_mul_ps(v.get_simd(), v.get_simd());
    __m128 sum = _mm_hadd_ps(squared, squared);
    sum = _mm_hadd_ps(sum, sum);
    return std::sqrt(_mm_cvtss_f32(sum));
}

inline float length_sq(const float4& v) noexcept {
    __m128 squared = _mm_mul_ps(v.get_simd(), v.get_simd());
    __m128 sum = _mm_hadd_ps(squared, squared);
    sum = _mm_hadd_ps(sum, sum);
    return _mm_cvtss_f32(sum);
}

inline float distance(const float4& a, const float4& b) noexcept {
    return length(a - b);
}

inline float distance_sq(const float4& a, const float4& b) noexcept {
    return length_sq(a - b);
}

// Normalization
inline float4 normalize(const float4& v) noexcept {
    float len = length(v);
    if (len < EPSILON) {
        return float4::zero();
    }
    return v / len;
}

// Dot products
inline float dot(const float4& a, const float4& b) noexcept {
    __m128 mul = _mm_mul_ps(a.get_simd(), b.get_simd());
    __m128 sum = _mm_hadd_ps(mul, mul);
    sum = _mm_hadd_ps(sum, sum);
    return _mm_cvtss_f32(sum);
}

inline float dot3(const float4& a, const float4& b) noexcept {
    // Sum of x*x + y*y + z*z (ignore w component)
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Cross product (3D, ignores w component)
inline float4 cross(const float4& a, const float4& b) noexcept {
    return float4(a.y * b.z - a.z * b.y,
                  a.z * b.x - a.x * b.z,
                  a.x * b.y - a.y * b.x,
                  0.0f);
}

// Comparison
inline bool approximately(const float4& a, const float4& b, float epsilon = EPSILON) noexcept {
    return std::abs(a.x - b.x) <= epsilon &&
           std::abs(a.y - b.y) <= epsilon &&
           std::abs(a.z - b.z) <= epsilon &&
           std::abs(a.w - b.w) <= epsilon;
}

inline bool isValid(const float4& v) noexcept {
    return std::isfinite(v.x) && 
           std::isfinite(v.y) &&
           std::isfinite(v.z) && 
           std::isfinite(v.w);
}

inline bool is_normalized(const float4& v, float epsilon = EPSILON) noexcept {
    return std::abs(length_sq(v) - 1.0f) <= epsilon;
}

// Interpolation
inline float4 lerp(const float4& a, const float4& b, float t) noexcept {
    t = std::max(0.0f, std::min(1.0f, t));
    return a + (b - a) * t;
}

// ============================================================================
// HLSL-like Global Functions
// ============================================================================

inline float4 abs(const float4& v) noexcept {
    __m128 mask = _mm_set1_ps(-0.0f);
    return float4(_mm_andnot_ps(mask, v.get_simd()));
}

inline float4 sign(const float4& v) noexcept {
    return float4((v.x > 0.0f) ? 1.0f : ((v.x < 0.0f) ? -1.0f : 0.0f),
                  (v.y > 0.0f) ? 1.0f : ((v.y < 0.0f) ? -1.0f : 0.0f),
                  (v.z > 0.0f) ? 1.0f : ((v.z < 0.0f) ? -1.0f : 0.0f),
                  (v.w > 0.0f) ? 1.0f : ((v.w < 0.0f) ? -1.0f : 0.0f));
}

inline float4 floor(const float4& v) noexcept {
    return float4(_mm_floor_ps(v.get_simd()));
}

inline float4 ceil(const float4& v) noexcept {
    return float4(_mm_ceil_ps(v.get_simd()));
}

inline float4 round(const float4& v) noexcept {
    return float4(_mm_round_ps(v.get_simd(), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));
}

inline float4 frac(const float4& v) noexcept {
    return float4(v.x - std::floor(v.x),
                  v.y - std::floor(v.y),
                  v.z - std::floor(v.z),
                  v.w - std::floor(v.w));
}

inline float4 pow(const float4& base, const float4& exp) noexcept {
    return float4(std::pow(base.x, exp.x),
                  std::pow(base.y, exp.y),
                  std::pow(base.z, exp.z),
                  std::pow(base.w, exp.w));
}

inline float4 pow(const float4& base, float exp) noexcept {
    return float4(std::pow(base.x, exp),
                  std::pow(base.y, exp),
                  std::pow(base.z, exp),
                  std::pow(base.w, exp));
}

inline float4 pow(float base, const float4& exp) noexcept {
    return float4(std::pow(base, exp.x),
                  std::pow(base, exp.y),
                  std::pow(base, exp.z),
                  std::pow(base, exp.w));
}

inline float4 saturate(const float4& v) noexcept {
    __m128 zero = _mm_setzero_ps();
    __m128 one = _mm_set1_ps(1.0f);
    __m128 result = _mm_max_ps(v.get_simd(), zero);
    result = _mm_min_ps(result, one);
    return float4(result);
}

inline float4 step(float edge, const float4& v) noexcept {
    return float4((v.x >= edge) ? 1.0f : 0.0f,
                  (v.y >= edge) ? 1.0f : 0.0f,
                  (v.z >= edge) ? 1.0f : 0.0f,
                  (v.w >= edge) ? 1.0f : 0.0f);
}

inline float4 smoothstep(float edge0, float edge1, const float4& v) noexcept {
    auto smooth = [edge0, edge1](float t) {
        t = std::max(0.0f, std::min(1.0f, (t - edge0) / (edge1 - edge0)));
        return t * t * (3.0f - 2.0f * t);
    };
    return float4(smooth(v.x), smooth(v.y), smooth(v.z), smooth(v.w));
}

inline float4 min(const float4& a, const float4& b) noexcept {
    return float4(_mm_min_ps(a.get_simd(), b.get_simd()));
}

inline float4 max(const float4& a, const float4& b) noexcept {
    return float4(_mm_max_ps(a.get_simd(), b.get_simd()));
}

inline float4 clamp(const float4& v, const float4& min_val, const float4& max_val) noexcept {
    return min(max(v, min_val), max_val);
}

inline float4 clamp(const float4& v, float min_val, float max_val) noexcept {
    return float4(std::max(min_val, std::min(max_val, v.x)),
                  std::max(min_val, std::min(max_val, v.y)),
                  std::max(min_val, std::min(max_val, v.z)),
                  std::max(min_val, std::min(max_val, v.w)));
}

// ============================================================================
// Global Color Operations
// ============================================================================

inline float luminance(const float4& color) noexcept {
    return color.luminance();
}

inline float brightness(const float4& color) noexcept {
    return color.brightness();
}

inline float4 premultiply_alpha(const float4& color) noexcept {
    return color.premultiply_alpha();
}

inline float4 unpremultiply_alpha(const float4& color) noexcept {
    return color.unpremultiply_alpha();
}

inline float4 grayscale(const float4& color) noexcept {
    return color.grayscale();
}

// ============================================================================
// Geometric Operations
// ============================================================================

// Note: project() would require float3 header
// inline float3 project(const float4& vec) noexcept;

inline float4 to_homogeneous(const float4& vec) noexcept {
    return vec.to_homogeneous();
}

// ============================================================================
// Comparison Operators
// ============================================================================

inline bool operator==(const float4& a, const float4& b) noexcept {
    return approximately(a, b);
}

inline bool operator!=(const float4& a, const float4& b) noexcept {
    return !approximately(a, b);
}

// ============================================================================
// Useful Constants
// ============================================================================

AFTERMATH_INLINE_VAR const float4 float4_Zero(0.0f, 0.0f, 0.0f, 0.0f);
AFTERMATH_INLINE_VAR const float4 float4_One(1.0f, 1.0f, 1.0f, 1.0f);
AFTERMATH_INLINE_VAR const float4 float4_UnitX(1.0f, 0.0f, 0.0f, 0.0f);
AFTERMATH_INLINE_VAR const float4 float4_UnitY(0.0f, 1.0f, 0.0f, 0.0f);
AFTERMATH_INLINE_VAR const float4 float4_UnitZ(0.0f, 0.0f, 1.0f, 0.0f);
AFTERMATH_INLINE_VAR const float4 float4_UnitW(0.0f, 0.0f, 0.0f, 1.0f);

// Color constants
AFTERMATH_INLINE_VAR const float4 float4_Red(1.0f, 0.0f, 0.0f, 1.0f);
AFTERMATH_INLINE_VAR const float4 float4_Green(0.0f, 1.0f, 0.0f, 1.0f);
AFTERMATH_INLINE_VAR const float4 float4_Blue(0.0f, 0.0f, 1.0f, 1.0f);
AFTERMATH_INLINE_VAR const float4 float4_White(1.0f, 1.0f, 1.0f, 1.0f);
AFTERMATH_INLINE_VAR const float4 float4_Black(0.0f, 0.0f, 0.0f, 1.0f);
AFTERMATH_INLINE_VAR const float4 float4_Transparent(0.0f, 0.0f, 0.0f, 0.0f);
AFTERMATH_INLINE_VAR const float4 float4_Yellow(1.0f, 1.0f, 0.0f, 1.0f);
AFTERMATH_INLINE_VAR const float4 float4_Cyan(0.0f, 1.0f, 1.0f, 1.0f);
AFTERMATH_INLINE_VAR const float4 float4_Magenta(1.0f, 0.0f, 1.0f, 1.0f);
AFTERMATH_INLINE_VAR const float4 float4_Gray(0.5f, 0.5f, 0.5f, 1.0f);
AFTERMATH_INLINE_VAR const float4 float4_Silver(0.75f, 0.75f, 0.75f, 1.0f);
AFTERMATH_INLINE_VAR const float4 float4_Purple(0.5f, 0.0f, 0.5f, 1.0f);
AFTERMATH_INLINE_VAR const float4 float4_Orange(1.0f, 0.5f, 0.0f, 1.0f);
AFTERMATH_INLINE_VAR const float4 float4_Brown(0.6f, 0.4f, 0.2f, 1.0f);
AFTERMATH_INLINE_VAR const float4 float4_Pink(1.0f, 0.75f, 0.8f, 1.0f);

AFTERMATH_END
