#include "stdafx.h"
#include "R_Backend_StateCache.h"
#include "R_Backend.h"   // for CRenderBackend & stat

// ---------------------------------------------------------------------------
// Invalidate – reset all cached states to force re‑apply on next use
// ---------------------------------------------------------------------------
void CBackendStateCache::Invalidate(CRenderBackend& /*backend*/)
{
    for (u32 i = 0; i < 4; ++i)
        m_pRT[i] = nullptr;
    m_pZB = nullptr;

    m_bBlend   = FALSE;
    m_srcBlend = D3DBLEND_ONE;
    m_dstBlend = D3DBLEND_ZERO;

    m_stencilEnable    = 0;
    m_stencilFunc      = 0;
    m_stencilRef       = 0;
    m_stencilMask      = 0;
    m_stencilWriteMask = 0;
    m_stencilFail      = 0;
    m_stencilPass      = 0;
    m_stencilZFail     = 0;

    m_colorWriteMask = u32(-1);
    m_cullMode       = D3DCULL_CCW;
    m_zWriteEnable   = true;
}

// ---------------------------------------------------------------------------
// Render Targets & Depth Stencil
// ---------------------------------------------------------------------------
void CBackendStateCache::SetRenderTarget(CRenderBackend& backend, IDirect3DSurface9* RT, u32 idx)
{
    if (m_pRT[idx] != RT)
    {
        m_pRT[idx] = RT;
        backend.stat.target_rt++;
        D3D_SetRenderTarget(backend.GetDevice(), idx, RT);
    }
}

void CBackendStateCache::SetDepthStencil(CRenderBackend& backend, IDirect3DSurface9* ZB)
{
    if (m_pZB != ZB)
    {
        m_pZB = ZB;
        backend.stat.target_zb++;
        D3D_SetDepthStencil(backend.GetDevice(), ZB);
    }
}

// ---------------------------------------------------------------------------
// Viewport / Scissor (no caching – rarely changed)
// ---------------------------------------------------------------------------
void CBackendStateCache::SetViewport(IDirect3DDevice9Ex* device, const D3DVIEWPORT9& vp)
{
    HRESULT hr = device->SetViewport(&vp);
    VERIFY(SUCCEEDED(hr));
}

void CBackendStateCache::SetScissor(IDirect3DDevice9Ex* device, const RECT* rect)
{
    if (rect)
    {
        D3D_SetRenderState(device, D3DRS_SCISSORTESTENABLE, TRUE);
        HRESULT hr = device->SetScissorRect(rect);
        VERIFY(SUCCEEDED(hr));
    }
    else
    {
        D3D_SetRenderState(device, D3DRS_SCISSORTESTENABLE, FALSE);
    }
}

// ---------------------------------------------------------------------------
// Blend
// ---------------------------------------------------------------------------
void CBackendStateCache::SetBlend(IDirect3DDevice9Ex* device, BOOL enable, D3DBLEND src, D3DBLEND dst)
{
    if (m_bBlend != enable || m_srcBlend != src || m_dstBlend != dst)
    {
        m_bBlend   = enable;
        m_srcBlend = src;
        m_dstBlend = dst;

        D3D_SetRenderState(device, D3DRS_ALPHABLENDENABLE, enable);
        if (enable)
        {
            D3D_SetRenderState(device, D3DRS_SRCBLEND,  src);
            D3D_SetRenderState(device, D3DRS_DESTBLEND, dst);
            D3D_SetRenderState(device, D3DRS_ALPHATESTENABLE, FALSE);
        }
    }
}

void CBackendStateCache::SetBlendEx(IDirect3DDevice9Ex* device, BOOL enable, D3DBLEND src, D3DBLEND dst, D3DBLENDOP op)
{
    if (m_bBlend != enable || m_srcBlend != src || m_dstBlend != dst)
    {
        m_bBlend   = enable;
        m_srcBlend = src;
        m_dstBlend = dst;

        D3D_SetRenderState(device, D3DRS_ALPHABLENDENABLE, enable);
        if (enable)
        {
            D3D_SetRenderState(device, D3DRS_SRCBLEND,  src);
            D3D_SetRenderState(device, D3DRS_DESTBLEND, dst);
            D3D_SetRenderState(device, D3DRS_BLENDOP,   op);
            D3D_SetRenderState(device, D3DRS_ALPHATESTENABLE, FALSE);
        }
    }
}

