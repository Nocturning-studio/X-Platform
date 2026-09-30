////////////////////////////////////////////////////////////////////////////////
// Created: 30.09.2026 10:49:35
// Author: NS_Deathman
// File: xrBackendDX9_RT.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "xrBackendDX9.h"
////////////////////////////////////////////////////////////////////////////////

// ============================================================================
// Slot pool
// ============================================================================

uint32_t CRenderBackendDX9::AllocRTVSlot(IDirect3DSurface9* surf)
{
	uint32_t idx;
	if (!m_freeRTVSlots.empty())
	{
		idx = m_freeRTVSlots.top();
		m_freeRTVSlots.pop();
	}
	else
	{
		idx = static_cast<uint32_t>(m_rtvSlots.size());
		m_rtvSlots.emplace_back();
	}
	m_rtvSlots[idx].surface = surf;
	return idx;
}

uint32_t CRenderBackendDX9::AllocDSVSlot(IDirect3DSurface9* surf)
{
	uint32_t idx;
	if (!m_freeDSVSlots.empty())
	{
		idx = m_freeDSVSlots.top();
		m_freeDSVSlots.pop();
	}
	else
	{
		idx = static_cast<uint32_t>(m_dsvSlots.size());
		m_dsvSlots.emplace_back();
	}
	m_dsvSlots[idx].surface = surf;
	return idx;
}

void CRenderBackendDX9::FreeRTVSlot(uint32_t id)
{
	if (id >= m_rtvSlots.size()) return;
	if (m_rtvSlots[id].surface)
	{
		m_rtvSlots[id].surface->Release();
		m_rtvSlots[id].surface = nullptr;
	}
	m_freeRTVSlots.push(id);
}

void CRenderBackendDX9::FreeDSVSlot(uint32_t id)
{
	if (id >= m_dsvSlots.size()) return;
	if (m_dsvSlots[id].surface)
	{
		m_dsvSlots[id].surface->Release();
		m_dsvSlots[id].surface = nullptr;
	}
	m_freeDSVSlots.push(id);
}

// ============================================================================
// Resolution
// ============================================================================

IDirect3DSurface9* CRenderBackendDX9::ResolveRTASurface(RHI_RenderTargetView rtv) const
{
	if (!rtv.IsValid() || rtv.id >= m_rtvSlots.size()) return nullptr;
	return m_rtvSlots[rtv.id].surface;
}

IDirect3DSurface9* CRenderBackendDX9::ResolveDSSurface(RHI_DepthStencilView dsv) const
{
	if (!dsv.IsValid() || dsv.id >= m_dsvSlots.size()) return nullptr;
	return m_dsvSlots[dsv.id].surface;
}

IDirect3DSurface9* CRenderBackendDX9::GetTextureSurfaceForRT(DX9Texture* t, uint32_t mip, uint32_t face) const
{
	if (!t) return nullptr;

	if (t->tex2D)
	{
		IDirect3DSurface9* s = nullptr;
		if (SUCCEEDED(t->tex2D->GetSurfaceLevel(mip, &s)))
			return s;
	}
	if (t->texCube)
	{
		IDirect3DSurface9* s = nullptr;
		if (SUCCEEDED(t->texCube->GetCubeMapSurface((D3DCUBEMAP_FACES)face, mip, &s)))
			return s;
	}
	return nullptr;
}

// ============================================================================
// Cache invalidation
// ============================================================================

void CRenderBackendDX9::InvalidateRenderTargetCache()
{
	for (auto*& s : m_currentRTASurfaces) s = nullptr;
	m_currentDSSurface = nullptr;
}

// ============================================================================
// Back buffer RTV/DSV
// ============================================================================

