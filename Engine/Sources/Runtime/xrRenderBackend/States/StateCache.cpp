////////////////////////////////////////////////////////////////////////////////
// Created: 24.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "StateCache.h"
////////////////////////////////////////////////////////////////////////////////
CStateCache::CStateCache()
{
    Invalidate();
}

CStateCache::~CStateCache()
{
}

void CStateCache::Invalidate()
{
    for (u32 i = 0; i < 4; ++i)
        m_pRT[i] = nullptr;
    m_pZB = nullptr;

    m_bBlend = u32(-1);
    m_srcBlend = (D3DBLEND)u32(-1);
    m_dstBlend = (D3DBLEND)u32(-1);
    m_blendOp = (D3DBLENDOP)u32(-1);

    m_stencilEnable = u32(-1);
    m_stencilFunc = u32(-1);
    m_stencilRef = u32(-1);
    m_stencilMask = u32(-1);
    m_stencilWriteMask = u32(-1);
    m_stencilFail = u32(-1);
    m_stencilPass = u32(-1);
    m_stencilZFail = u32(-1);

    m_colorWriteMask = u32(-1);
    m_cullMode = u32(-1);
    m_zWriteEnable = u32(-1);
}

bool CStateCache::SetRenderTarget(IDirect3DDevice9Ex* device, IDirect3DSurface9* RT, u32 idx)
{
    if (m_pRT[idx] != RT)
    {
        m_pRT[idx] = RT;
        D3D_SetRenderTarget(device, idx, RT);
        return true;
    }
    return false;
}

bool CStateCache::SetDepthStencil(IDirect3DDevice9Ex* device, IDirect3DSurface9* ZB)
{
    if (m_pZB != ZB)
    {
        m_pZB = ZB;
        D3D_SetDepthStencil(device, ZB);
        return true;
    }
    return false;
}

void CStateCache::SetViewport(IDirect3DDevice9Ex* device, const D3DVIEWPORT9& vp)
{
    HRESULT hr = device->SetViewport(&vp);
    VERIFY(SUCCEEDED(hr));
}

void CStateCache::SetScissor(IDirect3DDevice9Ex* device, const RECT* rect)
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

bool CStateCache::SetBlend(IDirect3DDevice9Ex* device, BOOL enable, D3DBLEND src, D3DBLEND dst)
{
    return SetBlendEx(device, enable, src, dst, D3DBLENDOP_ADD);
}

bool CStateCache::SetBlendEx(IDirect3DDevice9Ex* device, BOOL enable, D3DBLEND src, D3DBLEND dst, D3DBLENDOP op)
{
    const u32 bEnable = enable ? 1u : 0u;
    if (m_bBlend == bEnable && m_srcBlend == src && m_dstBlend == dst && m_blendOp == op)
        return false;

    m_bBlend = bEnable;
    m_srcBlend = src;
    m_dstBlend = dst;
    m_blendOp = op;

    D3D_SetRenderState(device, D3DRS_ALPHABLENDENABLE, bEnable);
    if (bEnable)
    {
        D3D_SetRenderState(device, D3DRS_SRCBLEND, src);
        D3D_SetRenderState(device, D3DRS_DESTBLEND, dst);
        D3D_SetRenderState(device, D3DRS_BLENDOP, op);
        D3D_SetRenderState(device, D3DRS_ALPHATESTENABLE, FALSE);
    }
    return true;
}

