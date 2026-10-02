////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
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

	RHI_BufferDesc   m_desc;
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

	RHI_BufferDesc   m_desc;
	IRenderBackend* m_rhi = nullptr;
	RHI_BufferHandle m_rhiHandle{};
};
using ref_indexbuffer = CSharedPtr<CIndexBuffer>;
////////////////////////////////////////////////////////////////////////////////
