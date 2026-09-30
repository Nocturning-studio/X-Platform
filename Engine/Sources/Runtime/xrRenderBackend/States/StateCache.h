////////////////////////////////////////////////////////////////////////////////
// Created: 24.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CStateCache
{
  public:
	CStateCache() = default;
	~CStateCache() = default;

	CStateCache(const CStateCache&) = delete;
	CStateCache& operator=(const CStateCache&) = delete;

	// =====================================================================
	// RHI-native — прямые форвардеры
	// =====================================================================

	void SetBlendState(IRenderBackend& rhi, const RHI_BlendState& state) { rhi.SetBlendState(state); }
	void SetDepthStencilState(IRenderBackend& rhi, const RHI_DepthStencilState& state) { rhi.SetDepthStencilState(state); }
	void SetRasterizerState(IRenderBackend& rhi, const RHI_RasterizerState& state) { rhi.SetRasterizerState(state); }

	void SetViewport(IRenderBackend& rhi, const RHI_Viewport& vp) { rhi.SetViewport(vp); }
	void SetScissor(IRenderBackend& rhi, const RHI_Rect* rect) { rhi.SetScissorRect(rect); }

	void SetRenderTargets(IRenderBackend& rhi,
						  const RHI_RenderTargetView* rtvs,
						  uint32_t count,
						  RHI_DepthStencilView dsv)
	{
		rhi.SetRenderTargets(rtvs, count, dsv);
	}

	void ClearRenderTarget(IRenderBackend& rhi, RHI_RenderTargetView rtv, const fvec4& color)
	{
		rhi.ClearRenderTarget(rtv, color);
	}

	void ClearDepthStencil(IRenderBackend& rhi, RHI_DepthStencilView dsv, float depth, uint8_t stencil)
	{
		rhi.ClearDepthStencil(dsv, depth, stencil);
	}

	// =====================================================================
	// Convenience — меняют одно поле поверх текущего состояния
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
};
////////////////////////////////////////////////////////////////////////////////
