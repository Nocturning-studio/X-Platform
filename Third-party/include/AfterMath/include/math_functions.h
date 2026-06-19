// Author: NSDeathman, DeepSeek
#pragma once

/**
* @file AfterMathFunctions.h
* @brief Floating-point comparison utilities and mathematical helper functions
* @note Includes advanced comparison algorithms for robust floating-point operations
*/

#include <cmath>        // std::abs, std::max, std::isfinite, etc.
#include <cstdint>      // std::int32_t
#include <algorithm>    // std::min, std::max
#include <type_traits>  // std::is_floating_point_v

#include "AfterMathInternal.h"
#include "math_constants.h"

AFTERMATH_BEGIN

    // ============================================================================
    // Basic Comparison Algorithms
    // ============================================================================

    /**
        * @brief Absolute epsilon comparison (fast but limited range)
        * @tparam T Floating-point type
        * @param a First value
        * @param b Second value
        * @param epsilon Absolute tolerance
        * @return True if |a - b| <= epsilon
        *
        * @note Best for numbers near zero or when magnitude is known
        */
    template<typename T>
    constexpr bool approximately_absolute(T a, T b, T epsilon) noexcept {
        static_assert(std::is_floating_point_v<T>, "T must be a floating-point type");
        return std::abs(a - b) <= epsilon;
    }

    /**
        * @brief Relative epsilon comparison (robust for large numbers)
        * @tparam T Floating-point type
        * @param a First value
        * @param b Second value
        * @param epsilon Relative tolerance
        * @return True if |a - b| <= epsilon * max(|a|, |b|)
        *
        * @note Handles large numbers well but fails near zero
        */
    template<typename T>
    constexpr bool approximately_relative(T a, T b, T epsilon) noexcept {
        static_assert(std::is_floating_point_v<T>, "T must be a floating-point type");

        // Early exit for exact equality
        if (a == b) return true;

        // Handle near-zero cases with absolute comparison
        if (a == T(0) || b == T(0))
            return std::abs(a - b) <= epsilon;

        // Relative comparison for non-zero values
        T magnitude = std::max(std::abs(a), std::abs(b));
        return std::abs(a - b) <= epsilon * magnitude;
    }

    /**
        * @brief Combined absolute and relative comparison (recommended)
        * @tparam T Floating-point type
        * @param a First value
        * @param b Second value
        * @param abs_epsilon Absolute tolerance for near-zero values
        * @param rel_epsilon Relative tolerance for large values
        * @return True if values are approximately equal
        *
        * @note This is the most robust general-purpose comparison method
        */
    template<typename T>
    constexpr bool approximately_combined(T a, T b, T abs_epsilon, T rel_epsilon) noexcept {
        static_assert(std::is_floating_point_v<T>, "T must be a floating-point type");

        // Absolute difference
        T diff = std::abs(a - b);

        // Absolute check for near-zero values
        if (diff <= abs_epsilon)
            return true;

        // Relative check for larger values
        T magnitude = std::max(std::abs(a), std::abs(b));
        return diff <= magnitude * rel_epsilon;
    }

    // ============================================================================
    // Primary Comparison Functions
    // ============================================================================

    /**
        * @brief Default approximate equality comparison
        * @tparam T Floating-point type
        * @param a First value
        * @param b Second value
        * @param epsilon Combined absolute/relative tolerance
        * @return True if values are approximately equal
        *
        * @note Uses combined comparison for robustness across all value ranges
        */
    inline bool approximately(float a, float b, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
        return std::abs(a - b) <= epsilon;
    }

    inline bool approximately_zero(float x, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
        return std::abs(x) <= epsilon;
    }

    // ============================================================================
    // Specialized Comparison Functions
    // ============================================================================

    /**
        * @brief ULPs (Units in Last Place) comparison
        * @param a First float value
        * @param b Second float value
        * @param max_ulps Maximum allowed ULPs difference
        * @return True if values are within specified ULPs
        *
        * @note More robust but slower. Good for critical comparisons.
        * @warning max_ulps should typically be between 1-10
        */
    inline bool approximately_ulps(float a, float b, int max_ulps = 4) noexcept {
        // Reinterpret float bits as integer for bitwise comparison
        std::int32_t int_a, int_b;
        static_assert(sizeof(float) == sizeof(std::int32_t), "float and int32_t size mismatch");

        std::memcpy(&int_a, &a, sizeof(float));
        std::memcpy(&int_b, &b, sizeof(float));

        // Handle sign bits by making negative numbers comparable
        if (int_a < 0) int_a = 0x80000000 - int_a;
        if (int_b < 0) int_b = 0x80000000 - int_b;

        return std::abs(int_a - int_b) <= max_ulps;
    }

    /**
        * @brief Angular comparison (handles periodicity)
        * @param a First angle in radians
        * @param b Second angle in radians
        * @param epsilon Angular tolerance in radians
        * @return True if angles are approximately equal modulo 2pi
        */
    inline bool approximately_angle(float a, float b, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
        float diff = std::abs(a - b);
        // Handle angular periodicity by considering the shorter arc
        diff = std::min(diff, Constants::Constants<float>::TwoPi - diff);
        return diff <= epsilon;
    }

    /**
        * @brief Color value comparison (tighter tolerance)
        * @param a First color component
        * @param b Second color component
        * @param epsilon Color tolerance (automatically scaled)
        * @return True if color components are approximately equal
        *
        * @note Uses tighter tolerance suitable for color operations
        */
    inline bool approximately_color(float a, float b, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
        // Colors typically need tighter comparison
        return approximately(a, b, epsilon * 0.1f);
    }

    // ============================================================================
    // Inequality Comparisons with Epsilon
    // ============================================================================

    /**
        * @brief Epsilon-aware greater than comparison
        * @tparam T Floating-point type
        * @param a First value
        * @param b Second value
        * @param epsilon Tolerance for equality
        * @return True if a > b + epsilon
        */
    template<typename T>
    constexpr bool greater_than(T a, T b, T epsilon = Constants<T>::Epsilon) noexcept {
        static_assert(std::is_floating_point_v<T>, "T must be a floating-point type");
        return a > b + epsilon;
    }

    /**
        * @brief Epsilon-aware less than comparison
        * @tparam T Floating-point type
        * @param a First value
        * @param b Second value
        * @param epsilon Tolerance for equality
        * @return True if a < b - epsilon
        */
    template<typename T>
    constexpr bool less_than(T a, T b, T epsilon = Constants<T>::Epsilon) noexcept {
        static_assert(std::is_floating_point_v<T>, "T must be a floating-point type");
        return a < b - epsilon;
    }

    /**
        * @brief Epsilon-aware greater than or equal comparison
        * @tparam T Floating-point type
        * @param a First value
        * @param b Second value
        * @param epsilon Tolerance for equality
        * @return True if a >= b - epsilon
        */
    template<typename T>
    constexpr bool greater_than_or_equal(T a, T b, T epsilon = Constants<T>::Epsilon) noexcept {
        static_assert(std::is_floating_point_v<T>, "T must be a floating-point type");
        return a >= b - epsilon;
    }

    /**
        * @brief Epsilon-aware less than or equal comparison
        * @tparam T Floating-point type
        * @param a First value
        * @param b Second value
        * @param epsilon Tolerance for equality
        * @return True if a <= b + epsilon
        */
    template<typename T>
    constexpr bool less_than_or_equal(T a, T b, T epsilon = Constants<T>::Epsilon) noexcept {
        static_assert(std::is_floating_point_v<T>, "T must be a floating-point type");
        return a <= b + epsilon;
    }

    // ============================================================================
    // Mathematical Helper Functions
    // ============================================================================

    /**
        * @brief Simultaneously compute sine and cosine
        * @param angle Angle in radians
        * @param[out] sin_val Pointer to store sine result
        * @param[out] cos_val Pointer to store cosine result
        *
        * @note May be more efficient than separate sin/cos calls on some platforms
        */
    inline void sin_cos(float angle, float* sin_val, float* cos_val) noexcept {
        *sin_val = std::sin(angle);
        *cos_val = std::cos(angle);
    }

    /**
        * @brief Safe inverse square root with epsilon protection
        * @param value Input value
        * @return 1/sqrt(value) or 0 if value <= epsilon
        */
    inline float safe_inverse_sqrt(float value, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
        if (value <= epsilon) return 0.0f;
        return 1.0f / std::sqrt(value);
    }

    /**
        * @brief Linear remapping from one range to another
        * @param value Input value
        * @param in_min Input range minimum
        * @param in_max Input range maximum
        * @param out_min Output range minimum
        * @param out_max Output range maximum
        * @return Value mapped to output range
        */
    template<typename T>
    inline T remap(T value, T in_min, T in_max, T out_min, T out_max) noexcept {
        return out_min + (value - in_min) * (out_max - out_min) / (in_max - in_min);
    }

    // ============================================================================
    // Validation and Debugging Utilities
    // ============================================================================

    /**
        * @brief Check if value is finite (not NaN or infinity)
        * @tparam T Floating-point type
        * @param value Value to check
        * @return True if value is finite
        */
    template<typename T>
    constexpr bool is_finite(T value) noexcept {
        static_assert(std::is_floating_point_v<T>, "T must be a floating-point type");
        return std::isfinite(value);
    }

    /**
        * @brief Check if value is NaN (Not a Number)
        * @tparam T Floating-point type
        * @param value Value to check
        * @return True if value is NaN
        */
    template<typename T>
    constexpr bool is_nan(T value) noexcept {
        static_assert(std::is_floating_point_v<T>, "T must be a floating-point type");
        return std::isnan(value);
    }

    /**
        * @brief Check if value is positive or negative infinity
        * @tparam T Floating-point type
        * @param value Value to check
        * @return True if value is infinity
        */
    template<typename T>
    constexpr bool is_infinity(T value) noexcept {
        static_assert(std::is_floating_point_v<T>, "T must be a floating-point type");
        return std::isinf(value);
    }

    /**
        * @brief Clamp value to [min, max] range
        * @tparam T Arithmetic type
        * @param value Value to clamp
        * @param min Minimum allowed value
        * @param max Maximum allowed value
        * @return Clamped value
        */
    template<typename T>
    constexpr T clamp(T value, T min, T max) noexcept {
        return (value < min) ? min : (value > max) ? max : value;
    }

    /**
        * @brief Linear interpolation between two values
        * @tparam T Floating-point type
        * @param a Start value
        * @param b End value
        * @param t Interpolation factor [0, 1]
        * @return Interpolated value
        */
    template<typename T>
    constexpr T lerp(T a, T b, T t) noexcept {
        static_assert(std::is_floating_point_v<T>, "T must be a floating-point type");
        return a + (b - a) * t;
    }

    inline float normalize_angle(float angle) {
        // Приводим угол к диапазону [-pi, pi]
        while (angle > Constants::PI) angle -= Constants::TWO_PI;
        while (angle < -Constants::PI) angle += Constants::TWO_PI;
        return angle;
    }

AFTERMATH_END
