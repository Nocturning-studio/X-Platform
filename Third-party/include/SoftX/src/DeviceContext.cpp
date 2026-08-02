/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "../include/SoftX.h"
#include "Rasterizer.h"
#include "TileGrid.h"
#include "ThreadPoolManager.h"
#include "ThreadUtils.h"
#include "InternalTypes.h"
/////////////////////////////////////////////////////////////////
//#define DEBUG_TILING
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class DeviceContext::Impl
{
public:
    PipelineStateObject frontState;
    PipelineStateObject backState;
    mutable std::mutex stateMutex;
    mutable std::mutex drawMutex;

    Impl() = default;

    explicit Impl(const PipelineStateObject& initialState): backState(initialState), frontState(initialState)
    {
    }

    VertexShader GetVertexShader() const
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        return frontState.vertexShader;
    }

    GeometryShader GetGeometryShader() const
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        return frontState.geometryShader;
    }

    PixelShader GetPixelShader() const
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        return frontState.pixelShader;
    }

    VertexBuffer GetVertexBuffer() const
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        return frontState.vertexBuffer;
    }

    IndexBuffer GetIndexBuffer() const
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        return frontState.indexBuffer;
    }

    ConstantBuffer GetConstantBuffer() const
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        return frontState.constantBuffer;
    }

    std::shared_ptr<Texture> GetRenderTarget() const
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        return frontState.renderTarget;
    }

    std::shared_ptr<DepthBuffer> GetDepthBuffer() const
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        return frontState.depthBuffer;
    }

    bool GetDepthWriteEnable() const
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        return frontState.depthWriteEnable;
    }

    CullMode GetCullMode() const
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        return frontState.cullMode;
    }

    FillMode GetFillMode() const
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        return frontState.fillMode;
    }

    ComparisonFunc GetDepthFunc() const
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        return frontState.depthFunc;
    }

    Viewport GetViewport() const
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        return frontState.viewport;
    }

    uint GetTileSize() const
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        return frontState.tileSize;
    }

    bool GetScissorEnable() const 
    { 
        std::lock_guard<std::mutex> lock(stateMutex);
        return frontState.scissorEnable;
    };

    Rect GetScissorRect() const 
    { 
        std::lock_guard<std::mutex> lock(stateMutex);
        return frontState.scissorRect;
    };

    void CommitState()
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        frontState = backState;
    }

    PipelineStateObject CaptureState() const
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        return frontState;
    }

    void Clear(ClearFlags flags, const float4& color, float depth)
    {
        if (flags == ClearFlags::None) return;

        std::lock_guard<std::mutex> lock(drawMutex);
        PROFILE_SCOPE("DeviceContext::Clear");
        CommitState();

        PipelineStateObject state = frontState;

        const bool clearColor = !!(flags & ClearFlags::RenderTarget) && state.renderTarget;
        const bool clearDepth = !!(flags & ClearFlags::DepthBuffer) && state.depthBuffer;

        if (!clearColor && !clearDepth) return;

        if (clearColor) state.renderTarget->Clear(color);
        if (clearDepth) state.depthBuffer->Clear(depth);
    }

    void Draw(uint vertexCount, uint startVertex)
    {
        std::lock_guard<std::mutex> lock(drawMutex);
        PROFILE_SCOPE("DeviceContext::Draw (non-indexed)");
        CommitState();
        PipelineStateObject state = frontState;
        ValidateDrawState(state, false);

        if (vertexCount == 0 || startVertex + vertexCount > state.vertexBuffer.Size())
            SOFTX_THROW(InvalidArgument("Draw: vertexCount out of range"));

        DrawImpl(state, vertexCount, startVertex);
    }

    void DrawAll()
    {
        std::lock_guard<std::mutex> lock(drawMutex);
        PROFILE_SCOPE("DeviceContext::Draw (all vertices)");
        CommitState();
        PipelineStateObject state = frontState;
        ValidateDrawState(state, false);

        uint count = static_cast<uint>(state.vertexBuffer.Size());
        if (count == 0)
            SOFTX_THROW(InvalidState("Draw: vertex buffer is empty"));

        DrawImpl(state, count, 0);
    }

    void DrawIndexed(uint indexCount, uint startIndex)
    {
        std::lock_guard<std::mutex> lock(drawMutex);
        PROFILE_SCOPE("DeviceContext::DrawIndexed");
        CommitState();
        PipelineStateObject state = frontState;
        ValidateDrawState(state, true);

        DrawIndexedImpl(state, indexCount, startIndex);
    }

    void DrawIndexedAll()
    {
        std::lock_guard<std::mutex> lock(drawMutex);
        PROFILE_SCOPE("DeviceContext::DrawIndexed (full buffer)");
        CommitState();
        PipelineStateObject state = frontState;
        ValidateDrawState(state, true);

        uint count = static_cast<uint>(state.indexBuffer.Size());
        DrawIndexedImpl(state, count, 0);
    }

    void DrawFullScreenQuad()
    {
        std::lock_guard<std::mutex> lock(drawMutex);
        PROFILE_SCOPE("DeviceContext::DrawFullScreenQuad");
        CommitState();
        PipelineStateObject state = frontState;
        ValidateFullScreenQuadState(state);

        Texture* rt = state.renderTarget.get();
        const uint w = rt->Width();
        const uint h = rt->Height();
        const float invW = 1.0f / (w - 1u);
        const float invH = 1.0f / (h - 1u);
        __m128* pixels = rt->GetRawPixels();

        auto ps = state.pixelShader;
        auto cb = state.constantBuffer;
        auto tt = state.textureTable;

        const uint ts = state.tileSize;
        TileGrid tileGrid;
        tileGrid.Build(w, h, ts, state.scissorEnable, state.scissorRect);
        const auto& tiles = tileGrid.GetTiles();
        uint numTiles = static_cast<uint>(tiles.size());
        std::atomic<uint> tileIndex(0);

        auto worker = [&]()
        {
            PROFILE_SCOPE("FullScreenQuad Worker");
            while (true)
            {
                uint idx = tileIndex.fetch_add(1);
                if (idx >= numTiles) break;
                const Tile& tile = tiles[idx];
                const uint startX = tile.min.x, endX = tile.max.x;
                const uint startY = tile.min.y, endY = tile.max.y;

                float v = startY * invH;
                for (uint y = startY; y <= endY; ++y, v += invH)
                {
                    float u = startX * invW;
                    uint x = startX;
                    __m128* row = pixels + y * w;

                    for (; x + 3 <= endX; x += 4, u += 4.0f * invW)
                    {
                        for (int i = 0; i < 4; ++i)
                        {
                            Interpolant input;
                            input.Attributes[0] = float2(u + i * invW, v);
                            float4 c = ps(input, cb, tt);
                            _mm_stream_ps(reinterpret_cast<float*>(row + x + i), _mm_set_ps(c.w, c.z, c.y, c.x));
                        }
                    }

                    for (; x <= endX; ++x, u += invW)
                    {
                        Interpolant input;
                        input.Attributes[0] = float2(u, v);
                        float4 c = ps(input, cb, tt);
                        _mm_stream_ps(reinterpret_cast<float*>(row + x), _mm_set_ps(c.w, c.z, c.y, c.x));
                    }
                }
            }
        };

        ThreadUtils::DispatchWorkers(worker);
    }

