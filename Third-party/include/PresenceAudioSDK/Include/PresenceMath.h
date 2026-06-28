/*
====================================================================================================
  Presence Audio SDK
  High-Performance Real-time Audio Path Tracing & EAX Simulation Library
====================================================================================================

  Copyright (c) 2026 Presence Collaboratory, NSDeathman & Gemini 3 & DeepSeek

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  1. The above copyright notice and this permission notice shall be included in all
     copies or substantial portions of the Software.

  2. Any project (commercial, free, open-source, or closed-source) using this Software
     must include attribution to "Presence Audio SDK by Presence Collaboratory" in its
     documentation, credits, or about screen.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

====================================================================================================
  Developed by: NSDeathman (Architecture & Core), Gemini 3 (Optimization & Math)
  Organization: Presence Collaboratory
====================================================================================================
*/
#pragma once

// =================================================================================================
// SIMD / SSE INTRINSICS
// =================================================================================================
// Include headers for vector processor instructions.
// SSE (Streaming SIMD Extensions) allows performing mathematical operations
// on 4 float numbers simultaneously in a single CPU cycle.
// =================================================================================================
#include <xmmintrin.h> // SSE  (Basic operations: add, sub, mul, div)
#include <pmmintrin.h> // SSE3 (Horizontal addition: hadd)
#include <smmintrin.h> // SSE4.1 (Dot product: dp)

#pragma warning(push)
#pragma warning(disable : 4995)
#include <cmath>
#pragma warning(pop)

#include <algorithm>
#include <string>
#include <cstdio>
#include <cstdint>

#include "PresenceMacros.h"

PRESENCE_BEGIN

// Small constant for floating point comparisons (kEpsilon)
constexpr float kkEpsilon = 1e-5f;
constexpr float kPi = 3.1415926535f;

/**
    * @brief 3-dimensional vector class with SSE optimization.
    *
    * Represents a 3D vector (x, y, z), optimized for high-performance calculations.
    * Uses 128-bit XMM register for data storage, allowing arithmetic operations
    * (addition, multiplication) to execute 4 times faster than standard FPU code.
    *
    * @note Structure size: 16 bytes (x, y, z, w). Field w is used for alignment (Padding).
    */
