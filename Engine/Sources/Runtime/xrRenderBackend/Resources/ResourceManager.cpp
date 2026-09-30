////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "ResourceManager.h"
#include "Geometry/D3D9Buffer.h"
#include "Geometry/D3D9VertexDeclaration.h"
////////////////////////////////////////////////////////////////////////////////

CResourceManager::~CResourceManager()
{
	DestroyAll();
}

void CResourceManager::DestroyAll()
{
	for (auto it = m_tracked.rbegin(); it != m_tracked.rend(); ++it)
	{
		if (it->ptr)
			xr_delete(it->ptr);
	}
	m_tracked.clear();
}

ref_texture CResourceManager::CreateTexture(const RHI_TextureDesc& desc)
{
	if (!m_rhi)
	{
		Msg("! [ResourceManager] CreateTexture: RHI is null");
		return ref_texture();
	}

	auto* tex = xr_new<CTexture>();
	if (!tex->Create(*m_rhi, desc))
	{
		xr_delete(tex);
		return ref_texture();
	}

	RegisterResource(tex);
	return ref_texture(tex);
}

ref_texture CResourceManager::CreateRenderTarget(uint32_t w, uint32_t h, RHI_Format fmt, uint32_t mips)
{
	return CreateTexture(RHI_TextureDesc::RenderTarget(w, h, fmt, mips));
}

ref_texture CResourceManager::CreateDepthStencil(uint32_t w, uint32_t h, RHI_Format fmt)
{
	return CreateTexture(RHI_TextureDesc::DepthStencil(w, h, fmt));
}

ref_vertexdecl CResourceManager::CreateVertexDeclaration(const CVertexLayoutDesc& layout)
{
	DX_DEPRECATED

	if (!m_rhi) { Msg("! [ResourceManager] CreateVertexDeclaration: RHI is null"); return {}; }

	auto* d = xr_new<CD3D9VertexDeclaration>();
	IDirect3DDevice9Ex* device = static_cast<IDirect3DDevice9Ex*>(m_rhi->GetDeviceHandle());
	if (FAILED(d->Create(device, layout))) { xr_delete(d); return {}; }

	RegisterResource(d);
	return ref_vertexdecl(d);
}

ref_vertexbuffer CResourceManager::CreateVertexBuffer(const CVertexBufferDesc& desc, const void* initialData)
{
	DX_DEPRECATED

	if (!m_rhi)
	{
		R_ASSERT2(false, "! [ResourceManager] CreateVertexBuffer: RHI is null");
		return {};
	}

	auto* vb = xr_new<CD3D9VertexBuffer>();
	IDirect3DDevice9Ex* device = static_cast<IDirect3DDevice9Ex*>(m_rhi->GetDeviceHandle());
	if (FAILED(vb->Create(device, desc)))
	{
		R_ASSERT2(false, "! [ResourceManager] Failed to create vertex buffer");
		xr_delete(vb); return {};
	}

	if (initialData)
	{
		void* p = vb->Lock(0, 0, 0);
		if (p) { memcpy(p, initialData, desc.sizeBytes); vb->Unlock(); }
		else { R_ASSERT2(false, "! [ResourceManager] CreateVertexBuffer: initial upload failed"); }
	}

	RegisterResource(vb);
	return ref_vertexbuffer(vb);
}

ref_indexbuffer CResourceManager::CreateIndexBuffer(const CIndexBufferDesc& desc, const void* initialData)
{
	DX_DEPRECATED

	if (!m_rhi) { Msg("! [ResourceManager] CreateIndexBuffer: RHI is null"); return {}; }

	auto* ib = xr_new<CD3D9IndexBuffer>();
	IDirect3DDevice9Ex* device = static_cast<IDirect3DDevice9Ex*>(m_rhi->GetDeviceHandle());
	if (FAILED(ib->Create(device, desc))) { xr_delete(ib); return {}; }

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
	auto* g = xr_new<CGeometry>();
	RegisterResource(g);
	return ref_geometry(g);
}

void CResourceManager::RegisterResource(CSharedResource* res)
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
////////////////////////////////////////////////////////////////////////////////
