////////////////////////////////////////////////////////////////////////////////
//  X-Platform — расширение функциональности X-Ray Engine
//
//  Автор:        NSDeathman
//  Студия:       Nocturning Studio
//
//  SPDX-FileCopyrightText: 2026 Nocturning Studio и участники X-Platform
//  SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
//
//  Этот файл распространяется на условиях PolyForm Noncommercial 1.0.0.
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "R_Backend_StateCache.h"

#include <d3d9.h>
#include <cstring>
////////////////////////////////////////////////////////////////////////////////

namespace
{

    // ---- Конвертеры RHI -> D3D9 -----------------------------------------------

    D3DBLEND ToD3DBlend(RHI_Blend b)
    {
        switch (b)
        {
        case RHI_Blend::Zero:         return D3DBLEND_ZERO;
        case RHI_Blend::One:          return D3DBLEND_ONE;
        case RHI_Blend::SrcColor:     return D3DBLEND_SRCCOLOR;
        case RHI_Blend::InvSrcColor:  return D3DBLEND_INVSRCCOLOR;
        case RHI_Blend::SrcAlpha:     return D3DBLEND_SRCALPHA;
        case RHI_Blend::InvSrcAlpha:  return D3DBLEND_INVSRCALPHA;
        case RHI_Blend::DestAlpha:    return D3DBLEND_DESTALPHA;
        case RHI_Blend::InvDestAlpha: return D3DBLEND_INVDESTALPHA;
        case RHI_Blend::DestColor:    return D3DBLEND_DESTCOLOR;
        case RHI_Blend::InvDestColor: return D3DBLEND_INVDESTCOLOR;
        case RHI_Blend::SrcAlphaSat:  return D3DBLEND_SRCALPHASAT;
        }
        return D3DBLEND_ONE;
    }

    D3DBLENDOP ToD3DBlendOp(RHI_BlendOp op)
    {
        switch (op)
        {
        case RHI_BlendOp::Add:         return D3DBLENDOP_ADD;
        case RHI_BlendOp::Subtract:    return D3DBLENDOP_SUBTRACT;
        case RHI_BlendOp::RevSubtract: return D3DBLENDOP_REVSUBTRACT;
        case RHI_BlendOp::Min:         return D3DBLENDOP_MIN;
        case RHI_BlendOp::Max:         return D3DBLENDOP_MAX;
        }
        return D3DBLENDOP_ADD;
    }

    D3DCMPFUNC ToD3DCmpFunc(RHI_CmpFunc f)
    {
        switch (f)
        {
        case RHI_CmpFunc::Never:        return D3DCMP_NEVER;
        case RHI_CmpFunc::Less:         return D3DCMP_LESS;
        case RHI_CmpFunc::Equal:        return D3DCMP_EQUAL;
        case RHI_CmpFunc::LessEqual:    return D3DCMP_LESSEQUAL;
        case RHI_CmpFunc::Greater:      return D3DCMP_GREATER;
        case RHI_CmpFunc::NotEqual:     return D3DCMP_NOTEQUAL;
        case RHI_CmpFunc::GreaterEqual: return D3DCMP_GREATEREQUAL;
        case RHI_CmpFunc::Always:       return D3DCMP_ALWAYS;
        }
        return D3DCMP_ALWAYS;
    }

    D3DSTENCILOP ToD3DStencilOp(RHI_StencilOp op)
    {
        switch (op)
        {
        case RHI_StencilOp::Keep:    return D3DSTENCILOP_KEEP;
        case RHI_StencilOp::Zero:    return D3DSTENCILOP_ZERO;
        case RHI_StencilOp::Replace: return D3DSTENCILOP_REPLACE;
        case RHI_StencilOp::IncrSat: return D3DSTENCILOP_INCRSAT;
        case RHI_StencilOp::DecrSat: return D3DSTENCILOP_DECRSAT;
        case RHI_StencilOp::Invert:  return D3DSTENCILOP_INVERT;
        case RHI_StencilOp::Incr:    return D3DSTENCILOP_INCR;
        case RHI_StencilOp::Decr:    return D3DSTENCILOP_DECR;
        }
        return D3DSTENCILOP_KEEP;
    }

    D3DCULL ToD3DCull(RHI_CullMode c)
    {
        switch (c)
        {
        case RHI_CullMode::None:             return D3DCULL_NONE;
        case RHI_CullMode::Clockwise:        return D3DCULL_CW;
        case RHI_CullMode::CounterClockwise: return D3DCULL_CCW;
        }
        return D3DCULL_CCW;
    }

