#pragma once

#include "RasterizerInterface.h"

SOFTX_BEGIN

class IQueryRasterizer {
public:
    virtual ~IQueryRasterizer() = default;

    virtual uint RasterizeTriangle(const VertexOutput& v0,
                                   const VertexOutput& v1,
                                   const VertexOutput& v2,
                                   const RasterizerState& state,
                                   DepthBuffer& depthBuffer,
                                   const ConstantBuffer& cb,
                                   uint2 tileMin,
                                   uint2 tileMax) = 0;
};

SOFTX_END
