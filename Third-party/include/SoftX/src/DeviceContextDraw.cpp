/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "../include/SoftX.h"
#include "RasterizerCommon.h"
#include "Renderer.h"
#include "ThreadPoolManager.h"
#include "ThreadUtils.h"
/////////////////////////////////////////////////////////////////
//#define DEBUG_TILING
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

void DeviceContext::DrawPoint(const PipelineStateObject& state, int x, int y, float z, const float4& color)
{
    IRenderTarget* rt = state.renderTarget.get();
    if (!rt)
        return;

    if (!state.depthBuffer)
        return;

    if (x >= (int)rt->Width() || y >= (int)rt->Height())
        return;

    if (x < 0 || y < 0)
        return;

    uint idx = y * rt->Width() + x;
    float depthValue = state.depthBuffer->At(idx);

    bool pass = RasterizerCommon::DepthTest(z, depthValue, state.depthFunc);

    if (pass)
    {
        if (state.depthWriteEnable) state.depthBuffer->At(idx) = z;
        if (state.renderTarget) rt->SetPixel(uint2(x, y), color);
    }
}

void DeviceContext::DrawLine(const PipelineStateObject& state, int x0, int y0, int x1, int y1, float z0, float z1, const float4& color)
{
    IRenderTarget* rt = state.renderTarget.get();
    if (!rt)
        return;

    if (!state.depthBuffer)
        return;

    int dx = std::abs((int)x1 - (int)x0);
    int dy = -std::abs((int)y1 - (int)y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    int steps = std::max(dx, -dy);
    float zStep = (steps > 0) ? (z1 - z0) / steps : 0.0f;
    float z = z0;
    uint x = x0, y = y0;

    for (int i = 0; i <= steps; ++i)
    {
        DrawPoint(state, x, y, z, color);

        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y += sy;
        }
        z += zStep;
    }
}

std::vector<Interpolant> DeviceContext::ProcessIndexedVertices(const PipelineStateObject& state,
                                                               uint indexCount, 
                                                               uint startIndex,
                                                               uint totalVertices)
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
        clipVerts[idx] = state.vertexShader(
            state.vertexBuffer.GetByIndex(idx),
            state.constantBuffer,
            state.textureTable);
    });

    return clipVerts;
}

std::vector<int3> DeviceContext::GatherIndexedTriangles(const PipelineStateObject& state,
                                                        uint indexCount, 
                                                        uint startIndex)
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

std::vector<Interpolant> DeviceContext::ProcessNonIndexedVertices(const PipelineStateObject& state,
                                                                  uint vertexCount, 
                                                                  uint startVertex)
{
    PROFILE_SCOPE("Vertex Shader (non-indexed)");
    std::vector<Interpolant> clipVerts(vertexCount);

    ThreadUtils::SmartParallelFor(uint(0), vertexCount, uint(1),
    [&](uint i)
    {
        uint idx = startVertex + i;
        clipVerts[i] = state.vertexShader(
            state.vertexBuffer.GetByIndex(idx),
            state.constantBuffer,
            state.textureTable);
    });

    return clipVerts;
}

std::vector<int3> DeviceContext::GatherNonIndexedTriangles(uint vertexCount)
{
    const uint triangleCount = vertexCount / 3;
    std::vector<int3> triangles;
    triangles.reserve(triangleCount);
    for (uint i = 0; i + 2 < vertexCount; i += 3)
        triangles.emplace_back(i, i + 1, i + 2);
    return triangles;
}

