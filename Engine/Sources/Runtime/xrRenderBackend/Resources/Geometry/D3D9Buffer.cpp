////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "D3D9Buffer.h"
#include "xrRenderBackendMacros.h"
////////////////////////////////////////////////////////////////////////////////

namespace
{
	void ApplyUsageFlags(u32 inUsage, DWORD& outD3DUsage, D3DPOOL& outPool)
	{
		outD3DUsage = 0;
		outPool = D3DPOOL_DEFAULT;

		if (inUsage & BufferUsage_Dynamic)
		{
			outD3DUsage |= D3DUSAGE_DYNAMIC;
			outD3DUsage |= D3DUSAGE_WRITEONLY;
		}
	}

DWORD LockFlags(u32 inFlags, u32 inUsage, bool wholeBuffer)
{
	DWORD f = 0;
	if(inFlags & BufferUsage_Dynamic)
		f |= D3DLOCK_DISCARD;
	if(wholeBuffer && (inUsage & BufferUsage_Dynamic))
		f |= D3DLOCK_DISCARD;
	if(inFlags & BufferUsage_CPUReadable)
		f |= D3DLOCK_READONLY;
	return f;
}
} // namespace

// ---------------------------------------------------------------------------
// CD3D9VertexBuffer
// ---------------------------------------------------------------------------

CD3D9VertexBuffer::~CD3D9VertexBuffer()
{
	RELEASE(m_vb);
}

HRESULT CD3D9VertexBuffer::Create(IDirect3DDevice9Ex* device, const CVertexBufferDesc& desc)
{
	if(!device || desc.sizeBytes == 0 || desc.stride == 0)
		return E_INVALIDARG;

	RELEASE(m_vb);
	m_desc = desc;

	DWORD usage = 0;
	D3DPOOL pool = D3DPOOL_MANAGED;
	ApplyUsageFlags(desc.usage, usage, pool);
	m_pool = pool;

	const HRESULT hr = device->CreateVertexBuffer(
		desc.sizeBytes, usage, 0, pool, &m_vb, nullptr);

	if(FAILED(hr))
	{
		Msg("! [D3D9VertexBuffer] CreateVertexBuffer failed (hr=0x%08X, size=%u, stride=%u) for '%s'", hr, 
																									   desc.sizeBytes, 
																									   desc.stride, 
																									   desc.debugName ? desc.debugName : "<unnamed>");
		return hr;
	}

	if(desc.debugName)
		m_vb->SetPrivateData(WKPDID_D3DDebugObjectName, desc.debugName, (DWORD)strlen(desc.debugName), 0);

	return S_OK;
}

void* CD3D9VertexBuffer::Lock(u32 offset, u32 size, u32 flags)
{
	if(!m_vb)
		return nullptr;

	const bool wholeBuffer = (offset == 0 && size == 0);
	DWORD d3dFlags = LockFlags(flags, m_desc.usage, wholeBuffer);

	void* p = nullptr;
	const HRESULT hr = m_vb->Lock(offset, size, &p, d3dFlags);
	if(FAILED(hr))
	{
		Msg("! [D3D9VertexBuffer] Lock failed (hr=0x%08X, offset=%u, size=%u)", hr, offset, size);
		return nullptr;
	}
	m_locked = true;
	return p;
}

void CD3D9VertexBuffer::Unlock()
{
	if(m_vb && m_locked)
	{
		m_vb->Unlock();
		m_locked = false;
	}
}

void CD3D9VertexBuffer::Bind(IDirect3DDevice9Ex* device, u32 stream, u32 offset) const
{
	if(!device)
		return;
	device->SetStreamSource(stream, m_vb, offset, m_desc.stride);
}

// ---------------------------------------------------------------------------
// CD3D9IndexBuffer
// ---------------------------------------------------------------------------

CD3D9IndexBuffer::~CD3D9IndexBuffer()
{
	RELEASE(m_ib);
}

HRESULT CD3D9IndexBuffer::Create(IDirect3DDevice9Ex* device, const CIndexBufferDesc& desc)
{
	if(!device || desc.sizeBytes == 0)
		return E_INVALIDARG;

	RELEASE(m_ib);
	m_desc = desc;

	DWORD usage = 0;
	D3DPOOL pool = D3DPOOL_MANAGED;
	ApplyUsageFlags(desc.usage, usage, pool);
	m_pool = pool;

	const D3DFORMAT fmt = (desc.format == EIndexFormat::UInt32) ? D3DFMT_INDEX32 : D3DFMT_INDEX16;

	const HRESULT hr = device->CreateIndexBuffer(
		desc.sizeBytes, usage, fmt, pool, &m_ib, nullptr);

	if(FAILED(hr))
	{
		Msg("! [D3D9IndexBuffer] CreateIndexBuffer failed (hr=0x%08X, size=%u) for '%s'", hr, 
																						  desc.sizeBytes, 
																						  desc.debugName ? desc.debugName : "<unnamed>");
		return hr;
	}

	if(desc.debugName)
		m_ib->SetPrivateData(WKPDID_D3DDebugObjectName, desc.debugName, (DWORD)strlen(desc.debugName), 0);

	return S_OK;
}

void* CD3D9IndexBuffer::Lock(u32 offset, u32 size, u32 flags)
{
	if(!m_ib)
		return nullptr;

	const bool wholeBuffer = (offset == 0 && size == 0);
	DWORD d3dFlags = LockFlags(flags, m_desc.usage, wholeBuffer);

	void* p = nullptr;
	const HRESULT hr = m_ib->Lock(offset, size, &p, d3dFlags);
	if(FAILED(hr))
	{
		Msg("! [D3D9IndexBuffer] Lock failed (hr=0x%08X, offset=%u, size=%u)", hr, offset, size);
		return nullptr;
	}
	m_locked = true;
	return p;
}

void CD3D9IndexBuffer::Unlock()
{
	if(m_ib && m_locked)
	{
		m_ib->Unlock();
		m_locked = false;
	}
}

void CD3D9IndexBuffer::Bind(IDirect3DDevice9Ex* device) const
{
	if(device)
		device->SetIndices(m_ib);
}
////////////////////////////////////////////////////////////////////////////////
