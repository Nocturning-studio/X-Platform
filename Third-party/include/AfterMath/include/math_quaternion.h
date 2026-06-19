// math_quaternion.h
// Description: Quaternion class with HLSL-like syntax and global functions
// Author: NSDeathman, DeepSeek

#pragma once

#include <cmath>
#include <string>
#include <cstdio>
#include <algorithm>
#include <xmmintrin.h>
#include <pmmintrin.h>
#include <smmintrin.h>

#include "math_config.h"
#include "math_constants.h"
#include "math_functions.h"
#include "math_fast_functions.h"
#include "math_float3.h"
#include "math_float4.h"
#include "math_float3x3.h"
#include "math_float4x4.h"
#include "AfterMathInternal.h"

AFTERMATH_BEGIN

// Forward declarations
class quaternion;

extern const AfterMath::float3 float3_UnitX;
extern const AfterMath::float3 float3_UnitY;
extern const AfterMath::float3 float3_UnitZ;

inline quaternion normalize(const quaternion& q) noexcept;

// ============================================================================
// Quaternion Class (Data Only)
// ============================================================================

class quaternion
{
public:
    union {
        struct {
            float x;
            float y;
            float z;
            float w;
        };
        float4 data_;
        __m128 simd_;
    };

    // ============================================================================
    // Constructors
    // ============================================================================

    quaternion() noexcept : data_(0.0f, 0.0f, 0.0f, 1.0f) {}
    quaternion(float x, float y, float z, float w) noexcept : data_(x, y, z, w) {}
    explicit quaternion(const float4& vec) noexcept : data_(vec) {}
    explicit quaternion(__m128 simd_val) noexcept : simd_(simd_val) {}
    quaternion(const quaternion&) noexcept = default;

    // ============================================================================
    // Assignment Operators
    // ============================================================================

    quaternion& operator=(const quaternion&) noexcept = default;
    quaternion& operator=(const float4& vec) noexcept
    {
        data_ = vec;
        return *this;
    }

    // ============================================================================
    // Access Methods
    // ============================================================================

    const float* data() const noexcept { return &x; }
    float* data() noexcept { return &x; }
    const float4& get_float4() const noexcept { return data_; }
    float4& get_float4() noexcept { return data_; }
    __m128 get_simd() const noexcept { return simd_; }
    void set_simd(__m128 new_simd) noexcept { simd_ = new_simd; }

    // ============================================================================
    // Conversion Operators
    // ============================================================================

    operator float4() const noexcept { return data_; }
    operator __m128() const noexcept { return simd_; }

    // ============================================================================
    // Utility Methods
    // ============================================================================

    std::string to_string() const {
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer), "(%.3f, %.3f, %.3f, %.3f)", x, y, z, w);
        return std::string(buffer);
    }

    bool operator==(const quaternion& rhs) const noexcept { return data_ == rhs.data_; }
    bool operator!=(const quaternion& rhs) const noexcept { return data_ != rhs.data_; }
};

// ============================================================================
// Global Functions (HLSL Style)
// ============================================================================

// ============================================================================
// Constants
// ============================================================================

AFTERMATH_INLINE_VAR const quaternion quaternion_Identity = quaternion(0.0f, 0.0f, 0.0f, 1.0f);
AFTERMATH_INLINE_VAR const quaternion quaternion_Zero = quaternion(0.0f, 0.0f, 0.0f, 0.0f);
AFTERMATH_INLINE_VAR const quaternion quaternion_One = quaternion(1.0f, 1.0f, 1.0f, 1.0f);

// ============================================================================
// Static Constructors
// ============================================================================

inline quaternion identity_quaternion() noexcept {
    return quaternion(0.0f, 0.0f, 0.0f, 1.0f);
}

inline quaternion zero_quaternion() noexcept {
    return quaternion(0.0f, 0.0f, 0.0f, 0.0f);
}

