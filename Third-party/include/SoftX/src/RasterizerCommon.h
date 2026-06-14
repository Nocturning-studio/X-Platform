#pragma once

#include <SoftX/SoftX.h>

SOFTX_BEGIN

namespace RasterizerCommon
{
    // ── Morton order (Z-order curve) ─────────────────────────────────────────────
    //
    // Interleaves bits of x into even positions: b3b2b1b0 → 0b3 0b2 0b1 0b0
    // Used to encode 2D pixel coordinates into a 1D Morton code such that
    // spatially adjacent pixels have nearby codes — improving cache locality.
    //
    inline uint32_t Part1By1(uint32_t x)
    {
        x &= 0x0000ffff;
        x = (x ^ (x << 8)) & 0x00ff00ff;
        x = (x ^ (x << 4)) & 0x0f0f0f0f;
        x = (x ^ (x << 2)) & 0x33333333;
        x = (x ^ (x << 1)) & 0x55555555;
        return x;
    }

    // Compacts even bit positions back: 0b3 0b2 0b1 0b0 → b3b2b1b0
    inline uint32_t Compact1By1(uint32_t x)
    {
        x &= 0x55555555;
        x = (x ^ (x >> 1)) & 0x33333333;
        x = (x ^ (x >> 2)) & 0x0f0f0f0f;
        x = (x ^ (x >> 4)) & 0x00ff00ff;
        x = (x ^ (x >> 8)) & 0x0000ffff;
        return x;
    }

    // Encodes 2D coordinates into a Morton code (Z-order)
    inline uint32_t EncodeMorton2(uint32_t x, uint32_t y)
    {
        return (Part1By1(y) << 1) | Part1By1(x);
    }

    inline uint32_t DecodeMorton2X(uint32_t code)
    {
        return Compact1By1(code);
    }
    inline uint32_t DecodeMorton2Y(uint32_t code)
    {
        return Compact1By1(code >> 1);
    }

    // Smallest power of two >= x
    inline uint32_t NextPow2(uint32_t x)
    {
        if (x <= 1)
            return 1;
        --x;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        return ++x;
    }

    // Bounding box side limit for Morton traversal.
    // Morton is beneficial when the bbox is roughly square and fits in L1 cache:
    //   side=32 → max 1024 Morton codes → ~4KB of pixel data (fits in L1).
    // For larger or very non-square bboxes, scanline is more efficient.
    static constexpr uint MORTON_MAX_SIDE = 32;

    // ─── Fixed-point sub-pixel rasterisation (28.4) ───────────────────────────
    //
    // Edge function at pixel P for edge A → B:
    //   E = (px − ax)·(by − ay) − (py − ay)·(bx − ax)  (all in fixed-point)
    //
    // Pixel x±1 step:  ΔE_x = S·(by − ay)   (int32 safe: ≤ 16 × 2·4096·16 ≈ 2²¹)
    // Row   y±1 step:  ΔE_y = −S·(bx − ax)  (same)
    // Initial value:   ≤ (2·4096·16)²        = 2³⁴  → int64 required
    //
    // int64→int32 cast in SIMD is lossless up to ~1920×1080 (SUBPIXEL_BITS=4).
    // For 4 K, set SUBPIXEL_BITS = 2.

    static constexpr int SUBPIXEL_BITS = 4;
    static constexpr int SUBPIXEL_STEP = 1 << SUBPIXEL_BITS; // 16

    // Screen-space float → fixed-point integer
    inline int ToFixed(float v)
    {
        return static_cast<int>(std::lround(v * float(SUBPIXEL_STEP)));
    }

    // Fixed-point coordinate of pixel-centre for pixel index i
    //   Pixel i occupies [i·S, (i+1)·S)  →  centre = i·S + S/2
    inline int PixelCentre(int i)
    {
        return i * SUBPIXEL_STEP + (SUBPIXEL_STEP >> 1);
    }

    // Integer edge function — int64 to avoid overflow during initial setup.
    // Units: SUBPIXEL_STEP² × float_edge_func (ratio f/area is preserved).
    inline int64_t EdgeFunctionInt(int ax, int ay, int bx, int by, int px, int py)
    {
        return int64_t(px - ax) * (by - ay) - int64_t(py - ay) * (bx - ax);
    }

    inline float EdgeFunction(const float4& a, const float4& b, const float2& c)
    {
        return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
    }

    inline float EdgeFunction(const float4& a, const float4& b, const float4& c)
    {
        return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
    }

