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

#include "AfterMathInternal.h"
#include "math_float3x3.h"
#include "math_float4x4.h"
#include "math_quaternion.h"

AFTERMATH_BEGIN

// ============================================================================
// float3x3 <-> float4x4
// ============================================================================

inline float3x3::float3x3(const float4x4& m) noexcept
    : row0(m.row0.x, m.row0.y, m.row0.z)
    , row1(m.row1.x, m.row1.y, m.row1.z)
    , row2(m.row2.x, m.row2.y, m.row2.z) {}

inline float3x3& float3x3::operator=(const float4x4& m) noexcept {
    row0 = float3(m.row0.x, m.row0.y, m.row0.z);
    row1 = float3(m.row1.x, m.row1.y, m.row1.z);
    row2 = float3(m.row2.x, m.row2.y, m.row2.z);
    return *this;
}

inline float4x4::float4x4(const float3x3& m) noexcept
    : row0(float4(m.row0.x, m.row0.y, m.row0.z, 0.0f))
    , row1(float4(m.row1.x, m.row1.y, m.row1.z, 0.0f))
    , row2(float4(m.row2.x, m.row2.y, m.row2.z, 0.0f))
    , row3(float4(0.0f, 0.0f, 0.0f, 1.0f)) {}

inline float4x4& float4x4::operator=(const float3x3& m) noexcept {
    row0 = float4(m.row0.x, m.row0.y, m.row0.z, 0.0f);
    row1 = float4(m.row1.x, m.row1.y, m.row1.z, 0.0f);
    row2 = float4(m.row2.x, m.row2.y, m.row2.z, 0.0f);
    row3 = float4(0.0f, 0.0f, 0.0f, 1.0f);
    return *this;
}

// ============================================================================
// float3x3 <-> quaternion
// ============================================================================

inline float3x3::float3x3(const quaternion& q) noexcept {
    *this = quaternion_to_matrix3x3(q);
}

inline float3x3& float3x3::operator=(const quaternion& q) noexcept {
    *this = quaternion_to_matrix3x3(q);
    return *this;
}

inline quaternion::quaternion(const float3x3& m) noexcept
    : quaternion(quaternion_from_matrix(m)) {}

inline quaternion& quaternion::operator=(const float3x3& m) noexcept {
    *this = quaternion_from_matrix(m);
    return *this;
}

// ============================================================================
// float4x4 <-> quaternion
// ============================================================================

inline float4x4::float4x4(const quaternion& q) noexcept {
    *this = quaternion_to_matrix4x4(q);
}

inline float4x4& float4x4::operator=(const quaternion& q) noexcept {
    *this = quaternion_to_matrix4x4(q);
    return *this;
}

inline quaternion::quaternion(const float4x4& m) noexcept
    : quaternion(quaternion_from_matrix(m)) {}

inline quaternion& quaternion::operator=(const float4x4& m) noexcept {
    *this = quaternion_from_matrix(m);
    return *this;
}

AFTERMATH_END
