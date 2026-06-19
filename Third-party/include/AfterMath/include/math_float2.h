// math_float2.h
// Description: 2-dimensional vector class with HLSL-like syntax and SSE optimization
// Author: NSDeathman, DeepSeek

#pragma once

#include <string>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cassert>
#include <xmmintrin.h>
#include <pmmintrin.h>

#include "AfterMathInternal.h"

AFTERMATH_BEGIN

// Forward declaration
class float2;

// ============================================================================
// 2D Vector Class
// ============================================================================

/**
    * @class float2
    * @brief 2-dimensional vector with HLSL-like syntax
    *
    * Represents a 2D vector (x, y) with optimized operations for 2D graphics,
    * physics simulations, and 2D mathematics. Features HLSL-like syntax
    * with global mathematical functions.
    */
class float2 {
public:
    // Data members (public for direct access)
    float x; ///< X component of the vector
    float y; ///< Y component of the vector

    // ============================================================================
    // Constructors
    // ============================================================================

    constexpr float2() noexcept : x(0.0f), y(0.0f) {}
    constexpr float2(float x, float y) noexcept : x(x), y(y) {}
    explicit constexpr float2(float scalar) noexcept : x(scalar), y(scalar) {}
    constexpr float2(const float2&) noexcept = default;

    explicit float2(const float* data) noexcept : x(data[0]), y(data[1]) {}
    explicit float2(__m128 simd_) noexcept {
        alignas(16) float data[4];
        _mm_store_ps(data, simd_);
        x = data[0]; y = data[1];
    }

    // ============================================================================
    // Assignment Operators
    // ============================================================================

    float2& operator=(const float2&) noexcept = default;

    float2& operator=(float scalar) noexcept {
        x = scalar;
        y = scalar;
        return *this;
    }

    // ============================================================================
    // Compound Assignment Operators
    // ============================================================================