// ---------------------------------------------------------------------------
// Stencil
// ---------------------------------------------------------------------------
void CBackendStateCache::SetStencil(IDirect3DDevice9Ex* device,
                                    u32 enable, u32 func, u32 ref, u32 mask,
                                    u32 writemask, u32 fail, u32 pass, u32 zfail)
{
    if (m_stencilEnable != enable)
    {
        m_stencilEnable = enable;
        D3D_SetRenderState(device, D3DRS_STENCILENABLE, enable);
        if (!enable) return;
    }

    #define UPDATE_STENCIL_STATE(member, d3drs, value) \
        if (member != value) { \
            member = value; \
            D3D_SetRenderState(device, d3drs, value); \
        }

    UPDATE_STENCIL_STATE(m_stencilFunc,      D3DRS_STENCILFUNC,      func);
    UPDATE_STENCIL_STATE(m_stencilRef,       D3DRS_STENCILREF,       ref);
    UPDATE_STENCIL_STATE(m_stencilMask,      D3DRS_STENCILMASK,      mask);
    UPDATE_STENCIL_STATE(m_stencilWriteMask, D3DRS_STENCILWRITEMASK, writemask);
    UPDATE_STENCIL_STATE(m_stencilFail,      D3DRS_STENCILFAIL,      fail);
    UPDATE_STENCIL_STATE(m_stencilPass,      D3DRS_STENCILPASS,      pass);
    UPDATE_STENCIL_STATE(m_stencilZFail,     D3DRS_STENCILZFAIL,     zfail);

    #undef UPDATE_STENCIL_STATE
}

// ---------------------------------------------------------------------------
// Color write mask
// ---------------------------------------------------------------------------
void CBackendStateCache::SetColorWriteEnable(IDirect3DDevice9Ex* device, u32 mask)
{
    if (m_colorWriteMask != mask)
    {
        m_colorWriteMask = mask;
        D3D_SetRenderState(device, D3DRS_COLORWRITEENABLE,  mask);
        D3D_SetRenderState(device, D3DRS_COLORWRITEENABLE1, mask);
        D3D_SetRenderState(device, D3DRS_COLORWRITEENABLE2, mask);
        D3D_SetRenderState(device, D3DRS_COLORWRITEENABLE3, mask);
    }
}

// ---------------------------------------------------------------------------
// Depth write & culling
// ---------------------------------------------------------------------------
void CBackendStateCache::SetZWriteEnable(IDirect3DDevice9Ex* device, bool enable)
{
    if (m_zWriteEnable != enable)
    {
        m_zWriteEnable = enable;
        D3D_SetRenderState(device, D3DRS_ZWRITEENABLE, enable);
    }
}

void CBackendStateCache::SetCullMode(IDirect3DDevice9Ex* device, u32 mode)
{
    if (m_cullMode != mode)
    {
        m_cullMode = mode;
        D3D_SetRenderState(device, D3DRS_CULLMODE, mode);
    }
}

// ---------------------------------------------------------------------------
// Save / Restore (for temporary render target switches)
// ---------------------------------------------------------------------------
void CBackendStateCache::SaveRenderState(IDirect3DDevice9Ex* device)
{
    for (int i = 0; i < 4; i++)
        device->GetRenderTarget(i, &m_savedState.rt[i]);
    device->GetDepthStencilSurface(&m_savedState.zb);
    device->GetViewport(&m_savedState.viewport);
}

void CBackendStateCache::RestoreRenderState(IDirect3DDevice9Ex* device, CRenderBackend& backend)
{
    for (int i = 0; i < 4; i++)
    {
        if (m_savedState.rt[i])
        {
            SetRenderTarget(backend, m_savedState.rt[i], i);
            m_savedState.rt[i]->Release();
        }
    }
    if (m_savedState.zb)
    {
        SetDepthStencil(backend, m_savedState.zb);
        m_savedState.zb->Release();
    }
    SetViewport(device, m_savedState.viewport);
}

// ---------------------------------------------------------------------------
// Low‑level helpers
// ---------------------------------------------------------------------------
void CBackendStateCache::SetRawRenderState(IDirect3DDevice9Ex* device, D3DRENDERSTATETYPE State, DWORD Value)
{
    D3D_SetRenderState(device, State, Value);
};

// ---------------------------------------------------------------------------
// Low‑level D3D9 helpers
// ---------------------------------------------------------------------------
void CBackendStateCache::D3D_SetRenderState(IDirect3DDevice9Ex* device, D3DRENDERSTATETYPE state, DWORD value)
{
    HRESULT hr = device->SetRenderState(state, value);
    VERIFY(SUCCEEDED(hr));
}

void CBackendStateCache::D3D_SetRenderTarget(IDirect3DDevice9Ex* device, u32 idx, IDirect3DSurface9* surf)
{
    HRESULT hr = device->SetRenderTarget(idx, surf);
    VERIFY(SUCCEEDED(hr));
}

void CBackendStateCache::D3D_SetDepthStencil(IDirect3DDevice9Ex* device, IDirect3DSurface9* zb)
{
    HRESULT hr = device->SetDepthStencilSurface(zb);
    VERIFY(SUCCEEDED(hr));
}
