/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include <optional>
#include "../include/Types.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

namespace RasterizerCommon
{
    static constexpr int SUBPIXEL_BITS = 4;
    static constexpr int SUBPIXEL_STEP = 1 << SUBPIXEL_BITS;

    inline int ToFixed(float v)
    {
        return static_cast<int>(std::lround(v * float(SUBPIXEL_STEP)));
    }

    inline int PixelCentre(int i)
    {
        return i * SUBPIXEL_STEP + (SUBPIXEL_STEP >> 1);
    }

    inline int32_t EdgeFunctionInt(int ax, int ay, int bx, int by, int px, int py)
    {
        return int32_t(px - ax) * (by - ay) - int32_t(py - ay) * (bx - ax);
    }

    inline float EdgeFunction(const float4& a, const float4& b, const float2& c)
    {
        return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
    }

    inline float EdgeFunction(const float4& a, const float4& b, const float4& c)
    {
        return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
    }

    inline float ComputeDepth(float z_clip, float invW, const Viewport& vp)
    {
        float zNDC = z_clip * invW;
        return vp.minZ + zNDC * (vp.maxZ - vp.minZ);
    }

    inline __m128 ComputeDepthForBlock(__m128 z_clip, __m128 invW, const Viewport& vp)
    {
        __m128 zNDC = _mm_mul_ps(z_clip, invW);
        __m128 range = _mm_set1_ps(vp.maxZ - vp.minZ);
        __m128 minZ = _mm_set1_ps(vp.minZ);
        return _mm_add_ps(minZ, _mm_mul_ps(zNDC, range));
    }

#define PLERP(field) result.field = (w0 * v0.field + w1 * v1.field + w2 * v2.field) * wsum
#define LERP(field) result.field = (alpha * v0.field + beta * v1.field + gamma * v2.field)

    inline Interpolant TrilerpDepthOnly(const Interpolant& v0, 
                                        const Interpolant& v1, 
                                        const Interpolant& v2, 
                                        float alpha,
                                        float beta, 
                                        float gamma)
    {
        float w0 = alpha * v0.Position.w;
        float w1 = beta * v1.Position.w;
        float w2 = gamma * v2.Position.w;

        float invWsum = w0 + w1 + w2;
        float wsum = (std::abs(invWsum) > 1e-10f) ? (1.0f / invWsum) : 0.0f;

        Interpolant result;

        PLERP(Position.z);
        LERP(Position.x);
        LERP(Position.y);
        LERP(Position.w);

        return result;
    }

    inline Interpolant Trilerp(const Interpolant& v0, 
                               const Interpolant& v1, 
                               const Interpolant& v2, 
                               float alpha,
                               float beta, 
                               float gamma)
    {
        float w0 = alpha * v0.Position.w;
        float w1 = beta * v1.Position.w;
        float w2 = gamma * v2.Position.w;

        float invWsum = w0 + w1 + w2;
        float wsum = (std::abs(invWsum) > 1e-10f) ? (1.0f / invWsum) : 0.0f;

        Interpolant result;

        PLERP(Color);
        PLERP(Normal);
        PLERP(UV);
        PLERP(Position.z);
        LERP(Position.x);
        LERP(Position.y);
        LERP(Position.w);

        return result;
    }

#undef PLERP
#undef LERP

    inline void ClipSpaceToScreenSpace(Interpolant& vert, const Viewport& vp)
    {
        float w = vert.Position.w;
        float invW = (std::abs(w) > 1e-10f) ? (1.0f / w) : 0.0f;

        float xNDC = vert.Position.x * invW;
        float yNDC = vert.Position.y * invW;

        vert.Position.x = vp.pos.x + (xNDC * 0.5f + 0.5f) * static_cast<float>(vp.size.x);
        vert.Position.y = vp.pos.y + (1.0f - (yNDC * 0.5f + 0.5f)) * static_cast<float>(vp.size.y);
        vert.Position.z = vert.Position.z;
        vert.Position.w = invW;
    }

    inline Interpolant LerpVertexClipSpace(const Interpolant& a, const Interpolant& b, float t)
    {
        Interpolant r;

#define CSLERP(field) r.field = a.field + t * (b.field - a.field)
        CSLERP(Position);
        CSLERP(Color);
        CSLERP(Normal);
        CSLERP(UV);
#undef CSLERP

        return r;
    }

    inline int ClipTriangleNearPlane(const Interpolant& v0,
                                     const Interpolant& v1,
                                     const Interpolant& v2,
                                     Interpolant outTris[2][3])
    {
        const Interpolant* verts[3] = { &v0, &v1, &v2 };
        bool inside[3];
        int insideCount = 0;

        for (int i = 0; i < 3; ++i) 
        {
            inside[i] = (verts[i]->Position.z >= 0.0f) && (verts[i]->Position.w > 0.0f);
            if (inside[i]) ++insideCount;
        }

        if (insideCount == 0) return 0;
        if (insideCount == 3) 
        {
            outTris[0][0] = v0; outTris[0][1] = v1; outTris[0][2] = v2;
            return 1;
        }

        Interpolant poly[4];
        int polySize = 0;

        for (int i = 0; i < 3; ++i) 
        {
            int j = (i + 1) % 3;
            const Interpolant& A = *verts[i];
            const Interpolant& B = *verts[j];
            bool aIn = inside[i];
            bool bIn = inside[j];

            if (aIn) poly[polySize++] = A;

            if (aIn != bIn) 
            {
                float t = (0.0f - A.Position.z) / (B.Position.z - A.Position.z);
                t = AfterMath::clamp(t, 0.0f, 1.0f);
                poly[polySize++] = LerpVertexClipSpace(A, B, t);
            }
        }

        if (polySize < 3) return 0;

        outTris[0][0] = poly[0];
        outTris[0][1] = poly[1];
        outTris[0][2] = poly[2];
        if (polySize == 4) 
        {
            outTris[1][0] = poly[0];
            outTris[1][1] = poly[2];
            outTris[1][2] = poly[3];
            return 2;
        }
        return 1;
    }

