// Description: Axis-Aligned Bounding Box inline implementations
// Author: NSDeathman, DeepSeek
#pragma once

#include "AfterMathInternal.h"

AFTERMATH_BEGIN

// ============================================================================
// Constructors Implementation
// ============================================================================

inline AABB::AABB() noexcept
    : min(float3(Constants::INFINITY, Constants::INFINITY, Constants::INFINITY))
    , max(float3(-Constants::INFINITY, -Constants::INFINITY, -Constants::INFINITY))
{}

inline AABB::AABB(const float3& min, const float3& max) noexcept
    : min(min), max(max)
{}

inline AABB::AABB(const float3& point) noexcept
    : min(point), max(point)
{}

inline AABB AABB::from_center_extents(const float3& center, const float3& extents) noexcept
{
    return AABB(center - extents, center + extents);
}

inline AABB AABB::from_points(const float3* points, size_t count) noexcept
{
    if (count == 0) return AABB();

    float3 min_val = points[0];
    float3 max_val = points[0];

    for (size_t i = 1; i < count; ++i)
    {
        min_val = float3::min(min_val, points[i]);
        max_val = float3::max(max_val, points[i]);
    }

    return AABB(min_val, max_val);
}

inline AABB AABB::from_aabbs(const AABB& a, const AABB& b) noexcept
{
    return combine(a, b);
}

// ============================================================================
// Accessors and Queries Implementation
// ============================================================================

inline bool AABB::is_valid() const noexcept
{
    // Проверяем, что min <= max по всем осям
    bool min_le_max = min.x <= max.x && min.y <= max.y && min.z <= max.z;

    // Проверяем, что все компоненты конечны (не INFINITY и не NAN)
    bool min_finite = std::isfinite(min.x) && std::isfinite(min.y) && std::isfinite(min.z);
    bool max_finite = std::isfinite(max.x) && std::isfinite(max.y) && std::isfinite(max.z);

    return min_le_max && min_finite && max_finite;
}

inline bool AABB::is_empty(float epsilon) const noexcept
{
    return (max.x - min.x) <= epsilon &&
        (max.y - min.y) <= epsilon &&
        (max.z - min.z) <= epsilon;
}

inline float3 AABB::center() const noexcept
{
    return (min + max) * 0.5f;
}

inline float3 AABB::extents() const noexcept
{
    return (max - min) * 0.5f;
}

inline float3 AABB::size() const noexcept
{
    return max - min;
}

inline float AABB::surface_area() const noexcept
{
    float3 s = size();
    return 2.0f * (s.x * s.y + s.x * s.z + s.y * s.z);
}

inline float AABB::volume() const noexcept
{
    float3 s = size();
    return s.x * s.y * s.z;
}

inline float3 AABB::corner(int index) const noexcept
{
    return float3(
        (index & 1) ? max.x : min.x,
        (index & 2) ? max.y : min.y,
        (index & 4) ? max.z : min.z
    );
}

inline void AABB::get_corners(float3 corners[8]) const noexcept
{
    for (int i = 0; i < 8; ++i)
    {
        corners[i] = corner(i);
    }
}

// ============================================================================
// Containment Tests Implementation
// ============================================================================

inline bool AABB::contains(const float3& point, float epsilon) const noexcept
{
    return point.x >= min.x - epsilon && point.x <= max.x + epsilon &&
        point.y >= min.y - epsilon && point.y <= max.y + epsilon &&
        point.z >= min.z - epsilon && point.z <= max.z + epsilon;
}

inline bool AABB::contains(const AABB& other, float epsilon) const noexcept
{
    return other.min.x >= min.x - epsilon && other.max.x <= max.x + epsilon &&
        other.min.y >= min.y - epsilon && other.max.y <= max.y + epsilon &&
        other.min.z >= min.z - epsilon && other.max.z <= max.z + epsilon;
}

inline bool AABB::intersects(const AABB& other, float epsilon) const noexcept
{
    return max.x >= other.min.x - epsilon && min.x <= other.max.x + epsilon &&
        max.y >= other.min.y - epsilon && min.y <= other.max.y + epsilon &&
        max.z >= other.min.z - epsilon && min.z <= other.max.z + epsilon;
}

// ============================================================================
// Ray Intersection Implementation (Branchless)
// ============================================================================

inline bool AABB::intersect_ray(const float3& origin, const float3& direction,
    float& t_min, float& t_max) const noexcept
{
    // Вычисляем обратное направление луча
    float3 inv_direction = float3(1.0f) / direction;

    // Вычисляем пересечения для каждого измерения
    float3 t1 = (min - origin) * inv_direction;
    float3 t2 = (max - origin) * inv_direction;

    // Находим минимальные и максимальные t для каждого измерения
    float3 t_min_vec = float3::min(t1, t2);
    float3 t_max_vec = float3::max(t1, t2);

    // Находим максимальное из минимальных значений (t ближайшей точки входа)
    t_min = max_component(t_min_vec);

    // Находим минимальное из максимальных значений (t дальней точки выхода)
    t_max = min_component(t_max_vec);

    // Если луч начинается внутри AABB, t_min может быть отрицательным
    // В этом случае точка входа - это начало луча (t = 0)
    if (t_min < 0.0f && contains(origin)) {
        t_min = 0.0f;
    }

    // Проверяем, пересекает ли луч AABB
    // Условие: t_max >= max(0, t_min) И t_max > 0
    return t_max >= std::max(0.0f, t_min) && t_max > 0.0f;
}

inline bool AABB::intersect_ray(const float3& origin, const float3& direction) const noexcept
{
    float t_min, t_max;
    return intersect_ray(origin, direction, t_min, t_max);
}

