////////////////////////////////////////////////////////////////////////////////
// Created: 24.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "StateCache.h"
////////////////////////////////////////////////////////////////////////////////
CStateCache::CStateCache()
{
	Invalidate();
}

CStateCache::~CStateCache() = default;

void CStateCache::Invalidate()
{
	// Сбрасываем только legacy RT/DSV. State-кэш живёт в бэкенде и
	// инвалидируется отдельно (backend сам вызывает InvalidateStateCache()
	// на CreateDevice/Reset/DestroyDevice).
	for (auto*& s : m_pRT) s = nullptr;
	m_pZB = nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// Convenience
////////////////////////////////////////////////////////////////////////////////

void CStateCache::SetBlend(IRenderBackend & rhi, bool enable, RHI_Blend src, RHI_Blend dst)
{
	SetBlendEx(rhi, enable, src, dst, RHI_BlendOp::Add);
}

void CStateCache::SetBlendEx(IRenderBackend & rhi, bool enable, RHI_Blend src, RHI_Blend dst, RHI_BlendOp op)
{
	// Копируем — backend возвращает const& на свой кэш, а мы собираемся
	// его же и перезаписать через SetBlendState.
	RHI_BlendState s = rhi.GetBlendState();
	s.enable = enable;
	s.srcColor = src;
	s.dstColor = dst;
	s.opColor = op;
	rhi.SetBlendState(s);
}

void CStateCache::SetStencil(IRenderBackend & rhi,
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

void CStateCache::SetColorWriteEnable(IRenderBackend & rhi, u8 mask)
{
	RHI_BlendState s = rhi.GetBlendState();
	s.writeMask = mask;
	rhi.SetBlendState(s);
}

void CStateCache::SetDepthWriteEnable(IRenderBackend & rhi, bool enable)
{
	RHI_DepthStencilState s = rhi.GetDepthStencilState();
	s.depthWriteEnable = enable;
	rhi.SetDepthStencilState(s);
}

void CStateCache::SetCullMode(IRenderBackend & rhi, RHI_CullMode mode)
{
	RHI_RasterizerState s = rhi.GetRasterizerState();
	s.cullMode = mode;
	rhi.SetRasterizerState(s);
}

////////////////////////////////////////////////////////////////////////////////
// Legacy RT / DSV
////////////////////////////////////////////////////////////////////////////////

bool CStateCache::SetRenderTargetLegacy(IDirect3DDevice9Ex * device, IDirect3DSurface9 * RT, u32 idx)
{
	if (idx >= 4) return false;
	if (m_pRT[idx] == RT) return false;

	m_pRT[idx] = RT;
	D3D_SetRenderTarget(device, idx, RT);
	return true;
}

bool CStateCache::SetDepthStencilLegacy(IDirect3DDevice9Ex * device, IDirect3DSurface9 * ZB)
{
	if (m_pZB == ZB) return false;

	m_pZB = ZB;
	D3D_SetDepthStencil(device, ZB);
	return true;
}

void CStateCache::SaveRenderState(IDirect3DDevice9Ex * device)
{
	if (!device) return;

	for (int i = 0; i < 4; ++i)
		device->GetRenderTarget(i, &m_savedState.rt[i]);
	device->GetDepthStencilSurface(&m_savedState.zb);

	D3DVIEWPORT9 vp{};
	device->GetViewport(&vp);
	m_savedState.viewport.X = vp.X;
	m_savedState.viewport.Y = vp.Y;
	m_savedState.viewport.Width = vp.Width;
	m_savedState.viewport.Height = vp.Height;
	m_savedState.viewport.MinZ = vp.MinZ;
	m_savedState.viewport.MaxZ = vp.MaxZ;
}

void CStateCache::RestoreRenderState(IDirect3DDevice9Ex * device)
{
	if (!device) return;

	for (int i = 0; i < 4; ++i)
	{
		if (m_savedState.rt[i])
		{
			SetRenderTargetLegacy(device, m_savedState.rt[i], i);
			m_savedState.rt[i]->Release();
			m_savedState.rt[i] = nullptr;
		}
	}
	if (m_savedState.zb)
	{
		SetDepthStencilLegacy(device, m_savedState.zb);
		m_savedState.zb->Release();
		m_savedState.zb = nullptr;
	}

	D3DVIEWPORT9 vp{};
	vp.X = m_savedState.viewport.X;
	vp.Y = m_savedState.viewport.Y;
	vp.Width = m_savedState.viewport.Width;
	vp.Height = m_savedState.viewport.Height;
	vp.MinZ = m_savedState.viewport.MinZ;
	vp.MaxZ = m_savedState.viewport.MaxZ;
	device->SetViewport(&vp);
}

void CStateCache::D3D_SetRenderTarget(IDirect3DDevice9Ex * device, u32 idx, IDirect3DSurface9 * surf)
{
	if (!device) return;
	device->SetRenderTarget(idx, surf);
}

void CStateCache::D3D_SetDepthStencil(IDirect3DDevice9Ex * device, IDirect3DSurface9 * zb)
{
	if (!device) return;
	device->SetDepthStencilSurface(zb);
}
////////////////////////////////////////////////////////////////////////////////
