////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "ResourceManager.h"
#include "Textures/D3D9Texture.h"
#include "Geometry/D3D9Buffer.h"
#include "Geometry/D3D9VertexDeclaration.h"
////////////////////////////////////////////////////////////////////////////////

CResourceManager::~CResourceManager()
{
	for (auto& tr : m_tracked)
		xr_delete(tr.ptr);
	m_tracked.clear();
}

ref_texture CResourceManager::CreateTexture(const CTextureDesc& desc)
{
	if (!m_device)
	{
		Msg("! [ResourceManager] CreateTexture: device is null");
		return ref_texture();
	}

	auto* tex = xr_new<CD3D9Texture>();
	if (FAILED(tex->Create(m_device, desc)))
	{
		xr_delete(tex);
		return ref_texture();
	}

	RegisterResource(tex);

	return ref_texture(tex);
}

ref_texture CResourceManager::CreateRenderTarget(uint32_t w, uint32_t h, ETextureFormat fmt, uint32_t mips)
{
	return CreateTexture(CTextureDesc::RenderTarget(w, h, fmt, mips));
}

ref_texture CResourceManager::CreateDepthStencil(uint32_t w, uint32_t h, ETextureFormat fmt)
{
	return CreateTexture(CTextureDesc::DepthStencil(w, h, fmt));
}

ref_vertexdecl CResourceManager::CreateVertexDeclaration(const CVertexLayoutDesc& layout)
{
	if (!m_device) { Msg("! [ResourceManager] CreateVertexDeclaration: device is null"); return {}; }

	auto* d = xr_new<CD3D9VertexDeclaration>();
	if (FAILED(d->Create(m_device, layout))) { xr_delete(d); return {}; }

	RegisterResource(d);
	return ref_vertexdecl(d);
}

ref_vertexbuffer CResourceManager::CreateVertexBuffer(const CVertexBufferDesc& desc, const void* initialData)
{
	if (!m_device) 
	{ 
		R_ASSERT2(false, "! [ResourceManager] CreateVertexBuffer: device is null");
		return {}; 
	}

	auto* vb = xr_new<CD3D9VertexBuffer>();
	if (FAILED(vb->Create(m_device, desc))) 
	{
		R_ASSERT2(false, "! [ResourceManager] Failed to create vertex buffer");
		xr_delete(vb); return {};
	}

	if (initialData)
	{
		void* p = vb->Lock(0, 0, 0);
		if (p) 
		{ 
			memcpy(p, initialData, desc.sizeBytes); vb->Unlock(); 
		}
		else 
		{ 
			R_ASSERT2(false, "! [ResourceManager] CreateVertexBuffer: initial upload failed");
		}
	}

	RegisterResource(vb);
	return ref_vertexbuffer(vb);
}

ref_indexbuffer CResourceManager::CreateIndexBuffer(const CIndexBufferDesc& desc, const void* initialData)
{
	if (!m_device) { Msg("! [ResourceManager] CreateIndexBuffer: device is null"); return {}; }

	auto* ib = xr_new<CD3D9IndexBuffer>();
	if (FAILED(ib->Create(m_device, desc))) { xr_delete(ib); return {}; }

	if (initialData)
	{
		void* p = ib->Lock(0, 0, 0);
		if (p) { memcpy(p, initialData, desc.sizeBytes); ib->Unlock(); }
		else { Msg("! [ResourceManager] CreateIndexBuffer: initial upload failed"); }
	}

	RegisterResource(ib);
	return ref_indexbuffer(ib);
}

ref_geometry CResourceManager::CreateGeometry()
{
	// CGeometry — CPU-сайд хэндл, не владеет GPU-ресурсами напрямую,
	// поэтому в m_tracked его не кладём. Его буферы/декларация трекаются отдельно.
	return ref_geometry(xr_new<CGeometry>());
}

void CResourceManager::RegisterResource(CDeviceResource* res)
{
	if (!res) return;
	STrackedResource tr;
	tr.ptr = res;
	tr.frameReleased = u32(-1);
	m_tracked.push_back(tr);
}

void CResourceManager::OnFrameBegin()
{
}

void CResourceManager::OnFrameEnd()
{
	CollectGarbage();
	++m_frameIndex;
}

void CResourceManager::CollectGarbage()
{
	for (auto it = m_tracked.begin(); it != m_tracked.end(); )
	{
		STrackedResource& tr = *it;

		if (tr.ptr->RefCount() > 0)
		{
			tr.frameReleased = uint32_t(-1);
			++it;
			continue;
		}

		if (tr.frameReleased == uint32_t(-1))
		{
			tr.frameReleased = m_frameIndex;
			++it;
			continue;
		}

		const uint32_t elapsed = m_frameIndex - tr.frameReleased;
		if (elapsed >= kDeferredFrameCount)
		{
			xr_delete(tr.ptr);
			it = m_tracked.erase(it);
		}
		else
		{
			++it;
		}
	}
}

uint32_t CResourceManager::GetPendingDeleteCount() const
{
	uint32_t n = 0;
	for (const auto& tr : m_tracked)
		if (tr.ptr->RefCount() == 0)
			++n;
	return n;
}

void CResourceManager::OnDeviceLost()
{
	for (auto& tr : m_tracked)
		tr.ptr->OnDeviceLost();
}

void CResourceManager::OnDeviceReset(IDirect3DDevice9Ex* device)
{
	m_device = device;
	for (auto& tr : m_tracked)
		tr.ptr->OnDeviceReset(device);
}
////////////////////////////////////////////////////////////////////////////////
