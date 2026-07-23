/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "../include/SoftX.h"
#include "RasterizerCommon.h"
#include "QueryRasterizer.h"
#include "ThreadPoolManager.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

OcclusionQuery::OcclusionQuery() : stateMutex(std::make_unique<std::mutex>())
{
}

OcclusionQuery::~OcclusionQuery()
{
    if (future.valid())
        future.wait();
}

void OcclusionQuery::SetVertexBuffer(const VertexBuffer& vb) 
{
    std::lock_guard<std::mutex> lock(*stateMutex);
    state.vertexBuffer = vb;
}

void OcclusionQuery::SetIndexBuffer(const IndexBuffer& ib) 
{
    std::lock_guard<std::mutex> lock(*stateMutex);
    state.indexBuffer = ib;
}

void OcclusionQuery::SetConstantBuffer(const ConstantBuffer& cb) 
{
    std::lock_guard<std::mutex> lock(*stateMutex);
    state.constantBuffer = cb;
}

void OcclusionQuery::SetVertexShader(OcclusionVertexShader vs) 
{
    std::lock_guard<std::mutex> lock(*stateMutex);
    state.vertexShader = std::move(vs);
}

void OcclusionQuery::SetDepthBuffer(std::shared_ptr<DepthBuffer> db) 
{
    std::lock_guard<std::mutex> lock(*stateMutex);
    state.depthBuffer = std::move(db);
}

void OcclusionQuery::SetViewport(const Viewport& vp) 
{
    std::lock_guard<std::mutex> lock(*stateMutex);
    state.viewport = vp;
}

void OcclusionQuery::SetCullMode(CullMode mode) 
{
    std::lock_guard<std::mutex> lock(*stateMutex);
    state.cullMode = mode;
}

void OcclusionQuery::SetDepthFunc(ComparisonFunc func) 
{
    std::lock_guard<std::mutex> lock(*stateMutex);
    state.depthFunc = func;
}

void OcclusionQuery::SetDepthWriteEnable(bool enable) 
{
    std::lock_guard<std::mutex> lock(*stateMutex);
    state.depthWriteEnable = enable;
}

void OcclusionQuery::Begin()
{
    PROFILE_SCOPE("OcclusionQuery::Begin");

    if (future.valid() && !ready)
        future.wait();

    drawCalls.clear();
    ready = false;
    totalVisibleSamples = 0;
    begun = true;
    ended = false;
}

OcclusionQuery::queryID OcclusionQuery::DrawIndexed()
{
    if (!begun || ended)
        SOFTX_THROW(InvalidState("OcclusionQuery::DrawIndexed called outside Begin/End"));

    OcclusionPipelineState stateCaptured;
    {
        std::lock_guard<std::mutex> lock(*stateMutex);
        stateCaptured = state;
    }

    if (stateCaptured.vertexBuffer.IsEmpty() || stateCaptured.indexBuffer.IsEmpty() || !stateCaptured.vertexShader)
        SOFTX_THROW(InvalidState("OcclusionQuery: vertex buffer, index buffer or vertex shader not set"));

    DrawCall dc;
    dc.vb = stateCaptured.vertexBuffer;
    dc.ib = stateCaptured.indexBuffer;
    dc.constantBuffer = stateCaptured.constantBuffer;
    dc.vertexShader = stateCaptured.vertexShader;
    dc.visibleSamples = 0;

    queryID id = static_cast<queryID>(drawCalls.size());
    drawCalls.push_back(std::move(dc));
    return id;
}

void OcclusionQuery::End()
{
    PROFILE_SCOPE("OcclusionQuery::End");

    if (!begun || ended)
        SOFTX_THROW(InvalidState("OcclusionQuery::End called without Begin"));

    ended = true;
    begun = false;

    OcclusionPipelineState stateCaptured;
    {
        std::lock_guard<std::mutex> lock(*stateMutex);
        stateCaptured = state;
    }

    auto drawCallsCopy = std::make_shared<std::vector<DrawCall>>(drawCalls);
    DepthBuffer* db = stateCaptured.depthBuffer.get();
    Viewport vpData = stateCaptured.viewport;

    ready = false;
    totalVisibleSamples = 0;

    future = ThreadPoolManager::Get().enqueueBackground([this, drawCallsCopy, stateCaptured, db]()
    {
        PROFILE_THREAD("OcclusionQuery::AsyncExecution");
        PROFILE_SCOPE("OcclusionQuery::AsyncExecution");
        std::atomic<uint32_t> totalVisible(0);

        size_t count = drawCallsCopy->size();
        for (size_t i = 0; i < count; ++i)
            ProcessDrawCall((*drawCallsCopy)[i], stateCaptured, *db, totalVisible);

        for (size_t i = 0; i < drawCalls.size(); ++i)
            drawCalls[i].visibleSamples = (*drawCallsCopy)[i].visibleSamples;

        totalVisibleSamples = totalVisible.load();
        ready = true;
    });
}

bool OcclusionQuery::Flush(uint* outVisibleSamples)
{
    PROFILE_SCOPE("OcclusionQuery::Flush");

    if (!ended)
        return false;

    if (future.valid())
        future.wait();

    ready = true;
    return GetData(outVisibleSamples);
}

