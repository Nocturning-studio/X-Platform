////////////////////////////////////////////////////////////////////////////////
// Created: 24.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "R_Backend_StateCache.h"

// Временный мост до полного перехода на RHI-only API.
// SetBlendState/SetDepthStencilState/SetRasterizerState и Get'еры живут
// только в CRenderBackendDX9, поэтому тянем его заголовок и AsDX9.
#include <xrRHI/Backends/DirectX9/xrBackendDX9.h>
#include <xrRHI/xrRHI_BackendAccess.h>

////////////////////////////////////////////////////////////////////////////////
namespace
{
	// Все RHI-состояния хранит DX9-бэкенд (IRenderBackend не выставляет
	// SetBlendState/GetBlendState и т.п.). Хелпер кастует и возвращает nullptr
	// для чужого бэкенда, чтобы не уронить процесс при будущем DX12.
	inline CRenderBackendDX9* DX9StateCache(IRenderBackend& rhi)
	{
		return AsDX9(&rhi);
	}

	inline const CRenderBackendDX9* DX9StateCache(const IRenderBackend& rhi)
	{
		// AsDX9 принимает non-const, но нам нужен только const-доступ
		// к внутренним кэшам через константные Get'еры.
		return AsDX9(const_cast<IRenderBackend*>(&rhi));
	}
} // namespace
////////////////////////////////////////////////////////////////////////////////

// ============================================================================
// RHI-native форвардеры
// ============================================================================

void CStateCache::SetBlendState(IRenderBackend& rhi, const RHI_BlendState& state)
{
	if (CRenderBackendDX9* dx9 = DX9StateCache(rhi))
		dx9->SetBlendState(state);
}

void CStateCache::SetDepthStencilState(IRenderBackend& rhi, const RHI_DepthStencilState& state)
{
	if (CRenderBackendDX9* dx9 = DX9StateCache(rhi))
		dx9->SetDepthStencilState(state);
}

void CStateCache::SetRasterizerState(IRenderBackend& rhi, const RHI_RasterizerState& state)
{
	if (CRenderBackendDX9* dx9 = DX9StateCache(rhi))
		dx9->SetRasterizerState(state);
}

// ============================================================================
// Convenience
// ============================================================================

void CStateCache::SetBlend(IRenderBackend& rhi, bool enable, RHI_Blend src, RHI_Blend dst)
{
	SetBlendEx(rhi, enable, src, dst, RHI_BlendOp::Add);
}

void CStateCache::SetBlendEx(IRenderBackend& rhi, bool enable, RHI_Blend src, RHI_Blend dst, RHI_BlendOp op)
{
	CRenderBackendDX9* dx9 = DX9StateCache(rhi);
	if (!dx9)
		return;

	RHI_BlendState s = dx9->GetBlendState();
	s.enable = enable;
	s.srcColor = src;
	s.dstColor = dst;
	s.opColor = op;
	dx9->SetBlendState(s);
}

void CStateCache::SetStencil(IRenderBackend& rhi,
	bool enable,
	RHI_CmpFunc func,
	uint8_t ref, uint8_t mask, uint8_t writemask,
	RHI_StencilOp fail, RHI_StencilOp pass, RHI_StencilOp zfail)
{
	CRenderBackendDX9* dx9 = DX9StateCache(rhi);
	if (!dx9)
		return;

	RHI_DepthStencilState s = dx9->GetDepthStencilState();
	s.stencilEnable = enable;
	s.stencilFunc = func;
	//s.stencilRef = ref;
	s.stencilReadMask = mask;
	s.stencilWriteMask = writemask;
	s.stencilFailOp = fail;
	s.stencilPassOp = pass;
	s.stencilDepthFailOp = zfail;
	dx9->SetDepthStencilState(s);
}

void CStateCache::SetColorWriteEnable(IRenderBackend& rhi, uint8_t mask)
{
	CRenderBackendDX9* dx9 = DX9StateCache(rhi);
	if (!dx9)
		return;

	RHI_BlendState s = dx9->GetBlendState();
	s.writeMask = mask;
	dx9->SetBlendState(s);
}

void CStateCache::SetDepthWriteEnable(IRenderBackend& rhi, bool enable)
{
	CRenderBackendDX9* dx9 = DX9StateCache(rhi);
	if (!dx9)
		return;

	RHI_DepthStencilState s = dx9->GetDepthStencilState();
	s.depthWriteEnable = enable;
	dx9->SetDepthStencilState(s);
}

void CStateCache::SetCullMode(IRenderBackend& rhi, RHI_CullMode mode)
{
	CRenderBackendDX9* dx9 = DX9StateCache(rhi);
	if (!dx9)
		return;

	RHI_RasterizerState s = dx9->GetRasterizerState();
	s.cullMode = mode;
	dx9->SetRasterizerState(s);
}

// ============================================================================
// Getters
// ============================================================================

bool CStateCache::GetBlendEnable(const IRenderBackend& rhi) const
{
	const CRenderBackendDX9* dx9 = DX9StateCache(rhi);
	return dx9 ? dx9->GetBlendState().enable : false;
}

RHI_Blend CStateCache::GetSrcBlend(const IRenderBackend& rhi) const
{
	const CRenderBackendDX9* dx9 = DX9StateCache(rhi);
	return dx9 ? dx9->GetBlendState().srcColor : RHI_Blend::One;
}

RHI_Blend CStateCache::GetDstBlend(const IRenderBackend& rhi) const
{
	const CRenderBackendDX9* dx9 = DX9StateCache(rhi);
	return dx9 ? dx9->GetBlendState().dstColor : RHI_Blend::Zero;
}

RHI_CullMode CStateCache::GetCullMode(const IRenderBackend& rhi) const
{
	const CRenderBackendDX9* dx9 = DX9StateCache(rhi);
	return dx9 ? dx9->GetRasterizerState().cullMode : RHI_CullMode::CounterClockwise;
}
////////////////////////////////////////////////////////////////////////////////