inline quaternion quaternion_axis_angle(const float3& axis, float angle) noexcept
{
    float3 normalized_axis = normalize(axis);
    float half_angle = angle * 0.5f;
    float sin_half = std::sin(half_angle);
    float cos_half = std::cos(half_angle);

    return quaternion(float4(normalized_axis.x * sin_half, normalized_axis.y * sin_half, normalized_axis.z * sin_half, cos_half));
}

inline quaternion quaternion_euler(float yaw, float pitch, float roll) noexcept
{
    float half_yaw = yaw * 0.5f;
    float half_pitch = pitch * 0.5f;
    float half_roll = roll * 0.5f;

    float cy = std::cos(half_yaw);
    float sy = std::sin(half_yaw);
    float cp = std::cos(half_pitch);
    float sp = std::sin(half_pitch);
    float cr = std::cos(half_roll);
    float sr = std::sin(half_roll);

    return quaternion(
        cy * sp * cr + sy * cp * sr,
        sy * cp * cr - cy * sp * sr,
        cy * cp * sr - sy * sp * cr,
        cy * cp * cr + sy * sp * sr
    );
}

inline quaternion quaternion_euler(const float3& euler_angles) noexcept
{
    return quaternion_euler(euler_angles.x, euler_angles.y, euler_angles.z);
}

inline quaternion quaternion_from_matrix(const float3x3& m) noexcept
{
    float3x3 t = transpose(m);

    float trace = t(0, 0) + t(1, 1) + t(2, 2);
    quaternion q;

    if (trace > 0.0f) {
        float s = 0.5f / std::sqrt(trace + 1.0f);
        q.w = 0.25f / s;
        q.x = (t(2, 1) - t(1, 2)) * s;
        q.y = (t(0, 2) - t(2, 0)) * s;
        q.z = (t(1, 0) - t(0, 1)) * s;
    }
    else {
        if (t(0, 0) > t(1, 1) && t(0, 0) > t(2, 2)) {
            float s = 2.0f * std::sqrt(1.0f + t(0, 0) - t(1, 1) - t(2, 2));
            q.w = (t(2, 1) - t(1, 2)) / s;
            q.x = 0.25f * s;
            q.y = (t(0, 1) + t(1, 0)) / s;
            q.z = (t(0, 2) + t(2, 0)) / s;
        }
        else if (t(1, 1) > t(2, 2)) {
            float s = 2.0f * std::sqrt(1.0f + t(1, 1) - t(0, 0) - t(2, 2));
            q.w = (t(0, 2) - t(2, 0)) / s;
            q.x = (t(0, 1) + t(1, 0)) / s;
            q.y = 0.25f * s;
            q.z = (t(1, 2) + t(2, 1)) / s;
        }
        else {
            float s = 2.0f * std::sqrt(1.0f + t(2, 2) - t(0, 0) - t(1, 1));
            q.w = (t(1, 0) - t(0, 1)) / s;
            q.x = (t(0, 2) + t(2, 0)) / s;
            q.y = (t(1, 2) + t(2, 1)) / s;
            q.z = 0.25f * s;
        }
    }

    float len_sq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
    if (len_sq > Constants::Constants<float>::Epsilon && std::isfinite(len_sq)) {
        float inv_len = 1.0f / std::sqrt(len_sq);
        return quaternion(q.x * inv_len, q.y * inv_len, q.z * inv_len, q.w * inv_len);
    }
    return identity_quaternion();
}

inline quaternion quaternion_from_matrix(const float4x4& matrix) noexcept
{
    float3x3 rot_matrix(
        float3(matrix.row0.x, matrix.row0.y, matrix.row0.z),
        float3(matrix.row1.x, matrix.row1.y, matrix.row1.z),
        float3(matrix.row2.x, matrix.row2.y, matrix.row2.z)
    );
    return quaternion_from_matrix(rot_matrix);
}

