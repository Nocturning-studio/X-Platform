////////////////////////////////////////////////////////////////////////////////
// Created: 24.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "StateCache.h"
////////////////////////////////////////////////////////////////////////////////
void CStateCache::SetBlend(IRenderBackend& rhi, bool enable, RHI_Blend src, RHI_Blend dst)
{
	SetBlendEx(rhi, enable, src, dst, RHI_BlendOp::Add);
}

void CStateCache::SetBlendEx(IRenderBackend& rhi, bool enable, RHI_Blend src, RHI_Blend dst, RHI_BlendOp op)
{
	RHI_BlendState s = rhi.GetBlendState();
	s.enable = enable;
	s.srcColor = src;
	s.dstColor = dst;
	s.opColor = op;
	rhi.SetBlendState(s);
}

void CStateCache::SetStencil(IRenderBackend& rhi,
							 bool enable,
							 RHI_CmpFunc func,
							 uint8_t ref, uint8_t mask, uint8_t writemask,
							 RHI_StencilOp fail, RHI_StencilOp pass, RHI_StencilOp zfail)
{
	RHI_DepthStencilState s = rhi.GetDepthStencilState();
	s.stencilEnable = enable;
	s.stencilFunc = func;
	s.stencilRef = ref;
	s.stencilReadMask = mask;
	s.stencilWriteMask = writemask;
	s.stencilFailOp = fail;
	s.stencilPassOp = pass;
	s.stencilDepthFailOp = zfail;
	rhi.SetDepthStencilState(s);
}

void CStateCache::SetColorWriteEnable(IRenderBackend& rhi, uint8_t mask)
{
	RHI_BlendState s = rhi.GetBlendState();
	s.writeMask = mask;
	rhi.SetBlendState(s);
}

void CStateCache::SetDepthWriteEnable(IRenderBackend& rhi, bool enable)
{
	RHI_DepthStencilState s = rhi.GetDepthStencilState();
	s.depthWriteEnable = enable;
	rhi.SetDepthStencilState(s);
}

void CStateCache::SetCullMode(IRenderBackend& rhi, RHI_CullMode mode)
{
	RHI_RasterizerState s = rhi.GetRasterizerState();
	s.cullMode = mode;
	rhi.SetRasterizerState(s);
}
////////////////////////////////////////////////////////////////////////////////
