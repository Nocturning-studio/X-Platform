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
//  Копирование, изменение и распространение разрешены для некоммерческих
//  целей. Публичное указание авторства в вашем проекте, документации или
//  рекламе не требуется — оно приветствуется, но не является условием
//  использования.
//
//  При распространении копий или производных работ лицензия обязывает
//  сохранять настоящие уведомления и передавать получателю текст лицензии
//  или ссылку на него. Пожалуйста, не удаляйте эту плашку — это помогает
//  сохранять историю проекта и корректно соблюдать лицензию.
//
//  Коммерческое использование без отдельного разрешения правообладателей
//  запрещено.
//
//  Полный текст лицензии:
//      https://polyformproject.org/licenses/noncommercial/1.0.0
//
//  Этот файл является source-available, а не open source.
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
class IRenderBackend;
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API CStateCache
{
public:
	CStateCache() = default;
	~CStateCache() = default;

	CStateCache(const CStateCache&) = delete;
	CStateCache& operator=(const CStateCache&) = delete;

	// =====================================================================
	// RHI-native — прямые форвардеры.
	//
	// Реализация вынесена в .cpp: сигнатуры принимают IRenderBackend&,
	// но внутри кастятся на CRenderBackendDX9 через AsDX9, потому что
	// SetBlendState/SetDepthStencilState/SetRasterizerState и
	// соответствующие Get'еры объявлены только в DX9-бэкенде.
	// =====================================================================

	void SetBlendState(IRenderBackend& rhi, const RHI_BlendState& state);
	void SetDepthStencilState(IRenderBackend& rhi, const RHI_DepthStencilState& state);
	void SetRasterizerState(IRenderBackend& rhi, const RHI_RasterizerState& state);

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

	bool GetBlendEnable(const IRenderBackend& rhi) const;
	RHI_Blend GetSrcBlend(const IRenderBackend& rhi) const;
	RHI_Blend GetDstBlend(const IRenderBackend& rhi) const;
	RHI_CullMode GetCullMode(const IRenderBackend& rhi) const;
};
////////////////////////////////////////////////////////////////////////////////