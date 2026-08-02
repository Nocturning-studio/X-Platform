/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "../include/SoftX.h"
#include "QueryRasterizer.h"
#include "ThreadPoolManager.h"
#include "TileGrid.h"
#include "InternalTypes.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class OcclusionQuery::Impl
{
public:
    using OcclusionVertexShader = OcclusionQuery::OcclusionVertexShader;
    using queryID = OcclusionQuery::queryID;

    struct DrawCall
    {
        VertexBuffer vb;
        IndexBuffer ib;
        ConstantBuffer constantBuffer;
        OcclusionVertexShader vertexShader;
        uint visibleSamples = 0;
    };

    OcclusionPipelineState state;
    std::unique_ptr<std::mutex> stateMutex;
    std::vector<DrawCall> drawCalls;
    std::future<void> future;
    std::atomic<bool> ready{ false };
    bool begun = false;
    bool ended = false;
    uint totalVisibleSamples = 0;

    Impl() : stateMutex(std::make_unique<std::mutex>())
    {
    }

    ~Impl()
    {
        if (future.valid())
            future.wait();
    }

    void Begin()
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

    queryID DrawIndexed()
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

    void End()
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

    bool Flush(uint* outVisibleSamples)
    {
        PROFILE_SCOPE("OcclusionQuery::Flush");

        if (!ended)
            return false;

        if (future.valid())
            future.wait();

        ready = true;
        return GetData(outVisibleSamples);
    }

    void Release()
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

    bool GetData(uint* outVisibleSamples) const
    {
        if (!ready)
        {
            if (outVisibleSamples) *outVisibleSamples = 0;
            return false;
        }
        if (outVisibleSamples) *outVisibleSamples = totalVisibleSamples;
        return totalVisibleSamples > 0;
    }

    bool GetResult(queryID id, uint* outSamples) const
    {
        if (!ready || id >= drawCalls.size()) return false;
        const DrawCall& dc = drawCalls[id];
        if (outSamples) *outSamples = dc.visibleSamples;
        return dc.visibleSamples > 0;
    }

private:
    static void ProcessDrawCall(DrawCall& dc,
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
        rasterState.scissorEnable = pso.scissorEnable;
        rasterState.scissorRect = pso.scissorRect;

        const auto& vbData = *dc.vb;
        const auto& ibData = dc.ib;
        const size_t indexCount = ibData.Size();
        if (indexCount < 3)
            return;

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

        // Collect triangle setups after clipping and perspective divide
        std::vector<RasterizerCommon::TriangleSetup> setups;
        setups.reserve(indexCount / 3); // rough estimate

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
                if (optSetup)
                    setups.push_back(std::move(*optSetup));
            }
        }

        if (setups.empty())
        {
            dc.visibleSamples = 0;
            return;
        }

        // Tile binning
        const uint tileSize = pso.tileSize;
        TileGrid tileGrid;
        tileGrid.Build(db.Width(), db.Height(), tileSize, rasterState.scissorEnable, rasterState.scissorRect);
        tileGrid.BinTriangles(setups);

        const auto& tiles = tileGrid.GetTiles();

        uint32_t localVisible = 0;

        // Rasterise all triangles tile by tile
        for (const auto& tile : tiles)
        {
            for (int setupIndex : tile.triangleIndices)
            {
                const RasterizerCommon::TriangleSetup& s = setups[setupIndex];
                localVisible += QueryRasterizer::RasterizeTriangle(s, rasterState, db, pso.viewport, tile.min, tile.max);
            }
        }

        dc.visibleSamples = localVisible;
        totalVisible += localVisible;
    }
};

OcclusionQuery::OcclusionQuery() : pImpl(std::make_unique<Impl>()) {}

OcclusionQuery::~OcclusionQuery() = default;

void OcclusionQuery::SetVertexBuffer(const VertexBuffer& vb)
{
    std::lock_guard<std::mutex> lock(*pImpl->stateMutex);
    pImpl->state.vertexBuffer = vb;
}

