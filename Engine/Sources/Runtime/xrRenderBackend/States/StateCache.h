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

	bool SetRenderTarget(IDirect3DDevice9Ex* device, IDirect3DSurface9* RT, uint32_t idx);
	bool SetDepthStencil(IDirect3DDevice9Ex* device, IDirect3DSurface9* ZB);

	void SetViewport(IDirect3DDevice9Ex* device, const D3DVIEWPORT9& vp);
	void SetScissor(IDirect3DDevice9Ex* device, const RECT* rect);

	bool SetBlend(IDirect3DDevice9Ex* device, BOOL enable, D3DBLEND src, D3DBLEND dst);
	bool SetBlendEx(IDirect3DDevice9Ex* device, BOOL enable, D3DBLEND src, D3DBLEND dst, D3DBLENDOP op);

	void SetStencil(IDirect3DDevice9Ex* device, uint32_t enable, uint32_t func, uint32_t ref, uint32_t mask, uint32_t writemask, uint32_t fail, uint32_t pass, uint32_t zfail);

	bool SetColorWriteEnable(IDirect3DDevice9Ex* device, uint32_t mask);

	bool SetDepthWriteEnable(IDirect3DDevice9Ex* device, bool enable);
	bool SetCullMode(IDirect3DDevice9Ex* device, uint32_t mode);

	void SetRawRenderState(IDirect3DDevice9Ex* device, D3DRENDERSTATETYPE state, DWORD value);

	void SaveRenderState(IDirect3DDevice9Ex* device);
	void RestoreRenderState(IDirect3DDevice9Ex* device);

	BOOL GetBlendEnable() const { return m_bBlend != 0; }
	D3DBLEND GetSrcBlend() const { return m_srcBlend; }
	D3DBLEND GetDstBlend() const { return m_dstBlend; }
	uint32_t GetCullMode() const { return m_cullMode; }

  private:
	IDirect3DSurface9* m_pRT[4] = {};
	IDirect3DSurface9* m_pZB = nullptr;

	uint32_t m_bBlend = uint32_t(-1);
	D3DBLEND m_srcBlend = (D3DBLEND)uint32_t(-1);
	D3DBLEND m_dstBlend = (D3DBLEND)uint32_t(-1);
	D3DBLENDOP m_blendOp = (D3DBLENDOP)uint32_t(-1);

	uint32_t m_stencilEnable = 0;
	uint32_t m_stencilFunc = 0;
	uint32_t m_stencilRef = 0;
	uint32_t m_stencilMask = 0;
	uint32_t m_stencilWriteMask = 0;
	uint32_t m_stencilFail = 0;
	uint32_t m_stencilPass = 0;
	uint32_t m_stencilZFail = 0;

	uint32_t m_colorWriteMask = uint32_t(-1);
	uint32_t m_cullMode = uint32_t(-1);
	uint32_t m_zWriteEnable = uint32_t(-1);

	struct SavedState
	{
		IDirect3DSurface9* rt[4] = {};
		IDirect3DSurface9* zb = nullptr;
		D3DVIEWPORT9 viewport{};
	} m_savedState;

	void D3D_SetRenderState(IDirect3DDevice9Ex* device, D3DRENDERSTATETYPE state, DWORD value);
	void D3D_SetRenderTarget(IDirect3DDevice9Ex* device, uint32_t idx, IDirect3DSurface9* surf);
	void D3D_SetDepthStencil(IDirect3DDevice9Ex* device, IDirect3DSurface9* zb);
};
////////////////////////////////////////////////////////////////////////////////
