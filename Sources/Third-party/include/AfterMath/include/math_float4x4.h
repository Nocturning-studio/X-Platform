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

/**
 * @file math_float4x4.h
 * @brief 4x4 floating point precision matrix class with HLSL-like syntax and SSE optimization
 * @note Row-major storage, Left-Handed coordinate system
 * @note Vectors are ROW vectors: result = vec * mat
 * @note +Z points forward (into screen), +Y up, +X right
 * @note Depth range: [0, 1] (zero-to-one, Vulkan/DirectX style)
 */

#include <cmath>
#include <string>
#include <cstdio>
#include <algorithm>
#include <cassert>
#include <xmmintrin.h>
#include <pmmintrin.h>
#include <smmintrin.h>

#include "math_float2.h"
#include "math_float3.h"
#include "math_float4.h"
#include "math_float3x3.h"
#include "math_functions.h"
#include "AfterMathInternal.h"

AFTERMATH_BEGIN

// Forward declarations
class quaternion;

inline float4x4 operator*(const float4x4& a, const float4x4& b) noexcept;

// ============================================================================
// 4x4 Matrix Class
// ============================================================================

/**
 * @class float4x4
 * @brief 4x4 matrix with HLSL-like syntax
 *
 * Row-major storage, Left-Handed coordinate system.
 * Vectors are treated as ROW vectors: result = vec * mat
 * Concatenation order matches application order: vec * T * R * S
 *
 * Memory layout (row0..row3 stored contiguously):
 *   [ m00 m01 m02 m03 ]   <- row0
 *   [ m10 m11 m12 m13 ]   <- row1
 *   [ m20 m21 m22 m23 ]   <- row2
 *   [ m30 m31 m32 m33 ]   <- row3
 *
 * For affine transforms translation is stored in row3 (x, y, z).
 *
 * @note Size is exactly 64 bytes (4 float4 rows with 16-byte alignment each)
 */
class float4x4
{
public:
    // Data members (public for direct access, aligned for SSE)
    alignas(16) float4 row0; ///< First row  [m00, m01, m02, m03]
    alignas(16) float4 row1; ///< Second row [m10, m11, m12, m13]
    alignas(16) float4 row2; ///< Third row  [m20, m21, m22, m23]
    alignas(16) float4 row3; ///< Fourth row [m30, m31, m32, m33]  <- translation in .xyz

    // ============================================================================
    // Constructors
    // ============================================================================

    float4x4() noexcept
        : row0(1.0f, 0.0f, 0.0f, 0.0f)
        , row1(0.0f, 1.0f, 0.0f, 0.0f)
        , row2(0.0f, 0.0f, 1.0f, 0.0f)
        , row3(0.0f, 0.0f, 0.0f, 1.0f) {}

    float4x4(const float4& r0, const float4& r1, const float4& r2, const float4& r3) noexcept
        : row0(r0), row1(r1), row2(r2), row3(r3) {}

    float4x4(float m00, float m01, float m02, float m03,
        float m10, float m11, float m12, float m13,
        float m20, float m21, float m22, float m23,
        float m30, float m31, float m32, float m33) noexcept
        : row0(m00, m01, m02, m03)
        , row1(m10, m11, m12, m13)
        , row2(m20, m21, m22, m23)
        , row3(m30, m31, m32, m33) {}

    explicit float4x4(const float* data) noexcept
        : row0(data[0], data[1], data[2], data[3])
        , row1(data[4], data[5], data[6], data[7])
        , row2(data[8], data[9], data[10], data[11])
        , row3(data[12], data[13], data[14], data[15]) {}

    explicit float4x4(float scalar) noexcept
        : row0(scalar, 0.0f, 0.0f, 0.0f)
        , row1(0.0f, scalar, 0.0f, 0.0f)
        , row2(0.0f, 0.0f, scalar, 0.0f)
        , row3(0.0f, 0.0f, 0.0f, scalar) {}

    explicit float4x4(const float4& diagonal) noexcept
        : row0(diagonal.x, 0.0f, 0.0f, 0.0f)
        , row1(0.0f, diagonal.y, 0.0f, 0.0f)
        , row2(0.0f, 0.0f, diagonal.z, 0.0f)
        , row3(0.0f, 0.0f, 0.0f, diagonal.w) {}

    float4x4(const float4x4&) noexcept = default;

    explicit float4x4(const float3x3& m) noexcept;

    explicit float4x4(const quaternion& q) noexcept;

    // ============================================================================
    // Assignment Operators
    // ============================================================================

    float4x4& operator=(const float4x4&) noexcept = default;

    float4x4& operator=(float scalar) noexcept {
        row0 = float4(scalar, 0.0f, 0.0f, 0.0f);
        row1 = float4(0.0f, scalar, 0.0f, 0.0f);
        row2 = float4(0.0f, 0.0f, scalar, 0.0f);
        row3 = float4(0.0f, 0.0f, 0.0f, scalar);
        return *this;
    }

    float4x4& operator=(const float3x3& m) noexcept;

    float4x4& operator=(const quaternion& q) noexcept;

    // ============================================================================
    // Compound Assignment Operators
    // ============================================================================

    float4x4& operator+=(const float4x4& rhs) noexcept {
        row0 += rhs.row0; row1 += rhs.row1;
        row2 += rhs.row2; row3 += rhs.row3;
        return *this;
    }

    float4x4& operator-=(const float4x4& rhs) noexcept {
        row0 -= rhs.row0; row1 -= rhs.row1;
        row2 -= rhs.row2; row3 -= rhs.row3;
        return *this;
    }

