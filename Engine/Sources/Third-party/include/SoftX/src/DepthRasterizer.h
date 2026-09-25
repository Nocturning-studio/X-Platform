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

namespace DepthRasterizer
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

        // Align by 4 pixels (__m128 block)
        int startX = boundingBoxMinX & ~3;

        __m128i stepX01_x4 = _mm_set1_epi32(setup.stepX01 * 4);
        __m128i stepX12_x4 = _mm_set1_epi32(setup.stepX12 * 4);
        __m128i stepX20_x4 = _mm_set1_epi32(setup.stepX20 * 4);

        // Precompute Depth Constants and gradients
        float deltaZ = viewport.maxZ - viewport.minZ;

        float C0 = setup.invArea2 * setup.v0.ClipSpacePosition.w * setup.v0.ClipSpacePosition.z * deltaZ;
        float C1 = setup.invArea2 * setup.v1.ClipSpacePosition.w * setup.v1.ClipSpacePosition.z * deltaZ;
        float C2 = setup.invArea2 * setup.v2.ClipSpacePosition.w * setup.v2.ClipSpacePosition.z * deltaZ;

        // X depth gradient per pixel
        float dz_dx = (float)setup.stepX12 * C0 + (float)setup.stepX20 * C1 + (float)setup.stepX01 * C2;

        __m128 dz_dx_x4 = _mm_set1_ps(dz_dx * 4.0f);
        __m128 dz_dx_ramp = _mm_mul_ps(_mm_set1_ps(dz_dx), _mm_setr_ps(0.0f, 1.0f, 2.0f, 3.0f));

        // Index vector for masking [0, 1, 2, 3]
        __m128i ramp = _mm_setr_epi32(0, 1, 2, 3);

        int px_base = RasterizerCommon::PixelCentre(startX);

        for (int y = boundingBoxMinY; y <= boundingBoxMaxY; ++y)
        {
            int py = RasterizerCommon::PixelCentre(y);

            // Calculate the edge values for the first pixel of the block in the row
            int32_t e01_row = setup.normSign * RasterizerCommon::EdgeFunctionInt(setup.x0fp, setup.y0fp, setup.x1fp, setup.y1fp, px_base, py);
            int32_t e12_row = setup.normSign * RasterizerCommon::EdgeFunctionInt(setup.x1fp, setup.y1fp, setup.x2fp, setup.y2fp, px_base, py);
            int32_t e20_row = setup.normSign * RasterizerCommon::EdgeFunctionInt(setup.x2fp, setup.y2fp, setup.x0fp, setup.y0fp, px_base, py);

            // Initialize edge vectors for 4 pixels at once
            __m128i e01 = _mm_add_epi32(_mm_set1_epi32(e01_row), _mm_setr_epi32(0, setup.stepX01, setup.stepX01 * 2, setup.stepX01 * 3));
            __m128i e12 = _mm_add_epi32(_mm_set1_epi32(e12_row), _mm_setr_epi32(0, setup.stepX12, setup.stepX12 * 2, setup.stepX12 * 3));
            __m128i e20 = _mm_add_epi32(_mm_set1_epi32(e20_row), _mm_setr_epi32(0, setup.stepX20, setup.stepX20 * 2, setup.stepX20 * 3));

            // Setup exact depth for the first 4 pixels of this row block
            float z_base_row = viewport.minZ + (float)e12_row * C0 + (float)e20_row * C1 + (float)e01_row * C2;
            __m128 current_depths = _mm_add_ps(_mm_set1_ps(z_base_row), dz_dx_ramp);

            for (int x = startX; x <= boundingBoxMaxX; x += 4)
            {
                // Check if inside the triangle (all edges >= 0)
                __m128i maskInside = _mm_or_si128(_mm_or_si128(e01, e12), e20);
                maskInside = _mm_cmpgt_epi32(maskInside, _mm_set1_epi32(-1));

                // Check tile/BB bounds (mask extra pixels in the aligned block)
                __m128i x_vec = _mm_add_epi32(_mm_set1_epi32(x), ramp);
                __m128i maskBounds = _mm_and_si128(_mm_cmpgt_epi32(x_vec, _mm_set1_epi32(boundingBoxMinX - 1)), _mm_cmpgt_epi32(_mm_set1_epi32(boundingBoxMaxX + 1), x_vec));

                __m128i finalActiveMaskI = _mm_and_si128(maskInside, maskBounds);
                if (_mm_movemask_ps(_mm_castsi128_ps(finalActiveMaskI)) == 0)
                {
                    e01 = _mm_add_epi32(e01, stepX01_x4);
                    e12 = _mm_add_epi32(e12, stepX12_x4);
                    e20 = _mm_add_epi32(e20, stepX20_x4);
                    current_depths = _mm_add_ps(current_depths, dz_dx_x4); // Step Depth as well!
                    continue;
                }

                // Just assign our incrementally calculated depths
                __m128 depths = current_depths;

                __m128 storedDepths = depthBuffer.Read4(uint2(x, y));
                __m128 depthPass;

                switch (rasterizerState.depthFunc)
                {
                case ComparisonFunc::Never:        depthPass = _mm_setzero_ps(); break;
                case ComparisonFunc::Less:         depthPass = _mm_cmplt_ps(depths, storedDepths); break;
                case ComparisonFunc::Equal:        depthPass = _mm_cmpeq_ps(depths, storedDepths); break;
                case ComparisonFunc::LessEqual:    depthPass = _mm_cmple_ps(depths, storedDepths); break;
                case ComparisonFunc::Greater:      depthPass = _mm_cmpgt_ps(depths, storedDepths); break;
                case ComparisonFunc::NotEqual:     depthPass = _mm_cmpneq_ps(depths, storedDepths); break;
                case ComparisonFunc::GreaterEqual: depthPass = _mm_cmpge_ps(depths, storedDepths); break;
                case ComparisonFunc::Always:       depthPass = _mm_castsi128_ps(_mm_set1_epi32(-1)); break;
                default:                           depthPass = _mm_setzero_ps(); break;
                }

                __m128 visibilityMask = _mm_and_ps(_mm_castsi128_ps(finalActiveMaskI), depthPass);
                int moveMask = _mm_movemask_ps(visibilityMask);

                if (moveMask != 0)
                {
                    if (rasterizerState.depthWriteEnable)
                        depthBuffer.Write4(uint2(x, y), depths, visibilityMask);

                    visibleCount += __popcnt(moveMask);
                }

                // Advance X increments
                e01 = _mm_add_epi32(e01, stepX01_x4);
                e12 = _mm_add_epi32(e12, stepX12_x4);
                e20 = _mm_add_epi32(e20, stepX20_x4);
                current_depths = _mm_add_ps(current_depths, dz_dx_x4);
            }
        }

        return visibleCount;
    }

} // namespace DepthRasterizer

SOFTX_END
/////////////////////////////////////////////////////////////////
