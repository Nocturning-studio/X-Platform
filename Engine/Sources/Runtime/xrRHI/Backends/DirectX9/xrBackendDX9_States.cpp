////////////////////////////////////////////////////////////////////////////////
// Created: 30.09.2026 9:27:19
// Author: NS_Deathman
// File: xrBackendDX9_States.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "xrBackendDX9.h"
////////////////////////////////////////////////////////////////////////////////
namespace
{

	// ---- Конвертеры RHI -> D3D9 ----

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

void CRenderBackendDX9::InvalidateStateCache()
{
	m_blendCache = RHI_BlendState{};
	m_depthCache = RHI_DepthStencilState{};
	m_rasterCache = RHI_RasterizerState{};

	m_blendCacheValid = false;
	m_depthCacheValid = false;
	m_rasterCacheValid = false;

	m_viewportCacheValid = false;
	m_scissorCacheValid = false;
	m_scissorEnabled = false;

	m_currentVS = nullptr;
	m_currentPS = nullptr;
}

void CRenderBackendDX9::SetBlendState(const RHI_BlendState& s)
{
	if (!m_pDevice)
		return;

	RHI_BlendState& c = m_blendCache;
	const bool force = !m_blendCacheValid;

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
			m_pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, s.separateAlpha ? TRUE : FALSE);
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

	m_blendCacheValid = true;
}

void CRenderBackendDX9::SetDepthStencilState(const RHI_DepthStencilState& s)
{
	if (!m_pDevice)
		return;

	RHI_DepthStencilState& c = m_depthCache;
	const bool force = !m_depthCacheValid;

	// --- Depth ---
	if (force || c.depthEnable != s.depthEnable)
	{
		c.depthEnable = s.depthEnable;
		m_pDevice->SetRenderState(D3DRS_ZENABLE, s.depthEnable ? D3DZB_TRUE : D3DZB_FALSE);
	}
	if (force || c.depthWriteEnable != s.depthWriteEnable)
	{
		c.depthWriteEnable = s.depthWriteEnable;
		m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, s.depthWriteEnable ? TRUE : FALSE);
	}
	if (force || c.depthFunc != s.depthFunc)
	{
		c.depthFunc = s.depthFunc;
		m_pDevice->SetRenderState(D3DRS_ZFUNC, ToD3DCmpFunc(s.depthFunc));
	}

	// --- Stencil ---
	if (force || c.stencilEnable != s.stencilEnable)
	{
		c.stencilEnable = s.stencilEnable;
		m_pDevice->SetRenderState(D3DRS_STENCILENABLE, s.stencilEnable ? TRUE : FALSE);
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
		if (force || c.stencilRef != s.stencilRef)
		{
			c.stencilRef = s.stencilRef;
			m_pDevice->SetRenderState(D3DRS_STENCILREF, s.stencilRef);
		}
		if (force || c.stencilFailOp != s.stencilFailOp)
		{
			c.stencilFailOp = s.stencilFailOp;
			m_pDevice->SetRenderState(D3DRS_STENCILFAIL, ToD3DStencilOp(s.stencilFailOp));
		}
		if (force || c.stencilDepthFailOp != s.stencilDepthFailOp)
		{
			c.stencilDepthFailOp = s.stencilDepthFailOp;
			m_pDevice->SetRenderState(D3DRS_STENCILZFAIL, ToD3DStencilOp(s.stencilDepthFailOp));
		}
		if (force || c.stencilPassOp != s.stencilPassOp)
		{
			c.stencilPassOp = s.stencilPassOp;
			m_pDevice->SetRenderState(D3DRS_STENCILPASS, ToD3DStencilOp(s.stencilPassOp));
		}
	}

	m_depthCacheValid = true;
}

void CRenderBackendDX9::SetRasterizerState(const RHI_RasterizerState& s)
{
	if (!m_pDevice)
		return;

	RHI_RasterizerState& c = m_rasterCache;
	const bool force = !m_rasterCacheValid;

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
		m_pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, s.scissorEnable ? TRUE : FALSE);
	}
	if (force || c.depthBias != s.depthBias)
	{
		c.depthBias = s.depthBias;
		m_pDevice->SetRenderState(D3DRS_DEPTHBIAS, F2DW(s.depthBias));
	}
	if (force || c.slopeScaledDepthBias != s.slopeScaledDepthBias)
	{
		c.slopeScaledDepthBias = s.slopeScaledDepthBias;
		m_pDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, F2DW(s.slopeScaledDepthBias));
	}

	m_rasterCacheValid = true;
}

void CRenderBackendDX9::SetViewport(const RHI_Viewport& vp)
{
	if (!m_pDevice)
		return;

	if (m_viewportCacheValid && m_viewportCache == vp)
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
		Msg("! [DX9] SetViewport failed (0x%08x)", hr);
		return;
	}

	m_viewportCache = vp;
	m_viewportCacheValid = true;
}

RHI_Viewport CRenderBackendDX9::GetViewport() const
{
	return m_viewportCacheValid ? m_viewportCache : RHI_Viewport{};
}

void CRenderBackendDX9::SetScissorRect(const RHI_Rect* rect)
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
		m_scissorCacheValid = false;
		return;
	}

	if (m_scissorCacheValid && m_scissorEnabled && m_scissorCache == *rect)
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
		Msg("! [DX9] SetScissorRect failed (0x%08x)", hr);
		return;
	}

	m_scissorCache = *rect;
	m_scissorCacheValid = true;
}

bool CRenderBackendDX9::GetScissorRect(RHI_Rect& out) const
{
	if (!m_scissorEnabled || !m_scissorCacheValid)
		return false;
	out = m_scissorCache;
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
		Msg("! [DX9] CacheBackBufferDimensions: GetRenderTarget(0) failed");
		return;
	}

	D3DSURFACE_DESC desc{};
	if (SUCCEEDED(bb->GetDesc(&desc)))
	{
		m_backBufferWidth = desc.Width;
		m_backBufferHeight = desc.Height;
	}
	bb->Release();

	// D3D9 выставляет viewport по размеру back buffer'а при создании/reset
	// устройства. Синхронизируем кэш, чтобы getter сразу возвращал корректное
	// значение (иначе первый GetCurrentViewport() вернёт 0x0).
	m_viewportCache.X = 0;
	m_viewportCache.Y = 0;
	m_viewportCache.Width = m_backBufferWidth;
	m_viewportCache.Height = m_backBufferHeight;
	m_viewportCache.MinZ = 0.0f;
	m_viewportCache.MaxZ = 1.0f;
	m_viewportCacheValid = true;

	// После Reset D3D9 сбрасывает render states — кэш состояний невалиден.
	// InvalidateStateCache() выставляет valid=false, но getters всё равно
	// вернут "дефолтные" значения, соответствующие D3D9-дефолтам.
	RefreshBackBufferRTVs();
	InvalidateStateCache();
}
////////////////////////////////////////////////////////////////////////////////