void OcclusionQuery::Release()
{
    PROFILE_SCOPE("OcclusionQuery::Release");

    if (future.valid())
        future.wait();
    drawCalls.clear();
    {
        std::lock_guard<std::mutex> lock(*stateMutex);
        state.depthBuffer.reset();
    }
    begun = ended = false;
    ready = false;
}

bool OcclusionQuery::GetData(uint* outVisibleSamples) const
{
    if (!ready)
    {
        if (outVisibleSamples) *outVisibleSamples = 0;
        return false;
    }
    if (outVisibleSamples) *outVisibleSamples = totalVisibleSamples;
    return totalVisibleSamples > 0;
}

bool OcclusionQuery::GetResult(queryID id, uint* outSamples) const
{
    if (!ready || id >= drawCalls.size()) return false;
    const DrawCall& dc = drawCalls[id];
    if (outSamples) *outSamples = dc.visibleSamples;
    return dc.visibleSamples > 0;
}

void OcclusionQuery::ProcessDrawCall(const DrawCall& dc,
                                     const OcclusionPipelineState& pso,
                                     DepthBuffer& db,
                                     std::atomic<uint>& totalVisible)
{
    PROFILE_SCOPE("OcclusionQuery::ProcessDrawCall");

    // Rasterizer state for both TriangleSetup creation and rasterisation
    RasterizerState rasterState;
    rasterState.cullMode = pso.cullMode;
    rasterState.depthFunc = pso.depthFunc;
    rasterState.depthWriteEnable = pso.depthWriteEnable;

    const auto& vbData = *dc.vb;
    const auto& ibData = dc.ib;
    const size_t indexCount = ibData.Size();
    if (indexCount < 3) return;

    // Gather unique indices
    const size_t vertexCount = vbData.size();
    std::vector<bool> visited(vertexCount, false);
    std::vector<uint32_t> uniqueIndices;
    for (size_t i = 0; i < indexCount; ++i)
    {
        uint32_t idx = ibData.GetByIndex(static_cast<uint>(i));
        if (!visited[idx])
        {
            visited[idx] = true;
            uniqueIndices.push_back(idx);
        }
    }

    // Transform only the unique vertices
    std::vector<Interpolant> transformedVerts(vertexCount);
    for (uint32_t idx : uniqueIndices)
        transformedVerts[idx] = dc.vertexShader(vbData[idx], dc.constantBuffer);

    uint32_t localVisible = 0;
    for (size_t i = 0; i + 2 < indexCount; i += 3)
    {
        uint32_t i0 = ibData.GetByIndex(static_cast<uint>(i));
        uint32_t i1 = ibData.GetByIndex(static_cast<uint>(i + 1));
        uint32_t i2 = ibData.GetByIndex(static_cast<uint>(i + 2));

        Interpolant v0 = transformedVerts[i0];
        Interpolant v1 = transformedVerts[i1];
        Interpolant v2 = transformedVerts[i2];

        // Near-plane clipping
        Interpolant clipped[2][3];
        int numTris = RasterizerCommon::ClipTriangleNearPlane(v0, v1, v2, clipped);
        for (int t = 0; t < numTris; ++t)
        {
            // Perspective divide for all three vertices of the clipped triangle
            for (int j = 0; j < 3; ++j)
                RasterizerCommon::ClipSpaceToScreenSpace(clipped[t][j], pso.viewport);

            // Pre‑compute triangle setup (culling + edge deltas + invArea)
            auto optSetup = RasterizerCommon::CreateTriangleSetup(clipped[t][0], clipped[t][1], clipped[t][2], rasterState);
            if (!optSetup)
                continue;   // culled or degenerate

            const RasterizerCommon::TriangleSetup& s = *optSetup;

            // Screen-space bounding box from the already transformed vertices
            float minX = std::min({ s.v0.Position.x, s.v1.Position.x, s.v2.Position.x });
            float maxX = std::max({ s.v0.Position.x, s.v1.Position.x, s.v2.Position.x });
            float minY = std::min({ s.v0.Position.y, s.v1.Position.y, s.v2.Position.y });
            float maxY = std::max({ s.v0.Position.y, s.v1.Position.y, s.v2.Position.y });

            int tileMinX = std::max(0, (int)std::floor(minX));
            int tileMinY = std::max(0, (int)std::floor(minY));
            int tileMaxX = std::min((int)db.Width() - 1, (int)std::ceil(maxX));
            int tileMaxY = std::min((int)db.Height() - 1, (int)std::ceil(maxY));

            if (tileMinX > tileMaxX || tileMinY > tileMaxY)
                continue;

            // Rasterise using the pre‑computed setup
            localVisible += QueryRasterizer::RasterizeTriangle(s, rasterState, db, pso.viewport, uint2(tileMinX, tileMinY), uint2(tileMaxX, tileMaxY));
        }
    }

    const_cast<DrawCall&>(dc).visibleSamples = localVisible;
    totalVisible += localVisible;
}

SOFTX_END
/////////////////////////////////////////////////////////////////
