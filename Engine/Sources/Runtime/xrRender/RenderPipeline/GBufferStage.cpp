////////////////////////////////////////////////////////////////////////////////
// Created: 04.10.2026 18:54:55
// Author: NS_Deathman
// File: GBufferStage.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "GBufferStage.h"
////////////////////////////////////////////////////////////////////////////////
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

    Engine.RHI->SetBlendState(RHI_BlendState::Opaque());
    Engine.RHI->SetDepthStencilState(RHI_DepthStencilState::Default());
    Engine.RHI->SetRasterizerState(RHI_RasterizerState::CullBack());
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
    Engine.RHI->InvalidateStateCache();
}
////////////////////////////////////////////////////////////////////////////////