    float4x4& operator*=(float scalar) noexcept {
        row0 *= scalar; row1 *= scalar;
        row2 *= scalar; row3 *= scalar;
        return *this;
    }

    float4x4& operator/=(float scalar) noexcept {
        row0 /= scalar; row1 /= scalar;
        row2 /= scalar; row3 /= scalar;
        return *this;
    }

    float4x4& operator*=(const float4x4& rhs) noexcept {
        *this = *this * rhs;
        return *this;
    }

    // ============================================================================
    // Access Operators
    // ============================================================================

    float4& operator[](int rowIndex) noexcept {
        return (&row0)[rowIndex];
    }

    const float4& operator[](int rowIndex) const noexcept {
        return (&row0)[rowIndex];
    }

    float& operator()(int row, int col) noexcept {
        return (&row0)[row][col];
    }

    const float& operator()(int row, int col) const noexcept {
        return (&row0)[row][col];
    }

    // ============================================================================
    // Column Access Methods
    // ============================================================================

    float4 col0() const noexcept { return float4(row0.x, row1.x, row2.x, row3.x); }
    float4 col1() const noexcept { return float4(row0.y, row1.y, row2.y, row3.y); }
    float4 col2() const noexcept { return float4(row0.z, row1.z, row2.z, row3.z); }
    float4 col3() const noexcept { return float4(row0.w, row1.w, row2.w, row3.w); }

    void set_col0(const float4& col) noexcept { row0.x = col.x; row1.x = col.y; row2.x = col.z; row3.x = col.w; }
    void set_col1(const float4& col) noexcept { row0.y = col.x; row1.y = col.y; row2.y = col.z; row3.y = col.w; }
    void set_col2(const float4& col) noexcept { row0.z = col.x; row1.z = col.y; row2.z = col.z; row3.z = col.w; }
    void set_col3(const float4& col) noexcept { row0.w = col.x; row1.w = col.y; row2.w = col.z; row3.w = col.w; }

    // ============================================================================
    // Unary Operators
    // ============================================================================

    float4x4 operator+() const noexcept { return *this; }
    float4x4 operator-() const noexcept { return float4x4(-row0, -row1, -row2, -row3); }

    // ============================================================================
    // Static Constructors
    // ============================================================================

    static float4x4 identity() noexcept { return float4x4(); }
    static float4x4 zero() noexcept { return float4x4(0.0f); }

    /// Translation: stored in row3.xyz (vec * mat convention)
    static float4x4 translation(float x, float y, float z) noexcept {
        return float4x4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            x, y, z, 1.0f
        );
    }

    static float4x4 translation(const float3& t) noexcept {
        return translation(t.x, t.y, t.z);
    }

    static float4x4 scaling(float x, float y, float z) noexcept {
        return float4x4(
            x, 0.0f, 0.0f, 0.0f,
            0.0f, y, 0.0f, 0.0f,
            0.0f, 0.0f, z, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
    }

    static float4x4 scaling(const float3& s) noexcept { return scaling(s.x, s.y, s.z); }
    static float4x4 scaling(float uniformScale) noexcept { return scaling(uniformScale, uniformScale, uniformScale); }

    static float4x4 rotation_x(float angle) noexcept;
    static float4x4 rotation_y(float angle) noexcept;
    static float4x4 rotation_z(float angle) noexcept;

    /// Euler rotation: applied in intrinsic order X -> Y -> Z (i.e. matrix = Rx * Ry * Rz)
    static float4x4 rotation_euler(const float3& angles) noexcept;

    static float4x4 perspective_lh_zo(float fovY, float aspect, float zNear = 0.01f, float zFar = 100.0f) noexcept;
    static float4x4 perspective_rh_zo(float fovY, float aspect, float zNear = 0.01f, float zFar = 100.0f) noexcept;
    static float4x4 perspective_lh_no(float fovY, float aspect, float zNear = 0.01f, float zFar = 100.0f) noexcept;

    /// Default perspective: LH, zero-to-one depth (DirectX/Vulkan)
    static float4x4 perspective(float fovY, float aspect, float zNear = 0.01f, float zFar = 100.0f) noexcept;

    static float4x4 orthographic_lh_zo(float width, float height, float zNear = 0.01f, float zFar = 100.0f) noexcept;
    static float4x4 orthographic_off_center_lh_zo(float left, float right, float bottom, float top, float zNear = 0.01f, float zFar = 100.0f) noexcept;

    /// Default orthographic: LH, zero-to-one depth
    static float4x4 orthographic(float width, float height, float zNear = 0.01f, float zFar = 100.0f) noexcept;

    static float4x4 look_at_lh(const float3& eye, const float3& target, const float3& up);

    /// Default look_at: left-handed
    static float4x4 look_at(const float3& eye, const float3& target, const float3& up);

    // ============================================================================
    // Utility Methods
    // ============================================================================

    std::string to_string() const {
        char buffer[512];
        std::snprintf(buffer, sizeof(buffer),
            "[%8.4f, %8.4f, %8.4f, %8.4f]\n"
            "[%8.4f, %8.4f, %8.4f, %8.4f]\n"
            "[%8.4f, %8.4f, %8.4f, %8.4f]\n"
            "[%8.4f, %8.4f, %8.4f, %8.4f]",
            row0.x, row0.y, row0.z, row0.w,
            row1.x, row1.y, row1.z, row1.w,
            row2.x, row2.y, row2.z, row2.w,
            row3.x, row3.y, row3.z, row3.w);
        return std::string(buffer);
    }

    void to_row_major(float* data) const noexcept {
        data[0] = row0.x; data[1] = row0.y; data[2] = row0.z; data[3] = row0.w;
        data[4] = row1.x; data[5] = row1.y; data[6] = row1.z; data[7] = row1.w;
        data[8] = row2.x; data[9] = row2.y; data[10] = row2.z; data[11] = row2.w;
        data[12] = row3.x; data[13] = row3.y; data[14] = row3.z; data[15] = row3.w;
    }

    void to_column_major(float* data) const noexcept {
        data[0] = row0.x; data[1] = row1.x; data[2] = row2.x; data[3] = row3.x;
        data[4] = row0.y; data[5] = row1.y; data[6] = row2.y; data[7] = row3.y;
        data[8] = row0.z; data[9] = row1.z; data[10] = row2.z; data[11] = row3.z;
        data[12] = row0.w; data[13] = row1.w; data[14] = row2.w; data[15] = row3.w;
    }
};