inline quaternion quaternion_from_to_rotation(const float3& from, const float3& to) noexcept
{
    float3 v0 = normalize(from);
    float3 v1 = normalize(to);

    if (approximately_zero(v0) || approximately_zero(v1))
        return identity_quaternion();

    float cos_angle = dot(v0, v1);

    if (cos_angle > 0.9999f) return identity_quaternion();

    if (cos_angle < -0.9999f) {
        float3 axis = cross(float3_UnitX, v0);
        if (length_sq(axis) < 0.0001f) axis = cross(float3_UnitY, v0);
        axis = normalize(axis);
        return quaternion_axis_angle(axis, Constants::PI);
    }

    float3 axis = cross(v0, v1);
    float angle = std::acos(cos_angle);
    return quaternion_axis_angle(normalize(axis), angle);
}

inline quaternion quaternion_look_rotation(const float3& forward, const float3& up) noexcept
{
    float3 f = normalize(forward);
    if (length_sq(f) < 1e-6f) return identity_quaternion();

    float3 u = normalize(up);

    if (std::abs(dot(f, u)) > 0.9999f) {
        u = float3_UnitY;
        if (std::abs(dot(f, u)) > 0.9999f) u = float3_UnitZ;
    }

    float3 r = normalize(cross(u, f));
    u = normalize(cross(f, r));

    float3x3 rot_mat(r, u, f);
    return quaternion_from_matrix(rot_mat);
}

inline quaternion quaternion_rotation_x(float angle) noexcept {
    float half_angle = angle * 0.5f;
    return quaternion(std::sin(half_angle), 0.0f, 0.0f, std::cos(half_angle));
}

inline quaternion quaternion_rotation_y(float angle) noexcept {
    float half_angle = angle * 0.5f;
    return quaternion(0.0f, std::sin(half_angle), 0.0f, std::cos(half_angle));
}

inline quaternion quaternion_rotation_z(float angle) noexcept {
    float half_angle = angle * 0.5f;
    return quaternion(0.0f, 0.0f, std::sin(half_angle), std::cos(half_angle));
}

// ============================================================================
// Basic Operations
// ============================================================================

inline quaternion operator+(quaternion lhs, const quaternion& rhs) noexcept {
    lhs.data_ += rhs.data_;
    return lhs;
}

inline quaternion operator-(quaternion lhs, const quaternion& rhs) noexcept {
    lhs.data_ -= rhs.data_;
    return lhs;
}

inline quaternion operator*(quaternion q, float scalar) noexcept {
    q.data_ *= scalar;
    return q;
}

inline quaternion operator*(float scalar, quaternion q) noexcept {
    return q * scalar;
}

inline quaternion operator/(quaternion q, float scalar) noexcept {
    q.data_ /= scalar;
    return q;
}

inline quaternion& operator+=(quaternion& lhs, const quaternion& rhs) noexcept {
    lhs.data_ += rhs.data_;
    return lhs;
}

inline quaternion& operator-=(quaternion& lhs, const quaternion& rhs) noexcept {
    lhs.data_ -= rhs.data_;
    return lhs;
}

inline quaternion& operator*=(quaternion& q, float scalar) noexcept {
    q.data_ *= scalar;
    return q;
}

inline quaternion& operator/=(quaternion& q, float scalar) noexcept {
    q.data_ /= scalar;
    return q;
}

// ============================================================================
// Quaternion Multiplication
// ============================================================================

inline quaternion operator*(const quaternion& lhs, const quaternion& rhs) noexcept {
    return quaternion(
        lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x,
        lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w,
        lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z
    );
}

inline quaternion& operator*=(quaternion& lhs, const quaternion& rhs) noexcept {
    lhs = lhs * rhs;
    return lhs;
}

// ============================================================================
// Unary Operators
// ============================================================================

inline quaternion operator+(const quaternion& q) noexcept { return q; }

inline quaternion operator-(const quaternion& q) noexcept {
    return quaternion(-q.data_);
}

// ============================================================================
// Vector Transformation
// ============================================================================

inline float3 operator*(const quaternion& q, const float3& vec) noexcept {
    quaternion n = normalize(q);
    float3 q_xyz(n.x, n.y, n.z);
    float3 t = cross(q_xyz, vec) * 2.0f;
    return vec + n.w * t + cross(q_xyz, t);
}

// ============================================================================
// Mathematical Functions
// ============================================================================

