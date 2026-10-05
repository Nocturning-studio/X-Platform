////////////////////////////////////////////////////////////////////////////////
// Created: 04.10.2026 18:58:51
// Author: NS_Deathman
// File: DeferredLightingPipeline.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "DeferredLightingPipeline.h"
////////////////////////////////////////////////////////////////////////////////
#include "GBufferStage.h"
#include "IndirectLightingStage.h"
#include "OutputStage.h"
////////////////////////////////////////////////////////////////////////////////
void CDeferredLightingPipeline::Initialize()
{
    m_resources.Create();

    AddStage(std::make_unique<CGBufferStage>(m_resources));
    AddStage(std::make_unique<CIndirectLightingStage>(m_resources));
    AddStage(std::make_unique<CFrameOutputStage>(m_resources));
}

void CDeferredLightingPipeline::Destroy()
{
    for (auto& s : m_stages)
        s->OnDetach();
    m_stages.clear();

    m_resources.Destroy();
}

void CDeferredLightingPipeline::OnDeviceReset()
{
    m_resources.OnDeviceReset();

    for (auto& s : m_stages)
        s->OnDeviceReset();
}
////////////////////////////////////////////////////////////////////////////////
