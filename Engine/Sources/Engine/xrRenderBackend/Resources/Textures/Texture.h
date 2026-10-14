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
#include <xrRenderBackend/Resources/SharedResource.h>
#include <xrRenderBackend/Resources/ResourceState.h>
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CTexture : public CRefCountedResource
{
  public:
	CTexture() = default;
	~CTexture() override;

	CTexture(const CTexture&) = delete;
	CTexture& operator=(const CTexture&) = delete;

	bool Create(IRenderBackend& rhi, const RHI_TextureDesc& desc);

	const RHI_TextureDesc& GetDesc() const { return m_desc; }

	RHI_TextureHandle GetRHIHandle() const { return m_rhiHandle; }

	RHI_RenderTargetView CreateRTV(uint32_t mip = 0, uint32_t face = 0) const;
	RHI_DepthStencilView CreateDSV(uint32_t mip = 0, uint32_t face = 0) const;

	void Bind(IRenderBackend& rhi, uint32_t slot) const;

	void Transition(EResourceState /*state*/) {}

  private:
	void DestroyRHI();

	RHI_TextureDesc m_desc;
	IRenderBackend* m_rhi = nullptr;
	RHI_TextureHandle m_rhiHandle{};
};
////////////////////////////////////////////////////////////////////////////////
using ref_texture = CSharedPtr<CTexture>;
////////////////////////////////////////////////////////////////////////////////
