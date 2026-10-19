////////////////////////////////////////////////////////////////////////////////
// Created: 17.10.2026 23:20:58
// Author: NS_Deathman
// File: BackendDX9_StateCache.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include <source/pch.h>
#include <include/XRHI.h>
#include "BackendDX9_StateCache.h"
////////////////////////////////////////////////////////////////////////////////
namespace
{
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
// Blend
// ============================================================================
void DX9StateCache::ApplyBlend(IDirect3DDevice9Ex* device, const RHI_BlendState& s)
{
	if (!device) return;

	RHI_BlendState& c = m_blend;
	const bool force = !m_blendValid;

	if (force || c.enable != s.enable)
	{
		c.enable = s.enable;
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, s.enable ? TRUE : FALSE);
	}

	if (s.enable)
	{
		if (force || c.separateAlpha != s.separateAlpha)
		{
			c.separateAlpha = s.separateAlpha;
			device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, s.separateAlpha ? TRUE : FALSE);
		}
		if (force || c.srcColor != s.srcColor)
		{
			c.srcColor = s.srcColor;
			device->SetRenderState(D3DRS_SRCBLEND, ToD3DBlend(s.srcColor));
		}
		if (force || c.dstColor != s.dstColor)
		{
			c.dstColor = s.dstColor;
			device->SetRenderState(D3DRS_DESTBLEND, ToD3DBlend(s.dstColor));
		}
		if (force || c.opColor != s.opColor)
		{
			c.opColor = s.opColor;
			device->SetRenderState(D3DRS_BLENDOP, ToD3DBlendOp(s.opColor));
		}
		if (s.separateAlpha)
		{
			if (force || c.srcAlpha != s.srcAlpha)
			{
				c.srcAlpha = s.srcAlpha;
				device->SetRenderState(D3DRS_SRCBLENDALPHA, ToD3DBlend(s.srcAlpha));
			}
			if (force || c.dstAlpha != s.dstAlpha)
			{
				c.dstAlpha = s.dstAlpha;
				device->SetRenderState(D3DRS_DESTBLENDALPHA, ToD3DBlend(s.dstAlpha));
			}
			if (force || c.opAlpha != s.opAlpha)
			{
				c.opAlpha = s.opAlpha;
				device->SetRenderState(D3DRS_BLENDOPALPHA, ToD3DBlendOp(s.opAlpha));
			}
		}
	}

	if (force || c.writeMask != s.writeMask)
	{
		c.writeMask = s.writeMask;
		device->SetRenderState(D3DRS_COLORWRITEENABLE, s.writeMask);
		device->SetRenderState(D3DRS_COLORWRITEENABLE1, s.writeMask);
		device->SetRenderState(D3DRS_COLORWRITEENABLE2, s.writeMask);
		device->SetRenderState(D3DRS_COLORWRITEENABLE3, s.writeMask);
	}

	m_blendValid = true;
}

// ============================================================================
// Depth / Stencil
// ============================================================================
void DX9StateCache::ApplyDepthStencil(IDirect3DDevice9Ex* device, const RHI_DepthStencilState& s)
{
	if (!device) return;

	RHI_DepthStencilState& c = m_depth;
	const bool force = !m_depthValid;

	if (force || c.depthEnable != s.depthEnable)
	{
		c.depthEnable = s.depthEnable;
		device->SetRenderState(D3DRS_ZENABLE, s.depthEnable ? D3DZB_TRUE : D3DZB_FALSE);
	}
	if (force || c.depthWriteEnable != s.depthWriteEnable)
	{
		c.depthWriteEnable = s.depthWriteEnable;
		device->SetRenderState(D3DRS_ZWRITEENABLE, s.depthWriteEnable ? TRUE : FALSE);
	}
	if (force || c.depthFunc != s.depthFunc)
	{
		c.depthFunc = s.depthFunc;
		device->SetRenderState(D3DRS_ZFUNC, ToD3DCmpFunc(s.depthFunc));
	}
	if (force || c.stencilEnable != s.stencilEnable)
	{
		c.stencilEnable = s.stencilEnable;
		device->SetRenderState(D3DRS_STENCILENABLE, s.stencilEnable ? TRUE : FALSE);
	}
	if (s.stencilEnable)
	{
		if (force || c.stencilFunc != s.stencilFunc)
		{
			c.stencilFunc = s.stencilFunc;
			device->SetRenderState(D3DRS_STENCILFUNC, ToD3DCmpFunc(s.stencilFunc));
		}
		if (force || c.stencilReadMask != s.stencilReadMask)
		{
			c.stencilReadMask = s.stencilReadMask;
			device->SetRenderState(D3DRS_STENCILMASK, s.stencilReadMask);
		}
		if (force || c.stencilWriteMask != s.stencilWriteMask)
		{
			c.stencilWriteMask = s.stencilWriteMask;
			device->SetRenderState(D3DRS_STENCILWRITEMASK, s.stencilWriteMask);
		}
		if (force || c.stencilFailOp != s.stencilFailOp)
		{
			c.stencilFailOp = s.stencilFailOp;
			device->SetRenderState(D3DRS_STENCILFAIL, ToD3DStencilOp(s.stencilFailOp));
		}
		if (force || c.stencilDepthFailOp != s.stencilDepthFailOp)
		{
			c.stencilDepthFailOp = s.stencilDepthFailOp;
			device->SetRenderState(D3DRS_STENCILZFAIL, ToD3DStencilOp(s.stencilDepthFailOp));
		}
		if (force || c.stencilPassOp != s.stencilPassOp)
		{
			c.stencilPassOp = s.stencilPassOp;
			device->SetRenderState(D3DRS_STENCILPASS, ToD3DStencilOp(s.stencilPassOp));
		}
	}

	m_depthValid = true;
}