private:
    static void ValidateDrawState(const PipelineStateObject& state, bool needIndexBuffer)
    {
        uint32_t mask = PipelineResource::VertexShader |
                        PipelineResource::VertexBuffer |
                        PipelineResource::DepthBuffer |
                        PipelineResource::Viewport |
                        PipelineResource::TileSize;
        if (needIndexBuffer)
            mask = mask | PipelineResource::IndexBuffer;
        state.Validate(mask);
    }

    static void ValidateFullScreenQuadState(const PipelineStateObject& state)
    {
        state.Validate(PipelineResource::RenderTarget |
                       PipelineResource::PixelShader |
                       PipelineResource::Viewport |
                       PipelineResource::TileSize);
    }

    static void DrawImpl(const PipelineStateObject& state, uint vertexCount, uint startVertex);
    static void DrawIndexedImpl(const PipelineStateObject& state, uint indexCount, uint startIndex);
    static std::vector<Interpolant> ProcessNonIndexedVertices(const PipelineStateObject& state, uint vertexCount, uint startVertex);
    static std::vector<int3> GatherNonIndexedTriangles(uint vertexCount);
    static std::vector<Interpolant> ProcessIndexedVertices(const PipelineStateObject& state, uint indexCount, uint startIndex, uint totalVertices);
    static std::vector<int3> GatherIndexedTriangles(const PipelineStateObject& state, uint indexCount, uint startIndex);
    static void ClipAndRasterize(const PipelineStateObject& state, std::vector<Interpolant>& clipVerts, const std::vector<int3>& sourceTriangles);
    static void DrawPoint(Texture& rt, DepthBuffer& db, const RasterizerState& rasterState, int x, int y, float z, const float4& color);
    static void DrawLine(Texture& rt, DepthBuffer& db, const RasterizerState& rasterState, int x0, int y0, int x1, int y1, float z0, float z1, const float4& color);
    static void DrawDebugLine(Texture& rt, const RasterizerState& rasterState, int x0, int y0, int x1, int y1, const float4& color);
    static void DrawTileBorders(Texture& rt, uint tileSize);
    static void DrawActiveTileBorders(Texture& rt, const RasterizerState& rasterState, uint tileSize, const std::vector<Tile>& tiles);
};