// ============================================================================
// Arithmetic Operators
// ============================================================================

inline float4x4 operator+(const float4x4& a, const float4x4& b) noexcept {
    return float4x4(a.row0 + b.row0, a.row1 + b.row1, a.row2 + b.row2, a.row3 + b.row3);
}

inline float4x4 operator-(const float4x4& a, const float4x4& b) noexcept {
    return float4x4(a.row0 - b.row0, a.row1 - b.row1, a.row2 - b.row2, a.row3 - b.row3);
}

inline float4x4 operator*(const float4x4& mat, float scalar) noexcept {
    return float4x4(mat.row0 * scalar, mat.row1 * scalar, mat.row2 * scalar, mat.row3 * scalar);
}

inline float4x4 operator*(float scalar, const float4x4& mat) noexcept {
    return mat * scalar;
}

inline float4x4 operator/(const float4x4& mat, float scalar) noexcept {
    return float4x4(mat.row0 / scalar, mat.row1 / scalar, mat.row2 / scalar, mat.row3 / scalar);
}

// ============================================================================
// Matrix Multiplication
// ============================================================================

inline float4x4 operator*(const float4x4& a, const float4x4& b) noexcept
{
    // Cache b's rows as __m128 — no conversion needed since float4 is a union
    const __m128 b0 = b.row0.simd_;
    const __m128 b1 = b.row1.simd_;
    const __m128 b2 = b.row2.simd_;
    const __m128 b3 = b.row3.simd_;

    float4x4 result;

    // Lambda to compute one result row:
    //   row_out = splat(a0)*b0 + splat(a1)*b1 + splat(a2)*b2 + splat(a3)*b3
    auto mul_row = [&](const float4& row_a) -> __m128 {
#ifdef __FMA__
        // FMA path: 4 fused multiply-adds, no intermediate rounding
        __m128 r = _mm_mul_ps(_mm_set1_ps(row_a.x), b0);
        r = _mm_fmadd_ps(_mm_set1_ps(row_a.y), b1, r);
        r = _mm_fmadd_ps(_mm_set1_ps(row_a.z), b2, r);
        r = _mm_fmadd_ps(_mm_set1_ps(row_a.w), b3, r);
#else
        // SSE path: 4 multiplies + 3 adds
        __m128 r = _mm_mul_ps(_mm_set1_ps(row_a.x), b0);
        r = _mm_add_ps(r, _mm_mul_ps(_mm_set1_ps(row_a.y), b1));
        r = _mm_add_ps(r, _mm_mul_ps(_mm_set1_ps(row_a.z), b2));
        r = _mm_add_ps(r, _mm_mul_ps(_mm_set1_ps(row_a.w), b3));
#endif
        return r;
    };

    result.row0.simd_ = mul_row(a.row0);
    result.row1.simd_ = mul_row(a.row1);
    result.row2.simd_ = mul_row(a.row2);
    result.row3.simd_ = mul_row(a.row3);

    return result;
}

// ============================================================================
// float3x3 <-> float4x4 Mixed Multiplication  (SSE optimized)
// ============================================================================
//
// float3 has no __m128 union, so rows are loaded manually via _mm_set_ps.
// The fourth lane is zeroed on load and handled explicitly where needed.
//
// --- float3x3 * float4x4 ---
//   Only rows 0..2 of the result are computed; row3 is copied from b.row3.
//   For each of the 3 output rows:
//     result.row_i = splat(a[i].x)*b.row0 + splat(a[i].y)*b.row1 + splat(a[i].z)*b.row2
//   b.row* are native __m128 (float4 union); a elements are scalar broadcasts.
//
// --- float4x4 * float3x3 ---
//   b has no __m128, so its rows are loaded once as __m128 (w lane = 0).
//   The w column of every result row is preserved from a[i][3] (not touched by b).
//   For each of the 4 output rows the first three lanes are:
//     result.row_i.xyz = splat(a[i][0])*b.row0 + splat(a[i][1])*b.row1 + splat(a[i][2])*b.row2
//   Then the w lane is patched back from a[i][3] via _mm_insert_ps / _mm_blend_ps.