// ============================================================================
// Rasterizer
// ============================================================================
void DX9StateCache::ApplyRasterizer(IDirect3DDevice9Ex* device, const RHI_RasterizerState& s)
{
	if (!device) return;

	RHI_RasterizerState& c = m_raster;
	const bool force = !m_rasterValid;

	if (force || c.fillMode != s.fillMode)
	{
		c.fillMode = s.fillMode;
		device->SetRenderState(D3DRS_FILLMODE, ToD3DFill(s.fillMode));
	}
	if (force || c.cullMode != s.cullMode)
	{
		c.cullMode = s.cullMode;
		device->SetRenderState(D3DRS_CULLMODE, ToD3DCull(s.cullMode));
	}
	if (force || c.scissorEnable != s.scissorEnable)
	{
		c.scissorEnable = s.scissorEnable;
		device->SetRenderState(D3DRS_SCISSORTESTENABLE, s.scissorEnable ? TRUE : FALSE);
	}
	if (force || c.depthBias != s.depthBias)
	{
		c.depthBias = s.depthBias;
		device->SetRenderState(D3DRS_DEPTHBIAS, F2DW(s.depthBias));
	}
	if (force || c.slopeScaledDepthBias != s.slopeScaledDepthBias)
	{
		c.slopeScaledDepthBias = s.slopeScaledDepthBias;
		device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, F2DW(s.slopeScaledDepthBias));
	}

	m_rasterValid = true;
}

// ============================================================================
// Dynamic state: viewport / scissor / stencilRef / blendFactor
// ============================================================================
void DX9StateCache::ApplyViewport(IDirect3DDevice9Ex* device, const RHI_Viewport& vp)
{
	if (!device) return;
	if (!m_viewport.NeedsUpdate(vp))
		return;

	D3DVIEWPORT9 d3dvp{};
	d3dvp.X = vp.X;
	d3dvp.Y = vp.Y;
	d3dvp.Width = vp.Width;
	d3dvp.Height = vp.Height;
	d3dvp.MinZ = vp.MinZ;
	d3dvp.MaxZ = vp.MaxZ;

	const HRESULT hr = device->SetViewport(&d3dvp);
	if (FAILED(hr))
	{
		Print("! [DX9] SetViewport failed (0x%08x)", hr);
		return;
	}
	m_viewport.MarkUpdated(vp);
}

void DX9StateCache::ApplyScissorRect(IDirect3DDevice9Ex* device, const RHI_Rect* rect)
{
	if (!device) return;

	if (rect && !IsScissorEnabled())
	{
		static bool warned = false;
		if (!warned)
		{
			Print("! [DX9] SetScissorRect: rect set, but rasterizer.scissorEnable is false; "
				  "scissor will be ignored by D3D9.");
			warned = true;
		}
	}

	RHI_Rect effective;
	if (rect)
	{
		effective = *rect;
	}
	else
	{
		effective.left = 0;
		effective.top = 0;
		effective.right = static_cast<int>(GetRenderTargetWidth());
		effective.bottom = static_cast<int>(GetRenderTargetHeight());
	}

	if (!m_scissor.NeedsUpdate(effective))
		return;

	RECT d3drect{};
	d3drect.left = effective.left;
	d3drect.top = effective.top;
	d3drect.right = effective.right;
	d3drect.bottom = effective.bottom;

	const HRESULT hr = device->SetScissorRect(&d3drect);
	if (FAILED(hr))
	{
		Print("! [DX9] SetScissorRect failed (0x%08x)", hr);
		return;
	}
	m_scissor.MarkUpdated(effective);
}

void DX9StateCache::ApplyStencilRef(IDirect3DDevice9Ex* device, uint8_t ref)
{
	if (!device) return;
	if (!m_stencilRef.NeedsUpdate(ref))
		return;

	device->SetRenderState(D3DRS_STENCILREF, ref);
	m_stencilRef.MarkUpdated(ref);
}

void DX9StateCache::ApplyBlendFactor(IDirect3DDevice9Ex* device, const Float4& factor)
{
	if (!device) 
		return;

	if (!m_blendFactor.NeedsUpdate(factor)) 
		return;

	const DWORD c = D3DCOLOR_COLORVALUE(factor.x, factor.y, factor.z, factor.w);
	device->SetRenderState(D3DRS_BLENDFACTOR, c);
	m_blendFactor.MarkUpdated(factor);
}

// ============================================================================
// Shader / PSO invalidation
// ============================================================================
void DX9StateCache::InvalidateVertexShader(IDirect3DVertexShader9* native)
{
	if (m_currentVS == native)
	{
		m_currentVS = nullptr;
		m_psoValid = false;
	}
}

void DX9StateCache::InvalidatePixelShader(IDirect3DPixelShader9* native)
{
	if (m_currentPS == native)
	{
		m_currentPS = nullptr;
		m_psoValid = false;
	}
}
////////////////////////////////////////////////////////////////////////////////