inline float length(const quaternion& q) noexcept {
    return std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

inline float length_sq(const quaternion& q) noexcept {
    return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
}

inline quaternion normalize(const quaternion& q) noexcept {
    float len_sq = length_sq(q);
    if (len_sq > Constants::Constants<float>::Epsilon && std::isfinite(len_sq)) {
        float inv_len = 1.0f / std::sqrt(len_sq);
        return quaternion(q.x * inv_len, q.y * inv_len, q.z * inv_len, q.w * inv_len);
    }
    return identity_quaternion();
}

inline quaternion conjugate(const quaternion& q) noexcept {
    return quaternion(-q.x, -q.y, -q.z, q.w);
}

inline quaternion inverse(const quaternion& q) noexcept {
    float len_sq = length_sq(q);
    return (len_sq > Constants::Constants<float>::Epsilon) ? (conjugate(q) / len_sq) : identity_quaternion();
}

inline float dot(const quaternion& a, const quaternion& b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

// ============================================================================
// Interpolation Functions
// ============================================================================

inline quaternion nlerp(const quaternion& a, const quaternion& b, float t) noexcept {
    if (t <= 0.0f) return normalize(a);
    if (t >= 1.0f) return normalize(b);

    float cos_angle = dot(a, b);
    quaternion b_use = b;

    if (cos_angle < 0.0f) {
        b_use = -b;
        cos_angle = -cos_angle;
    }

    quaternion result(
        a.x * (1.0f - t) + b_use.x * t,
        a.y * (1.0f - t) + b_use.y * t,
        a.z * (1.0f - t) + b_use.z * t,
        a.w * (1.0f - t) + b_use.w * t
    );

    return normalize(result);
}

inline quaternion slerp(const quaternion& a, const quaternion& b, float t) noexcept {
    if (t <= 0.0f) return normalize(a);
    if (t >= 1.0f) return normalize(b);

    float cos_angle = dot(a, b);
    quaternion b_target = b;
    if (cos_angle < 0.0f) {
        b_target = -b;
        cos_angle = -cos_angle;
    }

    if (cos_angle > 0.9995f) return nlerp(a, b_target, t);

    float angle = std::acos(clamp(cos_angle, -1.0f, 1.0f));
    float sin_angle = std::sin(angle);
    if (std::abs(sin_angle) < 1e-6f) return nlerp(a, b_target, t);

    float inv_sin = 1.0f / sin_angle;
    float r_a = std::sin((1.0f - t) * angle) * inv_sin;
    float r_b = std::sin(t * angle) * inv_sin;
    return normalize(a * r_a + b_target * r_b);
}

inline quaternion lerp(const quaternion& a, const quaternion& b, float t) noexcept {
    return nlerp(a, b, t);
}

// ============================================================================
// Conversion Functions
// ============================================================================

inline float3x3 quaternion_to_matrix3x3(const quaternion& q) noexcept
{
    quaternion n = normalize(q);
    float xx = n.x * n.x, yy = n.y * n.y, zz = n.z * n.z;
    float xy = n.x * n.y, xz = n.x * n.z, yz = n.y * n.z;
    float wx = n.w * n.x, wy = n.w * n.y, wz = n.w * n.z;

    return float3x3(
        float3(1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy)),
        float3(2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx)),
        float3(2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy))
    );
}

inline float4x4 quaternion_to_matrix4x4(const quaternion& q) noexcept
{
    float3x3 rot = quaternion_to_matrix3x3(q);
    return float4x4(
        float4(rot.row0.x, rot.row0.y, rot.row0.z, 0.0f),
        float4(rot.row1.x, rot.row1.y, rot.row1.z, 0.0f),
        float4(rot.row2.x, rot.row2.y, rot.row2.z, 0.0f),
        float4(0.0f, 0.0f, 0.0f, 1.0f)
    );
}

