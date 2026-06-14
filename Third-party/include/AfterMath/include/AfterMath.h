// Description: Unified include file for Math Library - comprehensive mathematics
//              library for games and scientific computing with SSE optimization
// Author: DeepSeek, NS_Deathman
#pragma once

#define AFTERMATH_VERSION 0.8f
#define AFTERMATH_VERSION_TEXT "AfterMath In-Dev v0.8"

#include "AfterMathInternal.h"

// ============================================================================
// Configuration
// ============================================================================
#include "math_config.h"

// ============================================================================
// Core Constants and Functions
// ============================================================================
#include "math_constants.h"
#include "math_functions.h"
#include "math_fast_functions.h"

// ============================================================================
// Matrix Types
// ============================================================================
#include "math_float2x2.h"
#include "math_float3x3.h"
#include "math_float4x4.h"

// ============================================================================
// Advanced Types
// ============================================================================
//#include "math_quaternion.h"
//#include "math_aabb.h"

// ============================================================================
// Vector Types
// ============================================================================
#include "math_float2.h"
#include "math_float3.h"
#include "math_float4.h"

#include "math_half.h"
#include "math_half2.h"
#include "math_half3.h"
#include "math_half4.h"

#include "math_custom_template_vector.h"
#include "math_template_vector2.h"
#include "math_template_vector3.h"
#include "math_template_vector4.h"

// ============================================================================
// Global Using Declarations for Convenience
// ============================================================================
// Constants
using AfterMath::Constants::PI;
using AfterMath::Constants::TWO_PI;
using AfterMath::Constants::HALF_PI;
using AfterMath::Constants::DEG_TO_RAD;
using AfterMath::Constants::RAD_TO_DEG;
using AfterMath::Constants::EPSILON;
using AfterMath::Constants::INFINITY;
using AfterMath::Constants::NAN;

// Math Functions
using AfterMath::approximately;
using AfterMath::approximately_zero;
using AfterMath::approximately_angle;
using AfterMath::is_finite;
using AfterMath::clamp;
using AfterMath::lerp;

// Fast Math Functions
using AfterMath::FastMath::fast_sin;
using AfterMath::FastMath::fast_cos;
using AfterMath::FastMath::fast_tan;
using AfterMath::FastMath::fast_asin;
using AfterMath::FastMath::fast_acos;
using AfterMath::FastMath::fast_atan;
using AfterMath::FastMath::fast_atan2;
using AfterMath::FastMath::fast_sqrt;
using AfterMath::FastMath::fast_inv_sqrt;
using AfterMath::FastMath::fast_exp;
using AfterMath::FastMath::fast_log;
using AfterMath::FastMath::fast_pow;

// Common Type Aliases
using half = AfterMath::half;
using half2 = AfterMath::half2;
using half3 = AfterMath::half3;
using half4 = AfterMath::half4;

using float2 = AfterMath::float2;
using float3 = AfterMath::float3;
using float4 = AfterMath::float4;

using float2x2 = AfterMath::float2x2;
using float3x3 = AfterMath::float3x3;
using float4x4 = AfterMath::float4x4;

using quaternion = AfterMath::quaternion;

using uint = unsigned int;

using int2 = AfterMath::TemplateVector2<int>;
using uint2 = AfterMath::TemplateVector2<uint>;
using double2 = AfterMath::TemplateVector2<double>;

using int3 = AfterMath::TemplateVector3<int>;
using uint3 = AfterMath::TemplateVector3<uint>;
using double3 = AfterMath::TemplateVector3<double>;

using int4 = AfterMath::TemplateVector4<int>;
using uint4 = AfterMath::TemplateVector4<unsigned int>;
using double4 = AfterMath::TemplateVector4<double>;

// ============================================================================
// Common Global Constants
// ============================================================================

// Half-precision constants
extern const AfterMath::half half_Zero;
extern const AfterMath::half half_One;
extern const AfterMath::half half_Max;
extern const AfterMath::half half_Min;
extern const AfterMath::half half_Epsilon;
extern const AfterMath::half half_PI;
extern const AfterMath::half half_TwoPI;
extern const AfterMath::half half_HalfPI;
extern const AfterMath::half half_QuarterPI;
extern const AfterMath::half half_InvPI;
extern const AfterMath::half half_InvTwoPI;
extern const AfterMath::half half_DegToRad;
extern const AfterMath::half half_RadToDeg;
extern const AfterMath::half half_Sqrt2;
extern const AfterMath::half half_E;
extern const AfterMath::half half_GoldenRatio;

// 2D vector constants
extern const AfterMath::float2 float2_Zero;
extern const AfterMath::float2 float2_One;
extern const AfterMath::float2 float2_UnitX;
extern const AfterMath::float2 float2_UnitY;
extern const AfterMath::float2 float2_Right;
extern const AfterMath::float2 float2_Left;
extern const AfterMath::float2 float2_Up;
extern const AfterMath::float2 float2_Down;

// 3D vector constants
extern const AfterMath::float3 float3_Zero;
extern const AfterMath::float3 float3_One;
extern const AfterMath::float3 float3_UnitX;
extern const AfterMath::float3 float3_UnitY;
extern const AfterMath::float3 float3_UnitZ;
extern const AfterMath::float3 float3_Forward;
extern const AfterMath::float3 float3_Up;
extern const AfterMath::float3 float3_Right;

// 4D vector constants
extern const AfterMath::float4 float4_Zero;
extern const AfterMath::float4 float4_One;

// Matrix constants
extern const AfterMath::float2x2 float2x2_Identity;
extern const AfterMath::float2x2 float2x2_Zero;
extern const AfterMath::float3x3 float3x3_Identity;
extern const AfterMath::float3x3 float3x3_Zero;
extern const AfterMath::float4x4 float4x4_Identity;
extern const AfterMath::float4x4 float4x4_Zero;
