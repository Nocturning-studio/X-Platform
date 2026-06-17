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

void SoftXOcclusionCore::Initialize(uint depthMapSize)
{
    Shutdown();

    SoftX::PresentParameters params;
    params.BackBufferSize = uint2(1, 1);
    params.hDeviceWindow = nullptr;
    params.Windowed = true;
    params.Headless = true;

    m_device = std::make_unique<SoftX::Device>(params);

    // Создаём два depth-буфера для двойной буферизации
    m_depthBuffers[0] = std::make_unique<SoftX::DepthBuffer>(uint2(depthMapSize, depthMapSize));
    m_depthBuffers[1] = std::make_unique<SoftX::DepthBuffer>(uint2(depthMapSize, depthMapSize));

    // Начальные настройки контекста (общие для всех пользователей)
    SoftX::DeviceContext& ctx = m_device->GetImmediateContext();
    ctx.SetRenderTarget(nullptr, true);
    ctx.SetDepthBuffer(m_depthBuffers[m_writeIdx].get());
    ctx.SetCullMode(SoftX::CullMode::None);
    ctx.SetFillMode(SoftX::FillMode::Solid);
    ctx.SetDepthFunc(SoftX::ComparisonFunc::Less);
    ctx.SetDepthWriteEnable(true);
    ctx.SetViewport(SoftX::Viewport(0.0f, 0.0f, (float)depthMapSize, (float)depthMapSize, 0.0f, 1.0f));
    ctx.SetTileSize(64);
}

void SoftXOcclusionCore::Shutdown()
{
    if (m_buildFuture.valid())
        m_buildFuture.wait();

    m_depthBuffers[0].reset();
    m_depthBuffers[1].reset();
    m_device.reset();
}

void SoftXOcclusionCore::SwapBuffers()
{
    std::swap(m_writeIdx, m_readIdx);
}

void SoftXOcclusionCore::WaitForBuildAndSwap()
{
    if (m_buildFuture.valid())
    {
        m_buildFuture.wait();
        SwapBuffers();
    }
}

void SoftXOcclusionCore::StartBuildTask(std::function<void()> task)
{
    // Ждём завершения предыдущей задачи, если она ещё не закончилась
    if (m_buildFuture.valid())
        m_buildFuture.wait();

    // Запускаем новую асинхронную задачу
    m_buildFuture = std::async(std::launch::async, std::move(task));
}
////////////////////////////////////////////////////////////////////////////////
