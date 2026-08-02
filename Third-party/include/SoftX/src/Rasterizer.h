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

namespace Rasterizer
{
    /**
     * Rasterizes a single triangle into the provided depth buffer and
     * optionally the render target, using the pre‑computed triangle setup.
     * Only pixels whose edge functions are positive (after normalisation)
     * and that pass the depth test are processed.
     *
     * Depth is evaluated first; if the depth test fails, interpolation
     * of color, normal, UV, and screen‑space position is skipped.
     */
    static inline void RasterizeTriangle(const RasterizerCommon::TriangleSetup& setup,
                                         const RasterizerState& rasterizerState,
                                         DepthBuffer& depthBuffer,
                                         Texture* renderTarget,
                                         const Viewport& viewport,
                                         const PixelShader& pixelShader,
                                         const ConstantBuffer& constantBuffer,
                                         const TextureTable* textureTable,
                                         uint2 tileMin,
                                         uint2 tileMax)
    {
        // Intersect the triangle’s bounding box with the tile extent
        int boundingBoxMinX = std::max(static_cast<int>(tileMin.x), setup.bbMinX);
        int boundingBoxMaxX = std::min(static_cast<int>(tileMax.x), setup.bbMaxX);
        int boundingBoxMinY = std::max(static_cast<int>(tileMin.y), setup.bbMinY);
        int boundingBoxMaxY = std::min(static_cast<int>(tileMax.y), setup.bbMaxY);
        if (boundingBoxMinX > boundingBoxMaxX || boundingBoxMinY > boundingBoxMaxY)
            return;

        // Pixel centre of the first scanline’s starting pixel (sub‑pixel precision)
        int pixelCentreX = RasterizerCommon::PixelCentre(boundingBoxMinX);
        int pixelCentreY = RasterizerCommon::PixelCentre(boundingBoxMinY);

        // Evaluate edge functions at the first pixel centre (using fixed‑point vertices)
        int32_t edge01Row = setup.normSign * RasterizerCommon::EdgeFunctionInt(setup.x0fp, setup.y0fp, setup.x1fp, setup.y1fp, pixelCentreX, pixelCentreY);
        int32_t edge12Row = setup.normSign * RasterizerCommon::EdgeFunctionInt(setup.x1fp, setup.y1fp, setup.x2fp, setup.y2fp, pixelCentreX, pixelCentreY);
        int32_t edge20Row = setup.normSign * RasterizerCommon::EdgeFunctionInt(setup.x2fp, setup.y2fp, setup.x0fp, setup.y0fp, pixelCentreX, pixelCentreY);

        // Pre‑compute barycentric row start values (fractional part)
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
                // All edge functions are non‑negative → pixel is inside the triangle
                if ((edge01 | edge12 | edge20) >= 0)
                {
                    // ---- 1. Compute depth only (minimal interpolation) ----
                    float weight0 = alpha * setup.v0.ClipSpacePosition.w;
                    float weight1 = beta  * setup.v1.ClipSpacePosition.w;
                    float weight2 = gamma * setup.v2.ClipSpacePosition.w;

                    float totalWeight = weight0 + weight1 + weight2;
                    float inverseTotalWeight = (std::abs(totalWeight) > 1e-10f) ? (1.0f / totalWeight) : 0.0f;

                    // Perspective-correct interpolation of clip‑space z
                    float interpolatedZ = (weight0 * setup.v0.ClipSpacePosition.z +
                                           weight1 * setup.v1.ClipSpacePosition.z +
                                           weight2 * setup.v2.ClipSpacePosition.z) * inverseTotalWeight;
                    // Linear interpolation of w (needed for view‑space depth)
                    float interpolatedW = alpha * setup.v0.ClipSpacePosition.w +
                                          beta  * setup.v1.ClipSpacePosition.w +
                                          gamma * setup.v2.ClipSpacePosition.w;

                    float depth = RasterizerCommon::ComputeDepth(interpolatedZ, interpolatedW, viewport);
                    float storedDepth = depthBuffer.At(int2(x, y));

                    // ---- 2. Early exit if depth test fails ----
                    if (RasterizerCommon::DepthTest(depth, storedDepth, rasterizerState.depthFunc))
                    {
                        // Update depth buffer if needed
                        if (rasterizerState.depthWriteEnable)
                            depthBuffer.At(int2(x, y)) = depth;

                        // ---- 3. Full attribute interpolation only when visible ----
                        if (renderTarget)
                        {
                            Interpolant fragment;

                            // Perspective‑correct attributes
                            for(int attr = 0; attr < SOFT_MAX_ATTRIBUTES_COUNT; ++attr)
                                fragment.Attributes[attr] = (weight0 * setup.v0.Attributes[attr] + 
                                                             weight1 * setup.v1.Attributes[attr] + 
                                                             weight2 * setup.v2.Attributes[attr]) * inverseTotalWeight;

                            // Screen‑space linear interpolation
                            fragment.ClipSpacePosition.x = alpha * setup.v0.ClipSpacePosition.x + beta * setup.v1.ClipSpacePosition.x + gamma * setup.v2.ClipSpacePosition.x;
                            fragment.ClipSpacePosition.y = alpha * setup.v0.ClipSpacePosition.y + beta * setup.v1.ClipSpacePosition.y + gamma * setup.v2.ClipSpacePosition.y;
                            fragment.ClipSpacePosition.z = interpolatedZ;  // already computed
                            fragment.ClipSpacePosition.w = interpolatedW;

                            // Execute pixel shader and write to render target
                            renderTarget->StreamWrite(uint2(x, y), pixelShader(fragment, constantBuffer, *textureTable).simd_);
                        }
                    }
                }

                // Step the edge functions and barycentrics horizontally
                edge01 += setup.stepX01;
                edge12 += setup.stepX12;
                edge20 += setup.stepX20;
                alpha  += setup.faStepX;
                beta   += setup.fbStepX;
                gamma  += setup.fcStepX;
            }

            // Step the row start values vertically
            edge01Row += setup.stepY01;
            edge12Row += setup.stepY12;
            edge20Row += setup.stepY20;
            barycentricAlphaRow += setup.faStepY;
            barycentricBetaRow  += setup.fbStepY;
            barycentricGammaRow += setup.fcStepY;
        }
    }

} // namespace Rasterizer

SOFTX_END
/////////////////////////////////////////////////////////////////