inline float4x4 operator*(const float3x3& a, const float4x4& b) noexcept {
    // b rows are already __m128 via float4 union
    const __m128 b0 = b.row0.simd_;
    const __m128 b1 = b.row1.simd_;
    const __m128 b2 = b.row2.simd_;

    // Helper: compute one result row from a float3 row of a
    // result_row = splat(ax)*b0 + splat(ay)*b1 + splat(az)*b2
    auto mul_row = [&](const float3& ra) -> __m128 {
#ifdef __FMA__
        __m128 r = _mm_mul_ps(_mm_set1_ps(ra.x), b0);
        r = _mm_fmadd_ps(_mm_set1_ps(ra.y), b1, r);
        r = _mm_fmadd_ps(_mm_set1_ps(ra.z), b2, r);
#else
        __m128 r = _mm_mul_ps(_mm_set1_ps(ra.x), b0);
        r = _mm_add_ps(r, _mm_mul_ps(_mm_set1_ps(ra.y), b1));
        r = _mm_add_ps(r, _mm_mul_ps(_mm_set1_ps(ra.z), b2));
#endif
        return r;
    };

    float4x4 result;
    result.row0.simd_ = mul_row(a.row0);
    result.row1.simd_ = mul_row(a.row1);
    result.row2.simd_ = mul_row(a.row2);
    result.row3 = b.row3;           // a is 3x3: bottom row passes through from b
    return result;
}

inline float4x4 operator*(const float4x4& a, const float3x3& b) noexcept {
    // Load float3x3 rows into __m128 (w lane = 0, unused)
    const __m128 b0 = _mm_set_ps(0.0f, b.row0.z, b.row0.y, b.row0.x);
    const __m128 b1 = _mm_set_ps(0.0f, b.row1.z, b.row1.y, b.row1.x);
    const __m128 b2 = _mm_set_ps(0.0f, b.row2.z, b.row2.y, b.row2.x);

    // _mm_blend_ps(src0, src1, 0x8): lane 3 from src1, lanes 0..2 from src0.

    // Helper: compute xyz lanes via broadcast+FMA, restore w from a_row[3]
    auto mul_row = [&](const float4& ra) -> __m128 {
#ifdef __FMA__
        __m128 r = _mm_mul_ps(_mm_set1_ps(ra.x), b0);
        r = _mm_fmadd_ps(_mm_set1_ps(ra.y), b1, r);
        r = _mm_fmadd_ps(_mm_set1_ps(ra.z), b2, r);
#else
        __m128 r = _mm_mul_ps(_mm_set1_ps(ra.x), b0);
        r = _mm_add_ps(r, _mm_mul_ps(_mm_set1_ps(ra.y), b1));
        r = _mm_add_ps(r, _mm_mul_ps(_mm_set1_ps(ra.z), b2));
#endif
        // Lanes 0,1,2 are the correct dot products; lane 3 is 0 (from b load).
        // Restore original a[i][3] into lane 3.
        // _mm_blend_ps(src0, src1, 0x8): lane3 from src1 (a_w broadcast), rest from src0 (r).
        return _mm_blend_ps(r, _mm_set1_ps(ra.w), 0x8);
    };

    float4x4 result;
    result.row0.simd_ = mul_row(a.row0);
    result.row1.simd_ = mul_row(a.row1);
    result.row2.simd_ = mul_row(a.row2);
    result.row3.simd_ = mul_row(a.row3);
    return result;
}

// ============================================================================
// Vector * Matrix  (row-vector convention)
// ============================================================================

/// Transform a float4 row-vector by a matrix: result = vec * mat
///
/// Identical broadcast strategy as matrix * matrix:
///   result = splat(vec.x)*row0 + splat(vec.y)*row1 + splat(vec.z)*row2 + splat(vec.w)*row3
///
/// SSE:  4 mul + 3 add  (vs 16 mul + 12 add scalar)
/// FMA:  1 mul + 3 fmadd
inline float4 operator*(const float4& vec, const float4x4& mat) noexcept {
#ifdef __FMA__
    __m128 r = _mm_mul_ps(_mm_set1_ps(vec.x), mat.row0.simd_);
    r = _mm_fmadd_ps(_mm_set1_ps(vec.y), mat.row1.simd_, r);
    r = _mm_fmadd_ps(_mm_set1_ps(vec.z), mat.row2.simd_, r);
    r = _mm_fmadd_ps(_mm_set1_ps(vec.w), mat.row3.simd_, r);
#else
    __m128 r = _mm_mul_ps(_mm_set1_ps(vec.x), mat.row0.simd_);
    r = _mm_add_ps(r, _mm_mul_ps(_mm_set1_ps(vec.y), mat.row1.simd_));
    r = _mm_add_ps(r, _mm_mul_ps(_mm_set1_ps(vec.z), mat.row2.simd_));
    r = _mm_add_ps(r, _mm_mul_ps(_mm_set1_ps(vec.w), mat.row3.simd_));
#endif
    return float4(r);
}

/// Transform a point (w=1) and perform perspective divide
inline float3 operator*(const float3& point, const float4x4& mat) noexcept {
    float4 r = float4(point.x, point.y, point.z, 1.0f) * mat;
    return float3(r.x / r.w, r.y / r.w, r.z / r.w);
}

// ============================================================================
// Matrix Properties and Transformations
// ============================================================================

/// SSE-accelerated transpose
inline float4x4 transpose(const float4x4& mat) noexcept {
    __m128 t0 = _mm_shuffle_ps(mat.row0.get_simd(), mat.row1.get_simd(), 0x44);
    __m128 t2 = _mm_shuffle_ps(mat.row0.get_simd(), mat.row1.get_simd(), 0xEE);
    __m128 t1 = _mm_shuffle_ps(mat.row2.get_simd(), mat.row3.get_simd(), 0x44);
    __m128 t3 = _mm_shuffle_ps(mat.row2.get_simd(), mat.row3.get_simd(), 0xEE);

    return float4x4(
        float4(_mm_shuffle_ps(t0, t1, 0x88)),
        float4(_mm_shuffle_ps(t0, t1, 0xDD)),
        float4(_mm_shuffle_ps(t2, t3, 0x88)),
        float4(_mm_shuffle_ps(t2, t3, 0xDD))
    );
}

