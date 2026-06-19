// math_float2x2.h
// Description: 2x2 matrix class with HLSL-like syntax and SSE optimization
// Author: NSDeathman, DeepSeek

#pragma once

#include <cmath>
#include <string>
#include <cstdio>
#include <xmmintrin.h>
#include <pmmintrin.h>

#include "math_float2.h"
#include "math_functions.h"
#include "AfterMathInternal.h"

#pragma warning(push)
#pragma warning(disable : AFTERMATH_DISABLED_WARNINGS)

AFTERMATH_BEGIN

// Forward declarations
class float2;
class float2x2;

float2x2 operator*(const float2x2& lhs, const float2x2& rhs) noexcept;

// ============================================================================
// 2x2 Matrix Class
// ============================================================================

/**
    * @class float2x2
    * @brief 2x2 matrix with HLSL-like syntax
    *
    * Represents a 2x2 matrix for 2D transformations (rotation, scaling, shearing).
    * Uses row-major storage for compatibility with HLSL conventions.
    */
class float2x2 {
public:
    // Data members (public for direct access, aligned for SSE)
    alignas(16) float2 row0; ///< First row of the matrix (x-axis basis)
    alignas(16) float2 row1; ///< Second row of the matrix (y-axis basis)

    // ============================================================================
    // Constructors
    // ============================================================================

    constexpr float2x2() noexcept : row0(1.0f, 0.0f), row1(0.0f, 1.0f) {}

    constexpr float2x2(const float2& row0, const float2& row1) noexcept
        : row0(row0), row1(row1) {}

    constexpr float2x2(float m00, float m01, float m10, float m11) noexcept
        : row0(m00, m01), row1(m10, m11) {}

    explicit constexpr float2x2(float scalar) noexcept
        : row0(scalar, 0.0f), row1(0.0f, scalar) {}

    explicit constexpr float2x2(const float2& diagonal) noexcept
        : row0(diagonal.x, 0.0f), row1(0.0f, diagonal.y) {}

    explicit float2x2(const float* data) noexcept
        : row0(data[0], data[1]), row1(data[2], data[3]) {}

    explicit float2x2(__m128 sse_data) noexcept {
        set_sse_data(sse_data);
    }

    constexpr float2x2(const float2x2&) noexcept = default;

    // ============================================================================
    // Assignment Operators
    // ============================================================================

    float2x2& operator=(const float2x2&) noexcept = default;

    float2x2& operator=(float scalar) noexcept {
        row0.x = scalar; row0.y = 0.0f;
        row1.x = 0.0f; row1.y = scalar;
        return *this;
    }

    // ============================================================================
    // Compound Assignment Operators
    // ============================================================================

    float2x2& operator+=(const float2x2& rhs) noexcept {
        row0 += rhs.row0;
        row1 += rhs.row1;
        return *this;
    }

    float2x2& operator-=(const float2x2& rhs) noexcept {
        row0 -= rhs.row0;
        row1 -= rhs.row1;
        return *this;
    }

    float2x2& operator*=(float scalar) noexcept {
        row0 *= scalar;
        row1 *= scalar;
        return *this;
    }

    float2x2& operator/=(float scalar) noexcept {
        row0 /= scalar;
        row1 /= scalar;
        return *this;
    }

    float2x2& operator*=(const float2x2& rhs) noexcept {
        *this = *this * rhs;
        return *this;
    }

    // ============================================================================
    // Access Operators
    // ============================================================================

    float2& operator[](int rowIndex) noexcept {
        return (rowIndex == 0) ? row0 : row1;
    }

    const float2& operator[](int rowIndex) const noexcept {
        return (rowIndex == 0) ? row0 : row1;
    }

    float& operator()(int row, int col) noexcept {
        return (row == 0) ?
            (col == 0 ? row0.x : row0.y) :
            (col == 0 ? row1.x : row1.y);
    }

    const float& operator()(int row, int col) const noexcept {
        return (row == 0) ?
            (col == 0 ? row0.x : row0.y) :
            (col == 0 ? row1.x : row1.y);
    }

