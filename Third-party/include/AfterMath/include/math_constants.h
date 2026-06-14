// Author: NSDeathman, DeepSeek
#pragma once

/**
 * @file Constants.h
 * @brief Mathematical constants and type-specific precision values
 * @note This header is safe to include in other headers (minimal dependencies)
 */

#include <limits>  // std::numeric_limits
#include <type_traits>  // std::is_floating_point

#include "AfterMathInternal.h"

#ifdef INFINITY
#undef INFINITY
#endif

#ifdef NAN
#undef NAN
#endif

#define NOMINMAX
#undef max
#undef min

AFTERMATH_BEGIN
/**
    * @namespace Constants
    * @brief Mathematical constants and precision values for different floating-point types
    *
    * Provides type-safe access to common mathematical constants with appropriate
    * precision for float and double types. All values are constexpr for compile-time
    * evaluation.
    */
namespace Constants {

    // Forward declaration for template specialization
    template<typename T>
    struct Constants;

    /**
        * @struct Constants<float>
        * @brief Mathematical constants optimized for 32-bit floating point precision
        *
        * Provides mathematical constants with precision appropriate for float type.
        * Suitable for real-time graphics and performance-critical applications.
        */
    template<>
    struct Constants<float> {
        static_assert(std::is_floating_point_v<float>, "T must be a floating-point type");

        /// @brief pi (pi) constant - ratio of circle's circumference to its diameter
        static constexpr float Pi = 3.14159265358979323846f;

        /// @brief 2pi (tau) constant - full circle in radians
        static constexpr float TwoPi = 6.28318530717958647692f;

        /// @brief pi/2 constant - right angle in radians
        static constexpr float HalfPi = 1.57079632679489661923f;

        /// @brief 1/pi constant - reciprocal of pi
        static constexpr float InvPi = 0.31830988618379067154f;

        /// @brief 1/(2pi) constant - reciprocal of tau
        static constexpr float InvTwoPi = 0.15915494309189533577f;

        /// @brief pi/4 constant - 45 degrees in radians
        static constexpr float QuarterPi = 0.78539816339744830962f;

        /// @brief Conversion factor from degrees to radians (pi/180)
        static constexpr float DegToRad = Pi / 180.0f;

        /// @brief Conversion factor from radians to degrees (180/pi)
        static constexpr float RadToDeg = 180.0f / Pi;

        /// @brief Machine epsilon for float comparisons (1e-6f)
        static constexpr float Epsilon = 1e-6f;

        /// @brief Square root of 2
        static constexpr float Sqrt2 = 1.41421356237309504880f;

        /// @brief Square root of 3
        static constexpr float Sqrt3 = 1.73205080756887729353f;

        /// @brief Natural logarithm base (e)
        static constexpr float E = 2.71828182845904523536f;

        /// @brief Golden ratio φ (phi)
        static constexpr float GoldenRatio = 1.61803398874989484820f;

        /// @brief Positive infinity value
        static constexpr float Infinity = std::numeric_limits<float>::infinity();

        /// @brief Quiet NaN (Not-a-Number) value
        static constexpr float NaN = std::numeric_limits<float>::quiet_NaN();

        /// @brief Maximum finite value
        static constexpr float MaxValue = std::numeric_limits<float>::max();

        /// @brief Minimum positive normalized value
        static constexpr float MinValue = std::numeric_limits<float>::min();
    };

    /**
        * @struct Constants<double>
        * @brief Mathematical constants with 64-bit floating point precision
        *
        * Provides mathematical constants with higher precision for double type.
        * Suitable for scientific computing and high-precision applications.
        */
    template<>
    struct Constants<double> {
        static_assert(std::is_floating_point_v<double>, "T must be a floating-point type");

        /// @brief pi (pi) constant with double precision
        static constexpr double Pi = 3.14159265358979323846;

        /// @brief 2pi (tau) constant with double precision
        static constexpr double TwoPi = 6.28318530717958647692;

        /// @brief pi/2 constant with double precision
        static constexpr double HalfPi = 1.57079632679489661923;

        /// @brief 1/pi constant with double precision
        static constexpr double InvPi = 0.31830988618379067154;

        /// @brief 1/(2pi) constant with double precision
        static constexpr double InvTwoPi = 0.15915494309189533577;

        /// @brief pi/4 constant with double precision
        static constexpr double QuarterPi = 0.78539816339744830962;

        /// @brief Conversion factor from degrees to radians with double precision
        static constexpr double DegToRad = Pi / 180.0;

        /// @brief Conversion factor from radians to degrees with double precision
        static constexpr double RadToDeg = 180.0 / Pi;

        /// @brief Machine epsilon for double comparisons (1e-12)
        static constexpr double Epsilon = 1e-12;

        /// @brief Square root of 2 with double precision
        static constexpr double Sqrt2 = 1.41421356237309504880;

        /// @brief Square root of 3 with double precision
        static constexpr double Sqrt3 = 1.73205080756887729353;

        /// @brief Natural logarithm base (e) with double precision
        static constexpr double E = 2.71828182845904523536;

        /// @brief Golden ratio φ (phi) with double precision
        static constexpr double GoldenRatio = 1.61803398874989484820;

