////////////////////////////////////////////////////////////////////////////////
// Created: 04.10.2026 23:01:46
// Author: NS_Deathman
// File: RenderPipelineRegistry.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "RenderPipelineRegistry.h"
////////////////////////////////////////////////////////////////////////////////
CRenderPipelineRegistry::~CRenderPipelineRegistry()
{
    DestroyAll();
}

void CRenderPipelineRegistry::RegisterFlow(SRenderPipelineFlow flow)
{
    const std::string key = flow.name;
    m_flows.emplace(key, std::move(flow));
}

void CRenderPipelineRegistry::RegisterFlow(std::string name, std::vector<std::string> pipelines)
{
    SRenderPipelineFlow flow;
    flow.name = std::move(name);
    flow.pipelines = std::move(pipelines);
    RegisterFlow(std::move(flow));
}

void CRenderPipelineRegistry::InitializeAll()
{
    for (auto& [name, pipeline] : m_pipelines)
        pipeline->Initialize();
}

void CRenderPipelineRegistry::DestroyAll()
{
    for (auto& [name, pipeline] : m_pipelines)
        pipeline->Destroy();
    m_pipelines.clear();
    m_flows.clear();
    m_active_flow_name.clear();
}

void CRenderPipelineRegistry::OnDeviceResetAll()
{
    for (auto& [name, pipeline] : m_pipelines)
        pipeline->OnDeviceReset();
}

bool CRenderPipelineRegistry::SetActiveFlow(std::string_view flow_name)
{
    if (m_active_flow_name == flow_name)
        return true;

    auto it = m_flows.find(std::string(flow_name));
    if (it == m_flows.end())
        return false;

    if (!m_active_flow_name.empty())
    {
        auto old_it = m_flows.find(m_active_flow_name);
        if (old_it != m_flows.end())
        {
            for (const std::string& pname : old_it->second.pipelines)
                if (IRenderPipeline* p = FindPipeline(pname))
                    p->OnDeactivate();
        }
    }

    m_active_flow_name = std::string(flow_name);

    for (const std::string& pname : it->second.pipelines)
        if (IRenderPipeline* p = FindPipeline(pname))
            p->OnActivate();

    return true;
}

void CRenderPipelineRegistry::ExecuteActiveFlow()
{
    if (m_active_flow_name.empty())
        return;

    auto it = m_flows.find(m_active_flow_name);
    if (it == m_flows.end())
        return;

    for (const std::string& pname : it->second.pipelines)
        if (IRenderPipeline* p = FindPipeline(pname))
            p->Execute();
}

IRenderPipeline* CRenderPipelineRegistry::FindPipeline(std::string_view name)
{
    auto it = m_pipelines.find(std::string(name));
    return it != m_pipelines.end() ? it->second.get() : nullptr;
}
////////////////////////////////////////////////////////////////////////////////
