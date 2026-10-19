////////////////////////////////////////////////////////////////////////////////
// Created: 08.10.2026 23:15:09
// Author: NS_Deathman
// File: BackendDX9_PipelineState.cpp
// Nocturning studio for X-RHI
////////////////////////////////////////////////////////////////////////////////
#include <source/pch.h>
#include "BackendDX9.h"
////////////////////////////////////////////////////////////////////////////////
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
	return m_pipelineStates.Allocate(pso);
}

void CRenderBackendDX9::SetPipelineState(RHI_PipelineStateHandle handle)
{
	if (!m_pDevice) return;

	DX9PipelineState* pso = GetPipelineState(handle);
	if (!pso)
	{
		Print("! [DX9] SetPipelineState: invalid handle (id=%u)", handle.id);
		return;
	}

	if (!m_stateCache.NeedsPipelineState(handle))
		return;

	const RHI_PipelineStateDesc& d = pso->desc;

	ApplyInputLayout(d.inputLayout);
	ApplyVertexShader(d.vertexShader);
	ApplyPixelShader(d.pixelShader);

	ApplyBlendState(d.blend);
	ApplyDepthStencilState(d.depthStencil);
	ApplyRasterizerState(d.rasterizer);

	m_stateCache.MarkPipelineState(handle);
}

RHI_PipelineStateHandle CRenderBackendDX9::GetCurrentPipelineState() const
{
	return m_stateCache.GetPipelineState();
}

void CRenderBackendDX9::DestroyPipelineState(RHI_PipelineStateHandle handle)
{
	DX9PipelineState* pso = m_pipelineStates.Release(handle);
	if (!pso)
	{
		if (handle.IsValid() && handle.id < m_pipelineStates.Data().size())
			Print("! [DX9] DestroyPipelineState: double free of RHI_PipelineStateHandle(id=%u), ignoring", handle.id);
		return;
	}

	if (m_stateCache.GetPipelineState() == handle)
		m_stateCache.InvalidatePipelineState();

	delete pso;
}
////////////////////////////////////////////////////////////////////////////////