PRESENCE_ALIGN(16) struct float3
{
public:
    // ========================================================================
    // Data Union
    // ========================================================================
    // Union allows dual memory access:
    // 1. As individual components {x, y, z} for programmer convenience
    // 2. As __m128 vector for processor instructions
    // ========================================================================
    union 
    {
        struct 
        {
            float x;
            float y;
            float z;
            float _w; // Padding (unused, but needed for 16-byte alignment)
        };
        __m128 simd_; // SSE register containing {x, y, z, w}
    };

    // ========================================================================
    // Constructors
    // ========================================================================

    /** @brief Default constructor. Initializes to zero. */
    inline float3() noexcept : simd_(_mm_setzero_ps()) {}

    /** @brief Constructor from scalar. All components equal s. */
    inline explicit float3(float s) noexcept : simd_(_mm_set1_ps(s)) {}

    /**
        * @brief Main constructor.
        * @note In SSE, data is loaded in reverse order (Little Endian), hence _mm_set_ps(w, z, y, x).
        */
    inline float3(float _x, float _y, float _z) noexcept
        : simd_(_mm_set_ps(0.0f, _z, _y, _x)) {}

    /** @brief Constructor directly from SSE register (for internal operations). */
    inline explicit float3(__m128 s) noexcept : simd_(s) {}

    // ========================================================================
    // Unary Operators
    // ========================================================================

    /**
        * @brief Unary minus (Vector inversion).
        * @return Vector (-x, -y, -z).
        * @note Implemented as subtraction from zero: 0.0f - v. Faster than multiplication by -1.
        */
    inline float3 operator-() const noexcept 
    {
        return float3(_mm_sub_ps(_mm_setzero_ps(), simd_));
    }

    // ========================================================================
    // Access & Assignment Operators
    // ========================================================================

    inline float3& operator=(const float3& rhs) noexcept 
    {
        simd_ = rhs.simd_;
        return *this;
    }

    // Array access (0=x, 1=y, 2=z). Unsafe (no bounds checking), but fast.
    inline float& operator[](int index) noexcept { return (&x)[index]; }
    inline const float& operator[](int index) const noexcept { return (&x)[index]; }

    // ========================================================================
    // SSE Optimized Arithmetic
    // ========================================================================
    // All operations are performed in parallel for 4 components in 1 CPU cycle.

    inline float3 operator+(const float3& v) const noexcept 
    {
        return float3(_mm_add_ps(simd_, v.simd_));
    }

    inline float3 operator-(const float3& v) const noexcept 
    {
        return float3(_mm_sub_ps(simd_, v.simd_));
    }

    // Component-wise multiplication (x*x, y*y, z*z)
    inline float3 operator*(const float3& v) const noexcept 
    {
        return float3(_mm_mul_ps(simd_, v.simd_));
    }

    // Component-wise division
    inline float3 operator/(const float3& v) const noexcept 
    {
        return float3(_mm_div_ps(simd_, v.simd_));
    }

    // Scalar multiplication
    inline float3 operator*(float s) const noexcept 
    {
        return float3(_mm_mul_ps(simd_, _mm_set1_ps(s)));
    }

    // Scalar division (multiply by reciprocal 1/s, faster)
    inline float3 operator/(float s) const noexcept 
    {
        return float3(_mm_mul_ps(simd_, _mm_set1_ps(1.0f / s)));
    }

    // Compound assignment operators (+=, -=, *=)
    inline float3& operator+=(const float3& v) noexcept 
    {
        simd_ = _mm_add_ps(simd_, v.simd_);
        return *this;
    }

    inline float3& operator-=(const float3& v) noexcept 
    {
        simd_ = _mm_sub_ps(simd_, v.simd_);
        return *this;
    }

    inline float3& operator*=(float s) noexcept 
    {
        simd_ = _mm_mul_ps(simd_, _mm_set1_ps(s));
        return *this;
    }

    // ========================================================================
    // Vector Mathematics
    // ========================================================================

    /**
        * @brief Dot Product (Scalar product).
        * @return x*v.x + y*v.y + z*v.z
        * @note Uses horizontal addition for result folding.
        */
    inline float dot(const float3& v) const noexcept 
    {
        __m128 mul = _mm_mul_ps(simd_, v.simd_);
        // Horizontal addition: (x+y, z+w, x+y, z+w)
        __m128 shuf = _mm_movehdup_ps(mul);
        __m128 sums = _mm_add_ps(mul, shuf);
        // Final addition: (x+y) + (z+w)
        shuf = _mm_movehl_ps(shuf, sums);
        sums = _mm_add_ss(sums, shuf);
        return _mm_cvtss_f32(sums);
    }

    /** @brief Vector magnitude (Length). */
    inline float magnitude() const noexcept 
    {
        return std::sqrt(dot(*this));
    }

    /** @brief Squared magnitude (Length squared). Faster as no sqrt required. */
    inline float length_sq() const noexcept 
    {
        return dot(*this);
    }

    /**
        * @brief Vector normalization (scale to unit length).
        * @return Unit vector with same direction.
        */
    inline float3 normalize() const noexcept 
    {
        // Use SSE4.1 Dot Product if available (faster), otherwise manual dot()
        // Mask 0x7F means: multiply x,y,z (first 3 bits 7) and write result to all 4 floats (F)
        __m128 dp = _mm_dp_ps(simd_, simd_, 0x7F);

        float len = std::sqrt(_mm_cvtss_f32(dp));

        if (len > 1e-6f) {
            return *this / len;
        }
        return float3(); // Return (0,0,0) on division by zero
    }

    /** @brief Distance to another point. */
    inline float distance_to(const float3& v) const noexcept 
    {
        return (*this - v).magnitude();
    }

    /**
        * @brief Multiply-Add (MAD) operation.
        * @details Performs: this += v * s.
        * Efficiently used in ray tracing for forward step: pos += dir * distance.
        */
    inline void mad(const float3& v, float s) noexcept 
    {
        __m128 s_vec = _mm_set1_ps(s);
        __m128 mul = _mm_mul_ps(v.simd_, s_vec);
        simd_ = _mm_add_ps(simd_, mul);
    }

    /**
        * @brief Vector reflection from surface normal.
        * @param i Incident vector.
        * @param n Surface normal.
        * @return Reflected vector: i - 2 * dot(i, n) * n.
        */
    static float3 reflect(const float3& i, const float3& n) 
    {
        return i - n * (2.0f * i.dot(n));
    }

    // ========================================================================
    // Utilities / Debugging
    // ========================================================================

    /** @brief String representation "(x, y, z)" for logging. */
    std::string to_string() const 
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "(%.3f, %.3f, %.3f)", x, y, z);
        return std::string(buffer);
    }
};

/**
 * @brief 2-dimensional vector class (no SIMD optimization).
 *
 * Represents a 2D vector (x, y), suitable for general-purpose calculations.
 * This version uses plain float storage without SSE/AVX intrinsics, making it
 * portable and easy to understand.
 *
 * @note Structure size: 8 bytes (x, y). If alignment is required, use PRESENCE_ALIGN(8).
 */
