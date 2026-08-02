/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "../include/LibInternal.h"
#include "../include/ThirdPartyIncluding.h"
#include "RasterizerCommon.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

/**
 * Manages a uniform grid of tiles covering either the full render target
 * or a specific scissor rectangle. Tiles are always identically sized
 * (width and height chosen as close to the requested size as possible
 * while exactly dividing the target area).
 */
class TileGrid
{
public:
    TileGrid() = default;

    /**
     * Builds a grid of tiles.
     *
     * @param inWidth       Width of the full render target in pixels.
     * @param inHeight      Height of the full render target in pixels.
     * @param inTileSize    Desired side length of a tile (may be adjusted).
     * @param scissorEnable If true, the tile grid is restricted to the scissor rect.
     * @param scissor       Scissor rectangle in pixel coordinates (uint).
     */
    void Build(uint inWidth, uint inHeight, uint inTileSize,
               bool scissorEnable = false, const Rect& scissor = Rect())
    {
        width         = inWidth;
        height        = inHeight;
        targetTileSize = inTileSize;
        scissorActive = scissorEnable;

        // Determine the region covered by tiles
        if (scissorActive)
        {
            gridStartX = std::max(0u, scissor.left());
            gridStartY = std::max(0u, scissor.top());
            gridEndX   = std::min(width,  scissor.right());
            gridEndY   = std::min(height, scissor.bottom());

            if (gridStartX >= gridEndX || gridStartY >= gridEndY)
            {
                tiles.clear();
                tilesX = tilesY = 0;
                actualTileW = actualTileH = 0;
                return;
            }
        }
        else
        {
            gridStartX = 0;
            gridStartY = 0;
            gridEndX   = width;
            gridEndY   = height;
        }

        const uint areaW = gridEndX - gridStartX;
        const uint areaH = gridEndY - gridStartY;

        // Pick exact tile sizes that divide the area dimensions
        actualTileW = ChooseDivisor(areaW, targetTileSize);
        actualTileH = ChooseDivisor(areaH, targetTileSize);

        tilesX = areaW / actualTileW;
        tilesY = areaH / actualTileH;

        tiles.clear();
        tiles.reserve(tilesX * tilesY);

        for (uint ty = 0; ty < tilesY; ++ty)
        {
            for (uint tx = 0; tx < tilesX; ++tx)
            {
                const uint x0 = gridStartX + tx * actualTileW;
                const uint y0 = gridStartY + ty * actualTileH;
                const uint x1 = x0 + actualTileW - 1;  // inclusive
                const uint y1 = y0 + actualTileH - 1;  // inclusive

                tiles.emplace_back(uint2(x0, y0), uint2(x1, y1));
            }
        }
    }

