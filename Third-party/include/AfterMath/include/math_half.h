// Description: 16-bit floating point type (half-precision) with 
//              comprehensive mathematical operations and HLSL compatibility
// Author: Nikolay Partas, NSDeathman, DeepSeek, Gemini-3
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
    // Конструкторы и базовые операции
    // ============================================================================
    half() noexcept : data(0) {}
    half(float x) noexcept { data = float_to_half_correct(x); }
    half(double x) noexcept { data = float_to_half_correct(float(x)); }
    explicit half(storage_type bits) noexcept : data(bits) {}
    half(const half&) noexcept = default;

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    half(T x) noexcept : data(float_to_half_correct(static_cast<float>(x))) {}

    // Операторы присваивания
    half& operator=(const half&) noexcept = default;
    half& operator=(float x) noexcept { data = float_to_half_correct(x); return *this; }

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    half& operator=(T x) noexcept { data = float_to_half_correct(static_cast<float>(x)); return *this; }

    // Конвертации
    explicit operator float() const noexcept { return half_to_float_correct(data); }
    explicit operator double() const noexcept { return static_cast<double>(half_to_float_correct(data)); }

    // ============================================================================
    // Арифметические операторы (остаются в классе как базовые операции)
    // ============================================================================
    half operator+(half other) const noexcept { return half(float(*this) + float(other)); }
    half operator-(half other) const noexcept { return half(float(*this) - float(other)); }
    half operator*(half other) const noexcept { return multiply_direct(*this, other); }
    half operator/(half other) const noexcept { return half(float(*this) / float(other)); }

    half& operator+=(half other) noexcept { *this = *this + other; return *this; }
    half& operator-=(half other) noexcept { *this = *this - other; return *this; }
    half& operator*=(half other) noexcept { *this = *this * other; return *this; }
    half& operator/=(half other) noexcept { *this = *this / other; return *this; }

    // Унарные операторы
    half operator+() const noexcept { return *this; }
    half operator-() const noexcept { return from_bits(data ^ 0x8000); }

    // Инкремент/декремент
    half& operator++() noexcept { *this = *this + half(1.0f); return *this; }
    half operator++(int) noexcept { half temp = *this; ++(*this); return temp; }
    half& operator--() noexcept { *this = *this - half(1.0f); return *this; }
    half operator--(int) noexcept { half temp = *this; --(*this); return temp; }

    // ============================================================================
    // Операторы сравнения (остаются в классе как базовые операции)
    // ============================================================================
    bool operator==(half other) const noexcept {
        if (is_nan() || other.is_nan()) return false;
        if (is_zero() && other.is_zero()) return true;
        return data == other.data; // Сравниваем биты напрямую для одинаковых значений
    }

    bool operator!=(half other) const noexcept { return !(*this == other); }

    bool operator<(half other) const noexcept {
        if (is_nan() || other.is_nan()) return false;
        bool sign_a = (data >> 15) != 0;
        bool sign_b = (other.data >> 15) != 0;
        if (sign_a != sign_b) return sign_a;
        uint16_t abs_a = data & 0x7FFF;
        uint16_t abs_b = other.data & 0x7FFF;
        return sign_a ? (abs_a > abs_b) : (abs_a < abs_b);
    }

    bool operator>(half other) const noexcept { return other < *this; }
    bool operator<=(half other) const noexcept { return !(other < *this); }
    bool operator>=(half other) const noexcept { return !(*this < other); }

    // ============================================================================
    // Базовые проверки и свойства (остаются в классе как фундаментальные операции)
    // ============================================================================
    bool is_zero() const noexcept { return (bits() & 0x7FFF) == 0; }
    bool is_positive_zero() const noexcept { return bits() == 0x0000; }
    bool is_negative_zero() const noexcept { return bits() == 0x8000; }
    bool is_nan() const noexcept {
        uint16_t exp = (data >> 10) & 0x1F;
        uint16_t mant = data & 0x03FF;
        return (exp == 31) && (mant != 0);
    }
    bool is_inf() const noexcept {
        return ((data & 0x7C00) == 0x7C00) && ((data & 0x03FF) == 0);
    }
    bool is_positive_inf() const noexcept { return data == 0x7C00; }
    bool is_negative_inf() const noexcept { return data == 0xFC00; }
    bool is_finite() const noexcept { return (data & 0x7C00) != 0x7C00; }
    bool is_normal() const noexcept {
        uint16_t exp = (data >> 10) & 0x1F;
        return (exp >= 1) && (exp <= 30);
    }
    bool is_positive() const noexcept { return (data & 0x7FFF) != 0 && (data & 0x8000) == 0; }
    bool is_negative() const noexcept { return (data & 0x8000) != 0; }

    // ============================================================================
    // Битвые операции (остаются в классе как фундаментальные)
    // ============================================================================
    storage_type bits() const noexcept { return data; }
    static half from_bits(storage_type bits) noexcept { return half(bits); }
    int sign_bit() const noexcept { return (data >> 15) & 0x1; }
    int exponent() const noexcept { return (data >> 10) & 0x1F; }
    int mantissa() const noexcept { return data & 0x03FF; }

    // ============================================================================
    // Утилиты (остаются минимальные)
    // ============================================================================
    bool is_valid() const noexcept { return is_finite() || is_inf() || is_nan(); }
    std::string to_string() const { return std::to_string(float(*this)); }

    // ============================================================================
    // Статические константы (остаются как базовые константы типа)
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

    // ============================================================================
    // Отладочные методы (остаются как утилиты разработчика)
    // ============================================================================
    void debug_print(const char* name) const {
        std::printf("%s: bits=0x%04X, float=%f, is_nan=%d, is_inf=%d, is_finite=%d\n",
            name, data, float(*this), is_nan(), is_inf(), is_finite());
    }

    void debug_detailed(const char* name) const {
        int exp = exponent();
        int mant = mantissa();
        int sign = sign_bit();

        std::printf("%s:\n", name);
        std::printf("  bits: 0x%04X\n", data);
        std::printf("  float: %f\n", float(*this));
        std::printf("  sign: %d, exp: %d (0x%02X), mant: %d (0x%03X)\n",
            sign, exp, exp, mant, mant);
        std::printf("  is_nan: %d (exp==31: %d, mant!=0: %d)\n",
            is_nan(), (exp == 31), (mant != 0));
        std::printf("  is_inf: %d (exp==31: %d, mant==0: %d)\n",
            is_inf(), (exp == 31), (mant == 0));
        std::printf("  is_finite: %d\n", is_finite());
        std::printf("  std::isnan: %d, std::isinf: %d\n",
            std::isnan(float(*this)), std::isinf(float(*this)));
        std::printf("---\n");
    }

    static void debug_from_bits_detailed(storage_type bits, const char* name) {
        half result(bits);
        result.debug_detailed(name);
    }

    static void run_comprehensive_debug() {
        std::cout << "=== COMPREHENSIVE HALF DEBUG ===" << std::endl;
        debug_from_bits_detailed(0x0000, "Positive Zero");
        debug_from_bits_detailed(0x8000, "Negative Zero");
        debug_from_bits_detailed(0x7C00, "Positive Infinity");
        debug_from_bits_detailed(0xFC00, "Negative Infinity");
        debug_from_bits_detailed(0x7E00, "Quiet NaN");
        debug_from_bits_detailed(0x7D00, "Signaling NaN");

        half pos_inf_f(std::numeric_limits<float>::infinity());
        half neg_inf_f(-std::numeric_limits<float>::infinity());
        half nan_f(std::numeric_limits<float>::quiet_NaN());

        pos_inf_f.debug_detailed("From Float +Inf");
        neg_inf_f.debug_detailed("From Float -Inf");
        nan_f.debug_detailed("From Float NaN");

        std::cout << "=== END COMPREHENSIVE DEBUG ===" << std::endl;
    }