PRESENCE_ALIGN(8) struct float2
{
public:
    // ========================================================================
    // Data
    // ========================================================================
    // Direct component access for simplicity.
    // No union is needed because there is no SIMD register representation.
    // ========================================================================
    float x;
    float y;

    // ========================================================================
    // Constructors
    // ========================================================================

    /** @brief Default constructor. Initializes to zero. */
    inline float2() noexcept : x(0.0f), y(0.0f) {}

    /** @brief Constructor from scalar. All components equal s. */
    inline explicit float2(float s) noexcept : x(s), y(s) {}

    /**
     * @brief Main constructor.
     * @param _x X component.
     * @param _y Y component.
     */
    inline float2(float _x, float _y) noexcept : x(_x), y(_y) {}

    // ========================================================================
    // Unary Operators
    // ========================================================================

    /**
     * @brief Unary minus (Vector inversion).
     * @return Vector (-x, -y).
     * @note Implemented by simple negation – no SSE required.
     */
    inline float2 operator-() const noexcept
    {
        return float2(-x, -y);
    }

    // ========================================================================
    // Access & Assignment Operators
    // ========================================================================

    inline float2& operator=(const float2& rhs) noexcept
    {
        x = rhs.x;
        y = rhs.y;
        return *this;
    }

    // Array access (0=x, 1=y). Unsafe (no bounds checking), but fast.
    inline float& operator[](int index) noexcept { return (&x)[index]; }
    inline const float& operator[](int index) const noexcept { return (&x)[index]; }

    // ========================================================================
    // Arithmetic (scalar style)
    // ========================================================================
    // All operations are performed component-wise using standard FPU instructions.

    inline float2 operator+(const float2& v) const noexcept
    {
        return float2(x + v.x, y + v.y);
    }

    inline float2 operator-(const float2& v) const noexcept
    {
        return float2(x - v.x, y - v.y);
    }

    // Component-wise multiplication (x * v.x, y * v.y)
    inline float2 operator*(const float2& v) const noexcept
    {
        return float2(x * v.x, y * v.y);
    }

    // Component-wise division
    inline float2 operator/(const float2& v) const noexcept
    {
        return float2(x / v.x, y / v.y);
    }

    // Scalar multiplication
    inline float2 operator*(float s) const noexcept
    {
        return float2(x * s, y * s);
    }

    // Scalar division (multiply by reciprocal 1/s, slightly faster)
    inline float2 operator/(float s) const noexcept
    {
        float inv = 1.0f / s;
        return float2(x * inv, y * inv);
    }

    // Compound assignment operators (+=, -=, *=)
    inline float2& operator+=(const float2& v) noexcept
    {
        x += v.x;
        y += v.y;
        return *this;
    }

    inline float2& operator-=(const float2& v) noexcept
    {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    inline float2& operator*=(float s) noexcept
    {
        x *= s;
        y *= s;
        return *this;
    }

    // ========================================================================
    // Vector Mathematics
    // ========================================================================

    /**
     * @brief Dot Product (Scalar product).
     * @return x * v.x + y * v.y
     */
    inline float dot(const float2& v) const noexcept
    {
        return x * v.x + y * v.y;
    }

    /**
     * @brief Cross Product (2D analog – returns a scalar).
     * @return The signed area of the parallelogram formed by the two vectors.
     * @note In 2D the cross product yields the z-component of the 3D cross product.
     *       Formula: x * v.y - y * v.x.
     */
    inline float cross(const float2& v) const noexcept
    {
        return x * v.y - y * v.x;
    }

    /** @brief Vector magnitude (Length). */
    inline float magnitude() const noexcept
    {
        return std::sqrt(dot(*this));
    }

    /** @brief Squared magnitude (Length squared). Faster as no sqrt required. */
    inline float length_sq() const noexcept
    {
        return dot(*this);
    }

    /**
     * @brief Vector normalization (scale to unit length).
     * @return Unit vector with the same direction, or (0,0) if length is near zero.
     */
    inline float2 normalize() const noexcept
    {
        float len = magnitude();
        if (len > 1e-6f) {
            return *this / len;
        }
        return float2(); // Return (0,0) on division by zero
    }

    /** @brief Distance to another point. */
    inline float distance_to(const float2& v) const noexcept
    {
        return (*this - v).magnitude();
    }

    /**
     * @brief Multiply-Add (MAD) operation.
     * @details Performs: this += v * s.
     * Efficiently used in iterative algorithms: pos += dir * step.
     */
    inline void mad(const float2& v, float s) noexcept
    {
        x += v.x * s;
        y += v.y * s;
    }

    /**
     * @brief Vector reflection from a surface normal.
     * @param i Incident vector.
     * @param n Surface normal (should be normalized).
     * @return Reflected vector: i - 2 * dot(i, n) * n.
     */
    static float2 reflect(const float2& i, const float2& n)
    {
        return i - n * (2.0f * i.dot(n));
    }

    // ========================================================================
    // Utilities / Debugging
    // ========================================================================

    /** @brief String representation "(x, y)" for logging. */
    std::string to_string() const
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "(%.3f, %.3f)", x, y);
        return std::string(buffer);
    }
};

// =================================================================================================
// GLOBAL OPERATORS
// =================================================================================================

/** @brief Allows writing "float * vector" (multiplication commutativity). */
inline float3 operator*(float s, const float3& v) noexcept 
{
    return v * s;
}

PRESENCE_END