    /**
     * Assigns each triangle from the provided list to every tile
     * whose screen-space axis-aligned bounding box overlaps it.
     * Takes the active grid offset and actual tile dimensions into account.
     *
     * @param setups   Pre-computed triangle setups (in screen space).
     */
    void BinTriangles(const std::vector<RasterizerCommon::TriangleSetup>& setups)
    {
        for (auto& tile : tiles)
            tile.triangleIndices.clear();

        if (tiles.empty() || setups.empty())
            return;

        const float rtWidth  = static_cast<float>(width)  - 1.0f;
        const float rtHeight = static_cast<float>(height) - 1.0f;

        const int tilesXInt = static_cast<int>(tilesX);
        const int tilesYInt = static_cast<int>(tilesY);
        const int tileW     = static_cast<int>(actualTileW);
        const int tileH     = static_cast<int>(actualTileH);
        const int startX    = static_cast<int>(gridStartX);
        const int startY    = static_cast<int>(gridStartY);

        for (int setupIndex = 0; setupIndex < static_cast<int>(setups.size()); ++setupIndex)
        {
            const RasterizerCommon::TriangleSetup& setup = setups[setupIndex];
            const auto& v0 = setup.v0;
            const auto& v1 = setup.v1;
            const auto& v2 = setup.v2;

            float clampedX0 = AfterMath::clamp(v0.ClipSpacePosition.x, 0.0f, rtWidth);
            float clampedY0 = AfterMath::clamp(v0.ClipSpacePosition.y, 0.0f, rtHeight);
            float clampedX1 = AfterMath::clamp(v1.ClipSpacePosition.x, 0.0f, rtWidth);
            float clampedY1 = AfterMath::clamp(v1.ClipSpacePosition.y, 0.0f, rtHeight);
            float clampedX2 = AfterMath::clamp(v2.ClipSpacePosition.x, 0.0f, rtWidth);
            float clampedY2 = AfterMath::clamp(v2.ClipSpacePosition.y, 0.0f, rtHeight);

            float minX = std::min({ clampedX0, clampedX1, clampedX2 });
            float maxX = std::max({ clampedX0, clampedX1, clampedX2 });
            float minY = std::min({ clampedY0, clampedY1, clampedY2 });
            float maxY = std::max({ clampedY0, clampedY1, clampedY2 });

            if (minX >= rtWidth || maxX <= 0.0f ||
                minY >= rtHeight || maxY <= 0.0f)
                continue;

            // Convert screen coordinates to tile indices, using actual tile sizes
            int tileX0 = std::max((static_cast<int>(std::floor(minX)) - startX) / tileW, 0);
            int tileY0 = std::max((static_cast<int>(std::floor(minY)) - startY) / tileH, 0);
            int tileX1 = std::min((static_cast<int>(std::ceil(maxX)) - startX) / tileW, tilesXInt - 1);
            int tileY1 = std::min((static_cast<int>(std::ceil(maxY)) - startY) / tileH, tilesYInt - 1);

            for (int ty = tileY0; ty <= tileY1; ++ty)
                for (int tx = tileX0; tx <= tileX1; ++tx)
                    tiles[ty * tilesXInt + tx].triangleIndices.push_back(setupIndex);
        }
    }

    const std::vector<Tile>& GetTiles() const { return tiles; }

    uint GetWidth()          const { return width; }
    uint GetHeight()         const { return height; }
    uint GetTileSize()       const { return targetTileSize; }
    uint GetActualTileW()    const { return actualTileW; }
    uint GetActualTileH()    const { return actualTileH; }
    uint GetTilesX()         const { return tilesX; }
    uint GetTilesY()         const { return tilesY; }
    uint GetGridStartX()     const { return gridStartX; }
    uint GetGridStartY()     const { return gridStartY; }
    uint GetGridEndX()       const { return gridEndX; }
    uint GetGridEndY()       const { return gridEndY; }

private:
    /**
     * Returns a divisor of @p areaSize that is as close as possible to
     * @p desired. If multiple divisors have the same distance, the smaller
     * one is preferred to avoid exceeding the desired size too much.
     */
    static uint ChooseDivisor(uint areaSize, uint desired)
    {
        if (areaSize == 0 || desired == 0)
            return 1;

        uint bestDivisor = 1;
        int bestDist = std::abs(static_cast<int>(desired) - 1);

        // Iterate over possible divisors (up to areaSize)
        for (uint d = 1; d <= areaSize; ++d)
        {
            if (areaSize % d == 0)
            {
                int dist = std::abs(static_cast<int>(desired) - static_cast<int>(d));
                if (dist < bestDist || (dist == bestDist && d < bestDivisor))
                {
                    bestDivisor = d;
                    bestDist = dist;
                }
            }
        }
        return bestDivisor;
    }

    std::vector<Tile> tiles;
    uint width    = 0;
    uint height   = 0;
    uint targetTileSize = 0;
    uint actualTileW = 0;
    uint actualTileH = 0;
    uint tilesX   = 0;
    uint tilesY   = 0;

    uint gridStartX = 0;
    uint gridStartY = 0;
    uint gridEndX   = 0;
    uint gridEndY   = 0;
    bool scissorActive = false;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
