// Description: Axis-Aligned Bounding Box (AABB) for collision detection
//              and spatial partitioning
// Author: NSDeathman, DeepSeek
#pragma once

#include "AfterMath.h"
#include "AfterMathInternal.h"

#include <algorithm>

AFTERMATH_BEGIN
/**
    * @class AABB
    * @brief Axis-Aligned Bounding Box for spatial queries and collision detection
    *
    * Represents an axis-aligned bounding box defined by minimum and maximum corners.
    * Provides comprehensive functionality for intersection tests, volume operations,
    * and geometric transformations. Optimized for ray tracing, physics, and spatial
    * partitioning algorithms.
    *
    * @note Memory layout: 24 bytes (two float3)
    * @note All operations are branch-optimized and SIMD-friendly
    * @note Corner indices follow binary pattern: 0=min, 7=max
    */
class AABB
{
public:
    // ============================================================================
    // Data Members
    // ============================================================================

    float3 min; ///< Minimum corner of the bounding box
    float3 max; ///< Maximum corner of the bounding box

    // ============================================================================
    // Constructors
    // ============================================================================

    /**
        * @brief Default constructor (invalid AABB)
        * @note Creates an invalid AABB that can be expanded
        */
    AABB() noexcept;

    /**
        * @brief Construct from minimum and maximum corners
        * @param min Minimum corner point
        * @param max Maximum corner point
        */
    AABB(const float3& min, const float3& max) noexcept;

    /**
        * @brief Construct from single point (zero-volume AABB)
        * @param point Single point to initialize both corners
        */
    explicit AABB(const float3& point) noexcept;

    /**
        * @brief Construct from center and extents (half sizes)
        * @param center Center point of AABB
        * @param extents Half extents from center to sides
        */
    static AABB from_center_extents(const float3& center, const float3& extents) noexcept;

    /**
        * @brief Construct AABB from array of points
        * @param points Array of points
        * @param count Number of points
        * @return AABB containing all points
        */
    static AABB from_points(const float3* points, size_t count) noexcept;

    /**
        * @brief Construct AABB from two AABBs (union)
        * @param a First AABB
        * @param b Second AABB
        * @return Union AABB
        */
    static AABB from_aabbs(const AABB& a, const AABB& b) noexcept;

    // ============================================================================
    // Accessors and Queries
    // ============================================================================

    /**
        * @brief Check if AABB is valid (min <= max)
        * @return True if AABB is valid
        */
    bool is_valid() const noexcept;

    /**
        * @brief Check if AABB has zero volume
        * @param epsilon Tolerance for comparison
        * @return True if volume is approximately zero
        */
    bool is_empty(float epsilon = EPSILON) const noexcept;

    /**
        * @brief Get center point of AABB
        * @return Center point
        */
    float3 center() const noexcept;

    /**
        * @brief Get extents (half sizes) from center to sides
        * @return Extents vector
        */
    float3 extents() const noexcept;

    /**
        * @brief Get full size of AABB
        * @return Size vector (max - min)
        */
    float3 size() const noexcept;

    /**
        * @brief Get surface area of AABB
        * @return Surface area
        */
    float surface_area() const noexcept;

    /**
        * @brief Get volume of AABB
        * @return Volume
        */
    float volume() const noexcept;

    /**
        * @brief Get corner point by index (0-7)
        * @param index Corner index (bits: 0=x, 1=y, 2=z, where 0=min, 1=max)
        * @return Corner point
        * @note Corner index follows binary pattern: 0=min, 7=max
        */
    float3 corner(int index) const noexcept;

    /**
        * @brief Get all 8 corners of AABB
        * @param corners[8] Output array for corners
        */
    void get_corners(float3 corners[8]) const noexcept;

    // ============================================================================
    // Containment Tests
    // ============================================================================

    /**
        * @brief Check if point is inside AABB
        * @param point Point to test
        * @param epsilon Tolerance for boundary
        * @return True if point is inside or on boundary
        */
    bool contains(const float3& point, float epsilon = 0.0f) const noexcept;

    /**
        * @brief Check if AABB is fully contained within this AABB
        * @param other Other AABB
        * @param epsilon Tolerance for boundary
        * @return True if other is fully contained
        */
    bool contains(const AABB& other, float epsilon = 0.0f) const noexcept;

    /**
        * @brief Check if AABB intersects another AABB
        * @param other Other AABB
        * @param epsilon Tolerance for boundary
        * @return True if AABBs intersect (including touching)
        */
    bool intersects(const AABB& other, float epsilon = EPSILON) const noexcept;

    // ============================================================================
    // Ray Intersection (Fast Methods)
    // ============================================================================

    /**
        * @brief Fast ray-AABB intersection test (branchless)
        * @param origin Ray origin
        * @param direction Ray direction (must be normalized)
        * @param inv_direction Precomputed 1.0 / direction
        * @param t_min[out] Minimum intersection distance
        * @param t_max[out] Maximum intersection distance
        * @return True if ray hits AABB
        * @note Uses branchless slab method (Kay-Kajiya/Williams)
        */
    bool intersect_ray(const float3& origin, const float3& direction,
        float& t_min, float& t_max) const noexcept;

