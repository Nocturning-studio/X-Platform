/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "pch.h"

#include "../include/SoftX.h"
#include "RasterizerCommon.h"
#include "RasterizerScalar.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

// Shared depth test + pixel write — called from both traversal paths.
// Inlined by the compiler; extracted here to avoid duplicating the switch.
static inline void ShadeSinglePixel(
    uint x, uint y,
    int64_t sf12, int64_t sf20, int64_t sf01,
    int64_t area2Int,
    const VertexOutput& v0, const VertexOutput& v1, const VertexOutput& v2,
    DepthBuffer& depthBuffer, IRenderTarget* renderTarget,
    const PixelShader& ps, const ConstantBuffer& cb, const TextureTable* tt,
    const RasterizerState& state,
    uint width)
{
    const float fa = float(sf12) / float(area2Int); // weight for v0
    const float fb = float(sf20) / float(area2Int); // weight for v1
    const float fc = float(sf01) / float(area2Int); // weight for v2

    VertexOutput frag = RasterizerCommon::Trilerp(v0, v1, v2, fa, fb, fc);
    const uint idx    = y * width + x;

    bool depthPass = false;
    switch (state.depthFunc)
    {
    case ComparisonFunc::Never:        depthPass = false; break;
    case ComparisonFunc::Less:         depthPass = frag.Position.z <  depthBuffer.At(idx); break;
    case ComparisonFunc::Equal:        depthPass = frag.Position.z == depthBuffer.At(idx); break;
    case ComparisonFunc::LessEqual:    depthPass = frag.Position.z <= depthBuffer.At(idx); break;
    case ComparisonFunc::Greater:      depthPass = frag.Position.z >  depthBuffer.At(idx); break;
    case ComparisonFunc::NotEqual:     depthPass = frag.Position.z != depthBuffer.At(idx); break;
    case ComparisonFunc::GreaterEqual: depthPass = frag.Position.z >= depthBuffer.At(idx); break;
    case ComparisonFunc::Always:       depthPass = true; break;
    }

    if (depthPass)
    {
        if (state.depthWriteEnable)
            depthBuffer.At(idx) = frag.Position.z;
        if(renderTarget != nullptr)
            renderTarget->SetPixel(uint2(x, y), ps(frag, cb, *tt));
    }
}

