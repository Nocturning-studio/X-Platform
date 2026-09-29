////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "D3D9TextureFormat.h"
#include "D3D9Texture.h"
#include "xrRenderBackendMacros.h"
////////////////////////////////////////////////////////////////////////////////

CD3D9Texture::~CD3D9Texture()
{
	ReleaseSurfaces();
	RELEASE(m_texture);
}

uint32_t CD3D9Texture::FlatIndex(uint32_t mip, uint32_t face) const
{
	return mip * m_faces + face;
}

HRESULT CD3D9Texture::Create(IDirect3DDevice9Ex* device, const CTextureDesc& desc)
{
	if (!device) return E_FAIL;

	// На случай пересоздания (device reset) — чистим.
	ReleaseSurfaces();
	RELEASE(m_texture);

	m_desc = desc;
	m_faces = (desc.dim == ETextureDim::Cube) ? 6 : 1;

	DWORD  d3dUsage = 0;
	D3DPOOL pool = D3DPOOL_DEFAULT;

	if (desc.usage & TexUsage_RenderTarget) d3dUsage |= D3DUSAGE_RENDERTARGET;
	if (desc.usage & TexUsage_DepthStencil) d3dUsage |= D3DUSAGE_DEPTHSTENCIL;
	if (desc.usage & TexUsage_GenerateMips) d3dUsage |= D3DUSAGE_AUTOGENMIPMAP;

	// CPU-доступ: на DX9 это отдельный пул.
	if (desc.usage & (TexUsage_CPUReadable | TexUsage_CPUWritable))
	{
		pool = D3DPOOL_MANAGED;
		d3dUsage &= ~(D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL);
	}

	const D3DFORMAT fmt = ToD3DFormat(desc.format);
	if (fmt == D3DFMT_UNKNOWN)
	{
		Msg("! [D3D9Texture] Unknown format for '%s'",
			desc.debugName ? desc.debugName : "<unnamed>");
		return E_FAIL;
	}

	HRESULT hr = E_FAIL;

	switch (desc.dim)
	{
	case ETextureDim::Tex1D:
	case ETextureDim::Tex2D:
	{
		// D3D9 не имеет отдельного типа для 1D — это 2D с height = 1.
		IDirect3DTexture9* tex = nullptr;
		hr = device->CreateTexture(desc.width, desc.height, desc.mips, d3dUsage, fmt, pool, &tex, nullptr);
		m_texture = tex;
		break;
	}

	case ETextureDim::Cube:
	{
		IDirect3DCubeTexture9* tex = nullptr;
		hr = device->CreateCubeTexture(desc.width, desc.mips, d3dUsage, fmt, pool, &tex, nullptr);
		m_texture = tex;
		break;
	}

	case ETextureDim::Tex3D:
	{
		IDirect3DVolumeTexture9* tex = nullptr;
		hr = device->CreateVolumeTexture(desc.width, desc.height, desc.depth, desc.mips, d3dUsage, fmt, pool, &tex, nullptr);
		m_texture = tex;
		break;
	}
	}

	if (FAILED(hr))
	{
		Msg("! [D3D9Texture] Create failed (hr=0x%08X, %ux%u, fmt=%u) for '%s'", hr, 
																				 desc.width, 
																				 desc.height, 
																				 uint32_t(desc.format), 
																				 desc.debugName ? desc.debugName : "<unnamed>");
		return hr;
	}

	if (desc.debugName && m_texture)
	{
		// Сигнатура: SetPrivateData(REFGUID, const void*, DWORD, DWORD)
		m_texture->SetPrivateData(WKPDID_D3DDebugObjectName, desc.debugName, (DWORD)strlen(desc.debugName), 0);
	}

	m_surfaces.assign(desc.mips * m_faces, nullptr);
	return S_OK;
}

void CD3D9Texture::ReleaseSurfaces()
{
	for (auto*& s : m_surfaces)
	{
		RELEASE(s);
	}
	m_surfaces.clear();
}

IDirect3DSurface9* CD3D9Texture::GetD3D9Surface(uint32_t mip, uint32_t face) const
{
	if (!m_texture)         return nullptr;
	if (mip >= m_desc.mips) return nullptr;
	if (face >= m_faces)     return nullptr;

	const uint32_t idx = FlatIndex(mip, face);
	if (idx >= m_surfaces.size()) return nullptr;

	if (!m_surfaces[idx])
	{
		IDirect3DSurface9* s = nullptr;
		HRESULT hr = E_FAIL;

		switch (m_desc.dim)
		{
		case ETextureDim::Cube:
		{
			// GetCubeMapSurface есть только у IDirect3DCubeTexture9.
			auto* cube = static_cast<IDirect3DCubeTexture9*>(m_texture);
			hr = cube->GetCubeMapSurface(static_cast<D3DCUBEMAP_FACES>(face), mip, &s);
			break;
		}

		case ETextureDim::Tex1D:
		case ETextureDim::Tex2D:
		{
			// GetSurfaceLevel есть только у IDirect3DTexture9.
			auto* tex2d = static_cast<IDirect3DTexture9*>(m_texture);
			hr = tex2d->GetSurfaceLevel(mip, &s);
			break;
		}

		case ETextureDim::Tex3D:
			// У volume-текстуры в D3D9 нет surface-level, и её нельзя
			// использовать как render target.
			return nullptr;
		}

		if (FAILED(hr)) return nullptr;
		m_surfaces[idx] = s;
	}

	return m_surfaces[idx];
}

void CD3D9Texture::Bind(IDirect3DDevice9Ex* device, uint32_t slot) const
{
	if (!device) return;
	device->SetTexture(slot, m_texture);
}

CSurface CD3D9Texture::CreateRenderTargetView(uint32_t mip, uint32_t face) const
{
	IDirect3DSurface9* s = GetD3D9Surface(mip, face);
	if (!s) return CSurface();
	return CSurface::CreateD3D9(s);
}

CSurface CD3D9Texture::CreateDepthStencilView(uint32_t mip, uint32_t face) const
{
	return CreateRenderTargetView(mip, face);
}

void CD3D9Texture::OnDeviceLost()
{
	ReleaseSurfaces();
	RELEASE(m_texture);
}

HRESULT CD3D9Texture::OnDeviceReset(IDirect3DDevice9Ex* device)
{
	if (m_texture) return S_OK;
	return Create(device, m_desc);
}
////////////////////////////////////////////////////////////////////////////////