    // Perspective-correct interpolation of triangle attributes.
    //
    // Linear interpolation in screen space yields incorrect results
    // because perspective division is non-linear — equal steps on screen
    // do not correspond to equal steps in 3D space.
    //
    // Formula:
    //   A = (α * A0*invW0 + β * A1*invW1 + γ * A2*invW2) / (α*invW0 + β*invW1 + γ*invW2)
    //
    // where invW0/1/2 = Position.w of each vertex (set in ClipSpaceToScreenSpace),
    // and α, β, γ are barycentric coordinates in screen space.
    inline VertexOutput Trilerp(const VertexOutput& v0, 
                                const VertexOutput& v1, 
                                const VertexOutput& v2, 
                                float alpha,
                                float beta, 
                                float gamma)
    {
        // Position.w stores 1/w — weight each vertex
        float w0 = alpha * v0.Position.w;
        float w1 = beta * v1.Position.w;
        float w2 = gamma * v2.Position.w;

        float invWsum = w0 + w1 + w2;
        float wsum = (std::abs(invWsum) > 1e-10f) ? (1.0f / invWsum) : 0.0f;

        VertexOutput result;

#define PLERP(field) result.field = (w0 * v0.field + w1 * v1.field + w2 * v2.field) * wsum
        PLERP(Color);
        PLERP(Normal);
        PLERP(UV);
#undef PLERP

        // Position.xyz — linear, perspective correction not needed
#define LERP(field) result.field = (alpha * v0.field + beta * v1.field + gamma * v2.field)
        LERP(Position);
#undef LERP

        result.Position.w = invWsum; // interpolated 1/w available in PS

        return result;
    }

    // Transforms vertex from clip space to screen space.
    // Stores 1/w in Position.w for perspective-correct interpolation
    // in the rasterizer (DX11 style: after rasterization Position.w == 1/w).
    inline void ClipSpaceToScreenSpace(VertexOutput& vert, const Viewport& vp)
    {
        float w = vert.Position.w;
        float invW = (std::abs(w) > 1e-10f) ? (1.0f / w) : 0.0f;

        float xNDC = vert.Position.x * invW;
        float yNDC = vert.Position.y * invW;
        float zNDC = vert.Position.z * invW;

        vert.Position.x = vp.pos.x + (xNDC * 0.5f + 0.5f) * vp.size.x;
        vert.Position.y = vp.pos.y + (1.0f - (yNDC * 0.5f + 0.5f)) * vp.size.y;
        vert.Position.z = vp.minZ + zNDC * (vp.maxZ - vp.minZ);
        vert.Position.w = invW; // DX11 style: Position.w = 1/w after rasterization
    }

    // Linear interpolation of two vertices in clip space
    inline VertexOutput LerpVertexClipSpace(const VertexOutput& a, const VertexOutput& b, float t)
    {
        VertexOutput r;

#define CSLERP(field) r.field = a.field + t * (b.field - a.field)
        CSLERP(Position);
        CSLERP(Color);
        CSLERP(Normal);
        CSLERP(UV);
#undef CSLERP

        return r;
    }

    // Clips triangle against near plane (w = nearW) in clip space.
    // Returns 0, 1 or 2 triangles in outTris[2][3].
    // Sutherland-Hodgman algorithm for a single plane.
    inline int ClipTriangleNearPlane(
        const VertexOutput& v0,
        const VertexOutput& v1,
        const VertexOutput& v2,
        VertexOutput outTris[2][3])
    {
        const VertexOutput* verts[3] = { &v0, &v1, &v2 };
        bool inside[3];
        int insideCount = 0;

        for (int i = 0; i < 3; ++i) {
            inside[i] = (verts[i]->Position.z >= 0.0f) && (verts[i]->Position.w > 0.0f);
            if (inside[i]) ++insideCount;
        }

        if (insideCount == 0) return 0;
        if (insideCount == 3) {
            outTris[0][0] = v0; outTris[0][1] = v1; outTris[0][2] = v2;
            return 1;
        }

        VertexOutput poly[4];
        int polySize = 0;

        for (int i = 0; i < 3; ++i) {
            int j = (i + 1) % 3;
            const VertexOutput& A = *verts[i];
            const VertexOutput& B = *verts[j];
            bool aIn = inside[i];
            bool bIn = inside[j];

            if (aIn) poly[polySize++] = A;

            if (aIn != bIn) {
                float t = (0.0f - A.Position.z) / (B.Position.z - A.Position.z);
                t = clamp(t, 0.0f, 1.0f);
                poly[polySize++] = LerpVertexClipSpace(A, B, t);
            }
        }

        if (polySize < 3) return 0;

        outTris[0][0] = poly[0];
        outTris[0][1] = poly[1];
        outTris[0][2] = poly[2];
        if (polySize == 4) {
            outTris[1][0] = poly[0];
            outTris[1][1] = poly[2];
            outTris[1][2] = poly[3];
            return 2;
        }
        return 1;
    }

} // namespace RasterizerCommon

SOFTX_END
