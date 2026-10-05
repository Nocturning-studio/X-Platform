////////////////////////////////////////////////////////////////////////////////
// Created: 05.10.2026 14:06:55
// Author: NS_Deathman
// File: IndirectLightingStage.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "IndirectLightingStage.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
////////////////////////////////////////////////////////////////////////////////
namespace
{
    constexpr LPCSTR kGBuffer0Sampler = "s_gbuffer_0";
    constexpr LPCSTR kGBuffer1Sampler = "s_gbuffer_1";
    constexpr LPCSTR kGBuffer2Sampler = "s_gbuffer_2";
}
////////////////////////////////////////////////////////////////////////////////
void CIndirectLightingStage::OnAttach()
{
    m_pass.SetVertexShader("utils\\fullscreen.hlsl", "vs_main");
    m_pass.SetPixelShader("deferred_lighting_pipeline\\indirect_lighting_stage.hlsl", "ps_main");

    m_pass_valid = (m_pass.Compile(*Engine.RHI.Get()) == TRUE);
    if (!m_pass_valid)
        Msg("! [IndirectLightingStage] failed to compile pass");

    RHI_SamplerDesc sd = RHI_SamplerDesc::Linear();
    sd.addressU = sd.addressV = sd.addressW = RHI_TextureAddress::Clamp;
    m_pass.SetSamplerDesc(kGBuffer0Sampler, sd);
    m_pass.SetSamplerDesc(kGBuffer1Sampler, sd);
    m_pass.SetSamplerDesc(kGBuffer2Sampler, sd);
}

void CIndirectLightingStage::OnDeviceReset()
{
    if (!m_pass_valid)
        m_pass_valid = (m_pass.Compile(*Engine.RHI.Get()) == TRUE);
}

void CIndirectLightingStage::OnDetach()
{
    m_pass.Release();
    m_pass_valid = false;
}

void CIndirectLightingStage::Execute()
{
    PROFILE_FUNCTION();

    if (!m_pass_valid)
        return;

    if (!m_res.FrameColorRTV.IsValid() || !m_res.GBuffer[0])
        return;

    RHI_RenderTargetView rtvs[1] = { m_res.FrameColorRTV };
    RHI_DepthStencilView  no_ds{};
    Engine.RHI->SetRenderTargets(rtvs, 1, no_ds);

    RHI_Viewport vp{ 0, 0, m_res.Width(), m_res.Height(), 0.0f, 1.0f };
    Engine.RHI->SetViewport(vp);

    Engine.RHI->SetBlendState(RHI_BlendState::PureAdditive());
    Engine.RHI->SetDepthStencilState(RHI_DepthStencilState::Default());
    Engine.RHI->SetRasterizerState(RHI_RasterizerState::CullBack());

    m_pass.SetTexture(kGBuffer0Sampler, m_res.GBuffer[0]);
    m_pass.SetTexture(kGBuffer1Sampler, m_res.GBuffer[1]);
    m_pass.SetTexture(kGBuffer2Sampler, m_res.GBuffer[2]);

    m_pass.Apply(*Engine.RHI.Get());
    Engine.RHI->DrawFullscreen();
}
////////////////////////////////////////////////////////////////////////////////
