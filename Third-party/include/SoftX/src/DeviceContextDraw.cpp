#include "pch.h"

#include <ppl.h>

#include "RasterizerCommon.h"
#include <SoftX/SoftX.h>
#include <SoftX/ThreadPoolManager.h>

#define DEBUG_TILING

SOFTX_BEGIN

void DeviceContext::DrawPoint(int x, int y, float z, const float4& color)
{
    IRenderTarget* rt = renderTarget;
    if (!rt)
        return;

    if (!depthBuffer)
        return;

    if (x >= (int)rt->Width() || y >= (int)rt->Height())
        return;

    if (x < 0 || y < 0)
        return;

    uint idx = y * rt->Width() + x;
    if (z < depthBuffer->At(idx))
    {
        if(depthWriteEnable) depthBuffer->At(idx) = z;
        if (renderTarget) rt->SetPixel(uint2(x, y), color);
    }
}

void DeviceContext::DrawLine(int x0, int y0, int x1, int y1, float z0, float z1, const float4& color)
{
    IRenderTarget* rt = renderTarget;
    if (!rt)
        return;

    if (!depthBuffer)
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
        DrawPoint(x, y, z, color);

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

void DeviceContext::DrawIndexed(uint indexCount, uint startIndex)
{
    PROFILE_SCOPE("DeviceContext::DrawIndexed");

    if (!vertexShader || vertexBuffer.IsEmpty() || indexBuffer.IsEmpty() || !depthBuffer)
        return;

    // Step 1: VS → clip space (without perspective divide)
    std::vector<VertexOutput> clipVerts(vertexBuffer.Size());
    {
        PROFILE_SCOPE("Vertex Shader (VS -> clip space)");
        std::vector<uint> uniqueIndices;
        {
            std::vector<bool> visited(vertexBuffer.Size(), false);
            for (uint i = startIndex; i < startIndex + indexCount; ++i)
            {
                uint idx = indexBuffer.GetByIndex(i);
                if (!visited[idx])
                {
                    visited[idx] = true;
                    uniqueIndices.push_back(idx);
                }
            }
        }

        concurrency::parallel_for_each(uniqueIndices.begin(), uniqueIndices.end(),
            [&](uint idx)
            {
                // Only VS — ClipSpaceToScreenSpace not called yet
                clipVerts[idx] = vertexShader(vertexBuffer.GetByIndex(idx), constantBuffer, textureTable);
            });
    }

    // Step 2: Gather source triangles
    std::vector<int3> sourceTriangles;
    {
        PROFILE_SCOPE("Gather source triangles");
        for (uint i = startIndex; i + 2 < startIndex + indexCount; i += 3)
        {
            sourceTriangles.push_back({ (int)indexBuffer.GetByIndex(i), (int)indexBuffer.GetByIndex(i + 1), (int)indexBuffer.GetByIndex(i + 2) });
        }
    }

    // Step 3: Near plane clipping in clip space
    std::vector<VertexOutput> finalVerts;
    std::vector<int3> finalTriangles;
    finalVerts.reserve(sourceTriangles.size() * 3);
    finalTriangles.reserve(sourceTriangles.size() * 2);

    {
        PROFILE_SCOPE("Near plane clipping in clip space");
        for (const auto& tri : sourceTriangles)
        {
            VertexOutput clipped[2][3];
            int numTris = RasterizerCommon::ClipTriangleNearPlane(clipVerts[tri.x], clipVerts[tri.y], clipVerts[tri.z], clipped);

            for (int t = 0; t < numTris; ++t)
            {
                int base = (int)finalVerts.size();
                finalVerts.push_back(clipped[t][0]);
                finalVerts.push_back(clipped[t][1]);
                finalVerts.push_back(clipped[t][2]);
                finalTriangles.push_back({ base, base + 1, base + 2 });
            }
        }
    }

    if (finalTriangles.empty())
        return;

    // Step 4: Perspective divide on surviving vertices
    {
        PROFILE_SCOPE("Perspective divide on surviving vertices");
        for (auto& v : finalVerts)
            RasterizerCommon::ClipSpaceToScreenSpace(v, viewport);
    }

    // Step 5: Geometry shader (on screen-space vertices)
    if (geometryShader)
    {
        PROFILE_SCOPE("Geometry shader");
        std::vector<VertexOutput> gsVerts;
        std::vector<int3> gsTriangles;

        for (const auto& tri : finalTriangles)
        {
            VertexOutput inVerts[3] = {finalVerts[tri.x], finalVerts[tri.y], finalVerts[tri.z]};
            std::vector<VertexOutput> outVerts;
            std::vector<int> outIndices;
            geometryShader(inVerts, outVerts, outIndices, textureTable);

            int base = (int)gsVerts.size();
            gsVerts.insert(gsVerts.end(), outVerts.begin(), outVerts.end());
            for (size_t j = 0; j + 2 < outIndices.size(); j += 3)
                gsTriangles.push_back({base + outIndices[j], base + outIndices[j + 1], base + outIndices[j + 2]});
        }
        finalVerts = std::move(gsVerts);
        finalTriangles = std::move(gsTriangles);
    }

    // Step 6: Render
    if (fillMode == FillMode::Solid)
    {
        PROFILE_SCOPE("Render Solid");
        RasterizerState state;
        state.cullMode = cullMode;
        state.fillMode = fillMode;
        state.depthFunc = depthFunc;
        state.depthWriteEnable = depthWriteEnable;

        Renderer renderer(*rasterizer, renderTarget, *depthBuffer, pixelShader, constantBuffer, &textureTable, state, tileSize);
        renderer.Execute(finalVerts, finalTriangles);

#ifdef DEBUG_TILING
        DrawActiveTileBorders(renderer.GetTiles());
#endif
    }
    else if (fillMode == FillMode::Wireframe)
    {
        PROFILE_SCOPE("Render Wireframe");

        if (renderTarget == nullptr)
            return;

        float4 wireColor(1, 1, 1, 1);
        for (const auto& tri : finalTriangles)
        {
            const auto& v0 = finalVerts[tri.x];
            const auto& v1 = finalVerts[tri.y];
            const auto& v2 = finalVerts[tri.z];
            DrawLine((int)round(v0.Position.x), 
                     (int)round(v0.Position.y), 
                     (int)round(v1.Position.x),
                     (int)round(v1.Position.y), 
                     v0.Position.z, 
                     v1.Position.z, 
                     wireColor);
            DrawLine((int)round(v1.Position.x), 
                     (int)round(v1.Position.y), 
                     (int)round(v2.Position.x),
                     (int)round(v2.Position.y), 
                     v1.Position.z, v2.Position.z, 
                     wireColor);
            DrawLine((int)round(v2.Position.x), 
                     (int)round(v2.Position.y), 
                     (int)round(v0.Position.x),
                     (int)round(v0.Position.y), 
                     v2.Position.z, 
                     v0.Position.z, 
                     wireColor);
        }
    }
    else if (fillMode == FillMode::Point)
    {
        PROFILE_SCOPE("Render Point");

        if (renderTarget == nullptr)
            return;

        std::vector<bool> drawn(finalVerts.size(), false);
        for (const auto& tri : finalTriangles)
            for (int idx : {tri.x, tri.y, tri.z})
                if (!drawn[idx])
                {
                    drawn[idx] = true;
                    const auto& v = finalVerts[idx];
                    DrawPoint((int)round(v.Position.x), (int)round(v.Position.y), v.Position.z, v.Color);
                }
    }
}

void DeviceContext::DrawIndexed()
{
    uint count = (uint)indexBuffer.Size();
    DrawIndexed(count, 0);
}

void DeviceContext::RenderTileQuad(const Tile& tile, float invW, float invH)
{
    PROFILE_SCOPE("DeviceContext::RenderTileQuad");
    IRenderTarget* rt = renderTarget;
    if (!rt)
        return;

    VertexOutput input = {};
    auto ps = pixelShader;
    auto cb = constantBuffer;
    auto tt = textureTable;

    for (uint y = tile.min.y; y <= (uint)tile.max.y; ++y)
    {
        float v = y * invH;
        for (uint x = tile.min.x; x <= (uint)tile.max.x; ++x)
        {
            float u = x * invW;
            input.UV = float2(u, v);
            float4 color = ps(input, cb, tt);
            if (renderTarget) rt->SetPixel(uint2(x, y), color);
        }
    }
}

void DeviceContext::DrawFullScreenQuad()
{
    PROFILE_SCOPE("DeviceContext::DrawFullScreenQuad");

    if (!pixelShader || !renderTarget)
        return;

    IRenderTarget* rt = renderTarget;
    const uint w = rt->Width();
    const uint h = rt->Height();
    const float invW = 1.0f / (w - 1u);
    const float invH = 1.0f / (h - 1u);

    Framebuffer* fb = dynamic_cast<Framebuffer*>(rt);
    RenderTargetTexture* rtt = dynamic_cast<RenderTargetTexture*>(rt);
    uint32_t* fbPixels = fb ? fb->GetRawPixels() : nullptr;
    __m128* texPixels = rtt ? rtt->Texture().GetRawPixels() : nullptr;

    auto ps = pixelShader;
    auto cb = constantBuffer;
    auto tt = textureTable;

    const uint ts = tileSize;
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

    if (fbPixels) {
        auto workerFB = [&, ps, cb, tt, fbPixels, w, invW, invH]() {
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
                            VertexOutput input;
                            input.UV = float2(u + i * invW, v);
                            packed[i] = Framebuffer::PackColor(ps(input, cb, tt));
                        }
                        _mm_store_si128((__m128i*)(row + x),
                            _mm_loadu_si128((__m128i*)packed));
                    }

                    for (; x <= endX; ++x, u += invW) {
                        VertexOutput input;
                        input.UV = float2(u, v);
                        row[x] = Framebuffer::PackColor(ps(input, cb, tt));
                    }
                }
            }
        };
        auto& pool = ThreadPoolManager::Get();
        for (uint i = 0; i < pool.threadCount(); ++i)
            pool.enqueue(workerFB);
        pool.wait();
    }
    else if (texPixels) {
        auto workerTex = [&, ps, cb, tt, texPixels, w, invW, invH]() {
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
                            VertexOutput input;
                            input.UV = float2(u + i * invW, v);
                            float4 c = ps(input, cb, tt);
                            _mm_store_ps((float*)(row + x + i),
                                _mm_set_ps(c.w, c.z, c.y, c.x));
                        }
                    }

                    for (; x <= endX; ++x, u += invW) {
                        VertexOutput input;
                        input.UV = float2(u, v);
                        float4 c = ps(input, cb, tt);
                        _mm_store_ps((float*)(row + x),
                            _mm_set_ps(c.w, c.z, c.y, c.x));
                    }
                }
            }
        };
        auto& pool = ThreadPoolManager::Get();
        for (uint i = 0; i < pool.threadCount(); ++i)
            pool.enqueue(workerTex);
        pool.wait();
    }
    else {
        auto workerFallback = [&, ps, cb, tt, rt, w, invW, invH]() {
            PROFILE_SCOPE("FullScreenQuad Fallback Worker");
            while (true) {
                uint idx = tileIndex.fetch_add(1);
                if (idx >= numTiles) break;
                const Tile& tile = tiles[idx];
                const uint startX = tile.min.x, endX = tile.max.x;
                const uint startY = tile.min.y, endY = tile.max.y;

                float v = startY * invH;
                for (uint y = startY; y <= endY; ++y, v += invH) {
                    float u = startX * invW;
                    for (uint x = startX; x <= endX; ++x, u += invW) {
                        VertexOutput input;
                        input.UV = float2(u, v);
                        if (renderTarget) rt->SetPixel(uint2(x, y), ps(input, cb, tt));
                    }
                }
            }
        };
        auto& pool = ThreadPoolManager::Get();
        for (uint i = 0; i < pool.threadCount(); ++i)
            pool.enqueue(workerFallback);
        pool.wait();
    }

#ifdef DEBUG_TILING
    DrawActiveTileBorders(tiles);
#endif
}

SOFTX_END