        /// @brief Positive infinity value for double
        static constexpr double Infinity = std::numeric_limits<double>::infinity();

        /// @brief Quiet NaN (Not-a-Number) value for double
        static constexpr double NaN = std::numeric_limits<double>::quiet_NaN();

        /// @brief Maximum finite value for double
        static constexpr double MaxValue = std::numeric_limits<double>::max();

        /// @brief Minimum positive normalized value for double
        static constexpr double MinValue = std::numeric_limits<double>::min();
    };

    // ============================================================================
    // Type Aliases for Common Use Cases
    // ============================================================================

    /// @brief Alias for float constants (most common use case)
    using FloatConstants = Constants<float>;

    /// @brief Alias for double constants (high precision)
    using DoubleConstants = Constants<double>;

    // ============================================================================
    // Convenience Constants (Float Precision)
    // ============================================================================

    /// @brief pi constant with float precision
    constexpr float PI = FloatConstants::Pi;

    /// @brief 2pi constant with float precision
    constexpr float TWO_PI = FloatConstants::TwoPi;

    /// @brief pi/2 constant with float precision  
    constexpr float HALF_PI = FloatConstants::HalfPi;

    /// @brief pi/4 constant with float precision
    constexpr float QUARTER_PI = FloatConstants::QuarterPi;

    /// @brief 1/pi constant with float precision
    constexpr float INV_PI = FloatConstants::InvPi;

    /// @brief 1/(2pi) constant with float precision
    constexpr float INV_TWO_PI = FloatConstants::InvTwoPi;

    /// @brief Degrees to radians conversion factor
    constexpr float DEG_TO_RAD = FloatConstants::DegToRad;

    /// @brief Radians to degrees conversion factor
    constexpr float RAD_TO_DEG = FloatConstants::RadToDeg;

    /// @brief Default epsilon for float comparisons
    constexpr float EPSILON = FloatConstants::Epsilon;

    /// @brief Square root of 2 with float precision
    constexpr float SQRT2 = FloatConstants::Sqrt2;

    /// @brief Square root of 3 with float precision
    constexpr float SQRT3 = FloatConstants::Sqrt3;

    /// @brief Natural logarithm base with float precision
    constexpr float E = FloatConstants::E;

    /// @brief Golden ratio with float precision
    constexpr float GOLDEN_RATIO = FloatConstants::GoldenRatio;

    /// @brief Positive infinity for float
    constexpr float INFINITY = FloatConstants::Infinity;

    /// @brief Quiet NaN for float
    constexpr float NAN = FloatConstants::NaN;

    /// @brief Maximum finite float value
    constexpr float MAX_FLOAT = FloatConstants::MaxValue;

    /// @brief Minimum positive normalized float value
    constexpr float MIN_FLOAT = FloatConstants::MinValue;

    // ============================================================================
    // Utility Functions
    // ============================================================================

    /**
        * @brief Checks if a value is approximately zero
        * @tparam T Floating point type
        * @param value Value to check
        * @param epsilon Comparison tolerance
        * @return True if value is within epsilon of zero
        */
    template<typename T>
    constexpr bool is_zero(T value, T epsilon = Constants<T>::Epsilon) noexcept {
        static_assert(std::is_floating_point_v<T>, "T must be a floating-point type");
        return std::abs(value) <= epsilon;
    }

    /**
        * @brief Checks if two values are approximately equal
        * @tparam T Floating point type
        * @param a First value
        * @param b Second value
        * @param epsilon Comparison tolerance
        * @return True if values are within epsilon of each other
        */
    template<typename T>
    constexpr bool approximately_equal(T a, T b, T epsilon = Constants<T>::Epsilon) noexcept {
        static_assert(std::is_floating_point_v<T>, "T must be a floating-point type");
        return std::abs(a - b) <= epsilon * std::max(T(1), std::max(std::abs(a), std::abs(b)));
    }

    /**
        * @brief Converts degrees to radians
        * @tparam T Floating point type
        * @param degrees Angle in degrees
        * @return Angle in radians
        */
    template<typename T>
    constexpr T degrees_to_radians(T degrees) noexcept {
        static_assert(std::is_floating_point_v<T>, "T must be a floating-point type");
        return degrees * Constants<T>::DegToRad;
    }

    /**
        * @brief Converts radians to degrees
        * @tparam T Floating point type
        * @param radians Angle in radians
        * @return Angle in degrees
        */
    template<typename T>
    constexpr T radians_to_degrees(T radians) noexcept {
        static_assert(std::is_floating_point_v<T>, "T must be a floating-point type");
        return radians * Constants<T>::RadToDeg;
    }

} // namespace Constants

// ============================================================================
// Global Using Declarations for Convenience
// ============================================================================

/// @brief Bring commonly used constants into MATH namespace
using Constants::PI;
using Constants::TWO_PI;
using Constants::HALF_PI;
using Constants::DEG_TO_RAD;
using Constants::RAD_TO_DEG;
using Constants::EPSILON;
using Constants::INFINITY;

/// @brief Bring utility functions into MATH namespace
using Constants::is_zero;
using Constants::approximately_equal;
using Constants::degrees_to_radians;
using Constants::radians_to_degrees;

AFTERMATH_END