/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include <memory>

#include "LibInternal.h"
#include "Types.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class OcclusionQuery
{
public:
    using queryID = uint;
    using OcclusionVertexShader = std::function<Interpolant(const Vertex&, const ConstantBuffer&)>;

    OcclusionQuery();
    ~OcclusionQuery();

    OcclusionQuery(const OcclusionQuery&) = delete;
    OcclusionQuery& operator=(const OcclusionQuery&) = delete;

    void SetVertexBuffer(const VertexBuffer& vb);
    void SetIndexBuffer(const IndexBuffer& ib);
    void SetConstantBuffer(const ConstantBuffer& cb);
    void SetVertexShader(OcclusionVertexShader vs);
    void SetDepthBuffer(std::shared_ptr<DepthBuffer> db);
    void SetViewport(const Viewport& vp);
    void SetCullMode(CullMode mode);
    void SetDepthFunc(ComparisonFunc func);
    void SetDepthWriteEnable(bool enable);
    void SetTileSize(uint size);
    void SetScissorRect(int left, int top, int right, int bottom);
    void SetScissorRect(const Rect& rect);
    void SetScissorEnable(bool enable);

    bool IsReady() const;

    bool GetData(uint* outVisibleSamples = nullptr) const;
    bool GetResult(queryID id, uint* outSamples) const;

    void Begin();
    queryID DrawIndexed();
    void End();
    bool Flush(uint* outVisibleSamples = nullptr);
    void Release();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