void CStateCache::SetStencil(IDirect3DDevice9Ex* device, u32 enable, u32 func, u32 ref, u32 mask, u32 writemask, u32 fail, u32 pass, u32 zfail)
{
    if (m_stencilEnable != enable)
    {
        m_stencilEnable = enable;
        D3D_SetRenderState(device, D3DRS_STENCILENABLE, enable);
        if (!enable)
            return;
    }

#define UPDATE_STENCIL_STATE(member, d3drs, value)  \
    if (member != (value))                          \
    {                                               \
        member = (value);                           \
        D3D_SetRenderState(device, d3drs, (value)); \
    }

    UPDATE_STENCIL_STATE(m_stencilFunc, D3DRS_STENCILFUNC, func);
    UPDATE_STENCIL_STATE(m_stencilRef, D3DRS_STENCILREF, ref);
    UPDATE_STENCIL_STATE(m_stencilMask, D3DRS_STENCILMASK, mask);
    UPDATE_STENCIL_STATE(m_stencilWriteMask, D3DRS_STENCILWRITEMASK, writemask);
    UPDATE_STENCIL_STATE(m_stencilFail, D3DRS_STENCILFAIL, fail);
    UPDATE_STENCIL_STATE(m_stencilPass, D3DRS_STENCILPASS, pass);
    UPDATE_STENCIL_STATE(m_stencilZFail, D3DRS_STENCILZFAIL, zfail);

#undef UPDATE_STENCIL_STATE
}

bool CStateCache::SetColorWriteEnable(IDirect3DDevice9Ex* device, u32 mask)
{
    if (m_colorWriteMask == mask)
        return false;

    m_colorWriteMask = mask;
    D3D_SetRenderState(device, D3DRS_COLORWRITEENABLE, mask);
    D3D_SetRenderState(device, D3DRS_COLORWRITEENABLE1, mask);
    D3D_SetRenderState(device, D3DRS_COLORWRITEENABLE2, mask);
    D3D_SetRenderState(device, D3DRS_COLORWRITEENABLE3, mask);
    return true;
}

bool CStateCache::SetDepthWriteEnable(IDirect3DDevice9Ex* device, bool enable)
{
    const u32 bEnable = enable ? 1u : 0u;
    if (m_zWriteEnable == bEnable)
        return false;

    m_zWriteEnable = bEnable;
    D3D_SetRenderState(device, D3DRS_ZWRITEENABLE, bEnable);
    return true;
}

bool CStateCache::SetCullMode(IDirect3DDevice9Ex* device, u32 mode)
{
    if (m_cullMode == mode)
        return false;

    m_cullMode = mode;
    D3D_SetRenderState(device, D3DRS_CULLMODE, mode);
    return true;
}

void CStateCache::SaveRenderState(IDirect3DDevice9Ex* device)
{
    for (int i = 0; i < 4; ++i)
        device->GetRenderTarget(i, &m_savedState.rt[i]);
    device->GetDepthStencilSurface(&m_savedState.zb);
    device->GetViewport(&m_savedState.viewport);
}

void CStateCache::RestoreRenderState(IDirect3DDevice9Ex* device)
{
    for (int i = 0; i < 4; ++i)
    {
        if (m_savedState.rt[i])
        {
            SetRenderTarget(device, m_savedState.rt[i], i);
            m_savedState.rt[i]->Release();
            m_savedState.rt[i] = nullptr;
        }
    }
    if (m_savedState.zb)
    {
        SetDepthStencil(device, m_savedState.zb);
        m_savedState.zb->Release();
        m_savedState.zb = nullptr;
    }
    SetViewport(device, m_savedState.viewport);
}

void CStateCache::SetRawRenderState(IDirect3DDevice9Ex* device, D3DRENDERSTATETYPE State, DWORD Value)
{
    D3D_SetRenderState(device, State, Value);
}

void CStateCache::D3D_SetRenderState(IDirect3DDevice9Ex* device, D3DRENDERSTATETYPE state, DWORD value)
{
    HRESULT hr = device->SetRenderState(state, value);
    VERIFY(SUCCEEDED(hr));
}

void CStateCache::D3D_SetRenderTarget(IDirect3DDevice9Ex* device, u32 idx, IDirect3DSurface9* surf)
{
    HRESULT hr = device->SetRenderTarget(idx, surf);
    VERIFY(SUCCEEDED(hr));
}

void CStateCache::D3D_SetDepthStencil(IDirect3DDevice9Ex* device, IDirect3DSurface9* zb)
{
    HRESULT hr = device->SetDepthStencilSurface(zb);
    VERIFY(SUCCEEDED(hr));
}
////////////////////////////////////////////////////////////////////////////////
