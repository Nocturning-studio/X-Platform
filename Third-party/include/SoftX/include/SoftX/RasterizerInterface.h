#pragma once

#include "LibInternal.h"
#include "RenderTargetInterface.h"
#include "DepthBuffer.h"
#include "Types.h"

SOFTX_BEGIN

struct RasterizerState {
    CullMode cullMode = CullMode::Back;
    FillMode fillMode = FillMode::Solid;
	ComparisonFunc depthFunc = ComparisonFunc::Less;
    bool depthWriteEnable = true;
};

class IRasterizer {
public:
    virtual ~IRasterizer() = default;

    virtual void RasterizeTriangle(const VertexOutput& v0,
                                   const VertexOutput& v1,
                                   const VertexOutput& v2,
                                   const RasterizerState& state,
                                   DepthBuffer& depthBuffer,
                                   IRenderTarget* renderTarget,
                                   const PixelShader& ps,
                                   const ConstantBuffer& cb,
                                   const TextureTable* tt,
                                   uint2 tileMin,
                                   uint2 tileMax) = 0;
};

SOFTX_END