    // ============================================================================
    // Conversion Operators
    // ============================================================================

    operator __m128() const noexcept {
        return _mm_setr_ps(row0.x, row0.y, row1.x, row1.y);
    }

    // ============================================================================
    // Basic Methods (minimal, most operations moved to global functions)
    // ============================================================================

    constexpr float2 col0() const noexcept { return float2(row0.x, row1.x); }
    constexpr float2 col1() const noexcept { return float2(row0.y, row1.y); }

    void set_col0(const float2& col) noexcept {
        row0.x = col.x;
        row1.x = col.y;
    }

    void set_col1(const float2& col) noexcept {
        row0.y = col.x;
        row1.y = col.y;
    }

    __m128 sse_data() const noexcept {
        return _mm_setr_ps(row0.x, row0.y, row1.x, row1.y);
    }

    void set_sse_data(__m128 sse_data) noexcept {
        alignas(16) float temp[4];
        _mm_store_ps(temp, sse_data);
        row0.x = temp[0]; row0.y = temp[1];
        row1.x = temp[2]; row1.y = temp[3];
    }

    std::string to_string() const {
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer),
            "[%8.4f, %8.4f]\n"
            "[%8.4f, %8.4f]",
            row0.x, row0.y,
            row1.x, row1.y);
        return std::string(buffer);
    }

    void to_row_major(float* data) const noexcept {
        data[0] = row0.x;
        data[1] = row0.y;
        data[2] = row1.x;
        data[3] = row1.y;
    }

    void to_column_major(float* data) const noexcept {
        data[0] = row0.x;
        data[1] = row1.x;
        data[2] = row0.y;
        data[3] = row1.y;
    }

    // ============================================================================
    // Static Constructors
    // ============================================================================

    static constexpr float2x2 identity() noexcept {
        return float2x2(float2(1.0f, 0.0f), float2(0.0f, 1.0f));
    }

    static constexpr float2x2 zero() noexcept {
        return float2x2(float2(0.0f, 0.0f), float2(0.0f, 0.0f));
    }

    static float2x2 rotation(float angle) noexcept {
        float s = std::sin(angle);
        float c = std::cos(angle);
        return float2x2(float2(c, -s), float2(s, c));
    }

    static constexpr float2x2 scaling(const float2& scale) noexcept {
        return float2x2(float2(scale.x, 0.0f), float2(0.0f, scale.y));
    }

    static constexpr float2x2 scaling(float x, float y) noexcept {
        return scaling(float2(x, y));
    }

    static constexpr float2x2 scaling(float uniformScale) noexcept {
        return scaling(float2(uniformScale, uniformScale));
    }

    static constexpr float2x2 shear(const float2& shear) noexcept {
        return float2x2(float2(1.0f, shear.x), float2(shear.y, 1.0f));
    }

    static constexpr float2x2 shear(float x, float y) noexcept {
        return shear(float2(x, y));
    }
};

// ============================================================================
// Global Mathematical Functions (HLSL Style)
// ============================================================================

// Matrix operations
inline float2x2 operator+(float2x2 lhs, const float2x2& rhs) noexcept {
    return float2x2(lhs.row0 + rhs.row0, lhs.row1 + rhs.row1);
}

inline float2x2 operator-(float2x2 lhs, const float2x2& rhs) noexcept {
    return float2x2(lhs.row0 - rhs.row0, lhs.row1 - rhs.row1);
}