public:
    storage_type data;

private:
    // ============================================================================
    // Внутренние реализации конвертации (остаются приватными)
    // ============================================================================
    static storage_type float_to_half_correct(float f) noexcept
    {
        if (std::isnan(f)) {
            uint32_t bits;
            std::memcpy(&bits, &f, 4);
            uint16_t nan_mantissa = (bits >> 13) & 0x03FF;
            if (nan_mantissa == 0) nan_mantissa = 1;
            return 0x7C00 | nan_mantissa;
        }

        if (std::isinf(f)) {
            return (f < 0) ? 0xFC00 : 0x7C00;
        }

        if (f == 0.0f) {
            uint32_t bits;
            std::memcpy(&bits, &f, 4);
            return (bits & 0x80000000) ? 0x8000 : 0x0000;
        }

        uint32_t u;
        std::memcpy(&u, &f, 4);

        uint32_t sign = u & 0x80000000;
        int32_t exp = ((u >> 23) & 0xFF) - 127;
        uint32_t mant = u & 0x007FFFFF;

        if (exp < -14) {
            if (exp < -24) return sign >> 16;
            mant |= 0x00800000;
            int32_t shift = 14 - exp;
            uint32_t round_bit = 1 << (shift - 1);
            uint32_t sticky_mask = (1 << (shift - 1)) - 1;

            if ((mant & sticky_mask) == round_bit) {
                mant += (mant >> shift) & 1;
            }
            else if ((mant & round_bit)) {
                mant += round_bit;
            }

            return static_cast<storage_type>((sign >> 16) | (mant >> shift));
        }

        exp += 15;
        uint32_t round_bit = 0x00001000;
        uint32_t sticky_mask = 0x00000FFF;

        if ((mant & sticky_mask) > round_bit) {
            mant += round_bit;
        }
        else if ((mant & sticky_mask) == round_bit) {
            mant += (mant & (round_bit << 1)) ? round_bit : 0;
        }

        if (mant & 0x00800000) {
            mant = 0;
            exp++;
        }

        if (exp > 30) {
            return static_cast<storage_type>((sign >> 16) | 0x7C00);
        }

        return static_cast<storage_type>((sign >> 16) | (exp << 10) | (mant >> 13));
    }

    static float half_to_float_correct(storage_type h) noexcept {
        uint32_t sign = (h & 0x8000) << 16;
        uint32_t exp = (h >> 10) & 0x1F;
        uint32_t mant = h & 0x03FF;

        if (exp == 0) {
            if (mant == 0) return (sign ? -0.0f : 0.0f);
            while ((mant & 0x0400) == 0) {
                mant <<= 1;
                exp--;
            }
            exp++;
            mant &= 0x03FF;
        }
        else if (exp == 31) {
            if (mant == 0) {
                uint32_t inf = sign | 0x7F800000;
                float result;
                std::memcpy(&result, &inf, 4);
                return result;
            }
            else {
                uint32_t nan = sign | 0x7FC00000 | (mant << 13);
                float result;
                std::memcpy(&result, &nan, 4);
                return result;
            }
        }

        exp += 112;
        uint32_t result = sign | (exp << 23) | (mant << 13);
        float f;
        std::memcpy(&f, &result, 4);
        return f;
    }

    static half multiply_direct(half a, half b) noexcept {
        return half(float(a) * float(b));
    }
};

