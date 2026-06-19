/////////////////////////////////////////////////////////////////
// SoftX – Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include <memory>
#include <vector>
#include <future>

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

    void SetVertexBuffer(const VertexBuffer& vb);
    void SetIndexBuffer(const IndexBuffer& ib);
    void SetConstantBuffer(const ConstantBuffer& cb);
    void SetVertexShader(OcclusionVertexShader vs);
    void SetDepthBuffer(DepthBuffer& db);
    void SetViewport(const Viewport& vp);
    void SetCullMode(CullMode mode);
    void SetDepthFunc(ComparisonFunc func);
    void SetDepthWriteEnable(bool enable);

    bool Validate() const;

    void Begin();

    queryID DrawIndexed();

    void End();

    bool GetData(uint* outVisibleSamples = nullptr) const;

    bool GetResult(queryID id, uint* outSamples) const;

    bool Flush(uint* outVisibleSamples = nullptr);

    bool IsReady() const;

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

    DepthBuffer* depthBuffer = nullptr;
    Viewport viewport;
    VertexBuffer currentVB;
    IndexBuffer currentIB;
    float4x4 currentWorld;
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
