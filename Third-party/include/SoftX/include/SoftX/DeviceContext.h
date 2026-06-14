#pragma once

#include "LibInternal.h"
#include "RasterizerInterface.h"
#include "RenderTargetInterface.h"
#include "ThirdPartyIncluding.h"
#include "Types.h"

SOFTX_BEGIN

class SOFTX_API DeviceContext
{
public:
    DeviceContext();
    ~DeviceContext();

    DeviceContext(DeviceContext&&) = default;
    DeviceContext& operator=(DeviceContext&&) = default;

    void SetVertexShader(VertexShader shader);
    VertexShader GetVertexShader() const;

    void SetGeometryShader(GeometryShader shader);
    GeometryShader GetGeometryShader() const;

    void SetPixelShader(PixelShader shader);
    PixelShader GetPixelShader() const;

    void SetVertexBuffer(const VertexBuffer& buffer);
    VertexBuffer GetVertexBuffer() const;

    void SetIndexBuffer(const IndexBuffer& buffer);
    IndexBuffer GetIndexBuffer() const;

    void SetConstantBuffer(const ConstantBuffer& buffer);
    ConstantBuffer GetConstantBuffer() const;

    void SetTexture(const std::string& name, const ITexture* texture, SamplerState sampler = SamplerState{});

    void SetRenderTarget(IRenderTarget* target);
    void SetRenderTarget(IRenderTarget* target, bool createDepthBuffer = true);
    IRenderTarget* GetRenderTarget() const;

    // Depth buffer management methods
    void SetDepthBuffer(DepthBuffer* depthBuffer);
    DepthBuffer* GetDepthBuffer() const;

    void SetDepthWriteEnable(bool enable);
    bool GetDepthWriteEnable() const;

    void Clear(const float4& color);
    void ClearDepth(float depth = 1.0f);
    void ClearColorAndDepth(const float4& color, float depth = 1.0f);

    void SetCullMode(CullMode mode);
    CullMode GetCullMode() const;

    void SetFillMode(FillMode mode);
    FillMode GetFillMode() const;

    void SetDepthFunc(ComparisonFunc func);
    ComparisonFunc GetDepthFunc() const;

    void SetViewport(const Viewport& vp);
    Viewport GetViewport() const;

    void SetTileSize(uint size);
    uint GetTileSize() const;

    bool Validate(std::string* errorMsg = nullptr) const;

    void DrawIndexed(uint indexCount, uint startIndex);
    void DrawIndexed();

    void DrawFullScreenQuad();

private:
    // Private methods (all PascalCase)
    void RenderTileQuad(const Tile& tile, float invW, float invH);
    void DrawPoint(int x, int y, float z, const float4& color);
    void DrawLine(int x0, int y0, int x1, int y1, float z0, float z1, const float4& color);
    void DrawDebugLine(int x0, int y0, int x1, int y1, const float4& color);
    void DrawTileBorders();
    void DrawActiveTileBorders(const std::vector<Tile>& tiles);

    // Fields (camelCase, no m_ prefix)
    VertexShader vertexShader;
    GeometryShader geometryShader;
    PixelShader pixelShader;

    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;
    ConstantBuffer constantBuffer;
    TextureTable textureTable;

    std::unique_ptr<DepthBuffer> ownDepthBuffer;
    DepthBuffer* depthBuffer;
    IRenderTarget* renderTarget;

    std::unique_ptr<IRasterizer> rasterizer;

    CullMode cullMode = CullMode::Back;
    FillMode fillMode = FillMode::Solid;
    ComparisonFunc depthFunc = ComparisonFunc::Less;
    bool depthWriteEnable;

    Viewport viewport;

    uint tileSize;
};

SOFTX_END
