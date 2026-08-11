/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "../include/Types.h"
#include "../include/DepthBuffer.h"
#include "../include/Exceptions.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

// ── Tile ─────────────────────────────────────────────────────
struct Tile
{
    uint2 min;
    uint2 max;
    std::vector<int> triangleIndices;

    Tile(uint2 min, uint2 max) : min(min), max(max) {}
};

// ── Rasterizer state ────────────────────────────────────────
struct RasterizerState
{
    CullMode cullMode = CullMode::Back;
    FillMode fillMode = FillMode::Solid;
    ComparisonFunc depthFunc = ComparisonFunc::Less;
    bool depthWriteEnable = true;
    Rect scissorRect;
    bool scissorEnable = false;
};

// ── Occlusion pipeline state ─────────────────────────────────
struct OcclusionPipelineState
{
    using OcclusionVertexShader = std::function<Interpolant(const Vertex&, const ConstantBuffer&)>;

    VertexBuffer vertexBuffer;
    IndexBuffer  indexBuffer;
    ConstantBuffer constantBuffer;
    OcclusionVertexShader vertexShader;

    std::shared_ptr<DepthBuffer> depthBuffer;
    Viewport viewport;
    CullMode cullMode = CullMode::Back;
    ComparisonFunc depthFunc = ComparisonFunc::Less;
    bool depthWriteEnable = false;
    uint tileSize = 64;

    Rect scissorRect;
    bool scissorEnable = false;

    void Validate(uint32_t requiredResourcesMask) const
    {
        std::string errors;
        auto check = [&](PipelineResource res, const char* name, bool present)
        {
            if ((requiredResourcesMask & static_cast<uint32_t>(res)) && !present)
            {
                if (!errors.empty()) errors += "; ";
                errors += name;
            }
        };
        check(PipelineResource::VertexShader, "vertex shader", vertexShader != nullptr);
        check(PipelineResource::VertexBuffer, "vertex buffer", !vertexBuffer.IsEmpty());
        check(PipelineResource::IndexBuffer, "index buffer", !indexBuffer.IsEmpty());
        check(PipelineResource::ConstantBuffer, "constant buffer", constantBuffer.Size() > 0);
        check(PipelineResource::DepthBuffer, "depth buffer", depthBuffer != nullptr);
        check(PipelineResource::Viewport, "viewport", viewport.size.x > 0 && viewport.size.y > 0);
        check(PipelineResource::TileSize, "tile size > 0", tileSize > 0);

        if (!errors.empty())
            SOFTX_THROW(InvalidState("Missing required pipeline state: " + errors));
    }
};


SOFTX_END
/////////////////////////////////////////////////////////////////
