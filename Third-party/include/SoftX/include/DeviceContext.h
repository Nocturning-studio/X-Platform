/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include <mutex>
#include "LibInternal.h"
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

    explicit DeviceContext(const PipelineStateObject& initialState);

    DeviceContext(const DeviceContext&) = delete;
    DeviceContext& operator=(const DeviceContext&) = delete;

    DeviceContext(DeviceContext&&) = delete;
    DeviceContext& operator=(DeviceContext&&) = delete;

    void SetVertexShader(VertexShader shader);
    SOFTX_FORCE_INLINE VertexShader GetVertexShader() const { return frontState.vertexShader; }

    void SetGeometryShader(GeometryShader shader);
    SOFTX_FORCE_INLINE GeometryShader GetGeometryShader() const { return frontState.geometryShader; }

    void SetPixelShader(PixelShader shader);
    SOFTX_FORCE_INLINE PixelShader GetPixelShader() const { return frontState.pixelShader; }

    void SetVertexBuffer(const VertexBuffer& buffer);
    SOFTX_FORCE_INLINE VertexBuffer GetVertexBuffer() const { return frontState.vertexBuffer; }

    void SetIndexBuffer(const IndexBuffer& buffer);
    SOFTX_FORCE_INLINE IndexBuffer GetIndexBuffer() const { return frontState.indexBuffer; }

    void SetConstantBuffer(const ConstantBuffer& buffer);
    SOFTX_FORCE_INLINE ConstantBuffer GetConstantBuffer() const { return frontState.constantBuffer; }

    void SetTexture(const std::string& name, std::shared_ptr<const ITexture> texture, const SamplerState& sampler = SamplerState{});

    void SetRenderTarget(std::shared_ptr<IRenderTarget> target, bool createDepthBuffer = false);
    SOFTX_FORCE_INLINE std::shared_ptr<IRenderTarget> GetRenderTarget() const { return frontState.renderTarget; }

    void SetDepthBuffer(std::shared_ptr<DepthBuffer> depth);
    SOFTX_FORCE_INLINE std::shared_ptr<DepthBuffer> GetDepthBuffer() const { return frontState.depthBuffer; }

    void SetDepthWriteEnable(bool enable);
    SOFTX_FORCE_INLINE bool GetDepthWriteEnable() const { return frontState.depthWriteEnable; }

    void SetCullMode(CullMode mode);
    SOFTX_FORCE_INLINE CullMode GetCullMode() const { return frontState.cullMode; }

    void SetFillMode(FillMode mode);
    SOFTX_FORCE_INLINE FillMode GetFillMode() const { return frontState.fillMode; }

    void SetDepthFunc(ComparisonFunc func);
    SOFTX_FORCE_INLINE ComparisonFunc GetDepthFunc() const { return frontState.depthFunc; }

    void SetViewport(const Viewport& vp);
    SOFTX_FORCE_INLINE Viewport GetViewport() const { return frontState.viewport; }

    void SetTileSize(uint size);
    SOFTX_FORCE_INLINE uint GetTileSize() const { return frontState.tileSize; }

    void Clear(ClearFlags clearTargetBitMask, const float4& color = {0.0f, 0.0f, 0.0f, 0.0f}, float depth = 1.0f);
    void ClearColor(const float4& color) { Clear(ClearFlags::RenderTarget, color); }
    void ClearDepth(float depth = 1.0f) { Clear(ClearFlags::DepthBuffer, {}, depth); }
    void ClearColorAndDepth(const float4& color, float depth = 1.0f) { Clear(ClearFlags::RenderTarget | ClearFlags::DepthBuffer, color, depth); }

    void Draw(uint vertexCount, uint startVertex);
    void Draw();

    void DrawIndexed(uint indexCount, uint startIndex);
    void DrawIndexed();

    void DrawFullScreenQuad();

private:
    void DrawPoint(const PipelineStateObject& state, int x, int y, float z, const float4& color);
    void DrawLine(const PipelineStateObject& state, int x0, int y0, int x1, int y1, float z0, float z1, const float4& color);
    void DrawDebugLine(const PipelineStateObject& state, int x0, int y0, int x1, int y1, const float4& color);
    void DrawTileBorders(const PipelineStateObject& state);
    void DrawActiveTileBorders(const PipelineStateObject& state, const std::vector<Tile>& tiles);
    std::vector<Interpolant> ProcessIndexedVertices(const PipelineStateObject& state, uint indexCount, uint startIndex, uint totalVertices);
    std::vector<int3> GatherIndexedTriangles(const PipelineStateObject& state, uint indexCount, uint startIndex);
    std::vector<Interpolant> ProcessNonIndexedVertices(const PipelineStateObject& state, uint vertexCount, uint startVertex);
    std::vector<int3> GatherNonIndexedTriangles(uint vertexCount);
    void ClipAndRasterize(const PipelineStateObject& state, std::vector<Interpolant>& clipVerts, const std::vector<int3>& sourceTriangles);
    void DrawImpl(const PipelineStateObject& state, uint vertexCount, uint startVertex);
    void DrawIndexedImpl(const PipelineStateObject& state, uint indexCount, uint startIndex);

    void CommitState();
    PipelineStateObject CaptureState() const;

private:
    PipelineStateObject frontState;
    PipelineStateObject backState;
    mutable std::mutex stateMutex;
    mutable std::mutex drawMutex;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