inline float2x2 operator*(const float2x2& lhs, const float2x2& rhs) noexcept {
    __m128 lhs_data = lhs.sse_data();
    __m128 rhs_data = rhs.sse_data();

    float m00 = _mm_cvtss_f32(lhs_data);
    float m01 = _mm_cvtss_f32(_mm_shuffle_ps(lhs_data, lhs_data, _MM_SHUFFLE(1, 1, 1, 1)));
    float m10 = _mm_cvtss_f32(_mm_shuffle_ps(lhs_data, lhs_data, _MM_SHUFFLE(2, 2, 2, 2)));
    float m11 = _mm_cvtss_f32(_mm_shuffle_ps(lhs_data, lhs_data, _MM_SHUFFLE(3, 3, 3, 3)));

    float n00 = _mm_cvtss_f32(rhs_data);
    float n01 = _mm_cvtss_f32(_mm_shuffle_ps(rhs_data, rhs_data, _MM_SHUFFLE(1, 1, 1, 1)));
    float n10 = _mm_cvtss_f32(_mm_shuffle_ps(rhs_data, rhs_data, _MM_SHUFFLE(2, 2, 2, 2)));
    float n11 = _mm_cvtss_f32(_mm_shuffle_ps(rhs_data, rhs_data, _MM_SHUFFLE(3, 3, 3, 3)));

    return float2x2(
        m00 * n00 + m01 * n10,
        m00 * n01 + m01 * n11,
        m10 * n00 + m11 * n10,
        m10 * n01 + m11 * n11
    );
}

inline float2x2 operator*(float2x2 mat, float scalar) noexcept {
    return float2x2(mat.row0 * scalar, mat.row1 * scalar);
}

inline float2x2 operator*(float scalar, float2x2 mat) noexcept {
    return mat * scalar;
}

inline float2x2 operator/(float2x2 mat, float scalar) noexcept {
    return float2x2(mat.row0 / scalar, mat.row1 / scalar);
}

// Matrix-vector multiplication
inline float2 operator*(const float2x2& mat, const float2& vec) noexcept {
    return float2(
        dot(mat.row0, vec),
        dot(mat.row1, vec)
    );
}

inline float2 operator*(const float2& vec, const float2x2& mat) noexcept {
    return float2(
        vec.x * mat.row0.x + vec.y * mat.row1.x,
        vec.x * mat.row0.y + vec.y * mat.row1.y
    );
}

// Unary operators
inline float2x2 operator+(const float2x2& mat) noexcept {
    return mat;
}

inline float2x2 operator-(const float2x2& mat) noexcept {
    return float2x2(-mat.row0, -mat.row1);
}

// Matrix properties and transformations
inline float2x2 transpose(const float2x2& mat) noexcept {
    return float2x2(
        float2(mat.row0.x, mat.row1.x),
        float2(mat.row0.y, mat.row1.y)
    );
}

inline float determinant(const float2x2& mat) noexcept {
    return mat.row0.x * mat.row1.y - mat.row0.y * mat.row1.x;
}

inline float2x2 inverse(const float2x2& mat) noexcept {
    float det = determinant(mat);
    if (std::abs(det) < 1e-8f) {
        return float2x2::identity();
    }

    float inv_det = 1.0f / det;
    return float2x2(
        float2(mat.row1.y, -mat.row0.y) * inv_det,
        float2(-mat.row1.x, mat.row0.x) * inv_det
    );
}

inline float2x2 adjugate(const float2x2& mat) noexcept {
    return float2x2(
        float2(mat.row1.y, -mat.row0.y),
        float2(-mat.row1.x, mat.row0.x)
    );
}

inline float trace(const float2x2& mat) noexcept {
    return mat.row0.x + mat.row1.y;
}

inline float2 diagonal(const float2x2& mat) noexcept {
    return float2(mat.row0.x, mat.row1.y);
}

inline float frobenius_norm(const float2x2& mat) noexcept {
    return std::sqrt(length_sq(mat.row0) + length_sq(mat.row1));
}

inline float2 transform_vector(const float2x2& mat, const float2& vec) noexcept {
    return mat * vec;
}

inline float2 transform_point(const float2x2& mat, const float2& point) noexcept {
    return mat * point;
}

// Matrix properties
inline bool is_orthonormal(const float2x2& mat, float epsilon = 1e-6f) noexcept {
    float2 c0 = mat.col0();
    float2 c1 = mat.col1();

    return std::abs(length_sq(c0) - 1.0f) <= epsilon &&
        std::abs(length_sq(c1) - 1.0f) <= epsilon &&
        std::abs(dot(c0, c1)) <= epsilon;
}

