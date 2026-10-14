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
#include <xrRHI/xrRHI.h>
#include <xrRenderBackend/Resources/SharedResource.h>
////////////////////////////////////////////////////////////////////////////////

class XRRB_API CVertexBuffer : public CRefCountedResource
{
  public:
	CVertexBuffer() = default;
	~CVertexBuffer() override;

	CVertexBuffer(const CVertexBuffer&) = delete;
	CVertexBuffer& operator=(const CVertexBuffer&) = delete;

	bool Create(IRenderBackend& rhi, const RHI_BufferDesc& desc, const void* initialData = nullptr);

	const RHI_BufferDesc& GetDesc() const { return m_desc; }
	uint32_t GetSizeBytes() const { return m_desc.sizeBytes; }
	uint32_t GetStride() const { return m_desc.stride; }

	RHI_BufferHandle GetRHIHandle() const { return m_rhiHandle; }

	void* Lock(uint32_t offset, uint32_t size, uint32_t flags);
	void Unlock();

	void Bind(IRenderBackend& rhi, uint32_t stream = 0, uint32_t offset = 0) const;

  private:
	void DestroyRHI();

	RHI_BufferDesc m_desc;
	IRenderBackend* m_rhi = nullptr;
	RHI_BufferHandle m_rhiHandle{};
};
using ref_vertexbuffer = CSharedPtr<CVertexBuffer>;

////////////////////////////////////////////////////////////////////////////////

class XRRB_API CIndexBuffer : public CRefCountedResource
{
  public:
	CIndexBuffer() = default;
	~CIndexBuffer() override;

	CIndexBuffer(const CIndexBuffer&) = delete;
	CIndexBuffer& operator=(const CIndexBuffer&) = delete;

	bool Create(IRenderBackend& rhi, const RHI_BufferDesc& desc, const void* initialData = nullptr);

	const RHI_BufferDesc& GetDesc() const { return m_desc; }
	uint32_t GetSizeBytes() const { return m_desc.sizeBytes; }
	RHI_IndexFormat GetFormat() const { return m_desc.indexFormat; }
	uint32_t GetIndexCount() const { return m_desc.sizeBytes / RHI_IndexFormatSize(m_desc.indexFormat); }

	RHI_BufferHandle GetRHIHandle() const { return m_rhiHandle; }

	void* Lock(uint32_t offset, uint32_t size, uint32_t flags);
	void Unlock();

	void Bind(IRenderBackend& rhi) const;

  private:
	void DestroyRHI();

	RHI_BufferDesc m_desc;
	IRenderBackend* m_rhi = nullptr;
	RHI_BufferHandle m_rhiHandle{};
};
using ref_indexbuffer = CSharedPtr<CIndexBuffer>;
////////////////////////////////////////////////////////////////////////////////
