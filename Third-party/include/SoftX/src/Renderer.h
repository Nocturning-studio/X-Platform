/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "../include/Types.h"
#include "RasterizerCommon.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class Renderer
{
public:
    Renderer();

    void Execute(const PipelineStateObject& pso, std::vector<RasterizerCommon::TriangleSetup>& setups);

    const std::vector<Tile>& GetTiles() const { return tiles; }

private:
    void BuildTiles();
    void BinTriangles(const std::vector<RasterizerCommon::TriangleSetup>& setups);
    void RenderTiles(const PipelineStateObject& pso, const std::vector<RasterizerCommon::TriangleSetup>& setups);

    uint width = 0;
    uint height = 0;
    uint tileSize = 0;
    std::vector<Tile> tiles;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
