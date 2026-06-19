/////////////////////////////////////////////////////////////////
// SoftX – Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "pch.h"

#include <SoftX.h>
#include "RasterizerCommon.h"
#include "QueryRasterizerFactory.h"
#include "ThreadPoolManager.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

OcclusionQuery::OcclusionQuery()
{
    rasterizer = CreateBestQueryRasterizer();
}

OcclusionQuery::~OcclusionQuery()
{
    if (future.valid())
        future.wait();
}

void OcclusionQuery::SetVertexBuffer(const VertexBuffer& vb)
{
    currentVB = vb;
}

void OcclusionQuery::SetIndexBuffer(const IndexBuffer& ib)
{
    currentIB = ib;
}

void OcclusionQuery::SetConstantBuffer(const ConstantBuffer& cb)
{
    currentCB = cb;
}

void OcclusionQuery::SetVertexShader(OcclusionVertexShader vs)
{
    currentVS = std::move(vs);
}

void OcclusionQuery::SetDepthBuffer(DepthBuffer& db)
{
    depthBuffer = &db;
}

void OcclusionQuery::SetViewport(const Viewport& vp)
{
    viewport = vp;
}

void OcclusionQuery::SetCullMode(CullMode mode)
{
    cullMode = mode;
}

void OcclusionQuery::SetDepthFunc(ComparisonFunc func)
{
    depthFunc = func;
}

void OcclusionQuery::SetDepthWriteEnable(bool enable)
{
    depthWriteEnable = enable;
}

bool OcclusionQuery::Validate() const
{
    if (depthBuffer == nullptr) return false;
    if (viewport.size.x <= 0 || viewport.size.y <= 0) return false;

    if (currentVB.IsEmpty()) return false;
    if (currentIB.IsEmpty()) return false;
    if (!currentVS) return false;

    if (!begun || ended) return false;

    return true;
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
    assert(Validate() && "OcclusionQuery: state is invalid before DrawIndexed");

    DrawCall dc;
    dc.vb = currentVB;
    dc.ib = currentIB;
    dc.constantBuffer = currentCB;
    dc.vertexShader = std::move(currentVS);
    dc.visibleSamples = 0;

    queryID id = static_cast<queryID>(drawCalls.size());
    drawCalls.push_back(std::move(dc));
    return id;
}

void OcclusionQuery::End()
{
    PROFILE_SCOPE("OcclusionQuery::End");

    assert(begun && !ended);
    ended = true;
    begun = false;

    auto drawCallsCopy = std::make_shared<std::vector<DrawCall>>(drawCalls);
    DepthBuffer* db = depthBuffer;
    Viewport vpData = viewport;
    IQueryRasterizer* rast = rasterizer.get();

    ready = false;
    totalVisibleSamples = 0;

    future = std::async(std::launch::async, [this, drawCallsCopy, db, vpData, rast]()
        {
            PROFILE_THREAD("OcclusionQuery::AsyncExecution");
            PROFILE_SCOPE("OcclusionQuery::AsyncExecution");
            std::atomic<uint32_t> totalVisible(0);

            std::atomic<size_t> idx(0);
            size_t count = drawCallsCopy->size();

            auto& pool = ThreadPoolManager::Get();
            for (size_t t = 0; t < pool.threadCount(); ++t)
            {
                pool.enqueue([&]()
                    {
                        while (true)
                        {
                            size_t i = idx.fetch_add(1);
                            if (i >= count) break;
                            ProcessDrawCall((*drawCallsCopy)[i], *db, vpData, *rast, totalVisible);
                        }
                    });
            }
            pool.wait();

            for (size_t i = 0; i < drawCalls.size(); ++i)
                drawCalls[i].visibleSamples = (*drawCallsCopy)[i].visibleSamples;

            totalVisibleSamples = totalVisible.load();
            ready = true;
        });
}

bool OcclusionQuery::GetData(uint* outVisibleSamples) const
{
    PROFILE_SCOPE("OcclusionQuery::GetData");

    if (!ready)
    {
        if (outVisibleSamples) *outVisibleSamples = 0;
        return false;
    }

    if (outVisibleSamples)
        *outVisibleSamples = totalVisibleSamples;

    return totalVisibleSamples > 0;
}

