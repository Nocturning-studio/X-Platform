////////////////////////////////////////////////////////////////////////////////
// Created: 04.10.2026 18:58:10
// Author: NS_Deathman
// File: DeferredLightingResources.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "DeferredLightingResources.h"
////////////////////////////////////////////////////////////////////////////////
CDeferredLightingResources::~CDeferredLightingResources()
{
    Destroy();
}

void CDeferredLightingResources::Create()
{
    m_width = Engine.RHI->GetBackBufferWidth();
    m_height = Engine.RHI->GetBackBufferHeight();

    CreateTextures();
    CreateViews();
}

void CDeferredLightingResources::Destroy()
{
    DestroyViews();

    FrameColor.Clear();
    DepthStencil.Clear();
    for (auto& g : GBuffer)
        g.Clear();

    m_width = 0;
    m_height = 0;
}

void CDeferredLightingResources::OnDeviceReset()
{
    Destroy();
    Create();
}

void CDeferredLightingResources::CreateTextures()
{
    FrameColor = Engine.RHI->CreateRenderTarget(m_width, m_height, RHI_Format::RGBA16_FLOAT);
    DepthStencil = Engine.RHI->CreateDepthStencil(m_width, m_height, RHI_Format::D24_UNORM_S8_UINT);
    GBuffer[0] = Engine.RHI->CreateRenderTarget(m_width, m_height, RHI_Format::RGBA8_UNORM);
    GBuffer[1] = Engine.RHI->CreateRenderTarget(m_width, m_height, RHI_Format::RGBA16_FLOAT);
    GBuffer[2] = Engine.RHI->CreateRenderTarget(m_width, m_height, RHI_Format::RGBA8_UNORM);
}

void CDeferredLightingResources::CreateViews()
{
    if (FrameColor)
        FrameColorRTV = FrameColor->CreateRTV();

    for (int i = 0; i < 3; ++i)
    {
        if (GBuffer[i])
            GBufferRTV[i] = GBuffer[i]->CreateRTV();
    }

    if (DepthStencil)
        DepthDSV = DepthStencil->CreateDSV();
}

void CDeferredLightingResources::DestroyViews()
{
    if (FrameColorRTV.IsValid())  
        Engine.RHI->DestroyRTV(FrameColorRTV);

    for (auto& rtv : GBufferRTV)
        if (rtv.IsValid()) 
            Engine.RHI->DestroyRTV(rtv);

    if (DepthDSV.IsValid())       
        Engine.RHI->DestroyDSV(DepthDSV);

    FrameColorRTV = {};
    for (auto& rtv : GBufferRTV)
        rtv = {};
    DepthDSV = {};
}
////////////////////////////////////////////////////////////////////////////////