void DeviceContext::ClipAndRasterize(const PipelineStateObject& state,
                                     std::vector<Interpolant>& clipVerts,
                                     const std::vector<int3>& sourceTriangles)
{
    // Step 3: Near plane clipping
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

    // Step 4: Perspective divide
    {
        PROFILE_SCOPE("Perspective divide");
        for (auto& v : finalVerts)
            RasterizerCommon::ClipSpaceToScreenSpace(v, state.viewport);
    }

    // Step 5: Geometry shader (optional)
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

    // Step 6: Rasterization
    if (state.fillMode == FillMode::Solid)
    {
        // ------------------------------------------------------------------
        // Pre‑compute TriangleSetups once for every triangle.
        // All heavy fixed‑point conversion, area, edge step deltas and
        // reciprocal area are evaluated here and stored compactly.
        // Culling is also applied at this stage so that back‑/front‑facing
        // triangles never reach the rasteriser.
        // ------------------------------------------------------------------
        PROFILE_SCOPE("Create TriangleSetups");
        RasterizerState rasterState;
        rasterState.cullMode = state.cullMode;   // only cull mode matters here

        std::vector<RasterizerCommon::TriangleSetup> setups;
        setups.reserve(finalTriangles.size());

        for (const auto& tri : finalTriangles)
        {
            auto optSetup = RasterizerCommon::CreateTriangleSetup(
                finalVerts[tri.x], finalVerts[tri.y], finalVerts[tri.z], rasterState);
            if (optSetup)
                setups.push_back(std::move(*optSetup));
        }

        if (!setups.empty())
        {
            PROFILE_SCOPE("Render Solid");
            Renderer renderer;
            renderer.Execute(state, setups);
#ifdef DEBUG_TILING
            DrawActiveTileBorders(state, renderer.GetTiles());
#endif
        }
    }
    else if (state.fillMode == FillMode::Wireframe)
    {
        PROFILE_SCOPE("Render Wireframe");
        if (!state.renderTarget) return;
        float4 wireColor(1, 1, 1, 1);
        for (const auto& tri : finalTriangles)
        {
            const auto& v0 = finalVerts[tri.x];
            const auto& v1 = finalVerts[tri.y];
            const auto& v2 = finalVerts[tri.z];
            float depth0 = RasterizerCommon::ComputeDepth(v0.Position.z, v0.Position.w, state.viewport);
            float depth1 = RasterizerCommon::ComputeDepth(v1.Position.z, v1.Position.w, state.viewport);
            float depth2 = RasterizerCommon::ComputeDepth(v2.Position.z, v2.Position.w, state.viewport);
            DrawLine(state, 
                     (int)round(v0.Position.x), 
                     (int)round(v0.Position.y),
                     (int)round(v1.Position.x), 
                     (int)round(v1.Position.y),
                     depth0, 
                     depth1, 
                     wireColor);
            DrawLine(state, 
                     (int)round(v1.Position.x), 
                     (int)round(v1.Position.y),
                     (int)round(v2.Position.x), 
                     (int)round(v2.Position.y),
                     depth1, 
                     depth2, 
                     wireColor);
            DrawLine(state, 
                     (int)round(v2.Position.x), 
                     (int)round(v2.Position.y),
                     (int)round(v0.Position.x), 
                     (int)round(v0.Position.y),
                     depth2, 
                     depth0, 
                     wireColor);
        }
    }
    else if (state.fillMode == FillMode::Point)
    {
        PROFILE_SCOPE("Render Point");
        if (!state.renderTarget) return;
        std::vector<bool> drawn(finalVerts.size(), false);
        for (const auto& tri : finalTriangles)
        {
            for (int idx : {tri.x, tri.y, tri.z})
            {
                if (!drawn[idx])
                {
                    drawn[idx] = true;
                    const auto& v = finalVerts[idx];
                    float depth = RasterizerCommon::ComputeDepth(v.Position.z, v.Position.w, state.viewport);
                    DrawPoint(state, (int)round(v.Position.x), (int)round(v.Position.y), depth, v.Color);
                }
            }
        }
    }
}

void DeviceContext::Draw(uint vertexCount, uint startVertex)
{
    std::lock_guard<std::mutex> lock(drawMutex);
    PROFILE_SCOPE("DeviceContext::Draw (non-indexed)");

    CommitState();
    PipelineStateObject state = frontState;

    state.Validate(PipelineResource::VertexShader |
                   PipelineResource::VertexBuffer |
                   PipelineResource::DepthBuffer |
                   PipelineResource::Viewport |
                   PipelineResource::TileSize);

    if (vertexCount == 0 || startVertex + vertexCount > state.vertexBuffer.Size())
        SOFTX_THROW(InvalidArgument("Draw: vertexCount out of range"));

    DrawImpl(state, vertexCount, startVertex);
}

