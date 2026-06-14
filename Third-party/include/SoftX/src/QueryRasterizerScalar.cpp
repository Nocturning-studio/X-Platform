#include "pch.h"

#include "RasterizerCommon.h"
#include <SoftX/SoftX.h>

SOFTX_BEGIN

static inline bool ProcessPixel(uint x, uint y,
                                int64_t sf12, int64_t sf20, int64_t sf01,
                                int64_t area2Int,
                                const VertexOutput& v0, const VertexOutput& v1, const VertexOutput& v2,
                                DepthBuffer& depthBuffer,
                                const RasterizerState& state,
                                uint width)
{
    const float fa = float(sf12) / float(area2Int);
    const float fb = float(sf20) / float(area2Int);
    const float fc = float(sf01) / float(area2Int);
    float z = fa * v0.Position.z + fb * v1.Position.z + fc * v2.Position.z;

    const uint idx = y * width + x;
    bool depthPass = false;
    switch (state.depthFunc)
    {
    case ComparisonFunc::Never:        depthPass = false; break;
    case ComparisonFunc::Less:         depthPass = z < depthBuffer.At(idx); break;
    case ComparisonFunc::Equal:        depthPass = z == depthBuffer.At(idx); break;
    case ComparisonFunc::LessEqual:    depthPass = z <= depthBuffer.At(idx); break;
    case ComparisonFunc::Greater:      depthPass = z > depthBuffer.At(idx); break;
    case ComparisonFunc::NotEqual:     depthPass = z != depthBuffer.At(idx); break;
    case ComparisonFunc::GreaterEqual: depthPass = z >= depthBuffer.At(idx); break;
    case ComparisonFunc::Always:       depthPass = true; break;
    }

    if (depthPass)
    {
        if (state.depthWriteEnable)
            depthBuffer.At(idx) = z;
        return true;
    }
    return false;
}

uint QueryRasterizerScalar::RasterizeTriangle(const VertexOutput& v0,
                                              const VertexOutput& v1,
                                              const VertexOutput& v2,
                                              const RasterizerState& state,
                                              DepthBuffer& depthBuffer,
                                              const ConstantBuffer& /*cb*/,
                                              uint2 tileMin,
                                              uint2 tileMax)
{
    float minX = std::min({ v0.Position.x, v1.Position.x, v2.Position.x });
    float maxX = std::max({ v0.Position.x, v1.Position.x, v2.Position.x });
    float minY = std::min({ v0.Position.y, v1.Position.y, v2.Position.y });
    float maxY = std::max({ v0.Position.y, v1.Position.y, v2.Position.y });

    int iMinX = std::max((int)tileMin.x, (int)std::floor(minX));
    int iMaxX = std::min((int)tileMax.x, (int)std::ceil(maxX));
    int iMinY = std::max((int)tileMin.y, (int)std::floor(minY));
    int iMaxY = std::min((int)tileMax.y, (int)std::ceil(maxY));

    if (iMinX > iMaxX || iMinY > iMaxY) UNLIKELY
        return 0;

    const int x0fp = RasterizerCommon::ToFixed(v0.Position.x);
    const int y0fp = RasterizerCommon::ToFixed(v0.Position.y);
    const int x1fp = RasterizerCommon::ToFixed(v1.Position.x);
    const int y1fp = RasterizerCommon::ToFixed(v1.Position.y);
    const int x2fp = RasterizerCommon::ToFixed(v2.Position.x);
    const int y2fp = RasterizerCommon::ToFixed(v2.Position.y);

    int64_t area2Int = RasterizerCommon::EdgeFunctionInt(x0fp, y0fp, x1fp, y1fp, x2fp, y2fp);
    const CullMode cull = state.cullMode;
    if (cull == CullMode::Back && area2Int < 0) return 0;
    if (cull == CullMode::Front && area2Int > 0) return 0;
    if (area2Int == 0) UNLIKELY return 0;

    const int normSign = (area2Int > 0) ? 1 : -1;
    if (area2Int < 0)
        area2Int = -area2Int;

    uint width = depthBuffer.Width();
    uint visibleCount = 0;

    const uint bbW = iMaxX - iMinX + 1;
    const uint bbH = iMaxY - iMinY + 1;
    const bool useMorton = (std::max(bbW, bbH) <= RasterizerCommon::MORTON_MAX_SIDE);

    if (useMorton)
    {
        const uint side = RasterizerCommon::NextPow2(std::max(bbW, bbH));
        const uint total = side * side;

        for (uint code = 0; code < total; ++code)
        {
            const uint dx = RasterizerCommon::DecodeMorton2X(code);
            const uint dy = RasterizerCommon::DecodeMorton2Y(code);
            if (dx >= bbW || dy >= bbH) continue;
            const uint x = iMinX + dx;
            const uint y = iMinY + dy;
            if (x < tileMin.x || x > tileMax.x || y < tileMin.y || y > tileMax.y) continue;

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

            if (ProcessPixel(x, y, sf12, sf20, sf01, area2Int, v0, v1, v2, depthBuffer, state, width))
                ++visibleCount;
        }
    }
    else
    {
        // Scanline traversal
        const int stepX01 = RasterizerCommon::SUBPIXEL_STEP * (y1fp - y0fp);
        const int stepX12 = RasterizerCommon::SUBPIXEL_STEP * (y2fp - y1fp);
        const int stepX20 = RasterizerCommon::SUBPIXEL_STEP * (y0fp - y2fp);
        const int stepY01 = -RasterizerCommon::SUBPIXEL_STEP * (x1fp - x0fp);
        const int stepY12 = -RasterizerCommon::SUBPIXEL_STEP * (x2fp - x1fp);
        const int stepY20 = -RasterizerCommon::SUBPIXEL_STEP * (x0fp - x2fp);

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

        for (int y = iMinY; y <= iMaxY;
            ++y, f01Row += stepY01, f12Row += stepY12, f20Row += stepY20)
        {
            int64_t f01 = f01Row;
            int64_t f12 = f12Row;
            int64_t f20 = f20Row;

            for (int x = iMinX; x <= iMaxX;
                ++x, f01 += stepX01, f12 += stepX12, f20 += stepX20)
            {
                if ((f01 | f12 | f20) < 0) continue;
                if (ProcessPixel(x, y, f12, f20, f01, area2Int, v0, v1, v2, depthBuffer, state, width))
                    ++visibleCount;
            }
        }
    }

    return visibleCount;
}

SOFTX_END
