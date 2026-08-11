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
 * @file math_half.h
 * @brief 16-bit half-precision floating point type
 * @note Implements IEEE 754-2008 half-precision (16-bit) floating point format
 * @note Optimized for memory bandwidth and GPU compatibility
 * @note Fully compatible with half2, half3, half4 vector types
 */

#include <cstdint>
#include <cmath>
#include <type_traits>
#include <string>
#include <algorithm>
#include <limits>
#include <iostream>

#include "math_config.h"
#include "math_constants.h"
#include "math_functions.h"
#include "AfterMathInternal.h"

AFTERMATH_BEGIN
/**
    * @class half
    * @brief 16-bit half-precision floating point type
    *
    * Implements IEEE 754-2008 half-precision (16-bit) floating point format
    * with comprehensive mathematical operations and HLSL compatibility.
    *
    * Format: 1 sign bit, 5 exponent bits, 10 mantissa bits
    * Range: ±65504.0, Precision: ~3 decimal digits
    *
    * @note Perfect for colors, normals, and other data where full 32-bit precision is not required
    * @note Optimized for memory bandwidth and GPU compatibility
    * @note Fully compatible with half2, half3, half4 vector types
    */
    class half
{
public:
    using storage_type = std::uint16_t;

    // ============================================================================
    // Constructors and basic operations
    // ============================================================================
    half() noexcept : data(0) {}
    half(float x) noexcept { data = float_to_half(x); }
    half(double x) noexcept { data = float_to_half(float(x)); }
    explicit half(storage_type bits) noexcept : data(bits) {}
    half(const half&) noexcept = default;

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    half(T x) noexcept : data(float_to_half(static_cast<float>(x))) {}

    // Assignment operators
    half& operator=(const half&) noexcept = default;
    half& operator=(float x) noexcept { data = float_to_half(x); return *this; }

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    half& operator=(T x) noexcept { data = float_to_half(static_cast<float>(x)); return *this; }

    // Conversions
    explicit operator float() const noexcept { return half_to_float(data); }
    explicit operator double() const noexcept { return static_cast<double>(half_to_float(data)); }

    // ============================================================================
    // Arithmetic operators
    // ============================================================================
    half operator+(half other) const noexcept { return half(float(*this) + float(other)); }
    half operator-(half other) const noexcept { return half(float(*this) - float(other)); }
    half operator*(half other) const noexcept { return multiply_direct(*this, other); }
    half operator/(half other) const noexcept { return half(float(*this) / float(other)); }

    half& operator+=(half other) noexcept { *this = *this + other; return *this; }
    half& operator-=(half other) noexcept { *this = *this - other; return *this; }
    half& operator*=(half other) noexcept { *this = *this * other; return *this; }
    half& operator/=(half other) noexcept { *this = *this / other; return *this; }

    // Unary operators
    half operator+() const noexcept { return *this; }
    half operator-() const noexcept { return from_bits(data ^ 0x8000u); }

    // Increment/decrement
    half& operator++() noexcept { *this = *this + half(1.0f); return *this; }
    half operator++(int) noexcept { half temp = *this; ++(*this); return temp; }
    half& operator--() noexcept { *this = *this - half(1.0f); return *this; }
    half operator--(int) noexcept { half temp = *this; --(*this); return temp; }

    // ============================================================================
    // Comparison operators
    // ============================================================================
    bool operator==(half other) const noexcept {
        if (is_nan() || other.is_nan()) return false;
        if (is_zero() && other.is_zero()) return true;
        return data == other.data; // Compare bits directly for identical values
    }

    bool operator!=(half other) const noexcept { return !(*this == other); }

    bool operator<(half other) const noexcept {
        if (is_nan() || other.is_nan()) return false;
        bool sign_a = (data >> 15) != 0;
        bool sign_b = (other.data >> 15) != 0;
        if (sign_a != sign_b) return sign_a;
        uint16_t abs_a = data & 0x7FFFu;
        uint16_t abs_b = other.data & 0x7FFFu;
        return sign_a ? (abs_a > abs_b) : (abs_a < abs_b);
    }

    bool operator>(half other) const noexcept { return other < *this; }
    bool operator<=(half other) const noexcept { return !(other < *this); }
    bool operator>=(half other) const noexcept { return !(*this < other); }