inline float determinant(const float4x4& mat) noexcept {
    float a = mat.row0.x, b = mat.row0.y, c = mat.row0.z, d = mat.row0.w;
    float e = mat.row1.x, f = mat.row1.y, g = mat.row1.z, h = mat.row1.w;
    float i = mat.row2.x, j = mat.row2.y, k = mat.row2.z, l = mat.row2.w;
    float m = mat.row3.x, n = mat.row3.y, o = mat.row3.z, p = mat.row3.w;

    float kplo = k * p - l * o;
    float jpln = j * p - l * n;
    float jokn = j * o - k * n;
    float iplm = i * p - l * m;
    float iokm = i * o - k * m;
    float in_jm = i * n - j * m;

    return  a * (f * kplo - g * jpln + h * jokn)
        - b * (e * kplo - g * iplm + h * iokm)
        + c * (e * jpln - f * iplm + h * in_jm)
        - d * (e * jokn - f * iokm + g * in_jm);
}

inline float4x4 adjugate(const float4x4& mat) noexcept {
    float a = mat.row0.x, b = mat.row0.y, c = mat.row0.z, d = mat.row0.w;
    float e = mat.row1.x, f = mat.row1.y, g = mat.row1.z, h = mat.row1.w;
    float i = mat.row2.x, j = mat.row2.y, k = mat.row2.z, l = mat.row2.w;
    float m = mat.row3.x, n = mat.row3.y, o = mat.row3.z, p = mat.row3.w;

    float kplo = k * p - l * o, jpln = j * p - l * n, jokn = j * o - k * n;
    float iplm = i * p - l * m, iokm = i * o - k * m, in_jm = i * n - j * m;
    float gpho = g * p - h * o, fphn = f * p - h * n, fogn = f * o - g * n;
    float ep_hm = e * p - h * m, eogm = e * o - g * m, en_fm = e * n - f * m;
    float gl_hk = g * l - h * k, fl_hj = f * l - h * j, fk_gj = f * k - g * j;
    float el_hi = e * l - h * i, ek_gi = e * k - g * i, ej_fi = e * j - f * i;

    return float4x4(
        f * kplo - g * jpln + h * jokn, -b * kplo + c * jpln - d * jokn,
        b * gpho - c * fphn + d * fogn, -b * gl_hk + c * fl_hj - d * fk_gj,
        -e * kplo + g * iplm - h * iokm, a * kplo - c * iplm + d * iokm,
        -a * gpho + c * ep_hm - d * eogm, a * gl_hk - c * el_hi + d * ek_gi,
        e * jpln - f * iplm + h * in_jm, -a * jpln + b * iplm - d * in_jm,
        a * fphn - b * ep_hm + d * en_fm, -a * fl_hj + b * el_hi - d * ej_fi,
        -e * jokn + f * iokm - g * in_jm, a * jokn - b * iokm + c * in_jm,
        -a * fogn + b * eogm - c * en_fm, a * fk_gj - b * ek_gi + c * ej_fi
    );
}

inline bool is_affine(const float4x4& mat, float epsilon = 1e-6f) noexcept {
    // In row-major LH convention the last COLUMN (mat[*].w) must be (0,0,0,1)
    return  std::abs(mat.row0.w) < epsilon &&
        std::abs(mat.row1.w) < epsilon &&
        std::abs(mat.row2.w) < epsilon &&
        std::abs(mat.row3.w - 1.0f) < epsilon;
}

/// Fast inverse for affine (non-projective) matrices with arbitrary non-uniform scale.
///
/// For a row-major affine matrix  M = [ R3x3 | 0 ; t | 1 ]  (vec * M convention):
///
///   inv(M) = [ inv(R3x3) | 0 ; -t * inv(R3x3) | 1 ]
///
/// inv(R3x3) is computed via the cofactor / determinant formula.
/// Key insight: cross(r1,r2), cross(r2,r0), cross(r0,r1) are the COLUMNS
/// of adj(R3x3), so they become the COLUMNS of inv(R3x3) = adj/det.
/// In a row-major result matrix they appear transposed across rows.
inline float4x4 inverse_affine(const float4x4& mat) noexcept {
    // Upper-left 3x3 basis vectors (rows of M)
    const float3 r0(mat.row0.x, mat.row0.y, mat.row0.z);
    const float3 r1(mat.row1.x, mat.row1.y, mat.row1.z);
    const float3 r2(mat.row2.x, mat.row2.y, mat.row2.z);
    const float3 t(mat.row3.x, mat.row3.y, mat.row3.z); // translation row

    // Determinant of upper-left 3x3
    const float det = dot(r0, cross(r1, r2));
    if (std::abs(det) < 1e-8f)
        return float4x4::identity();

    const float inv_det = 1.0f / det;

    // These three vectors are the COLUMNS of inv(R3x3) = adj(R3x3)/det.
    // cross(r1,r2) = column 0 of adj  ->  column 0 of inv(R3x3)
    // cross(r2,r0) = column 1 of adj  ->  column 1 of inv(R3x3)
    // cross(r0,r1) = column 2 of adj  ->  column 2 of inv(R3x3)
    const float3 col0 = cross(r1, r2) * inv_det;
    const float3 col1 = cross(r2, r0) * inv_det;
    const float3 col2 = cross(r0, r1) * inv_det;

    // Inverse translation row: -t * inv(R3x3)
    // Component k = -dot(t, column_k_of_inv(R))
    const float3 inv_t(
        -dot(t, col0),
        -dot(t, col1),
        -dot(t, col2)
    );

    // Lay out inv(R3x3) row-by-row (reading ROWS of the matrix whose COLUMNS are col0/1/2)
    // inv(R3x3) row i = ( col0[i], col1[i], col2[i] )
    return float4x4(
        col0.x, col1.x, col2.x, 0.0f,   // row 0 of inv(R3x3)
        col0.y, col1.y, col2.y, 0.0f,   // row 1 of inv(R3x3)
        col0.z, col1.z, col2.z, 0.0f,   // row 2 of inv(R3x3)
        inv_t.x, inv_t.y, inv_t.z, 1.0f // translation row
    );
}

