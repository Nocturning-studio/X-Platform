////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <type_traits>
#include <cstdint>
////////////////////////////////////////////////////////////////////////////////
#if defined(XRHI_USE_XR_MATH)
#include <xrMath/xrMath.h>
#define XRHI_MATH_TYPE fvec4
#elif defined(XRHI_MATH_PROVIDER_HEADER)
#include XRHI_MATH_PROVIDER_HEADER
#if !defined(XRHI_MATH_TYPE)
#error "XRHI_MATH_PROVIDER_HEADER must define XRHI_MATH_TYPE"
#endif
#else
namespace XRHI {
    namespace math
    {
        struct alignas(16) Float4
        {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            float w = 0.0f;

            constexpr bool operator==(const Float4& o) const noexcept
            {
                return x == o.x && y == o.y && z == o.z && w == o.w;
            }
            constexpr bool operator!=(const Float4& o) const noexcept { return !(*this == o); }
        };

        static_assert(sizeof(Float4) == 16, "XRHI::math::Float4 must be 16 bytes");
        static_assert(alignof(Float4) == 16, "XRHI::math::Float4 must be 16-byte aligned");
        static_assert(std::is_standard_layout<Float4>::value, "XRHI::math::Float4 must be standard-layout");
        static_assert(std::is_trivially_copyable<Float4>::value, "XRHI::math::Float4 must be trivially copyable");
    }
} // namespace XRHI::math
#define XRHI_MATH_TYPE ::XRHI::math::Float4
#endif
////////////////////////////////////////////////////////////////////////////////
using Float4 = XRHI_MATH_TYPE;
////////////////////////////////////////////////////////////////////////////////