    /**
        * @brief Simplified ray-AABB intersection test
        * @param origin Ray origin
        * @param direction Ray direction
        * @return True if ray hits AABB
        */
    bool intersect_ray(const float3& origin, const float3& direction) const noexcept;

    /**
        * @brief Ray-AABB intersection with detailed information
        * @param origin Ray origin
        * @param direction Ray direction
        * @param t_min[out] Minimum intersection distance
        * @param t_max[out] Maximum intersection distance
        * @param normal[out] Normal at entry point
        * @return True if ray hits AABB
        */
    bool intersect_ray(const float3& origin, const float3& direction,
        float& t_min, float& t_max, float3& normal) const noexcept;

    // ============================================================================
    // Transformation Operations
    // ============================================================================

    /**
        * @brief Expand AABB to include point
        * @param point Point to include
        * @return Reference to this AABB
        */
    AABB& expand(const float3& point) noexcept;

    /**
        * @brief Expand AABB to include another AABB
        * @param other Other AABB to include
        * @return Reference to this AABB
        */
    AABB& expand(const AABB& other) noexcept;

    /**
        * @brief Expand AABB uniformly in all directions
        * @param delta Expansion amount (added to all sides)
        * @return Reference to this AABB
        */
    AABB& expand(float delta) noexcept;

    /**
        * @brief Translate AABB by vector
        * @param translation Translation vector
        * @return Translated AABB
        */
    AABB translate(const float3& translation) const noexcept;

    /**
        * @brief Scale AABB from center
        * @param scale Scale factors for each axis
        * @return Scaled AABB
        */
    AABB scale(const float3& scale) const noexcept;

    /**
        * @brief Transform AABB by matrix
        * @param matrix Transformation matrix
        * @return Transformed AABB (conservative approximation)
        * @note This is not exact for rotations - returns containing AABB
        */
    AABB transform(const float4x4& matrix) const noexcept;

    // ============================================================================
    // Static Operations
    // ============================================================================

    /**
        * @brief Compute union of two AABBs
        * @param a First AABB
        * @param b Second AABB
        * @return Union AABB
        */
    static AABB combine(const AABB& a, const AABB& b) noexcept;

    /**
        * @brief Compute intersection of two AABBs
        * @param a First AABB
        * @param b Second AABB
        * @return Intersection AABB (may be invalid if no intersection)
        */
    static AABB intersect(const AABB& a, const AABB& b) noexcept;

    // ============================================================================
    // Utility Methods
    // ============================================================================

    /**
        * @brief Compute longest axis of AABB
        * @return Index of longest axis (0=x, 1=y, 2=z)
        */
    int longest_axis() const noexcept;

    /**
        * @brief Compute axis with maximum extent
        * @return Length of longest axis
        */
    float max_extent() const noexcept;

    /**
        * @brief Get normalized coordinates within AABB (0-1 range)
        * @param point Point to normalize
        * @return Normalized coordinates
        */
    float3 normalize_coords(const float3& point) const noexcept;

    /**
        * @brief Compute point from normalized coordinates
        * @param coords Normalized coordinates (0-1 range)
        * @return World-space point
        */
    float3 denormalize_coords(const float3& coords) const noexcept;

    /**
        * @brief Clamp point to AABB boundaries
        * @param point Point to clamp
        * @return Clamped point
        */
    float3 clamp_point(const float3& point) const noexcept;

    /**
        * @brief Compute squared distance from point to AABB
        * @param point Point to compute distance from
        * @return Squared distance (0 if inside)
        */
    float distance_sq(const float3& point) const noexcept;

    /**
        * @brief Compute distance from point to AABB
        * @param point Point to compute distance from
        * @return Distance (0 if inside)
        */
    float distance(const float3& point) const noexcept;

    // ============================================================================
    // Comparison Operators
    // ============================================================================

    bool operator==(const AABB& other) const noexcept;
    bool operator!=(const AABB& other) const noexcept;

    // ============================================================================
    // String Representation
    // ============================================================================

    std::string to_string() const;
};

// ============================================================================
// Global Functions
// ============================================================================

inline AABB combine(const AABB& a, const AABB& b) noexcept;
inline AABB intersect(const AABB& a, const AABB& b) noexcept;
inline bool intersects(const AABB& a, const AABB& b, float epsilon = EPSILON) noexcept;
inline float surface_area(const AABB& aabb) noexcept;
inline float volume(const AABB& aabb) noexcept;
inline bool contains(const AABB& container, const float3& point, float epsilon = 0.0f) noexcept;
inline bool contains(const AABB& container, const AABB& other, float epsilon = 0.0f) noexcept;

// ============================================================================
// Global Constants
// ============================================================================

extern const AABB AABB_Empty;
extern const AABB AABB_Infinite;
AFTERMATH_END

#include "math_aabb.inl"
