// math_float3x3.h
// Description: 3x3 matrix class with HLSL-like syntax
// Author: NSDeathman, DeepSeek

#pragma once

#include <cmath>
#include <string>
#include <cstdio>
#include <xmmintrin.h>
#include <pmmintrin.h>

#include "math_float3.h"
#include "AfterMathInternal.h"

AFTERMATH_BEGIN

// Forward declarations
class float2;
class float3;
class float4x4;
class quaternion;
class float3x3;

// Forward declare the multiplication operator to resolve circular dependency
float3x3 operator*(const float3x3& lhs, const float3x3& rhs) noexcept;

// ============================================================================
// 3x3 Matrix Class
// ============================================================================

/**
    * @class float3x3
    * @brief 3x3 matrix with HLSL-like syntax
    *
    * Represents a 3x3 matrix for 3D transformations (rotation, scaling, shearing).
    * Uses row-major storage for compatibility with HLSL conventions.
    * Size is exactly 36 bytes for proper shader usage.
    */
class float3x3 {
public:
    // Data members (public for direct access)
    float3 row0; ///< First row of the matrix (x-axis basis)
    float3 row1; ///< Second row of the matrix (y-axis basis)
    float3 row2; ///< Third row of the matrix (z-axis basis)

    // ============================================================================
    // Constructors
    // ============================================================================

    constexpr float3x3() noexcept : row0(1.0f, 0.0f, 0.0f),
        row1(0.0f, 1.0f, 0.0f),
        row2(0.0f, 0.0f, 1.0f) {}

    constexpr float3x3(const float3& row0, const float3& row1, const float3& row2) noexcept
        : row0(row0), row1(row1), row2(row2) {}

    constexpr float3x3(float m00, float m01, float m02,
        float m10, float m11, float m12,
        float m20, float m21, float m22) noexcept
        : row0(m00, m01, m02), row1(m10, m11, m12), row2(m20, m21, m22) {}

    explicit constexpr float3x3(float scalar) noexcept
        : row0(scalar, 0.0f, 0.0f), row1(0.0f, scalar, 0.0f), row2(0.0f, 0.0f, scalar) {}

    explicit constexpr float3x3(const float3& diagonal) noexcept
        : row0(diagonal.x, 0.0f, 0.0f),
        row1(0.0f, diagonal.y, 0.0f),
        row2(0.0f, 0.0f, diagonal.z) {}

    explicit float3x3(const float* data) noexcept
        : row0(data[0], data[1], data[2]),
        row1(data[3], data[4], data[5]),
        row2(data[6], data[7], data[8]) {}

    float3x3(const float3x3&) noexcept = default;

    //// Constructor from float4x4 (extracts upper-left 3x3)
    //explicit float3x3(const float4x4& mat4x4) noexcept;

    //// Constructor from quaternion
    //explicit float3x3(const quaternion& q) noexcept;

    // ============================================================================
    // Assignment Operators
    // ============================================================================

    float3x3& operator=(const float3x3&) noexcept = default;

    float3x3& operator=(float scalar) noexcept {
        row0 = float3(scalar, 0.0f, 0.0f);
        row1 = float3(0.0f, scalar, 0.0f);
        row2 = float3(0.0f, 0.0f, scalar);
        return *this;
    }

    // Assignment from float4x4
    /*float3x3& operator=(const float4x4& mat4x4) noexcept;*/

    // ============================================================================
    // Compound Assignment Operators
    // ============================================================================

    float3x3& operator+=(const float3x3& rhs) noexcept {
        row0 += rhs.row0;
        row1 += rhs.row1;
        row2 += rhs.row2;
        return *this;
    }

    float3x3& operator-=(const float3x3& rhs) noexcept {
        row0 -= rhs.row0;
        row1 -= rhs.row1;
        row2 -= rhs.row2;
        return *this;
    }

    float3x3& operator*=(float scalar) noexcept {
        row0 *= scalar;
        row1 *= scalar;
        row2 *= scalar;
        return *this;
    }

    float3x3& operator/=(float scalar) noexcept {
        row0 /= scalar;
        row1 /= scalar;
        row2 /= scalar;
        return *this;
    }

    float3x3& operator*=(const float3x3& rhs) noexcept {
        *this = ::AfterMath::operator*(*this, rhs);
        return *this;
    }

    // ============================================================================
    // Access Operators
    // ============================================================================

    float3& operator[](int rowIndex) noexcept {
        return (rowIndex == 0) ? row0 : (rowIndex == 1) ? row1 : row2;
    }

    const float3& operator[](int rowIndex) const noexcept {
        return (rowIndex == 0) ? row0 : (rowIndex == 1) ? row1 : row2;
    }