void DeviceContext::Impl::DrawPoint(Texture& rt, DepthBuffer& db,
                                    const RasterizerState& rasterState,
                                    int x, int y, float z, const float4& color)
{
    const uint w = rt.Width();
    const uint h = rt.Height();
    if (x < 0 || y < 0 || x >= static_cast<int>(w) || y >= static_cast<int>(h))
        return;

    uint idx = y * w + x;
    float depthValue = db.At(idx);

    if (RasterizerCommon::DepthTest(z, depthValue, rasterState.depthFunc))
    {
        if (rasterState.depthWriteEnable)
            db.At(idx) = z;

        __m128 col = _mm_set_ps(color.w, color.z, color.y, color.x);
        rt.StreamWrite(uint2(x, y), col);
    }
}

void DeviceContext::Impl::DrawLine(Texture& rt, DepthBuffer& db,
                                   const RasterizerState& rasterState,
                                   int x0, int y0, int x1, int y1,
                                   float z0, float z1, const float4& color)
{
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    int steps = std::max(dx, -dy);
    float zStep = (steps > 0) ? (z1 - z0) / static_cast<float>(steps) : 0.0f;
    float z = z0;
    int x = x0, y = y0;

    for (int i = 0; i <= steps; ++i)
    {
        DrawPoint(rt, db, rasterState, x, y, z, color);
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
        z += zStep;
    }
}