void CRenderBackendDX9::RefreshBackBufferRTVs()
{
	if (!m_pDevice) return;

	// Back buffer RTV/DSV всегда живут в слотах 0. Резервируем их, если ещё
	// не зарезервированы. Гарантирует стабильные хэндлы между Reset.
	if (!m_backBufferRTV.IsValid())
	{
		if (m_rtvSlots.empty()) m_rtvSlots.emplace_back();
		m_backBufferRTV = RHI_RenderTargetView{ 0 };
	}
	if (!m_backBufferDSV.IsValid())
	{
		if (m_dsvSlots.empty()) m_dsvSlots.emplace_back();
		m_backBufferDSV = RHI_DepthStencilView{ 0 };
	}

	// Освобождаем старые refs (могут быть от прошлого устройства).
	auto& rtvSlot = m_rtvSlots[m_backBufferRTV.id];
	auto& dsvSlot = m_dsvSlots[m_backBufferDSV.id];
	if (rtvSlot.surface) { rtvSlot.surface->Release(); rtvSlot.surface = nullptr; }
	if (dsvSlot.surface) { dsvSlot.surface->Release(); dsvSlot.surface = nullptr; }

	// Забираем свежие surface'ы у устройства. Каждый Get* отдаёт +1 ref.
	IDirect3DSurface9* bb = nullptr;
	if (SUCCEEDED(m_pDevice->GetRenderTarget(0, &bb)))
		rtvSlot.surface = bb;

	IDirect3DSurface9* ds = nullptr;
	if (SUCCEEDED(m_pDevice->GetDepthStencilSurface(&ds)))
		dsvSlot.surface = ds;

	// D3D9 после CreateDeviceEx/Reset уже привязывает back buffer и auto-DS.
	// Синхронизируем кэш, чтобы первый SetRenderTargets не делал лишних вызовов.
	InvalidateRenderTargetCache();
	m_currentRTASurfaces[0] = rtvSlot.surface;
	m_currentDSSurface = dsvSlot.surface;
}

// ============================================================================
// Lifecycle: destroy / reset
// ============================================================================

void CRenderBackendDX9::ReleaseAllRTVDSV()
{
	for (auto& s : m_rtvSlots) if (s.surface) { s.surface->Release(); s.surface = nullptr; }
	for (auto& s : m_dsvSlots) if (s.surface) { s.surface->Release(); s.surface = nullptr; }

	m_rtvSlots.clear();
	m_dsvSlots.clear();

	while (!m_freeRTVSlots.empty()) m_freeRTVSlots.pop();
	while (!m_freeDSVSlots.empty()) m_freeDSVSlots.pop();

	m_backBufferRTV = {};
	m_backBufferDSV = {};

	InvalidateRenderTargetCache();
}

void CRenderBackendDX9::InvalidateUserRTVDSVOnReset()
{
	// Освобождаем все пользовательские RTV/DSV. Back buffer слоты (id == 0)
	// не трогаем — их поверхность обновится в RefreshBackBufferRTVs.
	for (uint32_t i = 0; i < m_rtvSlots.size(); ++i)
	{
		if (m_backBufferRTV.IsValid() && i == m_backBufferRTV.id) continue;
		if (m_rtvSlots[i].surface)
		{
			m_rtvSlots[i].surface->Release();
			m_rtvSlots[i].surface = nullptr;
			m_freeRTVSlots.push(i);
		}
	}
	for (uint32_t i = 0; i < m_dsvSlots.size(); ++i)
	{
		if (m_backBufferDSV.IsValid() && i == m_backBufferDSV.id) continue;
		if (m_dsvSlots[i].surface)
		{
			m_dsvSlots[i].surface->Release();
			m_dsvSlots[i].surface = nullptr;
			m_freeDSVSlots.push(i);
		}
	}
}

// ============================================================================
// Create / Destroy
// ============================================================================

RHI_RenderTargetView CRenderBackendDX9::CreateRTV(RHI_TextureHandle tex, uint32_t mip, uint32_t face)
{
	DX9Texture* t = GetTexture(tex);
	if (!t) return RHI_RenderTargetView{};

	if (!t->isRenderTarget)
	{
		Msg("! [DX9] CreateRTV: texture (id=%u) was not created as a render target", tex.id);
		return RHI_RenderTargetView{};
	}

	IDirect3DSurface9* surf = GetTextureSurfaceForRT(t, mip, face);
	if (!surf) return RHI_RenderTargetView{};

	return RHI_RenderTargetView{ AllocRTVSlot(surf) };
}

RHI_DepthStencilView CRenderBackendDX9::CreateDSV(RHI_TextureHandle tex, uint32_t mip, uint32_t face)
{
	DX9Texture* t = GetTexture(tex);
	if (!t) return RHI_DepthStencilView{};

	if (!t->isDepthStencil)
	{
		Msg("! [DX9] CreateDSV: texture (id=%u) was not created as a depth/stencil", tex.id);
		return RHI_DepthStencilView{};
	}

	IDirect3DSurface9* surf = GetTextureSurfaceForRT(t, mip, face);
	if (!surf) return RHI_DepthStencilView{};

	return RHI_DepthStencilView{ AllocDSVSlot(surf) };
}

void CRenderBackendDX9::DestroyRTV(RHI_RenderTargetView rtv)
{
	if (!rtv.IsValid()) return;
	if (rtv == m_backBufferRTV)
	{
		Msg("! [DX9] DestroyRTV: refusing to destroy back buffer RTV");
		return;
	}
	FreeRTVSlot(rtv.id);
}