    float& operator()(int row, int col) noexcept {
        return (row == 0) ? row0[col] : (row == 1) ? row1[col] : row2[col];
    }

    const float& operator()(int row, int col) const noexcept {
        return (row == 0) ? row0[col] : (row == 1) ? row1[col] : row2[col];
    }

    // ============================================================================
    // Basic Methods (minimal, most operations moved to global functions)
    // ============================================================================

    constexpr float3 col0() const noexcept { return float3(row0.x, row1.x, row2.x); }
    constexpr float3 col1() const noexcept { return float3(row0.y, row1.y, row2.y); }
    constexpr float3 col2() const noexcept { return float3(row0.z, row1.z, row2.z); }

    void set_col0(const float3& col) noexcept {
        row0.x = col.x;
        row1.x = col.y;
        row2.x = col.z;
    }

    void set_col1(const float3& col) noexcept {
        row0.y = col.x;
        row1.y = col.y;
        row2.y = col.z;
    }

    void set_col2(const float3& col) noexcept {
        row0.z = col.x;
        row1.z = col.y;
        row2.z = col.z;
    }

    std::string to_string() const {
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer),
            "[%8.4f, %8.4f, %8.4f]\n"
            "[%8.4f, %8.4f, %8.4f]\n"
            "[%8.4f, %8.4f, %8.4f]",
            row0.x, row0.y, row0.z,
            row1.x, row1.y, row1.z,
            row2.x, row2.y, row2.z);
        return std::string(buffer);
    }

    void to_row_major(float* data) const noexcept {
        data[0] = row0.x; data[1] = row0.y; data[2] = row0.z;
        data[3] = row1.x; data[4] = row1.y; data[5] = row1.z;
        data[6] = row2.x; data[7] = row2.y; data[8] = row2.z;
    }

    void to_column_major(float* data) const noexcept {
        data[0] = row0.x; data[1] = row1.x; data[2] = row2.x;
        data[3] = row0.y; data[4] = row1.y; data[5] = row2.y;
        data[6] = row0.z; data[7] = row1.z; data[8] = row2.z;
    }

    // ============================================================================
    // Static Constructors
    // ============================================================================

    static constexpr float3x3 identity() noexcept {
        return float3x3(float3(1.0f, 0.0f, 0.0f),
            float3(0.0f, 1.0f, 0.0f),
            float3(0.0f, 0.0f, 1.0f));
    }

    static constexpr float3x3 zero() noexcept {
        return float3x3(float3(0.0f, 0.0f, 0.0f),
            float3(0.0f, 0.0f, 0.0f),
            float3(0.0f, 0.0f, 0.0f));
    }

    static constexpr float3x3 scaling(const float3& scale) noexcept {
        return float3x3(float3(scale.x, 0.0f, 0.0f),
            float3(0.0f, scale.y, 0.0f),
            float3(0.0f, 0.0f, scale.z));
    }

    static constexpr float3x3 scaling(float scaleX, float scaleY, float scaleZ) noexcept {
        return float3x3(float3(scaleX, 0.0f, 0.0f),
            float3(0.0f, scaleY, 0.0f),
            float3(0.0f, 0.0f, scaleZ));
    }

    static constexpr float3x3 scaling(float scale) noexcept {
        return float3x3(float3(scale, 0.0f, 0.0f),
            float3(0.0f, scale, 0.0f),
            float3(0.0f, 0.0f, scale));
    }

    static float3x3 rotation_x(float angle) noexcept {
        float s = std::sin(angle);
        float c = std::cos(angle);
        return  float3x3(float3(1.0f, 0.0f, 0.0f),
                         float3(0.0f, c,    s),
                         float3(0.0f, -s,   c));
    }

    static float3x3 rotation_y(float angle) noexcept {
        float s = std::sin(angle);
        float c = std::cos(angle);
        return  float3x3(float3(c,    0.0f, -s),
                         float3(0.0f, 1.0f, 0.0f),
                         float3(s,    0.0f, c));
    }

    static float3x3 rotation_z(float angle) noexcept {
        float s = std::sin(angle);
        float c = std::cos(angle);
        return  float3x3(float3(c,    s,    0.0f),
                         float3(-s,   c,    0.0f),
                         float3(0.0f, 0.0f, 1.0f));
    }

    /*static float3x3 rotation_axis(const float3& axis, float angle) noexcept;*/

    static float3x3 rotation_euler(const float3& angles) noexcept {
        return rotation_z(angles.z) * rotation_y(angles.y) * rotation_x(angles.x);
    }

    static float3x3 skew_symmetric(const float3& vec) noexcept {
        return  float3x3(
                float3(0.0f, vec.z, -vec.y),     // row 0: (0, vz, -vy)
                float3(-vec.z, 0.0f, vec.x),     // row 1: (-vz, 0, vx)
                float3(vec.y, -vec.x, 0.0f)      // row 2: (vy, -vx, 0)
        );
    }

    static float3x3 outer_product(const float3& u, const float3& v) noexcept {
        return  float3x3(float3(u.x * v.x, u.x * v.y, u.x * v.z),
                float3(u.y * v.x, u.y * v.y, u.y * v.z),
                float3(u.z * v.x, u.z * v.y, u.z * v.z));
    }
};

