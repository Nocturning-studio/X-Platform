/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "LibInternal.h"
#include "ThirdPartyIncluding.h"
#include "Types.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class SOFTX_API DeviceContext
{
public:
    DeviceContext();
    explicit DeviceContext(const PipelineStateObject& initialState);
    ~DeviceContext();

    DeviceContext(const DeviceContext&) = delete;
    DeviceContext& operator=(const DeviceContext&) = delete;
    DeviceContext(DeviceContext&&) = delete;
    DeviceContext& operator=(DeviceContext&&) = delete;

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

    void SetTexture(const std::string& name, std::shared_ptr<const Texture> texture, const SamplerState& sampler = SamplerState{});

    void SetRenderTarget(std::shared_ptr<Texture> target, bool createDepthBuffer = false);
    std::shared_ptr<Texture> GetRenderTarget() const;

    void SetDepthBuffer(std::shared_ptr<DepthBuffer> depth);
    std::shared_ptr<DepthBuffer> GetDepthBuffer() const;

    void SetDepthWriteEnable(bool enable);
    bool GetDepthWriteEnable() const;

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

    void SetScissorRect(int left, int top, int right, int bottom);
    void SetScissorRect(const Rect& rect);
    void SetScissorEnable(bool enable);

    bool GetScissorEnable() const;
    Rect GetScissorRect() const;

    void Clear(ClearFlags flags = ClearFlags::All, const float4& color = {0.0f, 0.0f, 0.0f, 0.0f}, float depth = 1.0f);

    // Drawing
    void Draw(uint vertexCount, uint startVertex);
    void Draw();

    void DrawIndexed(uint indexCount, uint startIndex);
    void DrawIndexed();

    void DrawFullScreenQuad();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
