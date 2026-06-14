#pragma once

#include "LibInternal.h"
#include "RasterizerInterface.h"

SOFTX_BEGIN

class Renderer
{
public:
    Renderer(IRasterizer& rasterizer,
             IRenderTarget* renderTarget,
             DepthBuffer& depthBuffer,
             const PixelShader& pixelShader,
             const ConstantBuffer& constantBuffer,
             const TextureTable* textureTable,
             const RasterizerState& state,
             uint tileSize);

    void Execute(const std::vector<VertexOutput>& verts, const std::vector<int3>& triangles);

    const std::vector<Tile>& GetTiles() const
    {
        return tiles;
    }

private:
    void BuildTiles(uint width, uint height);
    void BinTriangles(const std::vector<VertexOutput>& verts, const std::vector<int3>& triangles);
    void RenderTiles();

    IRasterizer& rasterizer;
    IRenderTarget* renderTarget;
    DepthBuffer& depthBuffer;
    const PixelShader& pixelShader;
    const ConstantBuffer& constantBuffer;
    const TextureTable* textureTable;
    RasterizerState state;
    uint tileSize;
    uint width;
    uint height;
    std::vector<Tile> tiles;
    const std::vector<VertexOutput>* verts = nullptr;
    const std::vector<int3>* triangles = nullptr;
};

SOFTX_END