void DeviceContext::Draw()
{
    std::lock_guard<std::mutex> lock(drawMutex);
    PROFILE_SCOPE("DeviceContext::Draw (all vertices)");

    CommitState();
    PipelineStateObject state = frontState;

    state.Validate(PipelineResource::VertexShader |
                   PipelineResource::VertexBuffer |
                   PipelineResource::DepthBuffer |
                   PipelineResource::Viewport |
                   PipelineResource::TileSize);

    uint count = static_cast<uint>(state.vertexBuffer.Size());
    if (count == 0)
        SOFTX_THROW(InvalidState("Draw: vertex buffer is empty"));

    DrawImpl(state, count, 0);
}

void DeviceContext::DrawImpl(const PipelineStateObject& state, uint vertexCount, uint startVertex)
{
    auto clipVerts = ProcessNonIndexedVertices(state, vertexCount, startVertex);
    auto triangles = GatherNonIndexedTriangles(vertexCount);
    ClipAndRasterize(state, clipVerts, triangles);
}

void DeviceContext::DrawIndexed(uint indexCount, uint startIndex)
{
    std::lock_guard<std::mutex> lock(drawMutex);
    PROFILE_SCOPE("DeviceContext::DrawIndexed");

    CommitState();
    PipelineStateObject state = frontState;

    state.Validate(PipelineResource::VertexShader |
                   PipelineResource::VertexBuffer |
                   PipelineResource::IndexBuffer |
                   PipelineResource::DepthBuffer |
                   PipelineResource::Viewport |
                   PipelineResource::TileSize);

    DrawIndexedImpl(state, indexCount, startIndex);
}

void DeviceContext::DrawIndexed()
{
    std::lock_guard<std::mutex> lock(drawMutex);
    PROFILE_SCOPE("DeviceContext::DrawIndexed (full buffer)");

    CommitState();
    PipelineStateObject state = frontState;

    state.Validate(PipelineResource::VertexShader |
                   PipelineResource::VertexBuffer |
                   PipelineResource::IndexBuffer |
                   PipelineResource::DepthBuffer |
                   PipelineResource::Viewport |
                   PipelineResource::TileSize);

    uint count = static_cast<uint>(state.indexBuffer.Size());
    DrawIndexedImpl(state, count, 0);
}

void DeviceContext::DrawIndexedImpl(const PipelineStateObject& state, uint indexCount, uint startIndex)
{
    uint totalVertices = static_cast<uint>(state.vertexBuffer.Size());
    auto clipVerts = ProcessIndexedVertices(state, indexCount, startIndex, totalVertices);
    auto triangles = GatherIndexedTriangles(state, indexCount, startIndex);
    ClipAndRasterize(state, clipVerts, triangles);
}