void OcclusionQuery::SetIndexBuffer(const IndexBuffer& ib)
{
    std::lock_guard<std::mutex> lock(*pImpl->stateMutex);
    pImpl->state.indexBuffer = ib;
}

void OcclusionQuery::SetConstantBuffer(const ConstantBuffer& cb)
{
    std::lock_guard<std::mutex> lock(*pImpl->stateMutex);
    pImpl->state.constantBuffer = cb;
}

void OcclusionQuery::SetVertexShader(OcclusionVertexShader vs)
{
    std::lock_guard<std::mutex> lock(*pImpl->stateMutex);
    pImpl->state.vertexShader = std::move(vs);
}

void OcclusionQuery::SetDepthBuffer(std::shared_ptr<DepthBuffer> db)
{
    std::lock_guard<std::mutex> lock(*pImpl->stateMutex);
    pImpl->state.depthBuffer = std::move(db);
}

void OcclusionQuery::SetViewport(const Viewport& vp)
{
    std::lock_guard<std::mutex> lock(*pImpl->stateMutex);
    pImpl->state.viewport = vp;
}

void OcclusionQuery::SetCullMode(CullMode mode)
{
    std::lock_guard<std::mutex> lock(*pImpl->stateMutex);
    pImpl->state.cullMode = mode;
}

void OcclusionQuery::SetDepthFunc(ComparisonFunc func)
{
    std::lock_guard<std::mutex> lock(*pImpl->stateMutex);
    pImpl->state.depthFunc = func;
}

void OcclusionQuery::SetDepthWriteEnable(bool enable)
{
    std::lock_guard<std::mutex> lock(*pImpl->stateMutex);
    pImpl->state.depthWriteEnable = enable;
}

void OcclusionQuery::SetTileSize(uint size)
{
    std::lock_guard<std::mutex> lock(*pImpl->stateMutex);
    pImpl->state.tileSize = size;
}

void OcclusionQuery::SetScissorRect(int left, int top, int right, int bottom)
{
    assert(right >= left && bottom >= top);
    uint l = static_cast<uint>(std::max(0, left));
    uint t = static_cast<uint>(std::max(0, top));
    uint r = static_cast<uint>(std::max(0, right));
    uint b = static_cast<uint>(std::max(0, bottom));

    if (r <= l || b <= t)
        l = t = r = b = 0;

    std::lock_guard<std::mutex> lock(*pImpl->stateMutex);
    pImpl->state.scissorRect = Rect(l, t, r - l, b - t);
}

void OcclusionQuery::SetScissorRect(const Rect& rect)
{
    std::lock_guard<std::mutex> lock(*pImpl->stateMutex);
    pImpl->state.scissorRect = rect;
}

void OcclusionQuery::SetScissorEnable(bool enable)
{
    std::lock_guard<std::mutex> lock(*pImpl->stateMutex);
    pImpl->state.scissorEnable = enable;
}

bool OcclusionQuery::IsReady() const
{
    return pImpl->ready;
}

bool OcclusionQuery::GetData(uint* outVisibleSamples) const
{
    return pImpl->GetData(outVisibleSamples);
}

bool OcclusionQuery::GetResult(queryID id, uint* outSamples) const
{
    return pImpl->GetResult(id, outSamples);
}

void OcclusionQuery::Begin()
{
    pImpl->Begin();
}

OcclusionQuery::queryID OcclusionQuery::DrawIndexed()
{
    return pImpl->DrawIndexed();
}

void OcclusionQuery::End()
{
    pImpl->End();
}

bool OcclusionQuery::Flush(uint* outVisibleSamples)
{
    return pImpl->Flush(outVisibleSamples);
}

void OcclusionQuery::Release()
{
    pImpl->Release();
}

SOFTX_END
/////////////////////////////////////////////////////////////////
