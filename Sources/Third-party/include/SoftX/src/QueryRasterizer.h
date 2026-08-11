/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "../include/LibInternal.h"
#include "../include/DepthBuffer.h"
#include "RasterizerCommon.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

namespace QueryRasterizer
{
    /**
     * Rasterizes a triangle for occlusion query – only depth is computed and
     * tested. Returns the number of visible (depth‑passing) pixels.
     */
    static inline uint RasterizeTriangle(const RasterizerCommon::TriangleSetup& setup,
                                         const RasterizerState& rasterizerState,
                                         DepthBuffer& depthBuffer,
                                         const Viewport& viewport,
                                         uint2 tileMin,
                                         uint2 tileMax)
    {
        uint visibleCount = 0;

        // Intersect the triangle’s bounding box with the tile extent
        int boundingBoxMinX = std::max(static_cast<int>(tileMin.x), setup.bbMinX);
        int boundingBoxMaxX = std::min(static_cast<int>(tileMax.x), setup.bbMaxX);
        int boundingBoxMinY = std::max(static_cast<int>(tileMin.y), setup.bbMinY);
        int boundingBoxMaxY = std::min(static_cast<int>(tileMax.y), setup.bbMaxY);
        if (boundingBoxMinX > boundingBoxMaxX || boundingBoxMinY > boundingBoxMaxY)
            return 0;

        int pixelCentreX = RasterizerCommon::PixelCentre(boundingBoxMinX);
        int pixelCentreY = RasterizerCommon::PixelCentre(boundingBoxMinY);

        int32_t edge01Row = setup.normSign * RasterizerCommon::EdgeFunctionInt(setup.x0fp, setup.y0fp, setup.x1fp, setup.y1fp, pixelCentreX, pixelCentreY);
        int32_t edge12Row = setup.normSign * RasterizerCommon::EdgeFunctionInt(setup.x1fp, setup.y1fp, setup.x2fp, setup.y2fp, pixelCentreX, pixelCentreY);
        int32_t edge20Row = setup.normSign * RasterizerCommon::EdgeFunctionInt(setup.x2fp, setup.y2fp, setup.x0fp, setup.y0fp, pixelCentreX, pixelCentreY);

        float barycentricAlphaRow = static_cast<float>(edge12Row) * setup.invArea2;
        float barycentricBetaRow  = static_cast<float>(edge20Row) * setup.invArea2;
        float barycentricGammaRow = static_cast<float>(edge01Row) * setup.invArea2;

        for (int y = boundingBoxMinY; y <= boundingBoxMaxY; ++y)
        {
            int32_t edge01 = edge01Row;
            int32_t edge12 = edge12Row;
            int32_t edge20 = edge20Row;

            float alpha = barycentricAlphaRow;
            float beta  = barycentricBetaRow;
            float gamma = barycentricGammaRow;

            for (int x = boundingBoxMinX; x <= boundingBoxMaxX; ++x)
            {
                if ((edge01 | edge12 | edge20) >= 0)
                {
                    Interpolant fragment;

                    float weight0 = alpha * setup.v0.ClipSpacePosition.w;
                    float weight1 = beta  * setup.v1.ClipSpacePosition.w;
                    float weight2 = gamma * setup.v2.ClipSpacePosition.w;

                    float totalWeight = weight0 + weight1 + weight2;
                    float inverseTotalWeight = (std::abs(totalWeight) > 1e-10f) ? (1.0f / totalWeight) : 0.0f;

                    // Perspective‑correct depth interpolation
                    fragment.ClipSpacePosition.z = (weight0 * setup.v0.ClipSpacePosition.z + weight1 * setup.v1.ClipSpacePosition.z + weight2 * setup.v2.ClipSpacePosition.z) * inverseTotalWeight;
                    // Linear interpolation for w (to reconstruct view‑space depth)
                    fragment.ClipSpacePosition.w = alpha * setup.v0.ClipSpacePosition.w + beta * setup.v1.ClipSpacePosition.w + gamma * setup.v2.ClipSpacePosition.w;

                    float depth = RasterizerCommon::ComputeDepth(fragment.ClipSpacePosition.z, fragment.ClipSpacePosition.w, viewport);
                    float storedDepth = depthBuffer.At(int2(x, y));

                    if (RasterizerCommon::DepthTest(depth, storedDepth, rasterizerState.depthFunc))
                    {
                        if (rasterizerState.depthWriteEnable)
                            depthBuffer.At(int2(x, y)) = depth;
                        ++visibleCount;
                    }
                }

                edge01 += setup.stepX01;
                edge12 += setup.stepX12;
                edge20 += setup.stepX20;
                alpha  += setup.faStepX;
                beta   += setup.fbStepX;
                gamma  += setup.fcStepX;
            }

            edge01Row += setup.stepY01;
            edge12Row += setup.stepY12;
            edge20Row += setup.stepY20;
            barycentricAlphaRow += setup.faStepY;
            barycentricBetaRow  += setup.fbStepY;
            barycentricGammaRow += setup.fcStepY;
        }

        return visibleCount;
    }

} // namespace QueryRasterizer

SOFTX_END
/////////////////////////////////////////////////////////////////