inline float get_rotation(const float2x2& mat) noexcept {
    float2 c0 = mat.col0();
    float2 c1 = mat.col1();

    float len0 = length(c0);
    float len1 = length(c1);

    if (len0 < 1e-8f || len1 < 1e-8f) {
        return 0.0f;
    }

    float2 v0 = c0 / len0;
    float2 v1 = c1 / len1;

    if (std::abs(dot(v0, v1)) > 1e-6f) {
        return 0.0f;
    }

    float angle = std::atan2(v0.y, v0.x);

    // Normalize angle to [-π, π]
    if (angle > PI) angle -= 2.0f * PI;
    else if (angle <= -PI) angle += 2.0f * PI;

    return angle;
}

inline float2 get_scale(const float2x2& mat) noexcept {
    return float2(length(mat.col0()), length(mat.col1()));
}

inline void set_rotation(float2x2& mat, float angle) noexcept {
    float2 current_scale = get_scale(mat);
    float det_sign = (determinant(mat) < 0.0f) ? -1.0f : 1.0f;

    float s = std::sin(angle);
    float c = std::cos(angle);

    mat.set_col0(float2(c, s) * current_scale.x);
    mat.set_col1(float2(-s * det_sign, c * det_sign) * current_scale.y);
}

inline void set_scale(float2x2& mat, const float2& scale) noexcept {
    float2 c0 = mat.col0();
    float2 c1 = mat.col1();

    float len0 = length(c0);
    float len1 = length(c1);

    if (len0 > 1e-8f && len1 > 1e-8f) {
        mat.set_col0(c0 * (scale.x / len0));
        mat.set_col1(c1 * (scale.y / len1));
    }
    else {
        mat = float2x2::scaling(scale);
    }
}

// Matrix checks
inline bool is_identity(const float2x2& mat, float epsilon = 1e-6f) noexcept {
    return approximately(mat.row0, float2(1.0f, 0.0f), epsilon) &&
        approximately(mat.row1, float2(0.0f, 1.0f), epsilon);
}

inline bool is_orthogonal(const float2x2& mat, float epsilon = 1e-6f) noexcept {
    return std::abs(dot(mat.col0(), mat.col1())) <= epsilon;
}

inline bool is_rotation(const float2x2& mat, float epsilon = 1e-6f) noexcept {
    return is_orthogonal(mat, epsilon) &&
        approximately(length_sq(mat.col0()), 1.0f, epsilon) &&
        approximately(length_sq(mat.col1()), 1.0f, epsilon) &&
        approximately(determinant(mat), 1.0f, epsilon);
}

inline bool approximately(const float2x2& a, const float2x2& b, float epsilon = 1e-6f) noexcept {
    return approximately(a.row0, b.row0, epsilon) &&
        approximately(a.row1, b.row1, epsilon);
}

inline bool approximately_zero(const float2x2& mat, float epsilon = 1e-6f) noexcept {
    return approximately_zero(mat.row0, epsilon) &&
        approximately_zero(mat.row1, epsilon);
}

// HLSL-like functions
inline float2x2 mul(const float2x2& a, const float2x2& b) noexcept {
    return a * b;
}

inline float2 mul(const float2& vec, const float2x2& mat) noexcept {
    return vec * mat;
}

inline float2 mul(const float2x2& mat, const float2& vec) noexcept {
    return mat * vec;
}

// Comparison operators
inline bool operator==(const float2x2& a, const float2x2& b) noexcept {
    return approximately(a, b);
}

inline bool operator!=(const float2x2& a, const float2x2& b) noexcept {
    return !approximately(a, b);
}

// ============================================================================
// Useful Constants
// ============================================================================

AFTERMATH_INLINE_VAR const float2x2 float2x2_Identity = float2x2::identity();
AFTERMATH_INLINE_VAR const float2x2 float2x2_Zero = float2x2::zero();

AFTERMATH_END

#pragma warning(pop)