bool OcclusionQuery::GetResult(queryID id, uint* outSamples) const
{
    PROFILE_SCOPE("OcclusionQuery::GetResult");

    if (!ready || id >= drawCalls.size())
        return false;
    const DrawCall& dc = drawCalls[id];
    if (outSamples)
        *outSamples = dc.visibleSamples;
    return dc.visibleSamples > 0;
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

bool OcclusionQuery::IsReady() const
{
    return ready;
}

void OcclusionQuery::Release()
{
    PROFILE_SCOPE("OcclusionQuery::Release");

    if (future.valid())
        future.wait();
    rasterizer.reset();
    drawCalls.clear();
    depthBuffer = nullptr;
    begun = ended = false;
    ready = false;
}

void OcclusionQuery::ProcessDrawCall(DrawCall& dc,
                                     DepthBuffer& db,
                                     const Viewport& vp,
                                     IQueryRasterizer& rasterzer,
                                     std::atomic<uint>& totalVisible)
{
    PROFILE_SCOPE("OcclusionQuery::ProcessDrawCall");

    RasterizerState state;
    state.cullMode = cullMode;
    state.depthFunc = depthFunc;
    state.depthWriteEnable = depthWriteEnable;

    const auto& vbData = *dc.vb;
    const auto& ibData = dc.ib;
    const size_t indexCount = ibData.Size();
    if (indexCount < 3) return;

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

    std::vector<VertexOutput> transformedVerts(vertexCount);
    for (uint32_t idx : uniqueIndices)
    {
        transformedVerts[idx] = dc.vertexShader(vbData[idx], dc.constantBuffer);
    }

    uint32_t localVisible = 0;
    for (size_t i = 0; i + 2 < indexCount; i += 3)
    {
        uint32_t i0 = ibData.GetByIndex(static_cast<uint>(i));
        uint32_t i1 = ibData.GetByIndex(static_cast<uint>(i + 1));
        uint32_t i2 = ibData.GetByIndex(static_cast<uint>(i + 2));

        VertexOutput v0 = transformedVerts[i0];
        VertexOutput v1 = transformedVerts[i1];
        VertexOutput v2 = transformedVerts[i2];

        // Near-plane clipping (всё ещё необходимо)
        VertexOutput clipped[2][3];
        int numTris = RasterizerCommon::ClipTriangleNearPlane(v0, v1, v2, clipped);
        for (int t = 0; t < numTris; ++t)
        {
            for (int j = 0; j < 3; ++j)
                RasterizerCommon::ClipSpaceToScreenSpace(clipped[t][j], vp);

            const VertexOutput& tv0 = clipped[t][0];
            const VertexOutput& tv1 = clipped[t][1];
            const VertexOutput& tv2 = clipped[t][2];

            float minX = std::min({ tv0.Position.x, tv1.Position.x, tv2.Position.x });
            float maxX = std::max({ tv0.Position.x, tv1.Position.x, tv2.Position.x });
            float minY = std::min({ tv0.Position.y, tv1.Position.y, tv2.Position.y });
            float maxY = std::max({ tv0.Position.y, tv1.Position.y, tv2.Position.y });

            int tileMinX = std::max(0, (int)std::floor(minX));
            int tileMinY = std::max(0, (int)std::floor(minY));
            int tileMaxX = std::min((int)db.Width() - 1, (int)std::ceil(maxX));
            int tileMaxY = std::min((int)db.Height() - 1, (int)std::ceil(maxY));

            if (tileMinX > tileMaxX || tileMinY > tileMaxY)
                continue;

            localVisible += rasterzer.RasterizeTriangle( tv0, tv1, tv2,
                                                         state,
                                                         db,
                                                         dc.constantBuffer,
                                                         uint2(tileMinX, tileMinY),
                                                         uint2(tileMaxX, tileMaxY) );
        }
    }

    dc.visibleSamples = localVisible;
    totalVisible += localVisible;
}

SOFTX_END
/////////////////////////////////////////////////////////////////
