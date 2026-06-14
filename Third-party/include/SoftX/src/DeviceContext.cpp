#include "pch.h"

#include <SoftX/SoftX.h>

SOFTX_BEGIN

DeviceContext::DeviceContext()
    : vertexShader(nullptr),
      pixelShader(nullptr),
      vertexBuffer(),
      indexBuffer(),
      constantBuffer(),
      renderTarget(nullptr),
      ownDepthBuffer(nullptr),
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

void DeviceContext::SetVertexShader(VertexShader shader)
{
    vertexShader = std::move(shader);
}

VertexShader DeviceContext::GetVertexShader() const
{
    return vertexShader;
}

void DeviceContext::SetGeometryShader(GeometryShader shader)
{
    geometryShader = std::move(shader);
}

GeometryShader DeviceContext::GetGeometryShader() const
{
    return geometryShader;
}

void DeviceContext::SetPixelShader(PixelShader shader)
{
    pixelShader = std::move(shader);
}

PixelShader DeviceContext::GetPixelShader() const
{
    return pixelShader;
}

void DeviceContext::SetVertexBuffer(const VertexBuffer& buffer)
{
    vertexBuffer = buffer;
}

VertexBuffer DeviceContext::GetVertexBuffer() const
{
    return vertexBuffer;
}

void DeviceContext::SetIndexBuffer(const IndexBuffer& buffer)
{
    indexBuffer = buffer;
}

IndexBuffer DeviceContext::GetIndexBuffer() const
{
    return indexBuffer;
}

void DeviceContext::SetConstantBuffer(const ConstantBuffer& buffer)
{
    constantBuffer = buffer;
}

ConstantBuffer DeviceContext::GetConstantBuffer() const
{
    return constantBuffer;
}

void DeviceContext::SetTexture(const std::string& name, const ITexture* texture, SamplerState sampler)
{
    textureTable.Set(name, texture, sampler);
}

void DeviceContext::SetRenderTarget(IRenderTarget* target)
{
    renderTarget = target;
}

IRenderTarget* DeviceContext::GetRenderTarget() const
{
    return renderTarget;
}

void DeviceContext::SetDepthBuffer(DepthBuffer* dpthBuffer)
{
    depthBuffer = dpthBuffer;
}

DepthBuffer* DeviceContext::GetDepthBuffer() const
{
    return depthBuffer;
}

void DeviceContext::SetDepthWriteEnable(bool enable)
{
    depthWriteEnable = enable;
}

bool DeviceContext::GetDepthWriteEnable() const
{
    return depthWriteEnable;
}

void DeviceContext::SetRenderTarget(IRenderTarget* rt, bool createDepthBuffer)
{
    renderTarget = rt;
    if (createDepthBuffer && rt)
    {
        uint2 newSize = rt->Size();
        if (!ownDepthBuffer || ownDepthBuffer->Size() != newSize)
        {
            ownDepthBuffer = std::make_unique<DepthBuffer>(newSize);
        }
        depthBuffer = ownDepthBuffer.get();
    }
    else
    {
        depthBuffer = nullptr;
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

void DeviceContext::ClearDepth(float depth)
{
    PROFILE_SCOPE("DeviceContext::ClearDepth");

    if (depthBuffer)
    {
        depthBuffer->Clear(depth);
    }
}

void DeviceContext::ClearColorAndDepth(const float4& color, float depth)
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

void DeviceContext::SetCullMode(CullMode mode)
{
    cullMode = mode;
}

CullMode DeviceContext::GetCullMode() const
{
    return cullMode;
}

void DeviceContext::SetFillMode(FillMode mode)
{
    fillMode = mode;
}

FillMode DeviceContext::GetFillMode() const
{
    return fillMode;
}

void DeviceContext::SetDepthFunc(ComparisonFunc func)
{
    depthFunc = func;
}

ComparisonFunc DeviceContext::GetDepthFunc() const
{
    return depthFunc;
}

void DeviceContext::SetViewport(const Viewport& vp)
{
    viewport = vp;
}

Viewport DeviceContext::GetViewport() const
{
    return viewport;
}

void DeviceContext::SetTileSize(uint size)
{
    tileSize = size;
}

uint DeviceContext::GetTileSize() const
{
    return tileSize;
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