    // ============================================================================
    // Basic checks and properties
    // ============================================================================
    bool is_zero() const noexcept { return (bits() & 0x7FFFu) == 0; }
    bool is_positive_zero() const noexcept { return bits() == 0x0000; }
    bool is_negative_zero() const noexcept { return bits() == 0x8000; }
    bool is_nan() const noexcept {
        uint16_t exp = (data >> 10) & 0x1Fu;
        uint16_t mant = data & 0x03FFu;
        return (exp == 31) && (mant != 0);
    }
    bool is_inf() const noexcept {
        return ((data & 0x7C00) == 0x7C00) && ((data & 0x03FF) == 0);
    }
    bool is_positive_inf() const noexcept { return data == 0x7C00; }
    bool is_negative_inf() const noexcept { return data == 0xFC00; }
    bool is_finite() const noexcept { return (data & 0x7C00) != 0x7C00; }
    bool is_normal() const noexcept {
        uint16_t exp = (data >> 10) & 0x1Fu;
        return (exp >= 1) && (exp <= 30);
    }
    bool is_positive() const noexcept { return (data & 0x7FFF) != 0 && (data & 0x8000) == 0; }
    bool is_negative() const noexcept { return (data & 0x8000) != 0; }

    // ============================================================================
    // Bitwise operations
    // ============================================================================
    storage_type bits() const noexcept { return data; }
    static half from_bits(storage_type bits) noexcept { return half(bits); }
    int sign_bit() const noexcept { return (data >> 15) & 0x1; }
    int exponent() const noexcept { return (data >> 10) & 0x1F; }
    int mantissa() const noexcept { return data & 0x03FF; }

    // ============================================================================
    // Utilities
    // ============================================================================
    bool is_valid() const noexcept { return is_finite() || is_inf() || is_nan(); }
    std::string to_string() const { return std::to_string(float(*this)); }

    // ============================================================================
    // Static constants
    // ============================================================================
    static half infinity() noexcept { return half(std::numeric_limits<float>::infinity()); }
    static half negative_infinity() noexcept { return half(-std::numeric_limits<float>::infinity()); }
    static half quiet_nan() noexcept { return half(std::numeric_limits<float>::quiet_NaN()); }
    static half signaling_nan() noexcept { return from_bits(0x7D00); }
    static half max_value() noexcept { return half(0x7BFF); }
    static half min_value() noexcept { return half(0x0400); }
    static half min_denormal_value() noexcept { return from_bits(0x0001); }
    static half epsilon() noexcept { return half(0x1400); }
    static half lowest() noexcept { return half(0xFBFF); }

public:
    storage_type data;

private:
    // ============================================================================
    // Internal conversion implementations
    // ============================================================================
    static storage_type float_to_half(float f) noexcept
    {
        uint32_t bits;
        std::memcpy(&bits, &f, sizeof(bits));

        const uint32_t sign = bits & 0x80000000u;
        int32_t e = static_cast<int32_t>((bits >> 23) & 0xFFu);   // biased exponent
        uint32_t m = bits & 0x007FFFFFu;                          // 23-bit mantissa

        // NaN / Inf
        if (e == 255) {
            if (m == 0) {
                return static_cast<storage_type>(sign ? 0xFC00u : 0x7C00u);
            }
            else {
                uint16_t nan_mant = static_cast<uint16_t>((m >> 13) & 0x03FFu);
                if (nan_mant == 0) nan_mant = 1;
                return static_cast<storage_type>(0x7C00u | nan_mant);
            }
        }

        // Determine effective mantissa and exponent
        uint32_t mant24;   // 24-bit mantissa with explicit leading bit
        int32_t exp;       // unbiased exponent

        if (e == 0) {
            // Subnormal float: exponent = -126, mantissa without leading 1
            if (m == 0) {
                // Signed zero
                return static_cast<storage_type>(sign >> 16);
            }
            exp = -126;
            mant24 = m;   // no hidden bit
        }
        else {
            // Normal float: add hidden leading 1, exponent = e - 127
            exp = e - 127;
            mant24 = m | 0x800000u;
        }

        // Target half exponent (biased by 15)
        int32_t h_exp = exp + 15;

        // Overflow to infinity?
        if (h_exp >= 31) {
            return static_cast<storage_type>((sign >> 16) | 0x7C00u);
        }

        // Subnormal (or zero) in half: h_exp <= 0
        if (h_exp <= 0) {
            // We need to shift mant24 right by (1 - h_exp) positions,
            // because half subnormals have effective exponent -14.
            // Actually half subnormal exponent is -14, and mantissa is shifted right by (14 - exp).
            // Derivation: effective half exponent = -14 + (number of leading zeros in 10-bit mantissa).
            // Equivalently, shift = 14 - exp.
            int32_t shift = 14 - exp;   // shift >= 14 for exp <= 0
            if (shift >= 25) {
                // Too small, becomes signed zero
                return static_cast<storage_type>(sign >> 16);
            }
            uint32_t mant10 = mant24 >> shift;
            uint32_t remainder = mant24 & ((1u << shift) - 1u);
            uint32_t half_bit = 1u << (shift - 1u);

            // Round to nearest even
            if ((remainder > half_bit) ||
                (remainder == half_bit && (mant10 & 1u)))
            {
                ++mant10;
            }

            // If rounding overflows into normal range
            if (mant10 >= 1024u) {
                return static_cast<storage_type>((sign >> 16) | (1u << 10)); // exp=1, mant=0
            }
            return static_cast<storage_type>((sign >> 16) | mant10);
        }

        // Normal half: 1 <= h_exp <= 30
        {
            // mant24 >> 13 gives bits [23:13] -> 11 bits, including hidden 1 at bit 10.
            // We need only the lower 10 bits for the mantissa field.
            uint32_t mant10 = (mant24 >> 13) & 0x3FFu;
            uint32_t remainder = mant24 & 0x1FFFu;          // bits 0..12
            const uint32_t half_bit = 0x1000u;              // bit 12

            // Round to nearest even
            if ((remainder > half_bit) ||
                (remainder == half_bit && (mant10 & 1u)))
            {
                ++mant10;
            }

            uint32_t final_exp = static_cast<uint32_t>(h_exp);
            if (mant10 >= 1024u) {   // rounding carried into hidden bit
                mant10 = 0;
                ++final_exp;
                if (final_exp > 30u) {
                    return static_cast<storage_type>((sign >> 16) | 0x7C00u); // became inf
                }
            }
            return static_cast<storage_type>(
                (sign >> 16) | (final_exp << 10) | mant10);
        }
    }

