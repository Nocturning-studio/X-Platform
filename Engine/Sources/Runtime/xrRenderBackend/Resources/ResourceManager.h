////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
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

	// --- Кадровый цикл ---
	void OnFrameBegin();
	void OnFrameEnd();

	// --- Уничтожение всех ресурсов ---
	void DestroyAll();

	// --- Диагностика ---
	uint32_t GetTrackedCount() const { return (uint32_t)m_tracked.size(); }
	uint32_t GetPendingDeleteCount() const;

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

	uint32_t m_frameIndex = 0;

	// Сколько кадров держать «мёртвый» ресурс, прежде чем удалить.
	// Нужно, чтобы команды кадра, уже отправленные в GPU, успели выполниться.
	static constexpr uint32_t kDeferredFrameCount = 3;
};
////////////////////////////////////////////////////////////////////////////////