    D3DFILLMODE ToD3DFill(RHI_FillMode f)
    {
        switch (f)
        {
        case RHI_FillMode::Point:     return D3DFILL_POINT;
        case RHI_FillMode::Wireframe: return D3DFILL_WIREFRAME;
        case RHI_FillMode::Solid:     return D3DFILL_SOLID;
        }
        return D3DFILL_SOLID;
    }

    // D3D9 хранит float-значения render states как битовые копии DWORD.
    inline DWORD F2DW(float f) noexcept
    {
        DWORD out;
        static_assert(sizeof(out) == sizeof(f), "F2DW: size mismatch");
        std::memcpy(&out, &f, sizeof(out));
        return out;
    }

} // namespace
////////////////////////////////////////////////////////////////////////////////

// ============================================================================
// Device binding / invalidation
// ============================================================================

void CStateCache::SetDevice(IDirect3DDevice9Ex* device)
{
    if (m_pDevice != device)
    {
        m_pDevice = device;
        Invalidate();
    }
}

void CStateCache::Invalidate()
{
    // Сбрасываем и значения (к D3D9-дефолтам), и флаги валидности.
    // Get* после этого возвращают корректные «умолчания», а первый
    // Set*State форсированно применит всё к устройству.
    m_blendCache = RHI_BlendState{};
    m_depthCache = RHI_DepthStencilState{};
    m_rasterCache = RHI_RasterizerState{};
    m_viewportCache = RHI_Viewport{};
    m_scissorCache = RHI_Rect{};

    m_stencilRef = 0;
    m_blendFactor = 0xFFFFFFFF;

    m_blendValid = false;
    m_depthValid = false;
    m_rasterValid = false;
    m_viewportValid = false;
    m_scissorValid = false;
    m_scissorEnabled = false;
    m_stencilRefValid = false;
    m_blendFactorValid = false;
}

// ============================================================================
// Blend state
// ============================================================================

void CStateCache::SetBlendState(const RHI_BlendState& s)
{
    if (!m_pDevice)
        return;

    const bool force = !m_blendValid;
    RHI_BlendState& c = m_blendCache;

    if (force || c.enable != s.enable)
    {
        c.enable = s.enable;
        m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, s.enable ? TRUE : FALSE);
    }

    if (s.enable)
    {
        if (force || c.separateAlpha != s.separateAlpha)
        {
            c.separateAlpha = s.separateAlpha;
            m_pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,
                s.separateAlpha ? TRUE : FALSE);
        }

        if (force || c.srcColor != s.srcColor)
        {
            c.srcColor = s.srcColor;
            m_pDevice->SetRenderState(D3DRS_SRCBLEND, ToD3DBlend(s.srcColor));
        }
        if (force || c.dstColor != s.dstColor)
        {
            c.dstColor = s.dstColor;
            m_pDevice->SetRenderState(D3DRS_DESTBLEND, ToD3DBlend(s.dstColor));
        }
        if (force || c.opColor != s.opColor)
        {
            c.opColor = s.opColor;
            m_pDevice->SetRenderState(D3DRS_BLENDOP, ToD3DBlendOp(s.opColor));
        }

        if (s.separateAlpha)
        {
            if (force || c.srcAlpha != s.srcAlpha)
            {
                c.srcAlpha = s.srcAlpha;
                m_pDevice->SetRenderState(D3DRS_SRCBLENDALPHA, ToD3DBlend(s.srcAlpha));
            }
            if (force || c.dstAlpha != s.dstAlpha)
            {
                c.dstAlpha = s.dstAlpha;
                m_pDevice->SetRenderState(D3DRS_DESTBLENDALPHA, ToD3DBlend(s.dstAlpha));
            }
            if (force || c.opAlpha != s.opAlpha)
            {
                c.opAlpha = s.opAlpha;
                m_pDevice->SetRenderState(D3DRS_BLENDOPALPHA, ToD3DBlendOp(s.opAlpha));
            }
        }
    }

    if (force || c.writeMask != s.writeMask)
    {
        c.writeMask = s.writeMask;
        m_pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, s.writeMask);
        m_pDevice->SetRenderState(D3DRS_COLORWRITEENABLE1, s.writeMask);
        m_pDevice->SetRenderState(D3DRS_COLORWRITEENABLE2, s.writeMask);
        m_pDevice->SetRenderState(D3DRS_COLORWRITEENABLE3, s.writeMask);
    }

    m_blendValid = true;
}

// ============================================================================
// Depth / stencil state
// ============================================================================

