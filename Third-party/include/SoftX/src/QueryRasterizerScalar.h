/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "../include/QueryRasterizerInterface.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class SOFTX_API QueryRasterizerScalar : public IQueryRasterizer
{
  public:
    uint RasterizeTriangle(const VertexOutput& v0,
                           const VertexOutput& v1,
                           const VertexOutput& v2,
                           const RasterizerState& state,
                           DepthBuffer& depthBuffer,
                           const ConstantBuffer& cb,
                           uint2 tileMin,
                           uint2 tileMax) override;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
