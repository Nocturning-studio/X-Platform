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
#include "QueryRasterizerInterface.h"
#include "Types.h"
#include "DepthBuffer.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class OcclusionQuery
{
public:
    using queryID = uint;
    using OcclusionVertexShader = std::function<VertexOutput(const VertexInput&, const ConstantBuffer&)>;

    OcclusionQuery();
    ~OcclusionQuery();

    SOFTX_FORCE_INLINE void SetVertexBuffer(const VertexBuffer& vb) { currentVB = vb; }
    SOFTX_FORCE_INLINE void SetIndexBuffer(const IndexBuffer& ib) { currentIB = ib; }
    SOFTX_FORCE_INLINE void SetConstantBuffer(const ConstantBuffer& cb) { currentCB = cb; }
    SOFTX_FORCE_INLINE void SetVertexShader(OcclusionVertexShader vs) { currentVS = std::move(vs); }
    SOFTX_FORCE_INLINE void SetDepthBuffer(std::shared_ptr<DepthBuffer> db) { depthBuffer = std::move(db); }
    SOFTX_FORCE_INLINE void SetViewport(const Viewport& vp) { viewport = vp; }
    SOFTX_FORCE_INLINE void SetCullMode(CullMode mode) { cullMode = mode; }
    SOFTX_FORCE_INLINE void SetDepthFunc(ComparisonFunc func) { depthFunc = func; }
    SOFTX_FORCE_INLINE void SetDepthWriteEnable(bool enable) { depthWriteEnable = enable; }

    SOFTX_FORCE_INLINE bool IsReady() const { return ready; }

    bool GetData(uint* outVisibleSamples = nullptr) const;
    bool GetResult(queryID id, uint* outSamples) const;

    bool Validate() const;

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

    std::shared_ptr<DepthBuffer> depthBuffer;
    Viewport viewport;
    VertexBuffer currentVB;
    IndexBuffer currentIB;
    ConstantBuffer currentCB;
    OcclusionVertexShader currentVS;

    CullMode cullMode = CullMode::Back;
    ComparisonFunc depthFunc = ComparisonFunc::Less;
    bool depthWriteEnable = false;

    std::vector<DrawCall> drawCalls;
    bool begun = false;
    bool ended = false;

    std::future<void> future;
    std::atomic<bool> ready{ false };
    uint totalVisibleSamples = 0;

    std::unique_ptr<IQueryRasterizer> rasterizer;

    void ProcessDrawCall(DrawCall& dc,
                         DepthBuffer& db,
                         const Viewport& vp,
                         IQueryRasterizer& rasterizer,
                         std::atomic<uint>& totalVisible);
};

SOFTX_END
/////////////////////////////////////////////////////////////////