    static float half_to_float(storage_type h) noexcept {
        uint32_t sign = (h & 0x8000u) << 16;
        uint32_t exp_mant = h & 0x7FFFu;
        uint32_t exp = (h >> 10) & 0x1Fu;

        // Infinity or NaN
        if (exp == 31) {
            if ((exp_mant & 0x03FFu) == 0) {          // Infinity
                uint32_t bits = sign | 0x7F800000u;
                float f;
                std::memcpy(&f, &bits, 4);
                return f;
            }
            else {                                   // NaN
                uint32_t mantissa_half = exp_mant & 0x03FFu;
                uint32_t mantissa_float = mantissa_half << 13;
                uint32_t bits = sign | 0x7F800000u | mantissa_float;
                float f;
                std::memcpy(&f, &bits, 4);
                return f;
            }
        }

        // Normalized numbers
        if (exp != 0) {
            uint32_t normal = ((uint32_t)(exp_mant + 0x1C000u)) << 13;
            uint32_t result = sign | normal;
            float f;
            std::memcpy(&f, &result, 4);
            return f;
        }

        // Zero or denormalized
        if (exp_mant == 0) {
            return (sign != 0) ? -0.0f : 0.0f;
        }

        // Denormalized (existing method is fine)
        float denorm_f = static_cast<float>(exp_mant) * (1.0f / 16777216.0f);
        uint32_t denorm;
        std::memcpy(&denorm, &denorm_f, 4);
        uint32_t result = sign | denorm;
        float f;
        std::memcpy(&f, &result, 4);
        return f;
    }

    static half multiply_direct(half a, half b) noexcept {
        return half(float(a) * float(b));
    }
};

// ============================================================================
// GLOBAL FUNCTIONS (HLSL-style)
// ============================================================================

// ============================================================================
// Basic mathematical functions
// ============================================================================
inline half abs(half x) noexcept { return half::from_bits(x.bits() & 0x7FFFu); }
inline half reciprocal(half x) noexcept { return half(1.0f / float(x)); }

// ============================================================================
// Trigonometric functions
// ============================================================================
inline half sin(half x) noexcept { return half(std::sin(float(x))); }
inline half cos(half x) noexcept { return half(std::cos(float(x))); }
inline half tan(half x) noexcept { return half(std::tan(float(x))); }
inline half asin(half x) noexcept { return half(std::asin(float(x))); }
inline half acos(half x) noexcept { return half(std::acos(float(x))); }
inline half atan(half x) noexcept { return half(std::atan(float(x))); }
inline half atan2(half y, half x) noexcept { return half(std::atan2(float(y), float(x))); }

