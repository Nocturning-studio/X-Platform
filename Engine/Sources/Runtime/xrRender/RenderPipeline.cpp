////////////////////////////////////////////////////////////////////////////////
// Created: 18.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "RenderPipeline.h"
////////////////////////////////////////////////////////////////////////////////
CRenderPipeline::CRenderPipeline()
{
}
CRenderPipeline::~CRenderPipeline()
{
}

void CRenderPipeline::Initialize()
{
}

void CRenderPipeline::Destroy()
{
}

void CRenderPipeline::Execute()
{
	for(auto& stage : m_stages)
	{
		if(!stage->IsEnabled())
			continue;
		stage->Execute();
	}
}

void CRenderPipeline::AddStage(std::unique_ptr<IRenderStage> stage)
{
	stage->OnAttach();
	m_stages.push_back(std::move(stage));
}

void CRenderPipeline::InsertStage(size_t index, std::unique_ptr<IRenderStage> stage)
{
	stage->OnAttach();
	m_stages.insert(m_stages.begin() + index, std::move(stage));
}

bool CRenderPipeline::RemoveStage(std::string_view name)
{
	auto it = std::find_if(m_stages.begin(), m_stages.end(), [name](const auto& s)
						   { return s->GetName() == name; });
	if(it == m_stages.end())
		return false;
	(*it)->OnDetach();
	m_stages.erase(it);
	return true;
}

IRenderStage* CRenderPipeline::FindStage(std::string_view name)
{
	auto it = std::find_if(m_stages.begin(), m_stages.end(), [name](const auto& s)
						   { return s->GetName() == name; });
	return it != m_stages.end() ? it->get() : nullptr;
}
////////////////////////////////////////////////////////////////////////////////
