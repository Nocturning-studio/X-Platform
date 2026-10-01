////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "ResourceManager.h"
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

ref_vertexdecl CResourceManager::CreateVertexDeclaration(const RHI_InputLayoutDesc& layout)
{
	if (!m_rhi) { Msg("! [ResourceManager] CreateVertexDeclaration: RHI is null"); return {}; }

	auto* d = xr_new<CVertexDeclaration>();
	if (!d->Create(*m_rhi, layout)) { xr_delete(d); return {}; }

	RegisterResource(d);
	return ref_vertexdecl(d);
}

ref_vertexbuffer CResourceManager::CreateVertexBuffer(const RHI_BufferDesc& desc, const void* initialData)
{
	if (!m_rhi)
	{
		R_ERROR("! [ResourceManager] CreateVertexBuffer: RHI is null");
		return {};
	}

	auto* vb = xr_new<CVertexBuffer>();
	if (!vb->Create(*m_rhi, desc, initialData))
	{
		R_ERROR("! [ResourceManager] Failed to create vertex buffer");
		xr_delete(vb); return {};
	}

	RegisterResource(vb);
	return ref_vertexbuffer(vb);
}

ref_indexbuffer CResourceManager::CreateIndexBuffer(const RHI_BufferDesc& desc, const void* initialData)
{
	if (!m_rhi) { Msg("! [ResourceManager] CreateIndexBuffer: RHI is null"); return {}; }

	auto* ib = xr_new<CIndexBuffer>();
	if (!ib->Create(*m_rhi, desc, initialData)) { xr_delete(ib); return {}; }

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
	for (auto it = m_tracked.rbegin(); it != m_tracked.rend(); )
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
			it = xr_vector<STrackedResource>::reverse_iterator(
				m_tracked.erase(std::next(it).base()));
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
