////////////////////////////////////////////////////////////////////////////////
// Created: 04.10.2026 18:54:55
// Author: NS_Deathman
// File: GBufferStage.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "GBufferStage.h"
////////////////////////////////////////////////////////////////////////////////
void CGBufferStage::OnAttach()
{
    RebuildBasePSO();
}

void CGBufferStage::OnDeviceReset()
{
    RebuildBasePSO();
}

void CGBufferStage::OnDetach()
{
    DestroyBasePSO();
}

void CGBufferStage::RebuildBasePSO()
{
    DestroyBasePSO();

    RHI_PipelineStateDesc psd;

    // Шейдеры и input layout намеренно пустые: SetPipelineState разбиндит
    // то, что было привязано legacy-путём, и применит только FF-состояния.
    // Legacy-путь затем восстановит шейдеры/декларацию через state blocks
    // и SetGeometryLegacy.
    psd.vertexShader = {};
    psd.pixelShader = {};
    psd.inputLayout = {};

    psd.topologyType = RHI_PrimitiveTopologyType::Triangle;

    psd.blend = RHI_BlendState::Opaque();                  // blend off, color write all
    psd.depthStencil = RHI_DepthStencilState::Default();        // depth test+write, LessEqual
    psd.rasterizer = RHI_RasterizerState::CullBack();           // back-face cull

    // В DX9 rtvFormats/dsvFormat не участвуют в биндинге (он идёт через
    // SetRenderTargets). Заполняем для валидации/будущих бэкендов.
    psd.rtvCount = 0;
    psd.dsvFormat = RHI_Format::D24_UNORM_S8_UINT;

    psd.debugName = "GBufferStage.BasePSO";

    m_basePSO = Engine.RHI->CreatePipelineState(psd);
    if (!m_basePSO.IsValid())
        Msg("! [GBufferStage] CreatePipelineState failed");
}

void CGBufferStage::DestroyBasePSO()
{
    if (m_basePSO.IsValid())
    {
        Engine.RHI->DestroyPipelineState(m_basePSO);
        m_basePSO = {};
    }
}

void CGBufferStage::Execute()
{
    PROFILE_FUNCTION();

    BindTargets();
    ClearTargets();
    RenderScene();
}

void CGBufferStage::BindTargets()
{
    Engine.RHI->SetRenderTargets(m_res.GBufferRTV, 3, m_res.DepthDSV);

    RHI_Viewport vp{ 0, 0, m_res.Width(), m_res.Height(), 0.0f, 1.0f };
    Engine.RHI->SetViewport(vp);

    if (m_basePSO.IsValid())
        Engine.RHI->SetPipelineState(m_basePSO);
    else
        Msg("! [GBufferStage] base PSO is invalid, depth state may be stale");
}

void CGBufferStage::ClearTargets()
{
    const fvec4 clear{ 0.0f, 0.0f, 0.0f, 0.0f };
    for (const auto& rtv : m_res.GBufferRTV)
    {
        if (rtv.IsValid())
            Engine.RHI->ClearRenderTarget(rtv, clear);
    }

    if (m_res.FrameColorRTV.IsValid())
        Engine.RHI->ClearRenderTarget(m_res.FrameColorRTV, fvec4{ 0.0f, 0.0f, 0.0f, 0.0f });

    if (m_res.DepthDSV.IsValid())
        Engine.RHI->ClearDepthStencil(m_res.DepthDSV, 1.0f, 0);
}

void CGBufferStage::RenderScene()
{
    RenderImplementation.set_active_phase(CRender::PHASE_NORMAL);
    RenderImplementation.Scene.Render(RenderImplementation.m_scene_visibility_data, SceneRenderPresets::Opaque);
    RenderImplementation.Scene.Render(RenderImplementation.m_scene_visibility_data, SceneRenderPresets::HUDOnly);
    Engine.RHI->InvalidateCache();
}
////////////////////////////////////////////////////////////////////////////////
