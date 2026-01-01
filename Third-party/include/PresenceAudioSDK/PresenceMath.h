/*
====================================================================================================
  Presence Audio SDK
  High-Performance Real-time Audio Path Tracing & EAX Simulation Library
====================================================================================================

  Copyright (c) 2025 Presence Collaboratory, NSDeathman & Gemini 3

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

// Suppress X-Ray Engine warnings about deprecated functions (sin, cos, sqrt)
#pragma warning(push)
#pragma warning(disable : 4995)
#include <cmath>
#pragma warning(pop)

#include <algorithm>
#include <string>
#include <cstdio>
#include <cstdint>

// =================================================================================================
// MEMORY ALIGNMENT MACRO
// =================================================================================================
// For efficient SSE operation, data must be aligned to 16-byte boundary.
// If memory address is not multiple of 16, _mm_load_ps will cause crash (Access Violation).
// =================================================================================================
#if defined(_MSC_VER)
#define PRESENCE_ALIGN(x) __declspec(align(x))
#else
#define PRESENCE_ALIGN(x) __attribute__((aligned(x)))
#endif

namespace Presence
{
    // Small constant for floating point comparisons (Epsilon)
    static constexpr float EPSILON = 1e-5f;

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
        union {
            struct {
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
        inline float3 operator-() const noexcept {
            return float3(_mm_sub_ps(_mm_setzero_ps(), simd_));
        }

        // ========================================================================
        // Access & Assignment Operators
        // ========================================================================

        inline float3& operator=(const float3& rhs) noexcept {
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

        inline float3 operator+(const float3& v) const noexcept {
            return float3(_mm_add_ps(simd_, v.simd_));
        }

        inline float3 operator-(const float3& v) const noexcept {
            return float3(_mm_sub_ps(simd_, v.simd_));
        }

        // Component-wise multiplication (x*x, y*y, z*z)
        inline float3 operator*(const float3& v) const noexcept {
            return float3(_mm_mul_ps(simd_, v.simd_));
        }

        // Component-wise division
        inline float3 operator/(const float3& v) const noexcept {
            return float3(_mm_div_ps(simd_, v.simd_));
        }

        // Scalar multiplication
        inline float3 operator*(float s) const noexcept {
            return float3(_mm_mul_ps(simd_, _mm_set1_ps(s)));
        }

        // Scalar division (multiply by reciprocal 1/s, faster)
        inline float3 operator/(float s) const noexcept {
            return float3(_mm_mul_ps(simd_, _mm_set1_ps(1.0f / s)));
        }

        // Compound assignment operators (+=, -=, *=)
        inline float3& operator+=(const float3& v) noexcept {
            simd_ = _mm_add_ps(simd_, v.simd_);
            return *this;
        }

        inline float3& operator-=(const float3& v) noexcept {
            simd_ = _mm_sub_ps(simd_, v.simd_);
            return *this;
        }

        inline float3& operator*=(float s) noexcept {
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
        inline float dot(const float3& v) const noexcept {
            __m128 mul = _mm_mul_ps(simd_, v.simd_);
            // Horizontal addition: (x+y, z+w, x+y, z+w)
            __m128 shuf = _mm_movehdup_ps(mul);
            __m128 sums = _mm_add_ps(mul, shuf);
            // Final addition: (x+y) + (z+w)
            shuf = _mm_movehl_ps(shuf, sums);
            sums = _mm_add_ss(sums, shuf);
            return _mm_cvtss_f32(sums);
        }

        /**
         * @brief Cross Product (Vector product).
         * @return Vector perpendicular to both input vectors.
         * @note Implemented using shuffles (byte permutation), without switching to scalar mode.
         */
        inline float3 cross(const float3& v) const noexcept {
            // Formula: (y1*z2 - z1*y2, z1*x2 - x1*z2, x1*y2 - y1*x2)

            // Permute vectors to align components for multiplication
            __m128 a_yzx = _mm_shuffle_ps(simd_, simd_, _MM_SHUFFLE(3, 0, 2, 1)); // (y, z, x, w)
            __m128 b_yzx = _mm_shuffle_ps(v.simd_, v.simd_, _MM_SHUFFLE(3, 0, 2, 1));

            __m128 a_zxy = _mm_shuffle_ps(simd_, simd_, _MM_SHUFFLE(3, 1, 0, 2)); // (z, x, y, w)
            __m128 b_zxy = _mm_shuffle_ps(v.simd_, v.simd_, _MM_SHUFFLE(3, 1, 0, 2));

            // (y1*z2, z1*x2, ...)
            __m128 mul1 = _mm_mul_ps(a_yzx, b_zxy);
            // (z1*y2, x1*z2, ...)
            __m128 mul2 = _mm_mul_ps(a_zxy, b_yzx);

            // Subtraction gives the final cross product result
            return float3(_mm_sub_ps(mul1, mul2));
        }

        /** @brief Vector magnitude (Length). */
        inline float magnitude() const noexcept {
            return std::sqrt(dot(*this));
        }

        /** @brief Squared magnitude (Length squared). Faster as no sqrt required. */
        inline float length_sq() const noexcept {
            return dot(*this);
        }

        /**
         * @brief Vector normalization (scale to unit length).
         * @return Unit vector with same direction.
         */
        inline float3 normalize() const noexcept {
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
        inline float distance_to(const float3& v) const noexcept {
            return (*this - v).magnitude();
        }

        /**
         * @brief Multiply-Add (MAD) operation.
         * @details Performs: this += v * s.
         * Efficiently used in ray tracing for forward step: pos += dir * distance.
         */
        inline void mad(const float3& v, float s) noexcept {
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
        static float3 reflect(const float3& i, const float3& n) {
            return i - n * (2.0f * i.dot(n));
        }

        // ========================================================================
        // Utilities / Debugging
        // ========================================================================

        /** @brief String representation "(x, y, z)" for logging. */
        std::string to_string() const {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "(%.3f, %.3f, %.3f)", x, y, z);
            return std::string(buffer);
        }
    };

    // =================================================================================================
    // GLOBAL OPERATORS
    // =================================================================================================

    /** @brief Allows writing "float * vector" (multiplication commutativity). */
    inline float3 operator*(float s, const float3& v) noexcept {
        return v * s;
    }
}
