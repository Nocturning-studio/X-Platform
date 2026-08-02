/*
 * AfterMath — high‑performance C++ math library (HLSL‑style, SSE‑accelerated)
 *
 * Project:   Presence AfterMath
 * Copyright: 2026 Presence Collaboratory
 * Authors:   NSDeathman (Architecture & Core)
 *            DeepSeek (Mathematics & HLSL Integration)
 *            Gemini 3 (Optimization & Fast Math)
 *            Nikolay Partas (Half precision data type prototype)
 * License:   MIT License with Attribution — see LICENSE.md for details.
 *
 * https://github.com/Presence-Collaboratory/AfterMath-CPP-Open-Math-Library
 */
#pragma once

#include <string>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <type_traits>

#include "AfterMathInternal.h"
#include "math_template_vector2.h"
#include "math_float2.h"

AFTERMATH_BEGIN

// ============================================================================
// 2D Axis-Aligned Rectangle (Template version using TemplateVector2<T>)
// ============================================================================

/**
 * @struct TemplateRect
 * @brief 2‑dimensional axis‑aligned rectangle (position + size) templated on coordinate type T.
 *
 * Uses TemplateVector2<T> for position and size. All operations are
 * constexpr/noexcept where possible and work naturally for both integer
 * and floating-point types.
 *
 * @note For intersection/union semantics, the rectangle is considered closed
 *       on the left/top edges and open on the right/bottom edges.
 */
template <typename T>
struct TemplateRect
{
    using VectorType = TemplateVector2<T>;

    VectorType position;  ///< Top‑left corner
    VectorType size;      ///< Width and height (must be ≥ 0)

    // ========================================================================
    // Constructors
    // ========================================================================

    constexpr TemplateRect() noexcept
        : position(), size() {}

    constexpr TemplateRect(const VectorType& top_left, const VectorType& dimensions) noexcept
        : position(top_left), size(dimensions) {}

    template <typename U = T, typename = std::enable_if_t<std::is_same_v<U, float>>>
    TemplateRect(const float2& top_left, const float2& dimensions) noexcept
        : position(top_left.x, top_left.y), size(dimensions.x, dimensions.y) {}

    template <typename U = T, typename = std::enable_if_t<std::is_same_v<U, float>>>
    TemplateRect(const std::pair<float2, float2>& p) noexcept
        : position(p.first.x, p.first.y), size(p.second.x, p.second.y) {}

    constexpr TemplateRect(T left, T top, T width, T height) noexcept
        : position(left, top), size(width, height) {}

    /**
     * @brief Construct from min and max corners.
     *        The resulting rectangle always has non‑negative size.
     */
    static constexpr TemplateRect from_min_max(const VectorType& min, const VectorType& max) noexcept
    {
        return TemplateRect(min, max - min);
    }

    /**
     * @brief Construct from center and half‑extents.
     */
    static constexpr TemplateRect from_center_extents(const VectorType& center, const VectorType& half_extents) noexcept
    {
        return TemplateRect(center - half_extents, half_extents * 2);
    }

    // ========================================================================
    // Basic Accessors
    // ========================================================================

    constexpr T left()   const noexcept { return position.x; }
    constexpr T top()    const noexcept { return position.y; }
    constexpr T width()  const noexcept { return size.x; }
    constexpr T height() const noexcept { return size.y; }

    constexpr T right()  const noexcept { return left() + width(); }
    constexpr T bottom() const noexcept { return top() + height(); }

    constexpr VectorType min() const noexcept { return position; }
    constexpr VectorType max() const noexcept { return position + size; }

    /**
     * @brief Center point of the rectangle.
     * @note For integer types, coordinates are truncated (integer division).
     */
    constexpr VectorType center() const noexcept
    {
        return position + size / 2;
    }

    /**
     * @brief Half-size (extent) of the rectangle.
     * @note For integer types, values are truncated (integer division).
     */
    constexpr VectorType extent() const noexcept
    {
        return size / 2;
    }

    // ========================================================================
    // Derived Properties
    // ========================================================================

    constexpr T area() const noexcept
    {
        return size.x * size.y;
    }

    /**
     * @brief Aspect ratio (width / height). Returns 0 if height is zero.
     *        Always computed as float for consistency.
     */
    float aspect_ratio() const noexcept
    {
        if (size.y != T(0))
            return static_cast<float>(size.x) / static_cast<float>(size.y);
        return 0.0f;
    }

    /**
     * @brief Returns true if the rectangle has non‑negative dimensions and
     *        (for floating-point types) finite coordinates.
     */
    bool is_valid() const noexcept
    {
        if (size.x < T(0) || size.y < T(0))
            return false;

        if constexpr (std::is_floating_point_v<T>)
        {
            return std::isfinite(position.x) && std::isfinite(position.y) &&
                   std::isfinite(size.x)     && std::isfinite(size.y);
        }
        else
        {
            return true; // integers are always finite and valid if non‑negative
        }
    }

