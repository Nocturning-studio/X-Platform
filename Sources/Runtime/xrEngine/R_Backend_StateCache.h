#pragma once

// Forward declaration to avoid circular includes
class CRenderBackend;

#include <d3d9.h>

class ENGINE_API CBackendStateCache
{
public:
    void Invalidate(CRenderBackend& backend);

    // Render targets & depth buffer
    IC void SetRenderTarget(CRenderBackend& backend, IDirect3DSurface9* RT, u32 idx);
    IC void SetDepthStencil(CRenderBackend& backend, IDirect3DSurface9* ZB);

    // Viewport / Scissor
    IC void SetViewport(IDirect3DDevice9Ex* device, const D3DVIEWPORT9& vp);
    IC void SetScissor(IDirect3DDevice9Ex* device, const RECT* rect);

    // Blend state
    IC void SetBlend(IDirect3DDevice9Ex* device, BOOL enable, D3DBLEND src, D3DBLEND dst);
    IC void SetBlendEx(IDirect3DDevice9Ex* device, BOOL enable, D3DBLEND src, D3DBLEND dst, D3DBLENDOP op);

    // Stencil state
    IC void SetStencil(IDirect3DDevice9Ex* device,
                    u32 enable, u32 func, u32 ref, u32 mask, u32 writemask,
                    u32 fail, u32 pass, u32 zfail);

    // Color write mask
    IC void SetColorWriteEnable(IDirect3DDevice9Ex* device, u32 mask);

    // Depth write & culling
    IC void SetZWriteEnable(IDirect3DDevice9Ex* device, bool enable);
    IC void SetCullMode(IDirect3DDevice9Ex* device, u32 mode);

    IC void SetRawRenderState(IDirect3DDevice9Ex* device, D3DRENDERSTATETYPE State, DWORD Value);

    // State save/restore (used by mip generation etc.)
    void SaveRenderState(IDirect3DDevice9Ex* device);
    void RestoreRenderState(IDirect3DDevice9Ex* device, CRenderBackend& backend);

    // Simple getters (used by CRenderBackend)
    IC BOOL GetBlendEnable() const { return m_bBlend != 0; }
    IC D3DBLEND GetSrcBlend() const { return m_srcBlend; }
    IC D3DBLEND GetDstBlend() const { return m_dstBlend; }
    IC u32 GetCullMode() const { return m_cullMode; }

private:
    // Cached pipeline states
    IDirect3DSurface9* m_pRT[4] = {};
    IDirect3DSurface9* m_pZB = nullptr;

    u32 m_bBlend = u32(-1);
	D3DBLEND m_srcBlend = (D3DBLEND)u32(-1);
	D3DBLEND m_dstBlend = (D3DBLEND)u32(-1);

    u32 m_stencilEnable    = 0;
    u32 m_stencilFunc      = 0;
    u32 m_stencilRef       = 0;
    u32 m_stencilMask      = 0;
    u32 m_stencilWriteMask = 0;
    u32 m_stencilFail      = 0;
    u32 m_stencilPass      = 0;
    u32 m_stencilZFail     = 0;

    u32 m_colorWriteMask = u32(-1);
	u32 m_cullMode = u32(-1);
	u32 m_zWriteEnable = u32(-1);

    // For Save/Restore
    struct SavedState
    {
        IDirect3DSurface9* rt[4] = {};
        IDirect3DSurface9* zb    = nullptr;
        D3DVIEWPORT9       viewport;
    } m_savedState;

    // Low‑level D3D9 wrappers
    void D3D_SetRenderState(IDirect3DDevice9Ex* device, D3DRENDERSTATETYPE state, DWORD value);
    void D3D_SetRenderTarget(IDirect3DDevice9Ex* device, u32 idx, IDirect3DSurface9* surf);
    void D3D_SetDepthStencil(IDirect3DDevice9Ex* device, IDirect3DSurface9* zb);
};
