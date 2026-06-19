/////////////////////////////////////////////////////////////////
// SoftX – Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "LibInternal.h"
#include "RasterizerInterface.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class SOFTX_API RasterizerScalar : public IRasterizer
{
  public:
    void RasterizeTriangle(const VertexOutput& v0,
                           const VertexOutput& v1,
                           const VertexOutput& v2,
                           const RasterizerState& state,
                           DepthBuffer& depthBuffer,
                           IRenderTarget* renderTarget,
                           const PixelShader& ps,
                           const ConstantBuffer& cb,
                           const TextureTable* tt,
                           uint2 tileMin,
                           uint2 tileMax) override;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