// ============================================================================
// Global Mathematical Functions (HLSL Style)
// ============================================================================

// Matrix operations
inline float3x3 operator+(float3x3 lhs, const float3x3& rhs) noexcept {
    return float3x3(lhs.row0 + rhs.row0, lhs.row1 + rhs.row1, lhs.row2 + rhs.row2);
}

inline float3x3 operator-(float3x3 lhs, const float3x3& rhs) noexcept {
    return float3x3(lhs.row0 - rhs.row0, lhs.row1 - rhs.row1, lhs.row2 - rhs.row2);
}

inline float3x3 operator*(const float3x3& lhs, const float3x3& rhs) noexcept {
    float3x3 result;

    for (int i = 0; i < 3; ++i) {
        float3 row = lhs[i];
        result[i] = float3(
            dot(row, rhs.col0()),
            dot(row, rhs.col1()),
            dot(row, rhs.col2())
        );
    }

    return result;
}

inline float3x3 operator*(float3x3 mat, float scalar) noexcept {
    return float3x3(mat.row0 * scalar, mat.row1 * scalar, mat.row2 * scalar);
}

inline float3x3 operator*(float scalar, float3x3 mat) noexcept {
    return mat * scalar;
}

inline float3x3 operator/(float3x3 mat, float scalar) noexcept {
    return float3x3(mat.row0 / scalar, mat.row1 / scalar, mat.row2 / scalar);
}

// Matrix-vector multiplication
inline float3 operator*(const float3& vec, const float3x3& mat) noexcept {
    return float3(
        vec.x * mat(0, 0) + vec.y * mat(1, 0) + vec.z * mat(2, 0),
        vec.x * mat(0, 1) + vec.y * mat(1, 1) + vec.z * mat(2, 1),
        vec.x * mat(0, 2) + vec.y * mat(1, 2) + vec.z * mat(2, 2)
    );
}

// Unary operators
inline float3x3 operator+(const float3x3& mat) noexcept {
    return mat;
}

inline float3x3 operator-(const float3x3& mat) noexcept {
    return float3x3(-mat.row0, -mat.row1, -mat.row2);
}

// Matrix properties and transformations
inline float3x3 transpose(const float3x3& mat) noexcept {
    return float3x3(mat.col0(), mat.col1(), mat.col2());
}

inline float determinant(const float3x3& mat) noexcept {
    const float3 r0 = mat.row0;
    const float3 r1 = mat.row1;
    const float3 r2 = mat.row2;

    const float a = r0.x, b = r0.y, c = r0.z;
    const float d = r1.x, e = r1.y, f = r1.z;
    const float g = r2.x, h = r2.y, i = r2.z;

    return a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
}

inline float3x3 inverse(const float3x3& mat) noexcept {
    const float3 r0 = mat.row0;
    const float3 r1 = mat.row1;
    const float3 r2 = mat.row2;

    const float a = r0.x, b = r0.y, c = r0.z;
    const float d = r1.x, e = r1.y, f = r1.z;
    const float g = r2.x, h = r2.y, i = r2.z;

    const float det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);

    if (std::abs(det) < 1e-8f) {
        return float3x3::identity();
    }

    const float inv_det = 1.0f / det;

    const float a11 = (e * i - f * h) * inv_det;
    const float a12 = (c * h - b * i) * inv_det;
    const float a13 = (b * f - c * e) * inv_det;
    const float a21 = (f * g - d * i) * inv_det;
    const float a22 = (a * i - c * g) * inv_det;
    const float a23 = (c * d - a * f) * inv_det;
    const float a31 = (d * h - e * g) * inv_det;
    const float a32 = (b * g - a * h) * inv_det;
    const float a33 = (a * e - b * d) * inv_det;

    return float3x3(
        float3(a11, a12, a13),
        float3(a21, a22, a23),
        float3(a31, a32, a33)
    );
}

inline float trace(const float3x3& mat) noexcept {
    return mat.row0.x + mat.row1.y + mat.row2.z;
}

inline float3 diagonal(const float3x3& mat) noexcept {
    return float3(mat.row0.x, mat.row1.y, mat.row2.z);
}

