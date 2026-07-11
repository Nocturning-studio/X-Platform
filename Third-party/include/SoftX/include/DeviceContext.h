/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "LibInternal.h"
#include "RasterizerInterface.h"
#include "RenderTargetInterface.h"
#include "ThirdPartyIncluding.h"
#include "Types.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class SOFTX_API DeviceContext
{
public:
    DeviceContext();
    ~DeviceContext();

    DeviceContext(DeviceContext&&) = default;
    DeviceContext& operator=(DeviceContext&&) = default;

    SOFTX_FORCE_INLINE void SetVertexShader(VertexShader shader) { vertexShader = std::move(shader); }
    SOFTX_FORCE_INLINE VertexShader GetVertexShader() const { return vertexShader; }

    SOFTX_FORCE_INLINE void SetGeometryShader(GeometryShader shader) { geometryShader = std::move(shader); }
    SOFTX_FORCE_INLINE GeometryShader GetGeometryShader() const { return geometryShader; }

    SOFTX_FORCE_INLINE void SetPixelShader(PixelShader shader) { pixelShader = std::move(shader); }
    SOFTX_FORCE_INLINE PixelShader GetPixelShader() const { return pixelShader; }

    SOFTX_FORCE_INLINE void SetVertexBuffer(const VertexBuffer& buffer) { vertexBuffer = buffer; }
    SOFTX_FORCE_INLINE VertexBuffer GetVertexBuffer() const { return vertexBuffer; }

    SOFTX_FORCE_INLINE void SetIndexBuffer(const IndexBuffer& buffer) { indexBuffer = buffer; }
    SOFTX_FORCE_INLINE IndexBuffer GetIndexBuffer() const { return indexBuffer; }

    SOFTX_FORCE_INLINE void SetConstantBuffer(const ConstantBuffer& buffer) { constantBuffer = buffer; }
    SOFTX_FORCE_INLINE ConstantBuffer GetConstantBuffer() const { return constantBuffer; }

    SOFTX_FORCE_INLINE void SetTexture(const std::string& name, const std::shared_ptr<ITexture> texture, SamplerState sampler = SamplerState{}) { textureTable.Set(name, texture, sampler); }

    SOFTX_FORCE_INLINE void SetRenderTarget(std::shared_ptr<IRenderTarget> target) { renderTarget = std::move(target); }
    void SetRenderTarget(std::shared_ptr<IRenderTarget> target, bool createDepthBuffer);
    SOFTX_FORCE_INLINE std::shared_ptr<IRenderTarget> GetRenderTarget() const { return renderTarget; }

    SOFTX_FORCE_INLINE void SetDepthBuffer(std::shared_ptr<DepthBuffer> depth) { this->depthBuffer = std::move(depth); }
    SOFTX_FORCE_INLINE std::shared_ptr<DepthBuffer> GetDepthBuffer() const { return depthBuffer; }

    SOFTX_FORCE_INLINE void SetDepthWriteEnable(bool enable) { depthWriteEnable = enable; }
    SOFTX_FORCE_INLINE bool GetDepthWriteEnable() const { return depthWriteEnable; }

    SOFTX_FORCE_INLINE void SetCullMode(CullMode mode) { cullMode = mode; }
    SOFTX_FORCE_INLINE CullMode GetCullMode() const { return cullMode; }

    SOFTX_FORCE_INLINE void SetFillMode(FillMode mode) { fillMode = mode; }
    SOFTX_FORCE_INLINE FillMode GetFillMode() const { return fillMode; }

    SOFTX_FORCE_INLINE void SetDepthFunc(ComparisonFunc func) { depthFunc = func; }
    SOFTX_FORCE_INLINE ComparisonFunc GetDepthFunc() const { return depthFunc; }

    SOFTX_FORCE_INLINE void SetViewport(const Viewport& vp) { viewport = vp; }
    SOFTX_FORCE_INLINE Viewport GetViewport() const { return viewport; }

    SOFTX_FORCE_INLINE void SetTileSize(uint size) { tileSize = size; }
    SOFTX_FORCE_INLINE uint GetTileSize() const { return tileSize; }

    void Clear(const float4& color);
    void ClearDepth(const float& depth = 1.0f);
    void ClearColorAndDepth(const float4& color, const float& depth = 1.0f);
    bool Validate(std::string* errorMsg = nullptr) const;

    void DrawIndexed(uint indexCount, uint startIndex);
    void DrawIndexed();
    void DrawFullScreenQuad();

private:
    void RenderTileQuad(const Tile& tile, float invW, float invH);
    void DrawPoint(int x, int y, float z, const float4& color);
    void DrawLine(int x0, int y0, int x1, int y1, float z0, float z1, const float4& color);
    void DrawDebugLine(int x0, int y0, int x1, int y1, const float4& color);
    void DrawTileBorders();
    void DrawActiveTileBorders(const std::vector<Tile>& tiles);

    VertexShader vertexShader;
    GeometryShader geometryShader;
    PixelShader pixelShader;

    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;
    ConstantBuffer constantBuffer;
    TextureTable textureTable;

    std::shared_ptr<DepthBuffer> depthBuffer;
    std::shared_ptr<IRenderTarget> renderTarget;

    std::unique_ptr<IRasterizer> rasterizer;

    CullMode cullMode = CullMode::Back;
    FillMode fillMode = FillMode::Solid;
    ComparisonFunc depthFunc = ComparisonFunc::Less;
    bool depthWriteEnable = true;

    Viewport viewport;
    uint tileSize = 64;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