    SOFTX_FORCE_INLINE bool DepthTest(float z, float depth, ComparisonFunc func)
    {
        switch (func) 
        {
        case ComparisonFunc::Never:        return false;
        case ComparisonFunc::Less:         return z < depth;
        case ComparisonFunc::Equal:        return z == depth;
        case ComparisonFunc::LessEqual:    return z <= depth;
        case ComparisonFunc::Greater:      return z > depth;
        case ComparisonFunc::NotEqual:     return z != depth;
        case ComparisonFunc::GreaterEqual: return z >= depth;
        case ComparisonFunc::Always:       return true;
        }
        return false;
    }

    struct TriangleSetup
    {
        // Fixed point (28.4) vert coords
        int x0fp, y0fp;
        int x1fp, y1fp;
        int x2fp, y2fp;

        int stepX01, stepX12, stepX20;
        int stepY01, stepY12, stepY20;

        // Precomputed pixel‑space bounding box (integer, floor/ceil)
        int bbMinX, bbMaxX;
        int bbMinY, bbMaxY;

        float faStepX, fbStepX, fcStepX;
        float faStepY, fbStepY, fcStepY;

        float invArea2;      // 1.0 / (2 * area in fixed-point units)
        int   normSign;      // 1 (CCW) or -1 (flipped CW)

        Interpolant v0, v1, v2;
    };

    inline std::optional<TriangleSetup> CreateTriangleSetup(const Interpolant& a,
                                                            const Interpolant& b,
                                                            const Interpolant& c,
                                                            const RasterizerState& state)
    {
        const int x0 = ToFixed(a.Position.x), y0 = ToFixed(a.Position.y);
        const int x1 = ToFixed(b.Position.x), y1 = ToFixed(b.Position.y);
        const int x2 = ToFixed(c.Position.x), y2 = ToFixed(c.Position.y);

        int64_t area2 = EdgeFunctionInt(x0, y0, x1, y1, x2, y2);
        if (area2 == 0) return std::nullopt;

        const CullMode cull = state.cullMode;
        if (cull == CullMode::Back && area2 > 0) return std::nullopt;
        if (cull == CullMode::Front && area2 < 0) return std::nullopt;

        int normSign = (area2 > 0) ? 1 : -1;
        if (area2 < 0) area2 = -area2;

        TriangleSetup s;
        s.x0fp = x0; s.y0fp = y0;
        s.x1fp = x1; s.y1fp = y1;
        s.x2fp = x2; s.y2fp = y2;

        s.invArea2 = 1.0f / static_cast<float>(area2);
        s.normSign = normSign;

        s.stepX01 = normSign * SUBPIXEL_STEP * (y1 - y0);
        s.stepX12 = normSign * SUBPIXEL_STEP * (y2 - y1);
        s.stepX20 = normSign * SUBPIXEL_STEP * (y0 - y2);

        s.stepY01 = -normSign * SUBPIXEL_STEP * (x1 - x0);
        s.stepY12 = -normSign * SUBPIXEL_STEP * (x2 - x1);
        s.stepY20 = -normSign * SUBPIXEL_STEP * (x0 - x2);

        s.faStepX = static_cast<float>(s.stepX12) * s.invArea2;
        s.fbStepX = static_cast<float>(s.stepX20) * s.invArea2;
        s.fcStepX = static_cast<float>(s.stepX01) * s.invArea2;
        s.faStepY = static_cast<float>(s.stepY12) * s.invArea2;
        s.fbStepY = static_cast<float>(s.stepY20) * s.invArea2;
        s.fcStepY = static_cast<float>(s.stepY01) * s.invArea2;

        s.v0 = a;
        s.v1 = b;
        s.v2 = c;

        float minX = std::min({ a.Position.x, b.Position.x, c.Position.x });
        float maxX = std::max({ a.Position.x, b.Position.x, c.Position.x });
        float minY = std::min({ a.Position.y, b.Position.y, c.Position.y });
        float maxY = std::max({ a.Position.y, b.Position.y, c.Position.y });

        s.bbMinX = static_cast<int>(std::floor(minX));
        s.bbMaxX = static_cast<int>(std::ceil(maxX));
        s.bbMinY = static_cast<int>(std::floor(minY));
        s.bbMaxY = static_cast<int>(std::ceil(maxY));

        return s;
    }
} // namespace RasterizerCommon

SOFTX_END
/////////////////////////////////////////////////////////////////