// ============================================================================
// ГЛОБАЛЬНЫЕ ФУНКЦИИ (HLSL-стиль)
// ============================================================================

// ============================================================================
// Базовые математические функции
// ============================================================================
inline half abs(half x) noexcept { return half::from_bits(x.bits() & 0x7FFF); }
inline half reciprocal(half x) noexcept { return half(1.0f / float(x)); }

// ============================================================================
// Тригонометрические функции
// ============================================================================
inline half sin(half x) noexcept { return half(std::sin(float(x))); }
inline half cos(half x) noexcept { return half(std::cos(float(x))); }
inline half tan(half x) noexcept { return half(std::tan(float(x))); }
inline half asin(half x) noexcept { return half(std::asin(float(x))); }
inline half acos(half x) noexcept { return half(std::acos(float(x))); }
inline half atan(half x) noexcept { return half(std::atan(float(x))); }
inline half atan2(half y, half x) noexcept { return half(std::atan2(float(y), float(x))); }

// ============================================================================
// Экспоненциальные и логарифмические функции
// ============================================================================
inline half exp(half x) noexcept { return half(std::exp(float(x))); }
inline half exp2(half x) noexcept { return half(std::exp2(float(x))); }
inline half log(half x) noexcept { return half(std::log(float(x))); }
inline half log2(half x) noexcept { return half(std::log2(float(x))); }
inline half log10(half x) noexcept { return half(std::log10(float(x))); }
inline half pow(half x, half y) noexcept { return half(std::pow(float(x), float(y))); }

// ============================================================================
// Математические функции округления
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
// Квадратный корень и обратный квадратный корень
// ============================================================================
inline half sqrt(half x) noexcept { return half(std::sqrt(float(x))); }
inline half rsqrt(half x) noexcept { return half(1.0f / std::sqrt(float(x))); }

// ============================================================================
// Специальные математические функции
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
// HLSL-стиль функции
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
// Утилитные функции
// ============================================================================
inline half min(half a, half b) noexcept { return (a < b) ? a : b; }
inline half max(half a, half b) noexcept { return (a > b) ? a : b; }
inline half copysign(half x, half y) noexcept { return half(std::copysign(float(x), float(y))); }

// ============================================================================
// Проверочные функции (аналоги isfinite, isnan, etc. из math.h)
// ============================================================================
inline bool is_valid(half x) noexcept { return x.is_valid(); }
inline bool is_finite(half x) noexcept { return x.is_finite(); }
inline bool is_nan(half x) noexcept { return x.is_nan(); }
inline bool is_inf(half x) noexcept { return x.is_inf(); }
inline bool is_normal(half x) noexcept { return x.is_normal(); }

// ============================================================================
// Бинарные операторы с другими типами (для совместимости)
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
// Константы half (аналоги HLSL констант)
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