void CStateCache::SetDepthStencilState(const RHI_DepthStencilState& s)
{
    if (!m_pDevice)
        return;

    const bool force = !m_depthValid;
    RHI_DepthStencilState& c = m_depthCache;

    if (force || c.depthEnable != s.depthEnable)
    {
        c.depthEnable = s.depthEnable;
        m_pDevice->SetRenderState(D3DRS_ZENABLE,
            s.depthEnable ? D3DZB_TRUE : D3DZB_FALSE);
    }
    if (force || c.depthWriteEnable != s.depthWriteEnable)
    {
        c.depthWriteEnable = s.depthWriteEnable;
        m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE,
            s.depthWriteEnable ? TRUE : FALSE);
    }
    if (force || c.depthFunc != s.depthFunc)
    {
        c.depthFunc = s.depthFunc;
        m_pDevice->SetRenderState(D3DRS_ZFUNC, ToD3DCmpFunc(s.depthFunc));
    }
    if (force || c.stencilEnable != s.stencilEnable)
    {
        c.stencilEnable = s.stencilEnable;
        m_pDevice->SetRenderState(D3DRS_STENCILENABLE,
            s.stencilEnable ? TRUE : FALSE);
    }

    if (s.stencilEnable)
    {
        if (force || c.stencilFunc != s.stencilFunc)
        {
            c.stencilFunc = s.stencilFunc;
            m_pDevice->SetRenderState(D3DRS_STENCILFUNC, ToD3DCmpFunc(s.stencilFunc));
        }
        if (force || c.stencilReadMask != s.stencilReadMask)
        {
            c.stencilReadMask = s.stencilReadMask;
            m_pDevice->SetRenderState(D3DRS_STENCILMASK, s.stencilReadMask);
        }
        if (force || c.stencilWriteMask != s.stencilWriteMask)
        {
            c.stencilWriteMask = s.stencilWriteMask;
            m_pDevice->SetRenderState(D3DRS_STENCILWRITEMASK, s.stencilWriteMask);
        }
        if (force || c.stencilFailOp != s.stencilFailOp)
        {
            c.stencilFailOp = s.stencilFailOp;
            m_pDevice->SetRenderState(D3DRS_STENCILFAIL, ToD3DStencilOp(s.stencilFailOp));
        }
        if (force || c.stencilDepthFailOp != s.stencilDepthFailOp)
        {
            c.stencilDepthFailOp = s.stencilDepthFailOp;
            m_pDevice->SetRenderState(D3DRS_STENCILZFAIL,
                ToD3DStencilOp(s.stencilDepthFailOp));
        }
        if (force || c.stencilPassOp != s.stencilPassOp)
        {
            c.stencilPassOp = s.stencilPassOp;
            m_pDevice->SetRenderState(D3DRS_STENCILPASS, ToD3DStencilOp(s.stencilPassOp));
        }
    }

    m_depthValid = true;
}

// ============================================================================
// Rasterizer state
// ============================================================================

void CStateCache::SetRasterizerState(const RHI_RasterizerState& s)
{
    if (!m_pDevice)
        return;

    const bool force = !m_rasterValid;
    RHI_RasterizerState& c = m_rasterCache;

    if (force || c.fillMode != s.fillMode)
    {
        c.fillMode = s.fillMode;
        m_pDevice->SetRenderState(D3DRS_FILLMODE, ToD3DFill(s.fillMode));
    }
    if (force || c.cullMode != s.cullMode)
    {
        c.cullMode = s.cullMode;
        m_pDevice->SetRenderState(D3DRS_CULLMODE, ToD3DCull(s.cullMode));
    }
    if (force || c.scissorEnable != s.scissorEnable)
    {
        c.scissorEnable = s.scissorEnable;
        m_pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE,
            s.scissorEnable ? TRUE : FALSE);
        m_scissorEnabled = s.scissorEnable;
        if (!s.scissorEnable)
            m_scissorValid = false;
    }
    if (force || c.depthBias != s.depthBias)
    {
        c.depthBias = s.depthBias;
        m_pDevice->SetRenderState(D3DRS_DEPTHBIAS, F2DW(s.depthBias));
    }
    if (force || c.slopeScaledDepthBias != s.slopeScaledDepthBias)
    {
        c.slopeScaledDepthBias = s.slopeScaledDepthBias;
        m_pDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS,
            F2DW(s.slopeScaledDepthBias));
    }

    m_rasterValid = true;
}

// ============================================================================
// Scalar states
// ============================================================================

void CStateCache::SetStencilRef(uint8_t ref)
{
    if (!m_pDevice)
        return;
    if (m_stencilRefValid && m_stencilRef == ref)
        return;

    m_pDevice->SetRenderState(D3DRS_STENCILREF, ref);
    m_stencilRef = ref;
    m_stencilRefValid = true;
}