void DeviceContext::Impl::DrawDebugLine(Texture& rt,
                                        const RasterizerState&,
                                        int x0, int y0, int x1, int y1,
                                        const float4& color)
{
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    int x = x0, y = y0;
    int w = rt.Width();
    int h = rt.Height();

    while (true)
    {
        if (x >= 0 && y >= 0 && x < w && y < h)
        {
            __m128 col = _mm_set_ps(color.w, color.z, color.y, color.x);
            rt.StreamWrite(uint2(x, y), col);
        }
        if (x == x1 && y == y1) break;

        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
}

void DeviceContext::Impl::DrawTileBorders(Texture& rt, uint tileSize)
{
    int w = rt.Width();
    int h = rt.Height();
    float4 borderColor(0.0f, 1.0f, 0.0f, 1.0f);
    RasterizerState dummyState;
    for (int x = tileSize; x < w; x += tileSize)
        DrawDebugLine(rt, dummyState, x, 0, x, h - 1, borderColor);
    for (int y = tileSize; y < h; y += tileSize)
        DrawDebugLine(rt, dummyState, 0, y, w - 1, y, borderColor);
}

void DeviceContext::Impl::DrawActiveTileBorders(Texture& rt,
                                                const RasterizerState& rasterState,
                                                uint tileSize,
                                                const std::vector<Tile>& tiles)
{
    float4 borderColor(0.0f, 1.0f, 0.0f, 1.0f);

    // Corner length — 25% of tile size, but not less than 4 pixels
    const int cornerLen = std::max(4, static_cast<int>(tileSize * 0.25f));

    for (const auto& tile : tiles)
    {
        if (!tile.triangleIndices.empty())
        {
            int x0 = tile.min.x, y0 = tile.min.y;
            int x1 = tile.max.x, y1 = tile.max.y;
            int cx = std::min(cornerLen, (x1 - x0) / 2);
            int cy = std::min(cornerLen, (y1 - y0) / 2);

            // ┌ top-left corner
            DrawDebugLine(rt, rasterState, x0, y0, x0 + cx, y0, borderColor); // horizontal
            DrawDebugLine(rt, rasterState, x0, y0, x0, y0 + cy, borderColor); // vertical

            // ┐ top-right corner
            DrawDebugLine(rt, rasterState, x1 - cx, y0, x1, y0, borderColor);
            DrawDebugLine(rt, rasterState, x1, y0, x1, y0 + cy, borderColor);

            // └ bottom-left corner
            DrawDebugLine(rt, rasterState, x0, y1, x0 + cx, y1, borderColor);
            DrawDebugLine(rt, rasterState, x0, y1 - cy, x0, y1, borderColor);

            // ┘ bottom-right corner
            DrawDebugLine(rt, rasterState, x1 - cx, y1, x1, y1, borderColor);
            DrawDebugLine(rt, rasterState, x1, y1 - cy, x1, y1, borderColor);
        }
    }
}

std::vector<Interpolant> DeviceContext::Impl::ProcessIndexedVertices(const PipelineStateObject& state, uint indexCount, uint startIndex, uint totalVertices)
{
    PROFILE_SCOPE("Vertex Shader (indexed)");
    std::vector<Interpolant> clipVerts(totalVertices);

    std::vector<uint> uniqueIndices;
    {
        std::vector<bool> visited(totalVertices, false);
        for (uint i = startIndex; i < startIndex + indexCount; ++i)
        {
            uint idx = state.indexBuffer.GetByIndex(i);
            if (!visited[idx])
            {
                visited[idx] = true;
                uniqueIndices.push_back(idx);
            }
        }
    }

    const size_t totalUnique = uniqueIndices.size();
    ThreadUtils::SmartParallelFor(size_t(0), totalUnique, size_t(1),
    [&](size_t i)
    {
        uint idx = uniqueIndices[i];
        clipVerts[idx] = state.vertexShader(state.vertexBuffer.GetByIndex(idx), state.constantBuffer, state.textureTable);
    });

    return clipVerts;
}

std::vector<int3> DeviceContext::Impl::GatherIndexedTriangles(const PipelineStateObject& state, uint indexCount, uint startIndex)
{
    PROFILE_SCOPE("Gather indexed triangles");
    const uint triangleCount = indexCount / 3;
    std::vector<int3> triangles;
    triangles.reserve(triangleCount);

    for (uint i = startIndex; i + 2 < startIndex + indexCount; i += 3)
    {
        triangles.emplace_back(static_cast<int>(state.indexBuffer.GetByIndex(i)),
                               static_cast<int>(state.indexBuffer.GetByIndex(i + 1)),
                               static_cast<int>(state.indexBuffer.GetByIndex(i + 2)));
    }
    return triangles;
}

std::vector<Interpolant> DeviceContext::Impl::ProcessNonIndexedVertices(
    const PipelineStateObject& state,
    uint vertexCount, uint startVertex)
{
    PROFILE_SCOPE("Vertex Shader (non-indexed)");
    std::vector<Interpolant> clipVerts(vertexCount);

    ThreadUtils::SmartParallelFor(uint(0), vertexCount, uint(1),
    [&](uint i)
    {
        uint idx = startVertex + i;
        clipVerts[i] = state.vertexShader(state.vertexBuffer.GetByIndex(idx), state.constantBuffer, state.textureTable);
    });

    return clipVerts;
}

std::vector<int3> DeviceContext::Impl::GatherNonIndexedTriangles(uint vertexCount)
{
    const uint triangleCount = vertexCount / 3;
    std::vector<int3> triangles;
    triangles.reserve(triangleCount);
    for (uint i = 0; i + 2 < vertexCount; i += 3)
        triangles.emplace_back(i, i + 1, i + 2);
    return triangles;
}

void DeviceContext::Impl::ClipAndRasterize(const PipelineStateObject& state,
                                           std::vector<Interpolant>& clipVerts,
                                           const std::vector<int3>& sourceTriangles)
{
    std::vector<Interpolant> finalVerts;
    std::vector<int3> finalTriangles;
    finalVerts.reserve(sourceTriangles.size() * 3);
    finalTriangles.reserve(sourceTriangles.size() * 2);

    {
        PROFILE_SCOPE("Near plane clipping");
        for (const auto& tri : sourceTriangles)
        {
            Interpolant clipped[2][3];
            int numTris = RasterizerCommon::ClipTriangleNearPlane(
                clipVerts[tri.x], clipVerts[tri.y], clipVerts[tri.z], clipped);
            for (int t = 0; t < numTris; ++t)
            {
                int base = static_cast<int>(finalVerts.size());
                finalVerts.push_back(clipped[t][0]);
                finalVerts.push_back(clipped[t][1]);
                finalVerts.push_back(clipped[t][2]);
                finalTriangles.emplace_back(base, base + 1, base + 2);
            }
        }
    }
    if (finalTriangles.empty())
        return;

    {
        PROFILE_SCOPE("Perspective divide");
        for (auto& v : finalVerts)
            RasterizerCommon::ClipSpaceToScreenSpace(v, state.viewport);
    }

    if (state.geometryShader)
    {
        PROFILE_SCOPE("Geometry shader");
        std::vector<Interpolant> gsVerts;
        std::vector<int3> gsTriangles;
        gsVerts.reserve(finalTriangles.size() * 6);
        gsTriangles.reserve(finalTriangles.size() * 2);

        for (const auto& tri : finalTriangles)
        {
            Interpolant inVerts[3] = { finalVerts[tri.x], finalVerts[tri.y], finalVerts[tri.z] };
            std::vector<Interpolant> outVerts;
            std::vector<int> outIndices;
            state.geometryShader(inVerts, outVerts, outIndices, state.textureTable);

            int base = static_cast<int>(gsVerts.size());
            gsVerts.insert(gsVerts.end(), outVerts.begin(), outVerts.end());
            for (size_t j = 0; j + 2 < outIndices.size(); j += 3)
                gsTriangles.emplace_back(base + outIndices[j], base + outIndices[j + 1], base + outIndices[j + 2]);
        }
        finalVerts = std::move(gsVerts);
        finalTriangles = std::move(gsTriangles);
    }

    Texture* rt = state.renderTarget.get();
    DepthBuffer* db = state.depthBuffer.get();

    RasterizerState rasterState;
    rasterState.cullMode = state.cullMode;
    rasterState.fillMode = state.fillMode;
    rasterState.depthFunc = state.depthFunc;
    rasterState.depthWriteEnable = state.depthWriteEnable;
    rasterState.scissorEnable = state.scissorEnable;
    rasterState.scissorRect = state.scissorRect;

    if (state.fillMode == FillMode::Solid)
    {
        PROFILE_SCOPE("Create TriangleSetups");

        std::vector<RasterizerCommon::TriangleSetup> setups;
        setups.reserve(finalTriangles.size());

        uint64_t totalPixelCoverage = 0;
        constexpr uint64_t PIXEL_COVERAGE_THRESHOLD = 4096;

        for (const auto& tri : finalTriangles)
        {
            auto optSetup = RasterizerCommon::CreateTriangleSetup(finalVerts[tri.x], finalVerts[tri.y], finalVerts[tri.z], rasterState);
            if (optSetup)
            {
                const auto& s = *optSetup;
                totalPixelCoverage += static_cast<uint64_t>(s.bbMaxX - s.bbMinX + 1) * static_cast<uint64_t>(s.bbMaxY - s.bbMinY + 1);
                setups.push_back(std::move(*optSetup));
            }
        }

        if (!setups.empty())
        {
            if (totalPixelCoverage < PIXEL_COVERAGE_THRESHOLD)
            {
                PROFILE_SCOPE("Render Solid (single-threaded)");
                const uint w = rt->Width();
                const uint h = rt->Height();
                for (const auto& s : setups)
                {
                    int minX = std::max(0, s.bbMinX);
                    int minY = std::max(0, s.bbMinY);
                    int maxX = std::min(static_cast<int>(w) - 1, s.bbMaxX);
                    int maxY = std::min(static_cast<int>(h) - 1, s.bbMaxY);

                    if (minX >= maxX || minY >= maxY) continue;

                    Rasterizer::RasterizeTriangle(s,
                                                  rasterState,
                                                  *db,
                                                  rt,
                                                  state.viewport,
                                                  state.pixelShader,
                                                  state.constantBuffer,
                                                  &state.textureTable,
                                                  uint2(minX, minY),
                                                  uint2(maxX, maxY));
                }
            }
            else
            {
                TileGrid tileGrid;
                {
                    PROFILE_SCOPE("Tile binning");
                    uint width  = (rt) ? rt->Width() : db->Width();
                    uint height = (rt) ? rt->Height() : db->Height();
                    tileGrid.Build(width, height, state.tileSize, rasterState.scissorEnable, rasterState.scissorRect);
                    tileGrid.BinTriangles(setups);
                }

                {
                    PROFILE_SCOPE("Render Solid");
                    const auto& tiles = tileGrid.GetTiles();
                    uint numTiles = static_cast<uint>(tiles.size());
                    std::atomic<int> tileIndex(0);

                    auto Task = [&]()
                    {
                        PROFILE_SCOPE("RenderTiles::tile worker");
                        while (true)
                        {
                            uint idx = static_cast<uint>(tileIndex.fetch_add(1));
                            if (idx >= numTiles)
                                break;

                            const Tile& tile = tiles[idx];
                            for (int triIdx : tile.triangleIndices)
                            {
                                Rasterizer::RasterizeTriangle(setups[triIdx],
                                                              rasterState,
                                                              *db,
                                                              rt,
                                                              state.viewport,
                                                              state.pixelShader,
                                                              state.constantBuffer,
                                                              &state.textureTable,
                                                              tile.min,
                                                              tile.max);
                            }
                        }
                    };
                    ThreadUtils::DispatchWorkers(Task);
                }
                #ifdef DEBUG_TILING
                DrawActiveTileBorders(*rt, rasterState, state.tileSize, tileGrid.GetTiles());
                #endif
            }
        }
    }
    else if (state.fillMode == FillMode::Wireframe)
    {
        PROFILE_SCOPE("Render Wireframe");
        float4 wireColor(1.0f, 1.0f, 1.0f, 1.0f);
        for (const auto& tri : finalTriangles)
        {
            const auto& v0 = finalVerts[tri.x];
            const auto& v1 = finalVerts[tri.y];
            const auto& v2 = finalVerts[tri.z];
            float depth0 = RasterizerCommon::ComputeDepth(v0.ClipSpacePosition.z, v0.ClipSpacePosition.w, state.viewport);
            float depth1 = RasterizerCommon::ComputeDepth(v1.ClipSpacePosition.z, v1.ClipSpacePosition.w, state.viewport);
            float depth2 = RasterizerCommon::ComputeDepth(v2.ClipSpacePosition.z, v2.ClipSpacePosition.w, state.viewport);

            DrawLine(*rt, 
                     *db, 
                     rasterState,
                     (int)round(v0.ClipSpacePosition.x),
                     (int)round(v0.ClipSpacePosition.y),
                     (int)round(v1.ClipSpacePosition.x),
                     (int)round(v1.ClipSpacePosition.y),
                     depth0, 
                     depth1, 
                     wireColor);

            DrawLine(*rt, 
                     *db, 
                     rasterState, 
                     (int)round(v1.ClipSpacePosition.x),
                     (int)round(v1.ClipSpacePosition.y),
                     (int)round(v2.ClipSpacePosition.x),
                     (int)round(v2.ClipSpacePosition.y),
                     depth1, 
                     depth2, 
                     wireColor);

            DrawLine(*rt, 
                     *db, 
                     rasterState, 
                     (int)round(v2.ClipSpacePosition.x),
                     (int)round(v2.ClipSpacePosition.y),
                     (int)round(v0.ClipSpacePosition.x),
                     (int)round(v0.ClipSpacePosition.y),
                     depth2, 
                     depth0, 
                     wireColor);
        }
    }
    else if (state.fillMode == FillMode::Point)
    {
        PROFILE_SCOPE("Render Point");
        std::vector<bool> drawn(finalVerts.size(), false);
        for (const auto& tri : finalTriangles)
        {
            for (int idx : {tri.x, tri.y, tri.z})
            {
                if (!drawn[idx])
                {
                    drawn[idx] = true;
                    const auto& v = finalVerts[idx];
                    float depth = RasterizerCommon::ComputeDepth(v.ClipSpacePosition.z, v.ClipSpacePosition.w, state.viewport);
                    DrawPoint(*rt, *db, rasterState, (int)round(v.ClipSpacePosition.x), (int)round(v.ClipSpacePosition.y), depth, float4(1.0f));
                }
            }
        }
    }
}

void DeviceContext::Impl::DrawImpl(const PipelineStateObject& state, uint vertexCount, uint startVertex)
{
    auto clipVerts = ProcessNonIndexedVertices(state, vertexCount, startVertex);
    auto triangles = GatherNonIndexedTriangles(vertexCount);
    ClipAndRasterize(state, clipVerts, triangles);
}

void DeviceContext::Impl::DrawIndexedImpl(const PipelineStateObject& state, uint indexCount, uint startIndex)
{
    uint totalVertices = static_cast<uint>(state.vertexBuffer.Size());
    auto clipVerts = ProcessIndexedVertices(state, indexCount, startIndex, totalVertices);
    auto triangles = GatherIndexedTriangles(state, indexCount, startIndex);
    ClipAndRasterize(state, clipVerts, triangles);
}

DeviceContext::DeviceContext() : pImpl(std::make_unique<Impl>()) {}
DeviceContext::DeviceContext(const PipelineStateObject& initialState) : pImpl(std::make_unique<Impl>(initialState)) {}
DeviceContext::~DeviceContext() = default;

void DeviceContext::SetVertexShader(VertexShader shader)
{
    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    pImpl->backState.vertexShader = std::move(shader);
}

void DeviceContext::SetGeometryShader(GeometryShader shader)
{
    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    pImpl->backState.geometryShader = std::move(shader);
}

void DeviceContext::SetPixelShader(PixelShader shader)
{
    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    pImpl->backState.pixelShader = std::move(shader);
}

void DeviceContext::SetVertexBuffer(const VertexBuffer& buffer)
{
    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    pImpl->backState.vertexBuffer = buffer;
}

void DeviceContext::SetIndexBuffer(const IndexBuffer& buffer)
{
    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    pImpl->backState.indexBuffer = buffer;
}

void DeviceContext::SetConstantBuffer(const ConstantBuffer& buffer)
{
    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    pImpl->backState.constantBuffer = buffer;
}

void DeviceContext::SetTexture(const std::string& name, std::shared_ptr<const Texture> texture, const SamplerState& sampler)
{
    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    pImpl->backState.textureTable.Set(name, std::move(texture), sampler);
}

void DeviceContext::SetRenderTarget(std::shared_ptr<Texture> target, bool createDepthBuffer)
{
    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    pImpl->backState.renderTarget = std::move(target);
    if (createDepthBuffer && pImpl->backState.renderTarget)
    {
        uint2 size = pImpl->backState.renderTarget->Size();
        if (!pImpl->backState.depthBuffer || pImpl->backState.depthBuffer->Size() != size)
            pImpl->backState.depthBuffer = std::make_shared<DepthBuffer>(size);
    }
}

void DeviceContext::SetDepthBuffer(std::shared_ptr<DepthBuffer> depth)
{
    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    pImpl->backState.depthBuffer = std::move(depth);
}

void DeviceContext::SetViewport(const Viewport& vp)
{
    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    pImpl->backState.viewport = vp;
}

void DeviceContext::SetTileSize(uint size)
{
    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    pImpl->backState.tileSize = size;
}

void DeviceContext::SetDepthWriteEnable(bool enable)
{
    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    pImpl->backState.depthWriteEnable = enable;
}

void DeviceContext::SetCullMode(CullMode mode)
{
    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    pImpl->backState.cullMode = mode;
}

void DeviceContext::SetFillMode(FillMode mode)
{
    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    pImpl->backState.fillMode = mode;
}

void DeviceContext::SetDepthFunc(ComparisonFunc func)
{
    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    pImpl->backState.depthFunc = func;
}

void DeviceContext::SetScissorRect(int left, int top, int right, int bottom)
{
    uint l = static_cast<uint>(std::max(0, left));
    uint t = static_cast<uint>(std::max(0, top));
    uint r = static_cast<uint>(std::max(0, right));
    uint b = static_cast<uint>(std::max(0, bottom));

    if (r <= l || b <= t)
        l = t = r = b = 0;

    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    pImpl->backState.scissorRect = Rect(l, t, r - l, b - t);
}

void DeviceContext::SetScissorRect(const Rect& rect)
{
    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    pImpl->backState.scissorRect = rect;
}

void DeviceContext::SetScissorEnable(bool enable)
{
    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    pImpl->backState.scissorEnable = enable;
}

VertexShader DeviceContext::GetVertexShader() const { return pImpl->GetVertexShader(); }
GeometryShader DeviceContext::GetGeometryShader() const { return pImpl->GetGeometryShader(); }
PixelShader DeviceContext::GetPixelShader() const { return pImpl->GetPixelShader(); }
VertexBuffer DeviceContext::GetVertexBuffer() const { return pImpl->GetVertexBuffer(); }
IndexBuffer DeviceContext::GetIndexBuffer() const { return pImpl->GetIndexBuffer(); }
ConstantBuffer DeviceContext::GetConstantBuffer() const { return pImpl->GetConstantBuffer(); }
std::shared_ptr<Texture> DeviceContext::GetRenderTarget() const { return pImpl->GetRenderTarget(); }
std::shared_ptr<DepthBuffer> DeviceContext::GetDepthBuffer() const { return pImpl->GetDepthBuffer(); }
bool DeviceContext::GetDepthWriteEnable() const { return pImpl->GetDepthWriteEnable(); }
CullMode DeviceContext::GetCullMode() const { return pImpl->GetCullMode(); }
FillMode DeviceContext::GetFillMode() const { return pImpl->GetFillMode(); }
ComparisonFunc DeviceContext::GetDepthFunc() const { return pImpl->GetDepthFunc(); }
Viewport DeviceContext::GetViewport() const { return pImpl->GetViewport(); }
uint DeviceContext::GetTileSize() const { return pImpl->GetTileSize(); }
bool DeviceContext::GetScissorEnable() const { return pImpl->GetScissorEnable();  };
Rect DeviceContext::GetScissorRect() const { return pImpl->GetScissorRect(); };

void DeviceContext::Clear(ClearFlags flags, const float4& color, float depth)
{
    pImpl->Clear(flags, color, depth);
}

void DeviceContext::Draw(uint vertexCount, uint startVertex)
{
    pImpl->Draw(vertexCount, startVertex);
}

void DeviceContext::Draw()
{
    pImpl->DrawAll();
}

void DeviceContext::DrawIndexed(uint indexCount, uint startIndex)
{
    pImpl->DrawIndexed(indexCount, startIndex);
}

void DeviceContext::DrawIndexed()
{
    pImpl->DrawIndexedAll();
}

void DeviceContext::DrawFullScreenQuad()
{
    pImpl->DrawFullScreenQuad();
}

SOFTX_END
/////////////////////////////////////////////////////////////////