    // ========================================================================
    // Containment & Overlap Tests
    // ========================================================================

    /**
     * @brief True if point is inside (including left/top edges, excluding right/bottom).
     */
    bool contains(const VectorType& point) const noexcept
    {
        return point.x >= left() && point.x < right() &&
               point.y >= top()  && point.y < bottom();
    }

    /**
     * @brief True if another rectangle is completely inside this one.
     */
    bool contains(const TemplateRect& other) const noexcept
    {
        return other.left()   >= left()   && other.right()  <= right() &&
               other.top()    >= top()    && other.bottom() <= bottom();
    }

    /**
     * @brief True if the two rectangles intersect (share at least one point).
     */
    bool overlaps(const TemplateRect& other) const noexcept
    {
        return left()   < other.right()  && right()  > other.left() &&
               top()    < other.bottom() && bottom() > other.top();
    }

    // ========================================================================
    // Mutating Operations (return new TemplateRect)
    // ========================================================================

    /**
     * @brief Intersection of two rectangles. Returns empty TemplateRect if no overlap.
     */
    TemplateRect intersected(const TemplateRect& other) const noexcept
    {
        T l = std::max(left(),   other.left());
        T t = std::max(top(),    other.top());
        T r = std::min(right(),  other.right());
        T b = std::min(bottom(), other.bottom());

        if (l < r && t < b)
            return TemplateRect(l, t, r - l, b - t);
        else
            return TemplateRect(); // empty
    }

    /**
     * @brief Union of two rectangles (bounding box containing both).
     */
    constexpr TemplateRect united(const TemplateRect& other) const noexcept
    {
        if (size.x == T(0) && size.y == T(0))
            return other;
        if (other.size.x == T(0) && other.size.y == T(0))
            return *this;

        T l = std::min(left(), other.left());
        T t = std::min(top(), other.top());
        T r = std::max(right(), other.right());
        T b = std::max(bottom(), other.bottom());
        return TemplateRect(l, t, r - l, b - t);
    }

    /**
     * @brief Expand the rectangle by adding margins on all sides.
     */
    TemplateRect expanded(const VectorType& amounts) const noexcept
    {
        return TemplateRect(position - amounts, size + amounts * 2);
    }

    /**
     * @brief Expand the rectangle by a uniform amount.
     */
    TemplateRect expanded(T amount) const noexcept
    {
        return expanded(VectorType(amount));
    }

    /**
     * @brief Translate the rectangle by a vector.
     */
    TemplateRect translated(const VectorType& offset) const noexcept
    {
        return TemplateRect(position + offset, size);
    }

    // ========================================================================
    // Overloads for float2 (only when T = float)
    // ========================================================================

    /**
     * @brief Expand the rectangle by amounts given as float2.
     */
    template <typename U = T, typename = std::enable_if_t<std::is_same_v<U, float>>>
    TemplateRect expanded(const float2& amounts) const noexcept
    {
        return expanded(VectorType(amounts.x, amounts.y));
    }

     /**
      * @brief Translate the rectangle by a float2 offset.
      */
    template <typename U = T, typename = std::enable_if_t<std::is_same_v<U, float>>>
    TemplateRect translated(const float2& offset) const noexcept
    {
        return translated(VectorType(offset.x, offset.y));
    }

    /**
     * @brief Point containment test accepting float2.
     */
    template <typename U = T, typename = std::enable_if_t<std::is_same_v<U, float>>>
    bool contains(const float2& point) const noexcept
    {
        return contains(VectorType(point.x, point.y));
    }

    template <typename U = T, typename = std::enable_if_t<std::is_same_v<U, float>>>
    TemplateRect& operator=(const std::pair<float2, float2>& p) noexcept
    {
        position = VectorType(p.first.x, p.first.y);
        size = VectorType(p.second.x, p.second.y);
        return *this;
    }

    template <typename U = T, typename = std::enable_if_t<std::is_same_v<U, float>>>
    static TemplateRect from_min_max(const float2& min, const float2& max) noexcept
    {
        return from_min_max(VectorType(min.x, min.y), VectorType(max.x, max.y));
    }

    template <typename U = T, typename = std::enable_if_t<std::is_same_v<U, float>>>
    static TemplateRect from_center_extents(const float2& center, const float2& half_extents) noexcept
    {
        return from_center_extents(VectorType(center.x, center.y), VectorType(half_extents.x, half_extents.y));
    }

    // ========================================================================
    // Comparison
    // ========================================================================

    friend bool operator==(const TemplateRect& a, const TemplateRect& b) noexcept
    {
        return a.position == b.position && a.size == b.size;
    }

    friend bool operator!=(const TemplateRect& a, const TemplateRect& b) noexcept
    {
        return !(a == b);
    }