inline bool AABB::intersect_ray(const float3& origin, const float3& direction,
    float& t_min, float& t_max, float3& normal) const noexcept
{
    if (!intersect_ray(origin, direction, t_min, t_max))
        return false;

    // Compute entry point and normal
    float3 entry = origin + direction * t_min;
    normal = float3::zero();

    // Determine which face was hit
    if (approximately(entry.x, min.x, EPSILON))
        normal.x = -1.0f;
    else if (approximately(entry.x, max.x, EPSILON))
        normal.x = 1.0f;
    else if (approximately(entry.y, min.y, EPSILON))
        normal.y = -1.0f;
    else if (approximately(entry.y, max.y, EPSILON))
        normal.y = 1.0f;
    else if (approximately(entry.z, min.z, EPSILON))
        normal.z = -1.0f;
    else if (approximately(entry.z, max.z, EPSILON))
        normal.z = 1.0f;

    return true;
}

// ============================================================================
// Transformation Operations Implementation
// ============================================================================

inline AABB& AABB::expand(const float3& point) noexcept
{
    min = float3::min(min, point);
    max = float3::max(max, point);
    return *this;
}

inline AABB& AABB::expand(const AABB& other) noexcept
{
    min = float3::min(min, other.min);
    max = float3::max(max, other.max);
    return *this;
}

inline AABB& AABB::expand(float delta) noexcept
{
    float3 d(delta, delta, delta);
    min -= d;
    max += d;
    return *this;
}

inline AABB AABB::translate(const float3& translation) const noexcept
{
    return AABB(min + translation, max + translation);
}

inline AABB AABB::scale(const float3& scale) const noexcept
{
    float3 center = this->center();
    float3 extents = this->extents();
    float3 new_extents = extents * scale;
    return from_center_extents(center, new_extents);
}

inline AABB AABB::transform(const float4x4& matrix) const noexcept
{
    // Transform all 8 corners and compute new AABB
    float3 corners[8];
    get_corners(corners);

    float3 new_min = float3(Constants::INFINITY);
    float3 new_max = float3(-Constants::INFINITY);

    for (int i = 0; i < 8; ++i)
    {
        float3 transformed = matrix.transform_point(corners[i]);
        new_min = float3::min(new_min, transformed);
        new_max = float3::max(new_max, transformed);
    }

    return AABB(new_min, new_max);
}

// ============================================================================
// Static Operations Implementation
// ============================================================================

inline AABB AABB::combine(const AABB& a, const AABB& b) noexcept
{
    if (!a.is_valid()) return b;
    if (!b.is_valid()) return a;

    return AABB(float3::min(a.min, b.min),
        float3::max(a.max, b.max));
}

inline AABB AABB::intersect(const AABB& a, const AABB& b) noexcept
{
    if (!a.intersects(b)) return AABB();

    return AABB(float3::max(a.min, b.min),
        float3::min(a.max, b.max));
}

// ============================================================================
// Utility Methods Implementation
// ============================================================================

inline int AABB::longest_axis() const noexcept
{
    float3 s = size();
    if (s.x > s.y && s.x > s.z) return 0;
    if (s.y > s.z) return 1;
    return 2;
}

inline float AABB::max_extent() const noexcept
{
    float3 s = size();
    return max_component(s);
}

inline float3 AABB::normalize_coords(const float3& point) const noexcept
{
    float3 s = size();
    float3 result = (point - min) / s;
    return clamp(result, float3::zero(), float3::one());
}

inline float3 AABB::denormalize_coords(const float3& coords) const noexcept
{
    return min + (coords * size());
}

inline float3 AABB::clamp_point(const float3& point) const noexcept
{
    return clamp(point, min, max);
}

inline float AABB::distance_sq(const float3& point) const noexcept
{
    float3 clamped = clamp_point(point);
    return (point - clamped).length_sq();
}

inline float AABB::distance(const float3& point) const noexcept
{
    return ::sqrt(distance_sq(point));
}

// ============================================================================
// Comparison Operators Implementation
// ============================================================================

inline bool AABB::operator==(const AABB& other) const noexcept
{
    return min == other.min && max == other.max;
}

inline bool AABB::operator!=(const AABB& other) const noexcept
{
    return !(*this == other);
}

// ============================================================================
// String Representation Implementation
// ============================================================================

inline std::string AABB::to_string() const
{
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "AABB[min=%s, max=%s]",
        min.to_string().c_str(), max.to_string().c_str());
    return std::string(buffer);
}

// ============================================================================
// Global Functions Implementation
// ============================================================================

inline AABB combine(const AABB& a, const AABB& b) noexcept
{
    return AABB::combine(a, b);
}

inline AABB intersect(const AABB& a, const AABB& b) noexcept
{
    return AABB::intersect(a, b);
}

inline bool intersects(const AABB& a, const AABB& b, float epsilon) noexcept
{
    return a.intersects(b, epsilon);
}

inline float surface_area(const AABB& aabb) noexcept
{
    return aabb.surface_area();
}

inline float volume(const AABB& aabb) noexcept
{
    return aabb.volume();
}

inline bool contains(const AABB& container, const float3& point, float epsilon) noexcept
{
    return container.contains(point, epsilon);
}

inline bool contains(const AABB& container, const AABB& other, float epsilon) noexcept
{
    return container.contains(other, epsilon);
}

// ============================================================================
// Global Constants Implementation
// ============================================================================

inline const AABB AABB_Empty = AABB();
inline const AABB AABB_Infinite = AABB(
    float3(-Constants::INFINITY),
    float3(Constants::INFINITY)
);

AFTERMATH_END