inline float4x4 inverse(const float4x4& mat) noexcept {
    if (is_affine(mat))
        return inverse_affine(mat);

    float det = determinant(mat);
    if (std::abs(det) < 1e-8f)
        return float4x4::identity();

    return adjugate(mat) / det;
}

inline float trace(const float4x4& mat) noexcept {
    return mat.row0.x + mat.row1.y + mat.row2.z + mat.row3.w;
}

inline float4 diagonal(const float4x4& mat) noexcept {
    return float4(mat.row0.x, mat.row1.y, mat.row2.z, mat.row3.w);
}

inline float frobenius_norm(const float4x4& mat) noexcept {
    return std::sqrt(
        length_sq(mat.row0) + length_sq(mat.row1) +
        length_sq(mat.row2) + length_sq(mat.row3));
}

// ============================================================================
// Vector Transformation Helpers
// ============================================================================

/// Transform float4 row-vector: result = vec * mat
inline float4 transform_vector(const float4x4& mat, const float4& vec) noexcept {
    return vec * mat;
}

/// Transform a 3D point (w=1) with perspective divide
inline float3 transform_point(const float4x4& mat, const float3& point) noexcept {
    float4 r = float4(point.x, point.y, point.z, 1.0f) * mat;
    return float3(r.x / r.w, r.y / r.w, r.z / r.w);
}

/// Transform a 3D direction vector (w=0), no translation applied
inline float3 transform_vector(const float4x4& mat, const float3& vec) noexcept {
    float4 r = float4(vec.x, vec.y, vec.z, 0.0f) * mat;
    return float3(r.x, r.y, r.z);
}

/// Transform and re-normalize a direction vector
inline float3 transform_direction(const float4x4& mat, const float3& dir) noexcept {
    return normalize(transform_vector(mat, dir));
}

// ============================================================================
// Matrix Property Extractors
// ============================================================================

/// Returns translation stored in row3.xyz (row-major LH convention)
inline float3 get_translation(const float4x4& mat) noexcept {
    return float3(mat.row3.x, mat.row3.y, mat.row3.z);
}

/// Returns per-axis scale as the length of each basis row (row0/row1/row2)
inline float3 get_scale(const float4x4& mat) noexcept {
    float3 basis_x(mat.row0.x, mat.row0.y, mat.row0.z);
    float3 basis_y(mat.row1.x, mat.row1.y, mat.row1.z);
    float3 basis_z(mat.row2.x, mat.row2.y, mat.row2.z);
    return float3(length(basis_x), length(basis_y), length(basis_z));
}

inline bool is_identity(const float4x4& mat, float epsilon = 1e-6f) noexcept {
    return  approximately(mat.row0, float4(1.0f, 0.0f, 0.0f, 0.0f), epsilon) &&
        approximately(mat.row1, float4(0.0f, 1.0f, 0.0f, 0.0f), epsilon) &&
        approximately(mat.row2, float4(0.0f, 0.0f, 1.0f, 0.0f), epsilon) &&
        approximately(mat.row3, float4(0.0f, 0.0f, 0.0f, 1.0f), epsilon);
}

inline bool is_orthogonal(const float4x4& mat, float epsilon = 1e-6f) noexcept {
    if (!is_affine(mat, epsilon)) return false;

    float3 r0(mat.row0.x, mat.row0.y, mat.row0.z);
    float3 r1(mat.row1.x, mat.row1.y, mat.row1.z);
    float3 r2(mat.row2.x, mat.row2.y, mat.row2.z);

    if (std::abs(dot(r0, r1)) > epsilon) return false;
    if (std::abs(dot(r0, r2)) > epsilon) return false;
    if (std::abs(dot(r1, r2)) > epsilon) return false;

    return  approximately(length_sq(r0), 1.0f, epsilon) &&
        approximately(length_sq(r1), 1.0f, epsilon) &&
        approximately(length_sq(r2), 1.0f, epsilon);
}

inline bool approximately(const float4x4& a, const float4x4& b, float epsilon = 1e-6f) noexcept {
    return  approximately(a.row0, b.row0, epsilon) &&
        approximately(a.row1, b.row1, epsilon) &&
        approximately(a.row2, b.row2, epsilon) &&
        approximately(a.row3, b.row3, epsilon);
}

inline bool approximately_zero(const float4x4& mat, float epsilon = 1e-6f) noexcept {
    return approximately(mat, float4x4::zero(), epsilon);
}

// ============================================================================
// HLSL-style mul() overloads  (row-vector convention)
// ============================================================================

inline float4 mul(const float4& vec, const float4x4& mat) noexcept { return vec * mat; }
inline float3 mul(const float3& point, const float4x4& mat) noexcept { return transform_point(mat, point); }

// ============================================================================
// Rotation Implementations
// ============================================================================

