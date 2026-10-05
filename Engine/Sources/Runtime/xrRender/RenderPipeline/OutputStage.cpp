////////////////////////////////////////////////////////////////////////////////
// Created: 04.10.2026 15:06:51
// Author: NS_Deathman
// File: OutputStage.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "OutputStage.h"
#include "DeferredLightingResources.h"
////////////////////////////////////////////////////////////////////////////////
namespace
{
    constexpr LPCSTR kFrameSampler = "s_frame";
}
////////////////////////////////////////////////////////////////////////////////
void CFrameOutputStage::OnAttach()
{
    m_pass.SetVertexShader("utils\\fullscreen.hlsl", "vs_main");
    m_pass.SetPixelShader("deferred_lighting_pipeline\\frame_output_stage.hlsl", "ps_main");

    m_pass_valid = (m_pass.Compile(*Engine.RHI.Get()) == TRUE);
    if (!m_pass_valid)
        Msg("! [FrameOutputStage] failed to compile resolve pass");

    RHI_SamplerDesc sd = RHI_SamplerDesc::Linear();
    sd.addressU = sd.addressV = sd.addressW = RHI_TextureAddress::Clamp;
    m_pass.SetSamplerDesc(kFrameSampler, sd);
}

void CFrameOutputStage::OnDeviceReset()
{
    if (!m_pass_valid)
        m_pass_valid = (m_pass.Compile(*Engine.RHI.Get()) == TRUE);
}

void CFrameOutputStage::OnDetach()
{
    m_pass.Release();
    m_pass_valid = false;
}

void CFrameOutputStage::Execute()
{
    PROFILE_FUNCTION();

    if (!m_pass_valid)
        return;

    if (!m_res.FrameColor)
        return;

    const RHI_RenderTargetView bb_rtv = Engine.RHI->GetBackBufferRTV();
    const RHI_DepthStencilView bb_dsv = Engine.RHI->GetBackBufferDSV();

    RHI_RenderTargetView rtvs[1] = { bb_rtv };
    Engine.RHI->SetRenderTargets(rtvs, 1, bb_dsv);

    RHI_Viewport vp{ 0, 0, Engine.RHI->GetBackBufferWidth(), Engine.RHI->GetBackBufferHeight(), 0.0f, 1.0f };
    Engine.RHI->SetViewport(vp);

    Engine.RHI->SetBlendState(RHI_BlendState::Opaque());
    Engine.RHI->SetDepthStencilState(RHI_DepthStencilState::NoDepth());
    Engine.RHI->SetRasterizerState(RHI_RasterizerState::CullNone());

    m_pass.SetTexture(kFrameSampler, m_res.FrameColor);
    m_pass.Apply(*Engine.RHI.Get());

    Engine.RHI->DrawFullscreen();
}
////////////////////////////////////////////////////////////////////////////////
