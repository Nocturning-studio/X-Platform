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
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CVertexDeclaration : public CRefCountedResource
{
  public:
	CVertexDeclaration() = default;
	~CVertexDeclaration() override;

	CVertexDeclaration(const CVertexDeclaration&) = delete;
	CVertexDeclaration& operator=(const CVertexDeclaration&) = delete;

	bool Create(IRenderBackend& rhi, const RHI_InputLayoutDesc& desc);

	const RHI_InputLayoutDesc& GetLayout() const { return m_desc; }
	RHI_InputLayoutHandle GetRHIHandle() const { return m_rhiHandle; }

  private:
	void DestroyRHI();

	RHI_InputLayoutDesc m_desc;
	IRenderBackend* m_rhi = nullptr;
	RHI_InputLayoutHandle m_rhiHandle{};
};
using ref_vertexdecl = CSharedPtr<CVertexDeclaration>;
////////////////////////////////////////////////////////////////////////////////
