////////////////////////////////////////////////////////////////////////////////
// Created: 24.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
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

	bool SetRenderTarget(IDirect3DDevice9Ex* device, IDirect3DSurface9* RT, u32 idx);
	bool SetDepthStencil(IDirect3DDevice9Ex* device, IDirect3DSurface9* ZB);

	void SetViewport(IDirect3DDevice9Ex* device, const D3DVIEWPORT9& vp);
	void SetScissor(IDirect3DDevice9Ex* device, const RECT* rect);

	bool SetBlend(IDirect3DDevice9Ex* device, BOOL enable, D3DBLEND src, D3DBLEND dst);
	bool SetBlendEx(IDirect3DDevice9Ex* device, BOOL enable, D3DBLEND src, D3DBLEND dst, D3DBLENDOP op);

	void SetStencil(IDirect3DDevice9Ex* device, u32 enable, u32 func, u32 ref, u32 mask, u32 writemask, u32 fail, u32 pass, u32 zfail);

	bool SetColorWriteEnable(IDirect3DDevice9Ex* device, u32 mask);

	bool SetDepthWriteEnable(IDirect3DDevice9Ex* device, bool enable);
	bool SetCullMode(IDirect3DDevice9Ex* device, u32 mode);

	void SetRawRenderState(IDirect3DDevice9Ex* device, D3DRENDERSTATETYPE state, DWORD value);

	void SaveRenderState(IDirect3DDevice9Ex* device);
	void RestoreRenderState(IDirect3DDevice9Ex* device);

	BOOL GetBlendEnable() const { return m_bBlend != 0; }
	D3DBLEND GetSrcBlend() const { return m_srcBlend; }
	D3DBLEND GetDstBlend() const { return m_dstBlend; }
	u32 GetCullMode() const { return m_cullMode; }

  private:
	IDirect3DSurface9* m_pRT[4] = {};
	IDirect3DSurface9* m_pZB = nullptr;

	u32 m_bBlend = u32(-1);
	D3DBLEND m_srcBlend = (D3DBLEND)u32(-1);
	D3DBLEND m_dstBlend = (D3DBLEND)u32(-1);
	D3DBLENDOP m_blendOp = (D3DBLENDOP)u32(-1);

	u32 m_stencilEnable = 0;
	u32 m_stencilFunc = 0;
	u32 m_stencilRef = 0;
	u32 m_stencilMask = 0;
	u32 m_stencilWriteMask = 0;
	u32 m_stencilFail = 0;
	u32 m_stencilPass = 0;
	u32 m_stencilZFail = 0;

	u32 m_colorWriteMask = u32(-1);
	u32 m_cullMode = u32(-1);
	u32 m_zWriteEnable = u32(-1);

	struct SavedState
	{
		IDirect3DSurface9* rt[4] = {};
		IDirect3DSurface9* zb = nullptr;
		D3DVIEWPORT9 viewport{};
	} m_savedState;

	void D3D_SetRenderState(IDirect3DDevice9Ex* device, D3DRENDERSTATETYPE state, DWORD value);
	void D3D_SetRenderTarget(IDirect3DDevice9Ex* device, u32 idx, IDirect3DSurface9* surf);
	void D3D_SetDepthStencil(IDirect3DDevice9Ex* device, IDirect3DSurface9* zb);
};
////////////////////////////////////////////////////////////////////////////////
