////////////////////////////////////////////////////////////////////////////////
// Created: 08.10.2026 23:15:09
// Author: NS_Deathman
// File: BackendDX9_PipelineState.cpp
// Nocturning studio for X-RHI
////////////////////////////////////////////////////////////////////////////////
#include <source/pch.h>
#include "BackendDX9.h"
////////////////////////////////////////////////////////////////////////////////
RHI_PipelineStateHandle CRenderBackendDX9::AllocPipelineStateHandle(DX9PipelineState* pso)
{
	uint32_t idx;
	if (!m_freePSOIndices.empty())
	{
		idx = m_freePSOIndices.top();
		m_freePSOIndices.pop();
		m_pipelineStates[idx] = pso;
	}
	else
	{
		idx = static_cast<uint32_t>(m_pipelineStates.size());
		m_pipelineStates.push_back(pso);
	}
	return RHI_PipelineStateHandle{ idx };
}

DX9PipelineState* CRenderBackendDX9::GetPipelineState(RHI_PipelineStateHandle h) const
{
	if (!h.IsValid() || h.id >= m_pipelineStates.size())
		return nullptr;
	return m_pipelineStates[h.id];
}

void CRenderBackendDX9::FreePipelineStateHandle(RHI_PipelineStateHandle h)
{
	if (!h.IsValid() || h.id >= m_pipelineStates.size())
		return;

	DX9PipelineState* pso = m_pipelineStates[h.id];
	if (!pso)
	{
		Print("! [DX9] Double free of RHI_PipelineStateHandle(id=%u), ignoring.", h.id);
		return;
	}

	if (m_currentPSO == h)
	{
		m_currentPSO = {};
		m_psoCacheValid = false;
	}

	delete pso;
	m_pipelineStates[h.id] = nullptr;
	m_freePSOIndices.push(h.id);
}

RHI_PipelineStateHandle CRenderBackendDX9::CreatePipelineState(const RHI_PipelineStateDesc& desc)
{
	if (!m_pDevice)
		return {};

	if (desc.rtvCount > RHI_PipelineStateDesc::kMaxRenderTargets)
	{
		Print("! [DX9] CreatePipelineState: rtvCount=%u exceeds max %u", desc.rtvCount, RHI_PipelineStateDesc::kMaxRenderTargets);
		return {};
	}

	if (desc.rtvCount > m_DeviceCaps.MaxSimultaneousRTs)
		Print("! [DX9] CreatePipelineState: rtvCount=%u exceeds device max %u", desc.rtvCount, m_DeviceCaps.MaxSimultaneousRTs);

	if (desc.sample.count > 1)
		Print("! [DX9] CreatePipelineState: MSAA in PSO is not supported (sampleCount=%u), ignoring", desc.sample.count);

	if (desc.depthBoundsTestEnable)
		Print("! [DX9] CreatePipelineState: depthBoundsTestEnable is not supported, ignoring");

	DX9PipelineState* pso = new DX9PipelineState;
	pso->desc = desc;
	return AllocPipelineStateHandle(pso);
}

void CRenderBackendDX9::DestroyPipelineState(RHI_PipelineStateHandle handle)
{
	FreePipelineStateHandle(handle);
}

void CRenderBackendDX9::SetPipelineState(RHI_PipelineStateHandle handle)
{
	if (!m_pDevice)
		return;

	DX9PipelineState* pso = GetPipelineState(handle);
	if (!pso)
	{
		Print("! [DX9] SetPipelineState: invalid handle (id=%u)", handle.id);
		return;
	}

	// Быстрый путь: тот же PSO уже применён и не инвалидирован вручную.
	if (m_psoCacheValid && m_currentPSO == handle)
		return;

	const RHI_PipelineStateDesc& d = pso->desc;

	// Порядок: сначала IA, потом шейдеры, потом FF.
	// (SetX-методы временно выставляют m_psoCacheValid = false —
	//  в конце мы восстановим его.)
	SetInputLayout(d.inputLayout);
	SetVertexShader(d.vertexShader);
	SetPixelShader(d.pixelShader);

	// В D3D9 топология-семейство (d.topologyType) игнорируется.
	// Реальная топология (list/strip/fan) — через SetPrimitiveTopology().

	SetBlendState(d.blend);
	SetDepthStencilState(d.depthStencil);
	SetRasterizerState(d.rasterizer);

	// Layout в D3D9 не применяется (регистры неявные).
	// Но сохраняем handle в PSO для возможной валидации.

	m_currentPSO = handle;
	m_psoCacheValid = true;
}

RHI_PipelineStateHandle CRenderBackendDX9::GetCurrentPipelineState() const
{
	return m_psoCacheValid ? m_currentPSO : RHI_PipelineStateHandle{};
}
////////////////////////////////////////////////////////////////////////////////
