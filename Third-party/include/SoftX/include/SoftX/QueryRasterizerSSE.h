#pragma once

#include "QueryRasterizerInterface.h"

SOFTX_BEGIN

class SOFTX_API QueryRasterizerSSE : public IQueryRasterizer
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