void RasterizerScalar::RasterizeTriangle(
    const VertexOutput& v0,
    const VertexOutput& v1,
    const VertexOutput& v2,
    const RasterizerState& state,
    DepthBuffer& depthBuffer,
    IRenderTarget* renderTarget,
    const PixelShader& ps,
    const ConstantBuffer& cb,
    const TextureTable* tt,
    uint2 tileMin,
    uint2 tileMax)
{
    float minX = std::min({v0.Position.x, v1.Position.x, v2.Position.x});
    float maxX = std::max({v0.Position.x, v1.Position.x, v2.Position.x});
    float minY = std::min({v0.Position.y, v1.Position.y, v2.Position.y});
    float maxY = std::max({v0.Position.y, v1.Position.y, v2.Position.y});

    int iMinX = std::max((int)tileMin.x, (int)std::floor(minX));
    int iMaxX = std::min((int)tileMax.x, (int)std::ceil(maxX));
    int iMinY = std::max((int)tileMin.y, (int)std::floor(minY));
    int iMaxY = std::min((int)tileMax.y, (int)std::ceil(maxY));

    if (iMinX > iMaxX || iMinY > iMaxY) SOFTX_UNLIKELY
        return;

    // ── Fixed-point vertex coordinates (28.4) ───────────────────────────────
    const int x0fp = RasterizerCommon::ToFixed(v0.Position.x);
    const int y0fp = RasterizerCommon::ToFixed(v0.Position.y);
    const int x1fp = RasterizerCommon::ToFixed(v1.Position.x);
    const int y1fp = RasterizerCommon::ToFixed(v1.Position.y);
    const int x2fp = RasterizerCommon::ToFixed(v2.Position.x);
    const int y2fp = RasterizerCommon::ToFixed(v2.Position.y);

    int64_t area2Int = RasterizerCommon::EdgeFunctionInt(x0fp, y0fp, x1fp, y1fp, x2fp, y2fp);

    const CullMode cull = state.cullMode;
    if (cull == CullMode::Back  && area2Int > 0) return;
    if (cull == CullMode::Front && area2Int < 0) return;
    if (area2Int == 0) SOFTX_UNLIKELY return;

    // Normalise to CCW so the inside test is always f >= 0
    const int normSign = (area2Int > 0) ? 1 : -1;
    if (area2Int < 0)
        area2Int = -area2Int;

    uint width = 0;
    if (renderTarget)
        width = renderTarget->Width();
    else
        width = depthBuffer.Width();

    // ── Traversal path selection ─────────────────────────────────────────────
    //
    // Scanline (row-major) has poor cache locality for small triangles:
    //   a 2×40 triangle visits ~2 pixels per row, jumping (width * 4) bytes
    //   between rows — each row is a separate cache miss.
    //
    // Morton order (Z-order curve) interleaves X and Y bits so that a 4×4
    // pixel block occupies 16 consecutive codes — all 16 pixels hit the same
    // or adjacent cache lines regardless of triangle shape.
    //
    // Morton overhead: iterates side² codes where side = NextPow2(max(W, H)).
    // For side=32 that is 1024 codes — negligible, and the bbox fits in L1.
    // For larger bboxes the wasted iterations outweigh the benefit, so we
    // fall back to scanline.

    const uint bbW = iMaxX - iMinX + 1;
    const uint bbH = iMaxY - iMinY + 1;
    const bool useMorton = (std::max(bbW, bbH) <= RasterizerCommon::MORTON_MAX_SIDE);

    if (useMorton)
    {
        // ── Morton order traversal ───────────────────────────────────────────
        //
        // Iterate Morton codes 0 … side²-1.
        // Each code decodes to an offset (dx, dy) from the bbox origin.
        // Codes outside the actual bbox are skipped cheaply.
        //
        // Visiting order example for side=4:
        //
        //   code:  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
        //   dx:    0  1  0  1  2  3  2  3  0  1  0  1  2  3  2  3
        //   dy:    0  0  1  1  0  0  1  1  2  2  3  3  2  2  3  3
        //
        // Pixels (0,0),(1,0),(0,1),(1,1) are codes 0-3 — a 2×2 block is
        // always contiguous, ensuring adjacent pixels share cache lines.

        const uint side  = RasterizerCommon::NextPow2(std::max(bbW, bbH));
        const uint total = side * side;

        for (uint code = 0; code < total; ++code)
        {
            const uint dx = RasterizerCommon::DecodeMorton2X(code);
            const uint dy = RasterizerCommon::DecodeMorton2Y(code);

            // Skip codes outside the actual (non-square) bounding box
            if (dx >= bbW || dy >= bbH) continue;

            const uint x = iMinX + dx;
            const uint y = iMinY + dy;

            // Tile boundary guard
            if (x < tileMin.x || x > tileMax.x ||
                y < tileMin.y  || y > tileMax.y) continue;

            // Integer edge test (exact, no floating-point error)
            const int64_t sf01 = normSign * RasterizerCommon::EdgeFunctionInt(
                x0fp, y0fp, x1fp, y1fp,
                RasterizerCommon::PixelCentre(x),
                RasterizerCommon::PixelCentre(y));
            const int64_t sf12 = normSign * RasterizerCommon::EdgeFunctionInt(
                x1fp, y1fp, x2fp, y2fp,
                RasterizerCommon::PixelCentre(x),
                RasterizerCommon::PixelCentre(y));
            const int64_t sf20 = normSign * RasterizerCommon::EdgeFunctionInt(
                x2fp, y2fp, x0fp, y0fp,
                RasterizerCommon::PixelCentre(x),
                RasterizerCommon::PixelCentre(y));

            if (sf01 < 0 || sf12 < 0 || sf20 < 0) continue;

            ShadeSinglePixel(x, y, sf12, sf20, sf01, area2Int,
                             v0, v1, v2, depthBuffer, renderTarget,
                             ps, cb, tt, state, width);
        }
    }
    else
    {
        // ── Scanline traversal ───────────────────────────────────────────────
        //
        // Efficient for large triangles: incremental edge functions advance
        // by a fixed integer step per pixel / per row — no per-pixel multiply.

        // Per-pixel x-step: ΔE = +S * Δy_fp
        // Per-row   y-step: ΔE = -S * Δx_fp
        const int stepX01 =  RasterizerCommon::SUBPIXEL_STEP * (y1fp - y0fp);
        const int stepX12 =  RasterizerCommon::SUBPIXEL_STEP * (y2fp - y1fp);
        const int stepX20 =  RasterizerCommon::SUBPIXEL_STEP * (y0fp - y2fp);
        const int stepY01 = -RasterizerCommon::SUBPIXEL_STEP * (x1fp - x0fp);
        const int stepY12 = -RasterizerCommon::SUBPIXEL_STEP * (x2fp - x1fp);
        const int stepY20 = -RasterizerCommon::SUBPIXEL_STEP * (x0fp - x2fp);

        // Row-start values at pixel centre (iMinX, iMinY)
        int64_t f01Row = normSign * RasterizerCommon::EdgeFunctionInt(
            x0fp, y0fp, x1fp, y1fp,
            RasterizerCommon::PixelCentre(iMinX),
            RasterizerCommon::PixelCentre(iMinY));
        int64_t f12Row = normSign * RasterizerCommon::EdgeFunctionInt(
            x1fp, y1fp, x2fp, y2fp,
            RasterizerCommon::PixelCentre(iMinX),
            RasterizerCommon::PixelCentre(iMinY));
        int64_t f20Row = normSign * RasterizerCommon::EdgeFunctionInt(
            x2fp, y2fp, x0fp, y0fp,
            RasterizerCommon::PixelCentre(iMinX),
            RasterizerCommon::PixelCentre(iMinY));

        const float invArea2 = 1.0f / float(area2Int);
        (void)invArea2; // used inside ShadeSinglePixel via area2Int

        for (int y = iMinY; y <= iMaxY;
             ++y, f01Row += stepY01, f12Row += stepY12, f20Row += stepY20)
        {
            int64_t f01 = f01Row;
            int64_t f12 = f12Row;
            int64_t f20 = f20Row;

            for (int x = iMinX; x <= iMaxX;
                 ++x, f01 += stepX01, f12 += stepX12, f20 += stepX20)
            {
                // Single branch: OR of sign bits — negative if any f < 0
                if ((f01 | f12 | f20) < 0) continue;

                ShadeSinglePixel(x, y, f12, f20, f01, area2Int,
                                 v0, v1, v2, depthBuffer, renderTarget,
                                 ps, cb, tt, state, width);
            }
        }
    }
}

SOFTX_END
/////////////////////////////////////////////////////////////////
