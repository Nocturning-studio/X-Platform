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
#include "SharedResource.h"
#include "Textures/Texture.h"
#include "Geometry/Geometry.h"
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CResourceManager
{
  public:
	CResourceManager() = default;
	~CResourceManager();

	void SetRHI(IRenderBackend* rhi) { m_rhi = rhi; }
	IRenderBackend* GetRHI() const { return m_rhi; }

	// --- Создание ---
	ref_texture CreateTexture(const RHI_TextureDesc& desc);
	ref_texture CreateRenderTarget(uint32_t w, uint32_t h, RHI_Format fmt, uint32_t mips = 1);
	ref_texture CreateDepthStencil(uint32_t w, uint32_t h, RHI_Format fmt = RHI_Format::D24_UNORM_S8_UINT);

	// --- Геометрия ---
	ref_vertexdecl CreateVertexDeclaration(const RHI_InputLayoutDesc& layout);
	ref_vertexbuffer CreateVertexBuffer(const RHI_BufferDesc& desc, const void* initialData = nullptr);
	ref_indexbuffer CreateIndexBuffer(const RHI_BufferDesc& desc, const void* initialData = nullptr);
	ref_geometry CreateGeometry();

	// --- PSO ---
	//
	// CreatePipelineState дедуплицирует по хэшу desc. Возвращает уже
	// существующий handle, если такой PSO уже создавался.
	//
	RHI_PipelineStateHandle CreatePipelineState(const RHI_PipelineStateDesc& desc);
	void DestroyPipelineState(RHI_PipelineStateHandle handle);

	// --- Кадровый цикл ---
	void OnFrameBegin();
	void OnFrameEnd();

	// --- Уничтожение всех ресурсов ---
	void DestroyAll();

	// --- Диагностика ---
	uint32_t GetTrackedCount() const { return (uint32_t)m_tracked.size(); }
	uint32_t GetPendingDeleteCount() const;
	uint32_t GetCachedPipelineCount() const { return (uint32_t)m_psoCache.size(); }

  private:
	void RegisterResource(CRefCountedResource* res);
	void CollectGarbage();

  private:
	IRenderBackend* m_rhi = nullptr;

	struct STrackedResource
	{
		CRefCountedResource* ptr = nullptr;
		uint32_t frameReleased = uint32_t(-1);
	};

	xr_vector<STrackedResource> m_tracked;

	std::unordered_map<RHI_PipelineStateDesc, RHI_PipelineStateHandle> m_psoCache;

	uint32_t m_frameIndex = 0;

	static constexpr uint32_t kDeferredFrameCount = 3;
};
////////////////////////////////////////////////////////////////////////////////
