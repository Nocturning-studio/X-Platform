////////////////////////////////////////////////////////////////////////////////
// Created: 24.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <d3d9.h>
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CStateCache
{
  public:
	CStateCache();
	~CStateCache();

	void Invalidate();

	// =====================================================================
	// RHI-native
	// =====================================================================
	void SetBlendState(IRenderBackend& rhi, const RHI_BlendState& state) { rhi.SetBlendState(state); }
	void SetDepthStencilState(IRenderBackend& rhi, const RHI_DepthStencilState& state) { rhi.SetDepthStencilState(state); }
	void SetRasterizerState(IRenderBackend& rhi, const RHI_RasterizerState& state) { rhi.SetRasterizerState(state); }

	void SetViewport(IRenderBackend& rhi, const RHI_Viewport& vp) { rhi.SetViewport(vp); }
	void SetScissor(IRenderBackend& rhi, const RHI_Rect* rect) { rhi.SetScissorRect(rect); }

	// =====================================================================
	// Convenience
	// =====================================================================

	void SetBlend(IRenderBackend& rhi, bool enable, RHI_Blend src, RHI_Blend dst);
	void SetBlendEx(IRenderBackend& rhi, bool enable, RHI_Blend src, RHI_Blend dst, RHI_BlendOp op);

	void SetStencil(IRenderBackend& rhi,
					bool enable,
					RHI_CmpFunc func,
					uint8_t ref, uint8_t mask, uint8_t writemask,
					RHI_StencilOp fail, RHI_StencilOp pass, RHI_StencilOp zfail);

	void SetColorWriteEnable(IRenderBackend& rhi, uint8_t mask);
	void SetDepthWriteEnable(IRenderBackend& rhi, bool enable);
	void SetCullMode(IRenderBackend& rhi, RHI_CullMode mode);

	// =====================================================================
	// Getters
	// =====================================================================

	bool GetBlendEnable(const IRenderBackend& rhi) const { return rhi.GetBlendState().enable; }
	RHI_Blend GetSrcBlend(const IRenderBackend& rhi) const { return rhi.GetBlendState().srcColor; }
	RHI_Blend GetDstBlend(const IRenderBackend& rhi) const { return rhi.GetBlendState().dstColor; }
	RHI_CullMode GetCullMode(const IRenderBackend& rhi) const { return rhi.GetRasterizerState().cullMode; }

	// =====================================================================
	// RT / DSV — legacy
	// =====================================================================
	bool SetRenderTargetLegacy(IDirect3DDevice9Ex* device, IDirect3DSurface9* RT, u32 idx);
	bool SetDepthStencilLegacy(IDirect3DDevice9Ex* device, IDirect3DSurface9* ZB);

	void SaveRenderState(IDirect3DDevice9Ex* device);
	void RestoreRenderState(IDirect3DDevice9Ex* device);

  private:
	IDirect3DSurface9* m_pRT[4] = {};
	IDirect3DSurface9* m_pZB = nullptr;

	struct SavedState
	{
		IDirect3DSurface9* rt[4] = {};
		IDirect3DSurface9* zb = nullptr;
		RHI_Viewport viewport{};
	} m_savedState;

	void D3D_SetRenderTarget(IDirect3DDevice9Ex* device, u32 idx, IDirect3DSurface9* surf);
	void D3D_SetDepthStencil(IDirect3DDevice9Ex* device, IDirect3DSurface9* zb);
};
////////////////////////////////////////////////////////////////////////////////
