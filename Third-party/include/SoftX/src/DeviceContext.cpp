/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "pch.h"

#include "../include/SoftX.h"
#include "RasterizerFactory.h"
#include "ThreadPoolManager.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

DeviceContext::DeviceContext(): vertexShader(nullptr),
                                pixelShader(nullptr),
                                vertexBuffer(),
                                indexBuffer(),
                                constantBuffer(),
                                renderTarget(nullptr),
                                depthBuffer(nullptr),
                                depthWriteEnable(true),
                                cullMode(CullMode::Back),
                                fillMode(FillMode::Solid),
                                viewport(),
                                tileSize(64),
                                rasterizer(CreateBestRasterizer())
{
}

DeviceContext::~DeviceContext() = default;

void DeviceContext::SetRenderTarget(std::shared_ptr<IRenderTarget> rt, bool createDepthBuffer)
{
    renderTarget = std::move(rt);
    if (createDepthBuffer && renderTarget)
    {
        uint2 newSize = renderTarget->Size();
        if (!depthBuffer || depthBuffer->Size() != newSize)
            depthBuffer = std::make_shared<DepthBuffer>(newSize);
    }
    else
    {
        depthBuffer.reset();
    }
}

void DeviceContext::Clear(const float4& color)
{
    PROFILE_SCOPE("DeviceContext::Clear");

    if (renderTarget)
    {
        renderTarget->Clear(color);
    }
}

void DeviceContext::ClearDepth(const float& depth)
{
    PROFILE_SCOPE("DeviceContext::ClearDepth");

    if (depthBuffer)
    {
        depthBuffer->Clear(depth);
    }
}

void DeviceContext::ClearColorAndDepth(const float4& color, const float& depth)
{
    PROFILE_SCOPE("DeviceContext::ClearColorAndDepth");

    auto& pool = ThreadPoolManager::Get();
    if (renderTarget && depthBuffer && (pool.threadCount() > 0))
    {
        pool.enqueue([this, color] { Clear(color); });
        pool.enqueue([this, depth] { ClearDepth(depth); });
        pool.wait();
    }
    else
    {
        Clear(color);
        ClearDepth(depth);
    }
}

bool DeviceContext::Validate(std::string* errorMsg) const
{
    bool result = true;

    if (!vertexShader)
    {
        if (errorMsg)
            *errorMsg = "Vertex shader not set ";
        result = false;
    }
    if (renderTarget != nullptr && !pixelShader)
    {
        if (errorMsg)
            *errorMsg += "Pixel shader not set ";
        result = false;
    }
    if (vertexBuffer.IsEmpty())
    {
        if (errorMsg)
            *errorMsg += "Vertex buffer is empty ";
        result = false;
    }
    if (indexBuffer.IsEmpty())
    {
        if (errorMsg)
            *errorMsg += "Index buffer is empty ";
        result = false;
    }
    if (renderTarget == nullptr && depthBuffer == nullptr)
    {
        if (errorMsg)
            *errorMsg += "Render target not set ";
        result = false;
    }
    if (viewport.size.x <= 0.0f || viewport.size.y <= 0.0f)
    {
        if (errorMsg)
            *errorMsg += "Viewport has non-positive size ";
        result = false;
    }
    if (tileSize == 0)
    {
        if (errorMsg)
            *errorMsg += "Tile size is zero ";
        result = false;
    }

    return result;
}

SOFTX_END
/////////////////////////////////////////////////////////////////
