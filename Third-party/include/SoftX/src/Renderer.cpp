/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "../include/SoftX.h"
#include "Renderer.h"
#include "Rasterizer.h"
#include "ThreadUtils.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

Renderer::Renderer()
{
}

void Renderer::Execute(const PipelineStateObject& pso, std::vector<RasterizerCommon::TriangleSetup>& setups)
{
    PROFILE_SCOPE("Renderer::Execute");

    width = pso.renderTarget ? pso.renderTarget->Width() : pso.depthBuffer->Width();
    height = pso.renderTarget ? pso.renderTarget->Height() : pso.depthBuffer->Height();
    tileSize = pso.tileSize;

    BuildTiles();
    BinTriangles(setups);
    RenderTiles(pso, setups);
}


void Renderer::BuildTiles()
{
    PROFILE_SCOPE("Renderer::BuildTiles");
    tiles.clear();
    uint ts = tileSize;
    uint tilesX = (width + ts - 1) / ts;
    uint tilesY = (height + ts - 1) / ts;
    for (uint ty = 0; ty < tilesY; ++ty)
    {
        for (uint tx = 0; tx < tilesX; ++tx)
        {
            uint2 mn(tx * ts, ty * ts);
            uint2 mx(std::min((tx + 1) * ts - 1, width - 1),
                     std::min((ty + 1) * ts - 1, height - 1));
            tiles.emplace_back(mn, mx);
        }
    }
}

void Renderer::BinTriangles(const std::vector<RasterizerCommon::TriangleSetup>& setups)
{
    PROFILE_SCOPE("Renderer::BinTriangles");
    for (auto& t : tiles) t.triangleIndices.clear();

    uint ts = tileSize;
    uint tilesX = (width + ts - 1) / ts;
    uint tilesY = (height + ts - 1) / ts;
    float rtWidthF = static_cast<float>(width) - 1.0f;
    float rtHeightF = static_cast<float>(height) - 1.0f;

    for (int setupIdx = 0; setupIdx < static_cast<int>(setups.size()); ++setupIdx)
    {
        const RasterizerCommon::TriangleSetup& s = setups[setupIdx];
        const auto& v0 = s.v0;
        const auto& v1 = s.v1;
        const auto& v2 = s.v2;

        float x0 = AfterMath::clamp(v0.Position.x, 0.0f, rtWidthF);
        float y0 = AfterMath::clamp(v0.Position.y, 0.0f, rtHeightF);
        float x1 = AfterMath::clamp(v1.Position.x, 0.0f, rtWidthF);
        float y1 = AfterMath::clamp(v1.Position.y, 0.0f, rtHeightF);
        float x2 = AfterMath::clamp(v2.Position.x, 0.0f, rtWidthF);
        float y2 = AfterMath::clamp(v2.Position.y, 0.0f, rtHeightF);

        float minX = std::min({ x0, x1, x2 });
        float maxX = std::max({ x0, x1, x2 });
        float minY = std::min({ y0, y1, y2 });
        float maxY = std::max({ y0, y1, y2 });

        if (minX >= rtWidthF || maxX <= 0.0f || minY >= rtHeightF || maxY <= 0.0f)
            continue;

        int tileX0 = AfterMath::clamp(static_cast<int>(std::floor(minX)) / static_cast<int>(ts), 0, static_cast<int>(tilesX) - 1);
        int tileY0 = AfterMath::clamp(static_cast<int>(std::floor(minY)) / static_cast<int>(ts), 0, static_cast<int>(tilesY) - 1);
        int tileX1 = AfterMath::clamp(static_cast<int>(std::ceil(maxX)) / static_cast<int>(ts), 0, static_cast<int>(tilesX) - 1);
        int tileY1 = AfterMath::clamp(static_cast<int>(std::ceil(maxY)) / static_cast<int>(ts), 0, static_cast<int>(tilesY) - 1);

        for (int ty = tileY0; ty <= tileY1; ++ty)
            for (int tx = tileX0; tx <= tileX1; ++tx)
                tiles[ty * tilesX + tx].triangleIndices.push_back(setupIdx);
    }
}

void Renderer::RenderTiles(const PipelineStateObject& pso, const std::vector<RasterizerCommon::TriangleSetup>& setups)
{
    PROFILE_SCOPE("Renderer::RenderTiles");

    RasterizerState rasterState;
    rasterState.cullMode = pso.cullMode;
    rasterState.fillMode = pso.fillMode;
    rasterState.depthFunc = pso.depthFunc;
    rasterState.depthWriteEnable = pso.depthWriteEnable;

    uint numTiles = static_cast<uint>(tiles.size());
    std::atomic<int> tileIndex(0);

    auto Task = [&]()
    {
        PROFILE_SCOPE("RenderTiles::tile worker");
        while (true)
        {
            uint idx = static_cast<uint>(tileIndex.fetch_add(1));
            if (idx >= numTiles) break;

            const Tile& tile = tiles[idx];
            for (int triIdx : tile.triangleIndices)
            {
                Rasterizer::RasterizeTriangle(setups[triIdx],
                                              rasterState,
                                              *pso.depthBuffer,
                                              pso.renderTarget.get(),
                                              pso.viewport,
                                              pso.pixelShader,
                                              pso.constantBuffer,
                                              &pso.textureTable,
                                              tile.min,
                                              tile.max);
            }
        }
    };
    ThreadUtils::DispatchWorkers(Task);
}

SOFTX_END
/////////////////////////////////////////////////////////////////