inline void quaternion_to_axis_angle(const quaternion& q, float3& axis, float& angle) noexcept
{
    quaternion normalized = normalize(q);
    angle = 2.0f * std::acos(clamp(normalized.w, -1.0f, 1.0f));
    float s = std::sqrt(1.0f - normalized.w * normalized.w);
    if (s > Constants::Constants<float>::Epsilon) {
        axis = float3(normalized.x / s, normalized.y / s, normalized.z / s);
        axis = normalize(axis);
    }
    else {
        axis = float3_UnitX;
    }
}

inline float3 quaternion_to_euler(const quaternion& q) noexcept
{
    quaternion n = normalize(q);
    float3 euler;

    float sinp = 2.0f * (n.w * n.x - n.y * n.z);
    if (std::abs(sinp) >= 1.0f) {
        euler.y = std::copysign(Constants::HALF_PI, sinp);
        euler.x = 2.0f * std::atan2(n.y, n.w);
        euler.z = 0.0f;
    }
    else {
        euler.y = std::asin(sinp);
        euler.x = std::atan2(2.0f * (n.w * n.y + n.x * n.z), 1.0f - 2.0f * (n.x * n.x + n.y * n.y));
        euler.z = std::atan2(2.0f * (n.w * n.z + n.x * n.y), 1.0f - 2.0f * (n.x * n.x + n.z * n.z));
    }
    return euler;
}

// ============================================================================
// Transformation Functions
// ============================================================================

inline float3 transform_vector(const quaternion& q, const float3& vec) noexcept
{
    return q * vec;
}

inline float3 transform_direction(const quaternion& q, const float3& dir) noexcept {
    return normalize(transform_vector(q, dir));
}

// ============================================================================
// Validation Functions
// ============================================================================

inline bool is_identity(const quaternion& q, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
    return (std::abs(q.x) < epsilon) &&
        (std::abs(q.y) < epsilon) &&
        (std::abs(q.z) < epsilon) &&
        (std::abs(q.w - 1.0f) < epsilon);
}

inline bool is_normalized(const quaternion& q, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
    float len_sq = length_sq(q);
    return std::isfinite(len_sq) && approximately(len_sq, 1.0f, epsilon);
}

inline bool is_valid(const quaternion& q) noexcept {
    return std::isfinite(q.x) && std::isfinite(q.y) &&
        std::isfinite(q.z) && std::isfinite(q.w);
}

inline bool approximately_zero(const quaternion& q, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
    return std::abs(q.x) <= epsilon &&
        std::abs(q.y) <= epsilon &&
        std::abs(q.z) <= epsilon &&
        std::abs(q.w) <= epsilon;
}

inline bool approximately(const quaternion& a, const quaternion& b, float epsilon = Constants::Constants<float>::Epsilon) noexcept {
    if (approximately(a.data_, b.data_, epsilon)) return true;
    float dot_val = std::abs(dot(a, b));
    return (1.0f - dot_val) < epsilon;
}

// ============================================================================
// Fast Math Functions
// ============================================================================

inline quaternion fast_normalize(const quaternion& q) noexcept {
    float len_sq = length_sq(q);
    if (len_sq > 1e-12f && std::isfinite(len_sq)) {
        float inv_len = FastMath::fast_inv_sqrt(len_sq);
        inv_len = inv_len * (1.5f - 0.5f * len_sq * inv_len * inv_len);
        return quaternion(q.x * inv_len, q.y * inv_len, q.z * inv_len, q.w * inv_len);
    }
    return identity_quaternion();
}

// ============================================================================
// HLSL Compatibility Aliases
// ============================================================================

inline quaternion mul(const quaternion& a, const quaternion& b) noexcept {
    return a * b;
}

inline float3 mul(const quaternion& q, const float3& v) noexcept {
    return q * v;
}

inline quaternion normalize_quat(const quaternion& q) noexcept {
    return normalize(q);
}

inline quaternion conjugate_quat(const quaternion& q) noexcept {
    return conjugate(q);
}

inline quaternion inverse_quat(const quaternion& q) noexcept {
    return inverse(q);
}

inline float dot_quat(const quaternion& a, const quaternion& b) noexcept {
    return dot(a, b);
}

AFTERMATH_END
