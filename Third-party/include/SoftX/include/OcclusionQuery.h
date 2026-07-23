/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include <memory>
#include <vector>
#include <future>

#include "LibInternal.h"
#include "Types.h"
#include "DepthBuffer.h"
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

    SOFTX_FORCE_INLINE bool IsReady() const { return ready; }

    bool GetData(uint* outVisibleSamples = nullptr) const;
    bool GetResult(queryID id, uint* outSamples) const;

    void Begin();
    queryID DrawIndexed();
    void End();
    bool Flush(uint* outVisibleSamples = nullptr);
    void Release();

private:
    struct DrawCall
    {
        VertexBuffer vb;
        IndexBuffer ib;
        ConstantBuffer constantBuffer;
        OcclusionVertexShader vertexShader;
        uint visibleSamples = 0;
    };

    OcclusionPipelineState state;
    mutable std::unique_ptr<std::mutex> stateMutex;

    std::vector<DrawCall> drawCalls;
    std::future<void> future;

    std::atomic<bool> ready{ false };
    bool begun = false;
    bool ended = false;
    uint totalVisibleSamples = 0;

    void ProcessDrawCall(const DrawCall& dc,
                         const OcclusionPipelineState& state,
                         DepthBuffer& db,
                         std::atomic<uint>& totalVisible);
};

SOFTX_END
/////////////////////////////////////////////////////////////////