inline float frobenius_norm(const float3x3& mat) noexcept {
    return std::sqrt(length_sq(mat.row0) + length_sq(mat.row1) + length_sq(mat.row2));
}

inline float3x3 symmetric_part(const float3x3& mat) noexcept {
    float3x3 trans = transpose(mat);
    return (mat + trans) * 0.5f;
}

inline float3x3 skew_symmetric_part(const float3x3& mat) noexcept {
    float3x3 trans = transpose(mat);
    return (mat - trans) * 0.5f;
}

inline float3 transform_vector(const float3x3& mat, const float3& vec) noexcept {
    return vec * mat;
}

inline float3 transform_point(const float3x3& mat, const float3& point) noexcept {
    return point * mat;
}

inline bool is_orthonormal(const float3x3& mat, float epsilon = 1e-6f) noexcept {
    float3 c0 = mat.col0();
    float3 c1 = mat.col1();
    float3 c2 = mat.col2();

    return std::abs(length_sq(c0) - 1.0f) <= epsilon &&
        std::abs(length_sq(c1) - 1.0f) <= epsilon &&
        std::abs(length_sq(c2) - 1.0f) <= epsilon &&
        std::abs(dot(c0, c1)) <= epsilon &&
        std::abs(dot(c0, c2)) <= epsilon &&
        std::abs(dot(c1, c2)) <= epsilon;
}

inline float3 transform_normal(const float3x3& mat, const float3& normal) noexcept {
    if (is_orthonormal(mat, 1e-4f)) {
        return normal * mat;
    }

    float3x3 inv = inverse(mat);
    return float3(
        dot(normal, inv.col0()),
        dot(normal, inv.col1()),
        dot(normal, inv.col2())
    );
}

// Matrix properties
inline bool is_identity(const float3x3& mat, float epsilon = 1e-6f) noexcept {
    return approximately(mat.row0, float3(1.0f, 0.0f, 0.0f), epsilon) &&
        approximately(mat.row1, float3(0.0f, 1.0f, 0.0f), epsilon) &&
        approximately(mat.row2, float3(0.0f, 0.0f, 1.0f), epsilon);
}

inline bool is_orthogonal(const float3x3& mat, float epsilon = 1e-6f) noexcept {
    float3 c0 = mat.col0();
    float3 c1 = mat.col1();
    float3 c2 = mat.col2();

    return std::abs(dot(c0, c1)) <= epsilon &&
        std::abs(dot(c0, c2)) <= epsilon &&
        std::abs(dot(c1, c2)) <= epsilon;
}

inline bool approximately(const float3x3& a, const float3x3& b, float epsilon = 1e-6f) noexcept {
    return approximately(a.row0, b.row0, epsilon) &&
        approximately(a.row1, b.row1, epsilon) &&
        approximately(a.row2, b.row2, epsilon);
}

inline bool approximately_zero(const float3x3& mat, float epsilon = 1e-6f) noexcept {
    return approximately_zero(mat.row0, epsilon) &&
        approximately_zero(mat.row1, epsilon) &&
        approximately_zero(mat.row2, epsilon);
}

inline float3x3 normal_matrix(const float3x3& model) noexcept {
    float3x3 inv = inverse(model);
    float3x3 result = transpose(inv); // это (M^{-1})^T

    float3 r0 = normalize(result.row0);
    float3 r1 = normalize(result.row1);
    float3 r2 = normalize(result.row2);
    return float3x3(r0, r1, r2);
}

inline float3 extract_scale(const float3x3& mat) noexcept {
    return float3(length(mat.row0), length(mat.row1), length(mat.row2));
}

inline float3x3 extract_rotation(const float3x3& mat) noexcept {
    if (is_orthonormal(mat, 1e-4f)) {
        return mat;
    }

    float3 r0 = normalize(mat.row0);
    float3 r1 = mat.row1 - r0 * dot(mat.row1, r0);
    r1 = normalize(r1);
    float3 r2 = cross(r0, r1);

    return float3x3(r0, r1, r2);
}

// HLSL-like functions
inline float3x3 mul(const float3x3& a, const float3x3& b) noexcept {
    return a * b;
}

inline float3 mul(const float3& vec, const float3x3& mat) noexcept {
    return vec * mat;
}

// Comparison operators
inline bool operator==(const float3x3& a, const float3x3& b) noexcept {
    return approximately(a, b);
}

inline bool operator!=(const float3x3& a, const float3x3& b) noexcept {
    return !approximately(a, b);
}

// ============================================================================
// Useful Constants
// ============================================================================

AFTERMATH_INLINE_VAR const float3x3 float3x3_Identity = float3x3::identity();
AFTERMATH_INLINE_VAR const float3x3 float3x3_Zero = float3x3::zero();

AFTERMATH_END
