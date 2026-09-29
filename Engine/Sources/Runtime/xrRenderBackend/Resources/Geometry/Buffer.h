////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrRenderBackend/Resources/DeviceResource.h>
#include "BufferDesc.h"
////////////////////////////////////////////////////////////////////////////////

class XRRB_API CVertexBuffer : public CDeviceResource
{
  public:
	virtual ~CVertexBuffer() = default;

	const CVertexBufferDesc& GetDesc() const { return m_desc; }
	uint32_t GetSizeBytes() const { return m_desc.sizeBytes; }
	uint32_t GetStride() const { return m_desc.stride; }

	virtual void* Lock(uint32_t offset, uint32_t size, uint32_t flags) = 0;
	virtual void Unlock() = 0;

	// Привязать к потоку контекста рендеринга.
	virtual void Bind(IDirect3DDevice9Ex* device, uint32_t stream = 0, uint32_t offset = 0) const = 0;

  protected:
	CVertexBufferDesc m_desc;
};
using ref_vertexbuffer = CSharedPtr<CVertexBuffer>;

////////////////////////////////////////////////////////////////////////////////

class XRRB_API CIndexBuffer : public CDeviceResource
{
  public:
	virtual ~CIndexBuffer() = default;

	const CIndexBufferDesc& GetDesc() const { return m_desc; }
	uint32_t GetSizeBytes() const { return m_desc.sizeBytes; }
	EIndexFormat GetFormat() const { return m_desc.format; }
	uint32_t GetIndexCount() const { return m_desc.sizeBytes / IndexFormatSize(m_desc.format); }

	virtual void* Lock(uint32_t offset, uint32_t size, uint32_t flags) = 0;
	virtual void Unlock() = 0;

	virtual void Bind(IDirect3DDevice9Ex* device) const = 0;

  protected:
	CIndexBufferDesc m_desc;
};
using ref_indexbuffer = CSharedPtr<CIndexBuffer>;
////////////////////////////////////////////////////////////////////////////////