// ============================================================================
// Exponential and logarithmic functions
// ============================================================================
inline half exp(half x) noexcept { return half(std::exp(float(x))); }
inline half exp2(half x) noexcept { return half(std::exp2(float(x))); }
inline half log(half x) noexcept { return half(std::log(float(x))); }
inline half log2(half x) noexcept { return half(std::log2(float(x))); }
inline half log10(half x) noexcept { return half(std::log10(float(x))); }
inline half pow(half x, half y) noexcept { return half(std::pow(float(x), float(y))); }

// ============================================================================
// Mathematical rounding functions
// ============================================================================
inline half floor(half x) noexcept { return half(std::floor(float(x))); }
inline half ceil(half x) noexcept { return half(std::ceil(float(x))); }
inline half round(half x) noexcept { return half(std::round(float(x))); }
inline half trunc(half x) noexcept { return half(std::trunc(float(x))); }
inline half frac(half x) noexcept {
    float f = float(x);
    return half(f - std::floor(f));
}
inline half fmod(half x, half y) noexcept { return half(std::fmod(float(x), float(y))); }

// ============================================================================
// Square root and inverse square root
// ============================================================================
inline half sqrt(half x) noexcept { return half(std::sqrt(float(x))); }
inline half rsqrt(half x) noexcept { return half(1.0f / std::sqrt(float(x))); }

// ============================================================================
// Special mathematical functions
// ============================================================================
inline half modf(half x, half* intpart) noexcept {
    float intpart_f;
    float result = std::modf(float(x), &intpart_f);
    *intpart = half(intpart_f);
    return half(result);
}
inline half frexp(half x, int* exponent) noexcept { return half(std::frexp(float(x), exponent)); }
inline half ldexp(half x, int exponent) noexcept { return half(std::ldexp(float(x), exponent)); }

// ============================================================================
// Comparison functions
// ============================================================================

inline bool approximately(half a, half b, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
    float adjusted_epsilon = std::max(epsilon, 1e-3f);
    return approximately(float(a), float(b), adjusted_epsilon);
}

inline bool approximately_zero(half x, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
    float adjusted_epsilon = std::max(epsilon, 1e-3f);
    return approximately_zero(float(x), adjusted_epsilon);
}

// ============================================================================
// HLSL-style functions
// ============================================================================
inline half saturate(half x) noexcept { return half(std::max(0.0f, std::min(1.0f, float(x)))); }

inline half clamp(half x, half min_val, half max_val) noexcept {
    return half(std::max(float(min_val), std::min(float(max_val), float(x))));
}

inline half lerp(half a, half b, half t) noexcept {
    return half(float(a) + (float(b) - float(a)) * float(t));
}

inline half step(half edge, half x) noexcept {
    return half(float(x) >= float(edge) ? 1.0f : 0.0f);
}

inline half smoothstep(half edge0, half edge1, half x) noexcept {
    float t = std::max(0.0f, std::min(1.0f,
        (float(x) - float(edge0)) / (float(edge1) - float(edge0))));
    return half(t * t * (3.0f - 2.0f * t));
}

inline half sign(half x) noexcept {
    float f = float(x);
    return half((f > 0.0f) ? 1.0f : ((f < 0.0f) ? -1.0f : 0.0f));
}

inline half radians(half degrees) noexcept {
    return half(float(degrees) * Constants::Constants<float>::DegToRad);
}

inline half degrees(half radians) noexcept {
    return half(float(radians) * Constants::Constants<float>::RadToDeg);
}

// ============================================================================
// Utility functions
// ============================================================================
inline half min(half a, half b) noexcept { return (a < b) ? a : b; }
inline half max(half a, half b) noexcept { return (a > b) ? a : b; }
inline half copysign(half x, half y) noexcept { return half(std::copysign(float(x), float(y))); }

// ============================================================================
// Validation functions (analogues of isfinite, isnan, etc. from math.h)
// ============================================================================
inline bool is_valid(half x) noexcept { return x.is_valid(); }
inline bool is_finite(half x) noexcept { return x.is_finite(); }
inline bool is_nan(half x) noexcept { return x.is_nan(); }
inline bool is_inf(half x) noexcept { return x.is_inf(); }
inline bool is_normal(half x) noexcept { return x.is_normal(); }