    float2& operator+=(const float2& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    float2& operator-=(const float2& rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    float2& operator*=(const float2& rhs) noexcept {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }

    float2& operator/=(const float2& rhs) noexcept {
        x /= rhs.x;
        y /= rhs.y;
        return *this;
    }

    float2& operator*=(float scalar) noexcept {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    float2& operator/=(float scalar) noexcept {
        float inv = 1.0f / scalar;
        x *= inv;
        y *= inv;
        return *this;
    }

    // ============================================================================
    // Binary Operators
    // ============================================================================

    float2 operator+(const float2& rhs) const noexcept {
        return float2(x + rhs.x, y + rhs.y);
    }

    float2 operator-(const float2& rhs) const noexcept {
        return float2(x - rhs.x, y - rhs.y);
    }

    float2 operator+(float rhs) const noexcept {
        return float2(x + rhs, y + rhs);
    }

    float2 operator-(float rhs) const noexcept {
        return float2(x - rhs, y - rhs);
    }

    // ============================================================================
    // Unary Operators
    // ============================================================================

    constexpr float2 operator+() const noexcept { return *this; }
    constexpr float2 operator-() const noexcept { return float2(-x, -y); }

    // ============================================================================
    // Access Operators
    // ============================================================================

    float& operator[](int index) noexcept {
        assert(index >= 0 && index < 2);
        return (&x)[index];
    }

    const float& operator[](int index) const noexcept {
        assert(index >= 0 && index < 2);
        return (&x)[index];
    }

    // ============================================================================
    // Conversion Operators
    // ============================================================================

    operator const float* () const noexcept { return &x; }
    operator float* () noexcept { return &x; }
    operator __m128() const noexcept {
        return _mm_set_ps(0.0f, 0.0f, y, x);
    }

    // ============================================================================
    // Static Constructors
    // ============================================================================

    static constexpr float2 zero() noexcept { return float2(0.0f, 0.0f); }
    static constexpr float2 one() noexcept { return float2(1.0f, 1.0f); }
    static constexpr float2 unit_x() noexcept { return float2(1.0f, 0.0f); }
    static constexpr float2 unit_y() noexcept { return float2(0.0f, 1.0f); }

    // ============================================================================
    // Basic Properties (minimal, most operations moved to global functions)
    // ============================================================================

    constexpr float length_sq() const noexcept { return x * x + y * y; }

    constexpr float distance_sq(const float2& other) const noexcept {
        float dx = x - other.x;
        float dy = y - other.y;
        return dx * dx + dy * dy;
    }

    // Swizzle operations (HLSL style)
    constexpr float2 yx() const noexcept { return float2(y, x); }
    constexpr float2 xx() const noexcept { return float2(x, x); }
    constexpr float2 yy() const noexcept { return float2(y, y); }

    // ============================================================================
    // Utility Methods
    // ============================================================================

    std::string to_string() const {
        char buf[64];
        std::snprintf(buf, 64, "(%.3f, %.3f)", x, y);
        return std::string(buf);
    }

    const float* data() const noexcept { return &x; }
    float* data() noexcept { return &x; }
};

// ============================================================================
// Global Mathematical Functions (HLSL Style)
// ============================================================================

// Vector operations
inline float2 operator*(float2 lhs, const float2& rhs) noexcept {
    return float2(lhs.x * rhs.x, lhs.y * rhs.y);
}

inline float2 operator/(float2 lhs, const float2& rhs) noexcept {
    return float2(lhs.x / rhs.x, lhs.y / rhs.y);
}

inline float2 operator*(float2 vec, float scalar) noexcept {
    return float2(vec.x * scalar, vec.y * scalar);
}

inline float2 operator*(float scalar, float2 vec) noexcept {
    return vec * scalar;
}

inline float2 operator/(float2 vec, float scalar) noexcept {
    return vec * (1.0f / scalar);
}

inline float2 operator+(float scalar, float2 vec) noexcept {
    return vec + scalar;
}

// Length and distance
inline float length(const float2& v) noexcept {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

inline float length_sq(const float2& v) noexcept {
    return v.x * v.x + v.y * v.y;
}

inline float distance(const float2& a, const float2& b) noexcept {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

inline float distance_sq(const float2& a, const float2& b) noexcept {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

// Normalization
inline float2 normalize(const float2& v) noexcept {
    float len = length(v);
    if (len < EPSILON) {
        return float2::zero();
    }
    return v / len;
}

// Dot and cross products
inline float dot(const float2& a, const float2& b) noexcept {
    return a.x * b.x + a.y * b.y;
}

inline float cross(const float2& a, const float2& b) noexcept {
    return a.x * b.y - a.y * b.x;
}

inline bool isValid(const float2& v) noexcept {
    return std::isfinite(v.x) && std::isfinite(v.y);
}

// Interpolation
inline float2 lerp(const float2& a, const float2& b, float t) noexcept {
    return a + (b - a) * t;
}

inline float2 slerp(const float2& a, const float2& b, float t) noexcept {
    // Normalize inputs
    float2 na = normalize(a);
    float2 nb = normalize(b);

    // Compute angle between vectors
    float dot_val = dot(na, nb);
    dot_val = std::max(-1.0f, std::min(1.0f, dot_val));

    float theta = std::acos(dot_val) * t;
    float2 relative_vec = normalize(nb - na * dot_val);

    return na * std::cos(theta) + relative_vec * std::sin(theta);
}

// Geometric operations
inline float2 perpendicular(const float2& v) noexcept {
    return float2(-v.y, v.x);
}

inline float2 reflect(const float2& incident, const float2& normal) noexcept {
    return incident - 2.0f * dot(incident, normal) * normal;
}

inline float2 refract(const float2& incident, const float2& normal, float eta) noexcept {
    // eta = n_incident / n_transmitted
    float cos_theta_i = -dot(incident, normal);
    float sin_theta_i_sq = 1.0f - cos_theta_i * cos_theta_i;
    float sin_theta_t_sq = (eta * eta) * sin_theta_i_sq;

    // Total internal reflection
    if (sin_theta_t_sq > 1.0f) {
        return float2::zero();
    }

    float cos_theta_t = std::sqrt(1.0f - sin_theta_t_sq);
    return eta * incident + (eta * cos_theta_i - cos_theta_t) * normal;
}

inline float2 rotate(const float2& v, float angle) noexcept {
    float s = std::sin(angle);
    float c = std::cos(angle);
    return float2(v.x * c - v.y * s, v.x * s + v.y * c);
}

// Angle operations
inline float angle(const float2& v) noexcept {
    return std::atan2(v.y, v.x);
}

inline float angle_between(const float2& a, const float2& b) noexcept {
    float2 na = normalize(a);
    float2 nb = normalize(b);
    float dot_val = dot(na, nb);
    dot_val = std::max(-1.0f, std::min(1.0f, dot_val));
    return std::acos(dot_val);
}

inline float signed_angle_between(const float2& from, const float2& to) noexcept {
    float2 nfrom = normalize(from);
    float2 nto = normalize(to);
    return std::atan2(cross(nfrom, nto), dot(nfrom, nto));
}

// Projection and rejection
inline float2 project(const float2& v, const float2& onto) noexcept {
    float len_sq = length_sq(onto);
    if (len_sq < EPSILON) {
        return float2::zero();
    }
    return onto * (dot(v, onto) / len_sq);
}

inline float2 reject(const float2& v, const float2& from) noexcept {
    return v - project(v, from);
}

// ============================================================================
// HLSL-like Global Functions
// ============================================================================

inline float2 abs(const float2& v) noexcept {
    return float2(std::abs(v.x), std::abs(v.y));
}

inline float2 sign(const float2& v) noexcept {
    return float2(
        (v.x > 0.0f) ? 1.0f : ((v.x < 0.0f) ? -1.0f : 0.0f),
        (v.y > 0.0f) ? 1.0f : ((v.y < 0.0f) ? -1.0f : 0.0f)
    );
}

inline float2 floor(const float2& v) noexcept {
    return float2(std::floor(v.x), std::floor(v.y));
}

inline float2 ceil(const float2& v) noexcept {
    return float2(std::ceil(v.x), std::ceil(v.y));
}

inline float2 round(const float2& v) noexcept {
    return float2(std::round(v.x), std::round(v.y));
}

inline float2 frac(const float2& v) noexcept {
    return float2(v.x - std::floor(v.x), v.y - std::floor(v.y));
}

inline float2 saturate(const float2& v) noexcept {
    return float2(
        std::max(0.0f, std::min(1.0f, v.x)),
        std::max(0.0f, std::min(1.0f, v.y))
    );
}

inline float2 step(float edge, const float2& v) noexcept {
    return float2(
        (v.x >= edge) ? 1.0f : 0.0f,
        (v.y >= edge) ? 1.0f : 0.0f
    );
}

inline float2 smoothstep(float edge0, float edge1, const float2& v) noexcept {
    auto smooth = [edge0, edge1](float t) {
        t = std::max(0.0f, std::min(1.0f, (t - edge0) / (edge1 - edge0)));
        return t * t * (3.0f - 2.0f * t);
    };
    return float2(smooth(v.x), smooth(v.y));
}

inline float2 clamp(const float2& v, const float2& min_val, const float2& max_val) noexcept {
    return float2(
        std::max(min_val.x, std::min(max_val.x, v.x)),
        std::max(min_val.y, std::min(max_val.y, v.y))
    );
}

inline float2 min(const float2& a, const float2& b) noexcept {
    return float2(std::min(a.x, b.x), std::min(a.y, b.y));
}

inline float2 max(const float2& a, const float2& b) noexcept {
    return float2(std::max(a.x, b.x), std::max(a.y, b.y));
}

// ============================================================================
// Utility Functions
// ============================================================================

inline float distance_to_line_segment(const float2& point,
    const float2& line_start,
    const float2& line_end) noexcept {
    float2 line_vec = line_end - line_start;
    float2 point_vec = point - line_start;

    float line_length_sq = length_sq(line_vec);
    if (line_length_sq < EPSILON) {
        return distance(point, line_start);
    }

    float t = dot(point_vec, line_vec) / line_length_sq;
    t = std::max(0.0f, std::min(1.0f, t));

    float2 projection = line_start + line_vec * t;
    return distance(point, projection);
}

// ============================================================================
// Useful Constants
// ============================================================================

AFTERMATH_INLINE_VAR const float2 float2_Zero(0.0f, 0.0f);
AFTERMATH_INLINE_VAR const float2 float2_One(1.0f, 1.0f);
AFTERMATH_INLINE_VAR const float2 float2_UnitX(1.0f, 0.0f);
AFTERMATH_INLINE_VAR const float2 float2_UnitY(0.0f, 1.0f);
AFTERMATH_INLINE_VAR const float2 float2_Right(1.0f, 0.0f);
AFTERMATH_INLINE_VAR const float2 float2_Left(-1.0f, 0.0f);
AFTERMATH_INLINE_VAR const float2 float2_Up(0.0f, 1.0f);
AFTERMATH_INLINE_VAR const float2 float2_Down(0.0f, -1.0f);

// ============================================================================
// Comparison Operators (defined after all functions for completeness)
// ============================================================================

inline bool approximately(const float2& a, const float2& b, float epsilon = EPSILON) noexcept {
    return std::abs(a.x - b.x) <= epsilon && std::abs(a.y - b.y) <= epsilon;
}

inline bool approximately_zero(const float2& v, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
    return std::abs(v.x) <= epsilon && std::abs(v.y) <= epsilon;
}

inline bool operator==(const float2& a, const float2& b) noexcept {
    return approximately(a, b);
}

inline bool operator!=(const float2& a, const float2& b) noexcept {
    return !approximately(a, b);
}

AFTERMATH_END