/// Rotation around X axis (row-major, LH)
/// Positive angle rotates Y toward Z.
///
/// R_x = [ 1,  0,  0, 0 ]
///       [ 0,  c,  s, 0 ]
///       [ 0, -s,  c, 0 ]
///       [ 0,  0,  0, 1 ]
inline float4x4 float4x4::rotation_x(float angle) noexcept {
    float c = std::cos(angle), s = std::sin(angle);
    return float4x4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, c, s, 0.0f,
        0.0f, -s, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

/// Rotation around Y axis (row-major, LH)
/// Positive angle rotates Z toward X.
///
/// R_y = [  c, 0, -s, 0 ]
///       [  0, 1,  0, 0 ]
///       [  s, 0,  c, 0 ]
///       [  0, 0,  0, 1 ]
inline float4x4 float4x4::rotation_y(float angle) noexcept {
    float c = std::cos(angle), s = std::sin(angle);
    return float4x4(
        c, 0.0f, -s, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        s, 0.0f, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

/// Rotation around Z axis (row-major, LH)
/// Positive angle rotates X toward Y.
///
/// R_z = [  c, s, 0, 0 ]
///       [ -s, c, 0, 0 ]
///       [  0, 0, 1, 0 ]
///       [  0, 0, 0, 1 ]
inline float4x4 float4x4::rotation_z(float angle) noexcept {
    float c = std::cos(angle), s = std::sin(angle);
    return float4x4(
        c, s, 0.0f, 0.0f,
        -s, c, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

/// Euler rotation: intrinsic X -> Y -> Z
/// Combined matrix = Rx * Ry * Rz  (applied left to right in vec*mat convention)
inline float4x4 float4x4::rotation_euler(const float3& angles) noexcept {
    return rotation_x(angles.x) * rotation_y(angles.y) * rotation_z(angles.z);
}

// ============================================================================
// Projection Implementations
// ============================================================================

/// Left-handed perspective, zero-to-one depth  [DirectX / Vulkan style]
/// Maps Z in [zNear, zFar] -> [0, 1]
inline float4x4 float4x4::perspective_lh_zo(float fovY, float aspect, float zNear, float zFar) noexcept {
    float cot = 1.0f / std::tan(fovY * 0.5f);
    float r = zFar / (zFar - zNear);
    return float4x4(
        cot / aspect, 0.0f, 0.0f, 0.0f,
        0.0f, cot, 0.0f, 0.0f,
        0.0f, 0.0f, r, 1.0f,
        0.0f, 0.0f, -r * zNear, 0.0f
    );
}

/// Right-handed perspective, zero-to-one depth  [Vulkan RH style]
/// Maps Z in [-zFar, -zNear] -> [0, 1]
inline float4x4 float4x4::perspective_rh_zo(float fovY, float aspect, float zNear, float zFar) noexcept {
    float cot = 1.0f / std::tan(fovY * 0.5f);
    float r = zFar / (zNear - zFar); // negative range
    return float4x4(
        cot / aspect, 0.0f, 0.0f, 0.0f,
        0.0f, cot, 0.0f, 0.0f,
        0.0f, 0.0f, r, -1.0f,  // w = -z (RH: z flipped)
        0.0f, 0.0f, r * zNear, 0.0f
    );
}

/// Left-handed perspective, negative-one-to-one depth  [OpenGL LH style]
/// Maps Z in [zNear, zFar] -> [-1, 1]
inline float4x4 float4x4::perspective_lh_no(float fovY, float aspect, float zNear, float zFar) noexcept {
    float cot = 1.0f / std::tan(fovY * 0.5f);
    float r = (zFar + zNear) / (zFar - zNear);
    float t = -2.0f * zFar * zNear / (zFar - zNear);
    return float4x4(
        cot / aspect, 0.0f, 0.0f, 0.0f,
        0.0f, cot, 0.0f, 0.0f,
        0.0f, 0.0f, r, 1.0f,
        0.0f, 0.0f, t, 0.0f
    );
}

/// Default: LH, zero-to-one (DirectX / Vulkan)
inline float4x4 float4x4::perspective(float fovY, float aspect, float zNear, float zFar) noexcept {
    return perspective_lh_zo(fovY, aspect, zNear, zFar);
}

/// Left-handed orthographic, zero-to-one depth
inline float4x4 float4x4::orthographic_lh_zo(float width, float height, float zNear, float zFar) noexcept {
    float r = 1.0f / (zFar - zNear);
    return float4x4(
        2.0f / width, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / height, 0.0f, 0.0f,
        0.0f, 0.0f, r, 0.0f,
        0.0f, 0.0f, -zNear * r, 1.0f
    );
}

/// Left-handed off-center orthographic, zero-to-one depth
inline float4x4 float4x4::orthographic_off_center_lh_zo(
    float left, float right, float bottom, float top, float zNear, float zFar) noexcept
{
    float fRange = 1.0f / (zFar - zNear);
    return float4x4(
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
        0.0f, 0.0f, fRange, 0.0f,
        -(left + right) / (right - left), -(top + bottom) / (top - bottom), -zNear * fRange, 1.0f
    );
}

/// Default: LH, zero-to-one
inline float4x4 float4x4::orthographic(float width, float height, float zNear, float zFar) noexcept {
    return orthographic_lh_zo(width, height, zNear, zFar);
}

// ============================================================================
// View Matrix
// ============================================================================

/// Left-handed look_at (row-major)
/// Builds a view matrix where:
///   xaxis = right,   yaxis = up (reorthogonalized),   zaxis = forward (+Z)
/// Result transforms world-space positions into camera/view space.
inline float4x4 float4x4::look_at_lh(const float3& eye, const float3& target, const float3& up) {
    float3 zaxis = normalize(target - eye);          // forward (+Z into scene, LH)
    float3 xaxis = normalize(cross(zaxis, up));      // right  (LH: zaxis x up)
    float3 yaxis = cross(xaxis, zaxis);              // reorthogonalized up

    return float4x4(
        xaxis.x, yaxis.x, zaxis.x, 0.0f,
        xaxis.y, yaxis.y, zaxis.y, 0.0f,
        xaxis.z, yaxis.z, zaxis.z, 0.0f,
        -dot(xaxis, eye), -dot(yaxis, eye), -dot(zaxis, eye), 1.0f
    );
}

inline float4x4 float4x4::look_at(const float3& eye, const float3& target, const float3& up) {
    return look_at_lh(eye, target, up);
}

// ============================================================================
// Global Transformation Functions (HLSL style free functions)
// ============================================================================

inline float4x4 translation(float x, float y, float z) noexcept { return float4x4::translation(x, y, z); }
inline float4x4 translation(const float3& t) noexcept { return float4x4::translation(t); }
inline float4x4 translation(float s) noexcept { return float4x4::translation(s, s, s); }

inline float4x4 scaling(float x, float y, float z) noexcept { return float4x4::scaling(x, y, z); }
inline float4x4 scaling(const float3& s) noexcept { return float4x4::scaling(s); }
inline float4x4 scaling(float s) noexcept { return float4x4::scaling(s); }

inline float4x4 rotation_x(float angle) noexcept { return float4x4::rotation_x(angle); }
inline float4x4 rotation_y(float angle) noexcept { return float4x4::rotation_y(angle); }
inline float4x4 rotation_z(float angle) noexcept { return float4x4::rotation_z(angle); }
inline float4x4 rotation_euler(const float3& angles) noexcept { return float4x4::rotation_euler(angles); }

inline float4x4 perspective_lh_zo(float fovY, float aspect, float zNear = 0.01f, float zFar = 100.0f) noexcept
{
    return float4x4::perspective_lh_zo(fovY, aspect, zNear, zFar);
}
inline float4x4 perspective_rh_zo(float fovY, float aspect, float zNear = 0.01f, float zFar = 100.0f) noexcept
{
    return float4x4::perspective_rh_zo(fovY, aspect, zNear, zFar);
}
inline float4x4 perspective_lh_no(float fovY, float aspect, float zNear = 0.01f, float zFar = 100.0f) noexcept
{
    return float4x4::perspective_lh_no(fovY, aspect, zNear, zFar);
}
inline float4x4 perspective(float fovY, float aspect, float zNear = 0.01f, float zFar = 100.0f) noexcept
{
    return float4x4::perspective(fovY, aspect, zNear, zFar);
}

inline float4x4 orthographic_lh_zo(float width, float height, float zNear = 0.01f, float zFar = 100.0f) noexcept
{
    return float4x4::orthographic_lh_zo(width, height, zNear, zFar);
}
inline float4x4 orthographic_off_center_lh_zo(float left, float right, float bottom, float top, float zNear = 0.01f, float zFar = 100.0f) noexcept
{
    return float4x4::orthographic_off_center_lh_zo(left, right, bottom, top, zNear, zFar);
}
inline float4x4 orthographic(float width, float height, float zNear = 0.01f, float zFar = 100.0f) noexcept
{
    return float4x4::orthographic(width, height, zNear, zFar);
}

inline float4x4 look_at_lh(
    const float3& eye = float3(0.0f, 0.0f, 0.0f),
    const float3& target = float3(0.0f, 0.0f, 1.0f),
    const float3& up = float3(0.0f, 1.0f, 0.0f)) noexcept
{
    return float4x4::look_at_lh(eye, target, up);
}

inline float4x4 look_at(
    const float3& eye = float3(0.0f, 0.0f, 0.0f),
    const float3& target = float3(0.0f, 0.0f, 1.0f),
    const float3& up = float3(0.0f, 1.0f, 0.0f)) noexcept
{
    return float4x4::look_at(eye, target, up);
}

/// Rotation around an arbitrary axis (Rodrigues' formula)
inline float4x4 rotation_axis(const float3& axis, float angle) noexcept {
    if (approximately_zero(axis, 1e-8f))
        return float4x4::identity();

    float s = std::sin(angle), c = std::cos(angle), t = 1.0f - c;
    float3 n = normalize(axis);
    float x = n.x, y = n.y, z = n.z;

    // Row-major form of Rodrigues' rotation matrix
    return float4x4(
        t * x * x + c, t * x * y + z * s, t * x * z - y * s, 0.0f,
        t * x * y - z * s, t * y * y + c, t * y * z + x * s, 0.0f,
        t * x * z + y * s, t * y * z - x * s, t * z * z + c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

// ============================================================================
// Comparison Operators
// Note: operator== uses epsilon comparison (HLSL-style convenience).
// For strict bit-exact comparison use memcmp or compare rows manually.
// ============================================================================

inline bool operator==(const float4x4& a, const float4x4& b) noexcept { return  approximately(a, b); }
inline bool operator!=(const float4x4& a, const float4x4& b) noexcept { return !approximately(a, b); }

// ============================================================================
// Useful Constants
// ============================================================================

AFTERMATH_INLINE_VAR const float4x4 float4x4_Identity = float4x4::identity();
AFTERMATH_INLINE_VAR const float4x4 float4x4_Zero = float4x4::zero();

AFTERMATH_END
