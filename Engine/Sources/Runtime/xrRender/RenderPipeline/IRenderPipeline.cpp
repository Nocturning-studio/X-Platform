////////////////////////////////////////////////////////////////////////////////
// Created: 18.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "IRenderPipeline.h"
////////////////////////////////////////////////////////////////////////////////
void IRenderPipeline::Execute()
{
    for (auto& stage : m_stages)
    {
        if (!stage->IsEnabled())
            continue;
        stage->Execute();
    }
}

void IRenderPipeline::AddStage(std::unique_ptr<IRenderStage> stage)
{
    stage->OnAttach();
    m_stages.push_back(std::move(stage));
}

void IRenderPipeline::InsertStage(size_t index, std::unique_ptr<IRenderStage> stage)
{
    stage->OnAttach();
    m_stages.insert(m_stages.begin() + index, std::move(stage));
}

bool IRenderPipeline::RemoveStage(std::string_view name)
{
    auto it = std::find_if(m_stages.begin(), m_stages.end(), [name](const auto& s) { return s->GetName() == name; });
    if (it == m_stages.end())
        return false;
    (*it)->OnDetach();
    m_stages.erase(it);
    return true;
}

IRenderStage* IRenderPipeline::FindStage(std::string_view name) const
{
    auto it = std::find_if(m_stages.begin(), m_stages.end(), [name](const auto& s) { return s->GetName() == name; });
    return it != m_stages.end() ? it->get() : nullptr;
}
////////////////////////////////////////////////////////////////////////////////
