/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "../include/LibInternal.h"
#include "../include/DepthBuffer.h"
#include "../include/RenderTargetInterface.h"
#include "RasterizerCommon.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

namespace Rasterizer
{
    static inline void RasterizeTriangle(const RasterizerCommon::TriangleSetup& s,
                                         const RasterizerState& state,
                                         DepthBuffer& depthBuffer,
                                         IRenderTarget* renderTarget,
                                         const Viewport& vp,
                                         const PixelShader& ps,
                                         const ConstantBuffer& cb,
                                         const TextureTable* tt,
                                         uint2 tileMin,
                                         uint2 tileMax)
    {
        const uint width = renderTarget ? renderTarget->Width() : depthBuffer.Width();

        int bbMinX = std::max(static_cast<int>(tileMin.x), s.bbMinX);
        int bbMaxX = std::min(static_cast<int>(tileMax.x), s.bbMaxX);
        int bbMinY = std::max(static_cast<int>(tileMin.y), s.bbMinY);
        int bbMaxY = std::min(static_cast<int>(tileMax.y), s.bbMaxY);
        if (bbMinX > bbMaxX || bbMinY > bbMaxY) return;

        int pcX = RasterizerCommon::PixelCentre(bbMinX);
        int pcY = RasterizerCommon::PixelCentre(bbMinY);

        int32_t f01Row = s.normSign * RasterizerCommon::EdgeFunctionInt(s.x0fp, s.y0fp, s.x1fp, s.y1fp, pcX, pcY);
        int32_t f12Row = s.normSign * RasterizerCommon::EdgeFunctionInt(s.x1fp, s.y1fp, s.x2fp, s.y2fp, pcX, pcY);
        int32_t f20Row = s.normSign * RasterizerCommon::EdgeFunctionInt(s.x2fp, s.y2fp, s.x0fp, s.y0fp, pcX, pcY);

        float faRow = static_cast<float>(f12Row) * s.invArea2;
        float fbRow = static_cast<float>(f20Row) * s.invArea2;
        float fcRow = static_cast<float>(f01Row) * s.invArea2;

        for (int y = bbMinY; y <= bbMaxY; ++y)
        {
            int32_t f01 = f01Row;
            int32_t f12 = f12Row;
            int32_t f20 = f20Row;

            float fa = faRow;
            float fb = fbRow;
            float fc = fcRow;

            for (int x = bbMinX; x <= bbMaxX; ++x)
            {
                if ((f01 | f12 | f20) >= 0)
                {
                    Interpolant frag = RasterizerCommon::Trilerp(s.v0, s.v1, s.v2, fa, fb, fc);
                    float depth = RasterizerCommon::ComputeDepth(frag.Position.z, frag.Position.w, vp);
                    float oldDepth = depthBuffer.At(int2(x, y));

                    if (RasterizerCommon::DepthTest(depth, oldDepth, state.depthFunc))
                    {
                        if (state.depthWriteEnable)
                            depthBuffer.At(int2(x, y)) = depth;

                        if (renderTarget)
                            renderTarget->SetPixel(uint2(x, y), ps(frag, cb, *tt));
                    }
                }

                f01 += s.stepX01;
                f12 += s.stepX12;
                f20 += s.stepX20;
                fa += s.faStepX;
                fb += s.fbStepX;
                fc += s.fcStepX;
            }

            f01Row += s.stepY01;
            f12Row += s.stepY12;
            f20Row += s.stepY20;
            faRow += s.faStepY;
            fbRow += s.fbStepY;
            fcRow += s.fcStepY;
        }
    }

} // namespace Rasterizer

SOFTX_END
/////////////////////////////////////////////////////////////////
