/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "../include/SoftX.h"
#include "ThreadUtils.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

DeviceContext::DeviceContext()
{
}

DeviceContext::DeviceContext(const PipelineStateObject& initialState) : backState(initialState), 
                                                                        frontState(initialState)
{
}

DeviceContext::~DeviceContext() = default;

void DeviceContext::SetVertexShader(VertexShader shader) 
{
    std::lock_guard<std::mutex> lock(stateMutex);
    backState.vertexShader = std::move(shader);
}

void DeviceContext::SetGeometryShader(GeometryShader shader)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    backState.geometryShader = std::move(shader);
}

void DeviceContext::SetPixelShader(PixelShader shader) 
{
    std::lock_guard<std::mutex> lock(stateMutex);
    backState.pixelShader = std::move(shader);
}

void DeviceContext::SetIndexBuffer(const IndexBuffer& buffer)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    backState.indexBuffer = buffer;
}

void DeviceContext::SetVertexBuffer(const VertexBuffer& buffer) 
{
    std::lock_guard<std::mutex> lock(stateMutex);
    backState.vertexBuffer = buffer;
}

void DeviceContext::SetTexture(const std::string& name,
                               std::shared_ptr<const ITexture> texture,
                               const SamplerState& sampler) 
{
    std::lock_guard<std::mutex> lock(stateMutex);
    backState.textureTable.Set(name, std::move(texture), sampler);
}

void DeviceContext::SetRenderTarget(std::shared_ptr<IRenderTarget> target, bool createDepthBuffer) 
{
    std::lock_guard<std::mutex> lock(stateMutex);
    backState.renderTarget = std::move(target);
    if (createDepthBuffer && backState.renderTarget) 
    {
        uint2 size = backState.renderTarget->Size();
        if (!backState.depthBuffer || backState.depthBuffer->Size() != size)
            backState.depthBuffer = std::make_shared<DepthBuffer>(size);
    }
}

void DeviceContext::SetDepthBuffer(std::shared_ptr<DepthBuffer> depth) 
{
    std::lock_guard<std::mutex> lock(stateMutex);
    backState.depthBuffer = std::move(depth);
}

void DeviceContext::SetViewport(const Viewport& vp) 
{
    std::lock_guard<std::mutex> lock(stateMutex);
    backState.viewport = vp;
}

void DeviceContext::SetTileSize(uint size) 
{
    std::lock_guard<std::mutex> lock(stateMutex);
    backState.tileSize = size;
}

void DeviceContext::SetDepthWriteEnable(bool enable)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    backState.depthWriteEnable = enable;
}

void DeviceContext::SetCullMode(CullMode mode)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    backState.cullMode = mode;
}

void DeviceContext::SetFillMode(FillMode mode)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    backState.fillMode = mode;
}

void DeviceContext::SetDepthFunc(ComparisonFunc func)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    backState.depthFunc = func;
}

void DeviceContext::SetConstantBuffer(const ConstantBuffer& buffer)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    backState.constantBuffer = buffer;
}

void DeviceContext::CommitState()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    frontState = backState;
}

PipelineStateObject DeviceContext::CaptureState() const
{
    return frontState;
}

void DeviceContext::Clear(ClearFlags flags,
                          const float4& color,
                          float depth)
{
    if (flags == ClearFlags::None) return;

    std::lock_guard<std::mutex> lock(drawMutex);
    PROFILE_SCOPE("DeviceContext::Clear");
    CommitState();

    PipelineStateObject state = frontState;

    const bool clearColor = !!(flags & ClearFlags::RenderTarget) && state.renderTarget;
    const bool clearDepth = !!(flags & ClearFlags::DepthBuffer) && state.depthBuffer;

    if (!clearColor && !clearDepth) return;

    const bool useParallel = clearColor && clearDepth;

    if (useParallel) 
    {
        state.renderTarget->Clear(color);
        ThreadUtils::DispatchWorkers([db = state.depthBuffer, depth]{ db->Clear(depth); });
    }
    else 
    {
        if (clearColor) state.renderTarget->Clear(color);
        if (clearDepth) state.depthBuffer->Clear(depth);
    }
}

SOFTX_END
/////////////////////////////////////////////////////////////////