void DeviceContext::DrawFullScreenQuad()
{
    std::lock_guard<std::mutex> lock(drawMutex);
    PROFILE_SCOPE("DeviceContext::DrawFullScreenQuad");

    CommitState();
    PipelineStateObject state = frontState;

    state.Validate(PipelineResource::RenderTarget |
                   PipelineResource::PixelShader |
                   PipelineResource::Viewport |
                   PipelineResource::TileSize);

    IRenderTarget* rt = state.renderTarget.get();
    const uint w = rt->Width();
    const uint h = rt->Height();
    const float invW = 1.0f / (w - 1u);
    const float invH = 1.0f / (h - 1u);

    FrameBuffer* fb = dynamic_cast<FrameBuffer*>(rt);
    RenderTargetTexture* rtt = dynamic_cast<RenderTargetTexture*>(rt);
    uint32_t* fbPixels = fb ? fb->GetRawPixels() : nullptr;
    __m128* texPixels = rtt ? rtt->Texture().GetRawPixels() : nullptr;

    auto ps = state.pixelShader;
    auto cb = state.constantBuffer;
    auto tt = state.textureTable;

    const uint ts = state.tileSize;
    const uint tilesX = (w + ts - 1u) / ts;
    const uint tilesY = (h + ts - 1u) / ts;
    std::vector<Tile> tiles;
    tiles.reserve(tilesX * tilesY);
    for (uint ty = 0; ty < tilesY; ++ty)
        for (uint tx = 0; tx < tilesX; ++tx) {
            uint2 mn(tx * ts, ty * ts);
            uint2 mx(std::min((tx + 1) * ts - 1u, w - 1u),
                std::min((ty + 1u) * ts - 1u, h - 1u));
            tiles.emplace_back(mn, mx);
        }

    const uint numTiles = (uint)tiles.size();
    std::atomic<uint> tileIndex(0);

    if (fbPixels) 
    {
        auto workerFB = [&, ps, cb, tt, fbPixels, w, invW, invH]() 
        {
            PROFILE_SCOPE("FullScreenQuad FB Worker");
            while (true) {
                uint idx = tileIndex.fetch_add(1);
                if (idx >= numTiles) break;
                const Tile& tile = tiles[idx];
                const uint startX = tile.min.x, endX = tile.max.x;
                const uint startY = tile.min.y, endY = tile.max.y;

                float v = startY * invH;
                for (uint y = startY; y <= endY; ++y, v += invH) {
                    float u = startX * invW;
                    uint x = startX;
                    uint32_t* row = fbPixels + y * w;

                    for (; x + 3 <= endX; x += 4, u += 4.0f * invW) {
                        uint32_t packed[4];
                        for (int i = 0; i < 4; ++i) {
                            Interpolant input;
                            input.UV = float2(u + i * invW, v);
                            packed[i] = FrameBuffer::PackColor(ps(input, cb, tt));
                        }
                        std::memcpy(row + x, packed, sizeof(packed));
                    }

                    for (; x <= endX; ++x, u += invW) {
                        Interpolant input;
                        input.UV = float2(u, v);
                        row[x] = FrameBuffer::PackColor(ps(input, cb, tt));
                    }
                }
            }
        };
        ThreadUtils::DispatchWorkers(workerFB);
    }
    else if (texPixels) 
    {
        auto workerTex = [&, ps, cb, tt, texPixels, w, invW, invH]() 
        {
            PROFILE_SCOPE("FullScreenQuad Tex Worker");
            while (true) {
                uint idx = tileIndex.fetch_add(1);
                if (idx >= numTiles) break;
                const Tile& tile = tiles[idx];
                const uint startX = tile.min.x, endX = tile.max.x;
                const uint startY = tile.min.y, endY = tile.max.y;

                float v = startY * invH;
                for (uint y = startY; y <= endY; ++y, v += invH) {
                    float u = startX * invW;
                    uint x = startX;
                    __m128* row = texPixels + y * w;

                    for (; x + 3 <= endX; x += 4, u += 4.0f * invW) {
                        for (int i = 0; i < 4; ++i) {
                            Interpolant input;
                            input.UV = float2(u + i * invW, v);
                            float4 c = ps(input, cb, tt);
                            _mm_store_ps((float*)(row + x + i), _mm_set_ps(c.w, c.z, c.y, c.x));
                        }
                    }

                    for (; x <= endX; ++x, u += invW) {
                        Interpolant input;
                        input.UV = float2(u, v);
                        float4 c = ps(input, cb, tt);
                        _mm_store_ps((float*)(row + x), _mm_set_ps(c.w, c.z, c.y, c.x));
                    }
                }
            }
        };
        ThreadUtils::DispatchWorkers(workerTex);
    }
    else 
    {
        auto workerFallback = [&, ps, cb, tt, rt, w, invW, invH]() 
        {
            PROFILE_SCOPE("FullScreenQuad Fallback Worker");
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
                    for (uint x = startX; x <= endX; ++x, u += invW) 
                    {
                        Interpolant input;
                        input.UV = float2(u, v);
                        rt->SetPixel(uint2(x, y), ps(input, cb, tt));
                    }
                }
            }
        };
        ThreadUtils::DispatchWorkers(workerFallback);
    }

#ifdef DEBUG_TILING
    DrawActiveTileBorders(state, tiles);
#endif
}

SOFTX_END
/////////////////////////////////////////////////////////////////