void CStateCache::SetBlendFactor(uint32_t argb)
{
    if (!m_pDevice)
        return;
    if (m_blendFactorValid && m_blendFactor == argb)
        return;

    m_pDevice->SetRenderState(D3DRS_BLENDFACTOR, (D3DCOLOR)argb);
    m_blendFactor = argb;
    m_blendFactorValid = true;
}

void CStateCache::SetBlendFactor(const fvec4& rgba)
{
    const D3DCOLOR c = D3DCOLOR_COLORVALUE(rgba.x, rgba.y, rgba.z, rgba.w);
    SetBlendFactor((uint32_t)c);
}

// ============================================================================
// Viewport / Scissor
// ============================================================================

void CStateCache::SetViewport(const RHI_Viewport& vp)
{
    if (!m_pDevice)
        return;
    if (m_viewportValid && m_viewportCache == vp)
        return;

    D3DVIEWPORT9 d3dvp{};
    d3dvp.X = vp.X;
    d3dvp.Y = vp.Y;
    d3dvp.Width = vp.Width;
    d3dvp.Height = vp.Height;
    d3dvp.MinZ = vp.MinZ;
    d3dvp.MaxZ = vp.MaxZ;

    const HRESULT hr = m_pDevice->SetViewport(&d3dvp);
    if (FAILED(hr))
    {
        Msg("! [StateCache] SetViewport failed (0x%08x)", hr);
        return;
    }

    m_viewportCache = vp;
    m_viewportValid = true;
}

void CStateCache::SetScissorRect(const RHI_Rect* rect)
{
    if (!m_pDevice)
        return;

    // nullptr — выключить scissor.
    if (!rect)
    {
        if (!m_scissorEnabled)
            return;

        m_pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        m_scissorEnabled = false;
        m_scissorValid = false;
        return;
    }

    if (m_scissorValid && m_scissorEnabled && m_scissorCache == *rect)
        return;

    if (!m_scissorEnabled)
    {
        m_pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
        m_scissorEnabled = true;
    }

    RECT d3drect{};
    d3drect.left = rect->left;
    d3drect.top = rect->top;
    d3drect.right = rect->right;
    d3drect.bottom = rect->bottom;

    const HRESULT hr = m_pDevice->SetScissorRect(&d3drect);
    if (FAILED(hr))
    {
        Msg("! [StateCache] SetScissorRect failed (0x%08x)", hr);
        return;
    }

    m_scissorCache = *rect;
    m_scissorValid = true;
}

bool CStateCache::GetScissorRect(RHI_Rect& out) const
{
    if (!m_scissorEnabled || !m_scissorValid)
        return false;
    out = m_scissorCache;
    return true;
}

// ============================================================================
// Convenience setters
// ============================================================================

void CStateCache::SetBlend(bool enable, RHI_Blend src, RHI_Blend dst)
{
    SetBlendEx(enable, src, dst, RHI_BlendOp::Add);
}

void CStateCache::SetBlendEx(bool enable, RHI_Blend src, RHI_Blend dst, RHI_BlendOp op)
{
    RHI_BlendState s = m_blendCache;
    s.enable = enable;
    s.srcColor = src;
    s.dstColor = dst;
    s.opColor = op;
    SetBlendState(s);
}

void CStateCache::SetStencil(bool enable,
    RHI_CmpFunc func,
    uint8_t ref, uint8_t mask, uint8_t writemask,
    RHI_StencilOp fail, RHI_StencilOp pass, RHI_StencilOp zfail)
{
    RHI_DepthStencilState s = m_depthCache;
    s.stencilEnable = enable;
    s.stencilFunc = func;
    s.stencilReadMask = mask;
    s.stencilWriteMask = writemask;
    s.stencilFailOp = fail;
    s.stencilPassOp = pass;
    s.stencilDepthFailOp = zfail;
    SetDepthStencilState(s);

    // stencilRef — отдельное динамическое состояние D3D9,
    // в RHI_DepthStencilState его нет. Раньше параметр ref игнорировался
    // (в старом коде строка была закомментирована); теперь применяем.
    SetStencilRef(ref);
}

void CStateCache::SetColorWriteEnable(uint8_t mask)
{
    RHI_BlendState s = m_blendCache;
    s.writeMask = mask;
    SetBlendState(s);
}

void CStateCache::SetDepthWriteEnable(bool enable)
{
    RHI_DepthStencilState s = m_depthCache;
    s.depthWriteEnable = enable;
    SetDepthStencilState(s);
}

void CStateCache::SetCullMode(RHI_CullMode mode)
{
    RHI_RasterizerState s = m_rasterCache;
    s.cullMode = mode;
    SetRasterizerState(s);
}
////////////////////////////////////////////////////////////////////////////////