void CRenderBackendDX9::DestroyDSV(RHI_DepthStencilView dsv)
{
	if (!dsv.IsValid()) return;
	if (dsv == m_backBufferDSV)
	{
		Msg("! [DX9] DestroyDSV: refusing to destroy back buffer DSV");
		return;
	}
	FreeDSVSlot(dsv.id);
}

RHI_RenderTargetView CRenderBackendDX9::GetBackBufferRTV() const
{
	return m_backBufferRTV;
}

RHI_DepthStencilView CRenderBackendDX9::GetBackBufferDSV() const
{
	return m_backBufferDSV;
}

// ============================================================================
// SetRenderTargets
// ============================================================================

void CRenderBackendDX9::SetRenderTargets(const RHI_RenderTargetView* rtvs,
	uint32_t count,
	RHI_DepthStencilView dsv)
{
	if (!m_pDevice) return;

	constexpr uint32_t kMaxRT = 4; // D3D9: 4 simultaneous RTs
	if (count > kMaxRT) count = kMaxRT;

	// --- Unbind tail (RTs beyond count) ---
	for (uint32_t i = count; i < kMaxRT; ++i)
	{
		if (m_currentRTASurfaces[i])
		{
			m_pDevice->SetRenderTarget(i, nullptr);
			m_currentRTASurfaces[i] = nullptr;
		}
	}

	// --- Bind color RTs ---
	for (uint32_t i = 0; i < count; ++i)
	{
		IDirect3DSurface9* surf = ResolveRTASurface(rtvs[i]);
		if (m_currentRTASurfaces[i] == surf) continue;

		const HRESULT hr = m_pDevice->SetRenderTarget(i, surf);
		if (FAILED(hr))
		{
			Msg("! [DX9] SetRenderTargets: SetRenderTarget(%u) failed (0x%08x)", i, hr);
			continue;
		}
		m_currentRTASurfaces[i] = surf;
	}

	// --- Bind depth/stencil ---
	IDirect3DSurface9* ds = ResolveDSSurface(dsv);
	if (m_currentDSSurface != ds)
	{
		const HRESULT hr = m_pDevice->SetDepthStencilSurface(ds);
		if (FAILED(hr))
		{
			Msg("! [DX9] SetRenderTargets: SetDepthStencilSurface failed (0x%08x)", hr);
		}
		else
		{
			m_currentDSSurface = ds;
		}
	}
}

// ============================================================================
// Clear
// ============================================================================

void CRenderBackendDX9::ClearRenderTarget(RHI_RenderTargetView rtv, const fvec4& color)
{
	if (!m_pDevice) return;

	IDirect3DSurface9* surf = ResolveRTASurface(rtv);
	if (!surf) return;

	const D3DCOLOR d3dColor = D3DCOLOR_COLORVALUE(color.x, color.y, color.z, color.w);

	// Если этот RTV уже привязан в slot 0 — просто Clear().
	if (m_currentRTASurfaces[0] == surf)
	{
		m_pDevice->Clear(0, nullptr, D3DCLEAR_TARGET, d3dColor, 1.0f, 0);
		return;
	}

	// Иначе: временно биндим в slot 0, чистим, восстанавливаем.
	// Кэш RTV-кэш не трогаем — состояние устройства после restore
	// то же, что было до вызова.
	IDirect3DSurface9* saved = m_currentRTASurfaces[0];
	m_pDevice->SetRenderTarget(0, surf);
	m_pDevice->Clear(0, nullptr, D3DCLEAR_TARGET, d3dColor, 1.0f, 0);
	m_pDevice->SetRenderTarget(0, saved);
}

void CRenderBackendDX9::ClearDepthStencil(RHI_DepthStencilView dsv, float depth, uint8_t stencil)
{
	if (!m_pDevice) return;

	IDirect3DSurface9* surf = ResolveDSSurface(dsv);
	if (!surf) return;

	DWORD flags = D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL;

	// Если этот DSV уже привязан — Clear() без временного биндинга.
	if (m_currentDSSurface == surf)
	{
		m_pDevice->Clear(0, nullptr, flags, 0, depth, stencil);
		return;
	}

	IDirect3DSurface9* saved = m_currentDSSurface;
	m_pDevice->SetDepthStencilSurface(surf);
	m_pDevice->Clear(0, nullptr, flags, 0, depth, stencil);
	m_pDevice->SetDepthStencilSurface(saved);
}
////////////////////////////////////////////////////////////////////////////////
