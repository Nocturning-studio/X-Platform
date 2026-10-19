////////////////////////////////////////////////////////////////////////////////
// Created: 30.09.2026 9:27:19
// Author: NS_Deathman
// File: BackendDX9_States.cpp
// Nocturning studio for X-RHI
////////////////////////////////////////////////////////////////////////////////
#include <source/pch.h>
#include "BackendDX9.h"
////////////////////////////////////////////////////////////////////////////////
void CRenderBackendDX9::InvalidateCache()
{
	InvalidateStateCache();
	InvalidateGeometryCache();
	InvalidateRenderTargetCache();
}

void CRenderBackendDX9::ApplyBlendState(const RHI_BlendState& s)
{
	m_stateCache.ApplyBlend(m_pDevice, s);
}

void CRenderBackendDX9::ApplyDepthStencilState(const RHI_DepthStencilState& s)
{
	m_stateCache.ApplyDepthStencil(m_pDevice, s);
}

void CRenderBackendDX9::ApplyRasterizerState(const RHI_RasterizerState& s)
{
	m_stateCache.ApplyRasterizer(m_pDevice, s);
}

void CRenderBackendDX9::SetStencilRef(uint8_t ref)
{
	m_stateCache.ApplyStencilRef(m_pDevice, ref);
}

void CRenderBackendDX9::SetBlendFactor(const Float4& factor)
{
	m_stateCache.ApplyBlendFactor(m_pDevice, factor);
}

void CRenderBackendDX9::SetDepthBounds(float, float)
{
	static bool warned = false;
	if (!warned)
	{
		Print("! [DX9] SetDepthBounds: not supported, ignored.");
		warned = true;
	}
}

void CRenderBackendDX9::SetViewport(const RHI_Viewport& vp)
{
	m_stateCache.ApplyViewport(m_pDevice, vp);
}

RHI_Viewport CRenderBackendDX9::GetViewport() const
{
	return m_stateCache.m_viewport.IsValid() ? m_stateCache.m_viewport.Get() : RHI_Viewport{};
}

void CRenderBackendDX9::SetScissorRect(const RHI_Rect* rect)
{
	m_stateCache.ApplyScissorRect(m_pDevice, rect);
}

bool CRenderBackendDX9::GetScissorRect(RHI_Rect& out) const
{
	if (!m_stateCache.IsScissorEnabled())
		return false;
	if (!m_stateCache.m_scissor.IsValid())
		return false;
	out = m_stateCache.m_scissor.Get();
	return true;
}

void CRenderBackendDX9::CacheBackBufferDimensions()
{
	m_backBufferWidth = 0;
	m_backBufferHeight = 0;

	if (!m_pDevice)
		return;

	IDirect3DSurface9* bb = nullptr;
	if (FAILED(m_pDevice->GetRenderTarget(0, &bb)) || !bb)
	{
		Print("! [DX9] CacheBackBufferDimensions: GetRenderTarget(0) failed");
		return;
	}

	D3DSURFACE_DESC desc{};
	if (SUCCEEDED(bb->GetDesc(&desc)))
	{
		m_backBufferWidth = desc.Width;
		m_backBufferHeight = desc.Height;
	}
	bb->Release();

	// После CreateDevice/Reset D3D9 автоматически ставит viewport по
	// размеру back buffer'а. Синхронизируем кэш с этим состоянием -
	// но ТОЛЬКО после InvalidateCache(), чтобы не затереть сброс.
	InvalidateCache();
	RefreshBackBufferRTVs();

	RHI_Viewport defaultVp{};
	defaultVp.Width = m_backBufferWidth;
	defaultVp.Height = m_backBufferHeight;
	m_stateCache.m_viewport.MarkUpdated(defaultVp);

	m_stateCache.SetRenderTargetSize(m_backBufferWidth, m_backBufferHeight);
}
////////////////////////////////////////////////////////////////////////////////