    // ========================================================================
    // String Output
    // ========================================================================

    std::string to_string() const
    {
        std::ostringstream oss;
        oss << "TemplateRect(L:" << left() << ", T:" << top()
            << ", R:" << right() << ", B:" << bottom()
            << " | W:" << width() << ", H:" << height() << ")";
        return oss.str();
    }
};

// ============================================================================
// HLSL‑like Global Functions
// ============================================================================

template <typename T>
inline TemplateRect<T> rect_from_min_max(const TemplateVector2<T>& min, const TemplateVector2<T>& max) noexcept
{
    return TemplateRect<T>::from_min_max(min, max);
}

template <typename T>
inline TemplateRect<T> rect_from_center_extents(const TemplateVector2<T>& center, const TemplateVector2<T>& half_extents) noexcept
{
    return TemplateRect<T>::from_center_extents(center, half_extents);
}

template <typename T>
inline TemplateRect<T> rect_intersection(const TemplateRect<T>& a, const TemplateRect<T>& b) noexcept
{
    return a.intersected(b);
}

template <typename T>
inline TemplateRect<T> rect_union(const TemplateRect<T>& a, const TemplateRect<T>& b) noexcept
{
    return a.united(b);
}

template <typename T>
inline bool rect_contains(const TemplateRect<T>& r, const TemplateVector2<T>& point) noexcept
{
    return r.contains(point);
}

template <typename T>
inline bool rect_contains(const TemplateRect<T>& a, const TemplateRect<T>& b) noexcept
{
    return a.contains(b);
}

template <typename T>
inline bool rect_overlaps(const TemplateRect<T>& a, const TemplateRect<T>& b) noexcept
{
    return a.overlaps(b);
}

template <typename T>
inline TemplateRect<T> rect_expand(const TemplateRect<T>& r, T amount) noexcept
{
    return r.expanded(amount);
}

template <typename T>
inline TemplateRect<T> rect_expand(const TemplateRect<T>& r, const TemplateVector2<T>& amounts) noexcept
{
    return r.expanded(amounts);
}

template <typename T>
inline TemplateRect<T> rect_translate(const TemplateRect<T>& r, const TemplateVector2<T>& offset) noexcept
{
    return r.translated(offset);
}

// ============================================================================
// HLSL‑like Global Functions overloads for float2
// ============================================================================

// -- rect_from_min_max -------------------------------------------------------
inline TemplateRect<float> rect_from_min_max(const float2& min, const float2& max) noexcept
{
    return rect_from_min_max(TemplateVector2<float>(min.x, min.y),
        TemplateVector2<float>(max.x, max.y));
}

// -- rect_from_center_extents -------------------------------------------------
inline TemplateRect<float> rect_from_center_extents(const float2& center,
    const float2& half_extents) noexcept
{
    return rect_from_center_extents(TemplateVector2<float>(center.x, center.y),
        TemplateVector2<float>(half_extents.x, half_extents.y));
}

// -- rect_contains (point) ----------------------------------------------------
inline bool rect_contains(const TemplateRect<float>& r, const float2& point) noexcept
{
    return rect_contains(r, TemplateVector2<float>(point.x, point.y));
}

// -- rect_contains (rect) -----------------------------------------------------
inline bool rect_contains(const TemplateRect<float>& a, const TemplateRect<float>& b) noexcept
{
    return rect_contains<float>(a, b);
}

// -- rect_expand (vector) -----------------------------------------------------
inline TemplateRect<float> rect_expand(const TemplateRect<float>& r,
    const float2& amounts) noexcept
{
    return rect_expand(r, TemplateVector2<float>(amounts.x, amounts.y));
}

// -- rect_translate -----------------------------------------------------------
inline TemplateRect<float> rect_translate(const TemplateRect<float>& r,
    const float2& offset) noexcept
{
    return rect_translate(r, TemplateVector2<float>(offset.x, offset.y));
}

// ============================================================================
// Mixed-type comparison operators
// ============================================================================

inline bool operator==(const TemplateVector2<float>& lhs, const float2& rhs) noexcept
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline bool operator==(const float2& lhs, const TemplateVector2<float>& rhs) noexcept
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline bool operator!=(const TemplateVector2<float>& lhs, const float2& rhs) noexcept
{
    return !(lhs == rhs);
}

inline bool operator!=(const float2& lhs, const TemplateVector2<float>& rhs) noexcept
{
    return !(lhs == rhs);
}

// ============================================================================
// Useful Constants
// ============================================================================

template <typename T>
AFTERMATH_INLINE_VAR const TemplateRect<T> Rect_Zero = TemplateRect<T>(T(0), T(0), T(0), T(0));

template <typename T>
AFTERMATH_INLINE_VAR const TemplateRect<T> Rect_Unit = TemplateRect<T>(T(0), T(0), T(1), T(1));

AFTERMATH_END
