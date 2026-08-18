////////////////////////////////////////////////////////////////////////////////
// Created: 17.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "SoftXOcclusionCore.h"
////////////////////////////////////////////////////////////////////////////////
SoftXOcclusionCore::~SoftXOcclusionCore()
{
    Shutdown();
}

void SoftXOcclusionCore::Initialize(uint2 depthMapSize)
{
    Shutdown();

    m_depth_resolution = depthMapSize;

    Msg("DepthBuffer resolution: %d, %d", m_depth_resolution.x, m_depth_resolution.y);

    SoftX::PresentParameters params;
    params.BackBufferSize = m_depth_resolution;
    params.hDeviceWindow = nullptr;
    params.Windowed = true;
    params.Headless = true;

    m_device = std::make_unique<SoftX::Device>(params, 4);

    // Создаём два depth-буфера для двойной буферизации
    m_depthBuffers[0] = std::make_unique<SoftX::DepthBuffer>(m_depth_resolution, 4);
    m_depthBuffers[1] = std::make_unique<SoftX::DepthBuffer>(m_depth_resolution, 4);

    // Начальные настройки контекста (общие для всех пользователей)
    SoftX::DeviceContext& ctx = m_device->GetImmediateContext();
    ctx.SetRenderTarget(nullptr, true);
    ctx.SetDepthBuffer(m_depthBuffers[m_writeIdx]);
    ctx.SetCullMode(SoftX::CullMode::None);
    ctx.SetFillMode(SoftX::FillMode::Solid);
    ctx.SetDepthFunc(SoftX::ComparisonFunc::Less);
    ctx.SetDepthWriteEnable(true);
    ctx.SetViewport(SoftX::Viewport(0.0f, 0.0f, (float)depthMapSize.x, (float)depthMapSize.y, 0.0f, 1.0f));
    ctx.SetTileSize(64);

    m_readBufferReady = false;
}

void SoftXOcclusionCore::Shutdown()
{
    if (m_buildFuture.valid())
        m_buildFuture.wait();

    m_depthBuffers[0].reset();
    m_depthBuffers[1].reset();
    m_device.reset();
    m_readBufferReady = false;
}

void SoftXOcclusionCore::SwapBuffers()
{
    std::swap(m_writeIdx, m_readIdx);
    m_readBufferReady = true;
}

void SoftXOcclusionCore::WaitForBuildAndSwap()
{
    if (m_buildFuture.valid())
    {
        m_buildFuture.wait();
        SwapBuffers();
        m_buildFuture = {};
    }
}
////////////////////////////////////////////////////////////////////////////////