// ============================================================================
// Binary operators with other types (for compatibility)
// ============================================================================
inline half operator+(float lhs, half rhs) noexcept { return half(lhs + float(rhs)); }
inline half operator-(float lhs, half rhs) noexcept { return half(lhs - float(rhs)); }
inline half operator*(float lhs, half rhs) noexcept { return half(lhs * float(rhs)); }
inline half operator/(float lhs, half rhs) noexcept { return half(lhs / float(rhs)); }
inline half operator+(half lhs, float rhs) noexcept { return half(float(lhs) + rhs); }
inline half operator-(half lhs, float rhs) noexcept { return half(float(lhs) - rhs); }
inline half operator*(half lhs, float rhs) noexcept { return half(float(lhs) * rhs); }
inline half operator/(half lhs, float rhs) noexcept { return half(float(lhs) / rhs); }

inline half operator+(double lhs, half rhs) noexcept { return half(static_cast<float>(lhs) + float(rhs)); }
inline half operator-(double lhs, half rhs) noexcept { return half(static_cast<float>(lhs) - float(rhs)); }
inline half operator*(double lhs, half rhs) noexcept { return half(static_cast<float>(lhs) * float(rhs)); }
inline half operator/(double lhs, half rhs) noexcept { return half(static_cast<float>(lhs) / float(rhs)); }
inline half operator+(half lhs, double rhs) noexcept { return half(float(lhs) + static_cast<float>(rhs)); }
inline half operator-(half lhs, double rhs) noexcept { return half(float(lhs) - static_cast<float>(rhs)); }
inline half operator*(half lhs, double rhs) noexcept { return half(float(lhs) * static_cast<float>(rhs)); }
inline half operator/(half lhs, double rhs) noexcept { return half(float(lhs) / static_cast<float>(rhs)); }

template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
inline half operator+(T lhs, half rhs) noexcept { return half(static_cast<float>(lhs) + float(rhs)); }

template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
inline half operator-(T lhs, half rhs) noexcept { return half(static_cast<float>(lhs) - float(rhs)); }

template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
inline half operator*(T lhs, half rhs) noexcept { return half(static_cast<float>(lhs) * float(rhs)); }

template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
inline half operator/(T lhs, half rhs) noexcept { return half(static_cast<float>(lhs) / float(rhs)); }

template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
inline half operator+(half lhs, T rhs) noexcept { return half(float(lhs) + static_cast<float>(rhs)); }

template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
inline half operator-(half lhs, T rhs) noexcept { return half(float(lhs) - static_cast<float>(rhs)); }

template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
inline half operator*(half lhs, T rhs) noexcept { return half(float(lhs) * static_cast<float>(rhs)); }

template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
inline half operator/(half lhs, T rhs) noexcept { return half(float(lhs) / static_cast<float>(rhs)); }

// ============================================================================
// half constants (HLSL constants analogues)
// ============================================================================
AFTERMATH_INLINE_VAR const half half_Zero(0.0f);
AFTERMATH_INLINE_VAR const half half_One(1.0f);
AFTERMATH_INLINE_VAR const half half_Max(65504.0f);
AFTERMATH_INLINE_VAR const half half_Min(6.10352e-5f);
AFTERMATH_INLINE_VAR const half half_Epsilon(0.00097656f);
AFTERMATH_INLINE_VAR const half half_PI(Constants::FloatConstants::Pi);
AFTERMATH_INLINE_VAR const half half_TwoPI(Constants::FloatConstants::TwoPi);
AFTERMATH_INLINE_VAR const half half_HalfPI(Constants::FloatConstants::HalfPi);
AFTERMATH_INLINE_VAR const half half_QuarterPI(Constants::FloatConstants::QuarterPi);
AFTERMATH_INLINE_VAR const half half_InvPI(Constants::FloatConstants::InvPi);
AFTERMATH_INLINE_VAR const half half_InvTwoPI(Constants::FloatConstants::InvTwoPi);
AFTERMATH_INLINE_VAR const half half_DegToRad(Constants::FloatConstants::DegToRad);
AFTERMATH_INLINE_VAR const half half_RadToDeg(Constants::FloatConstants::RadToDeg);
AFTERMATH_INLINE_VAR const half half_E(Constants::FloatConstants::E);
AFTERMATH_INLINE_VAR const half half_Sqrt2(Constants::FloatConstants::Sqrt2);
AFTERMATH_INLINE_VAR const half half_Sqrt3(Constants::FloatConstants::Sqrt3);
AFTERMATH_INLINE_VAR const half half_GoldenRatio(Constants::FloatConstants::GoldenRatio);

AFTERMATH_END
