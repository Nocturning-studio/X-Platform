////////////////////////////////////////////////////////////////////////////////
// Created: 12.10.2026 21:07:00
// Author: NS_Deathman
// File: BackendDX9_PipelineLayout.cpp
// Nocturning studio for X-RHI
////////////////////////////////////////////////////////////////////////////////
#include <source/pch.h>
#include "BackendDX9.h"
////////////////////////////////////////////////////////////////////////////////
RHI_PipelineLayoutHandle CRenderBackendDX9::AllocPipelineLayoutHandle(DX9PipelineLayout* lay)
{
	uint32_t idx;
	if(!m_freePipelineLayoutIndices.empty())
	{
		idx = m_freePipelineLayoutIndices.top();
		m_freePipelineLayoutIndices.pop();
		m_pipelineLayouts[idx] = lay;
	}
	else
	{
		idx = static_cast<uint32_t>(m_pipelineLayouts.size());
		m_pipelineLayouts.push_back(lay);
	}
	return RHI_PipelineLayoutHandle{idx};
}

DX9PipelineLayout* CRenderBackendDX9::GetPipelineLayout(RHI_PipelineLayoutHandle h) const
{
	if(!h.IsValid() || h.id >= m_pipelineLayouts.size())
		return nullptr;
	return m_pipelineLayouts[h.id];
}

void CRenderBackendDX9::FreePipelineLayoutHandle(RHI_PipelineLayoutHandle h)
{
	if(!h.IsValid() || h.id >= m_pipelineLayouts.size())
		return;

	DX9PipelineLayout* lay = m_pipelineLayouts[h.id];
	if(!lay)
	{
		Print("! [DX9] Double free of RHI_PipelineLayoutHandle(id=%u), ignoring.", h.id);
		return;
	}

	delete lay;
	m_pipelineLayouts[h.id] = nullptr;
	m_freePipelineLayoutIndices.push(h.id);
}

RHI_PipelineLayoutHandle CRenderBackendDX9::CreatePipelineLayout(const RHI_PipelineLayoutDesc& desc)
{
	if(!m_pDevice)
		return {};

	// В D3D9 layout ничего не создаёт — регистры неявные (c#/s#/t#),
	// а биндинги идут напрямую через SetSampler/SetShaderResource/
	// SetShaderConstants. Храним desc для симметрии с DX12.
	DX9PipelineLayout* lay = new DX9PipelineLayout;
	lay->desc = desc;
	return AllocPipelineLayoutHandle(lay);
}

void CRenderBackendDX9::DestroyPipelineLayout(RHI_PipelineLayoutHandle handle)
{
	FreePipelineLayoutHandle(handle);
}
////////////////////////////////////////////////////////////////////////////////
