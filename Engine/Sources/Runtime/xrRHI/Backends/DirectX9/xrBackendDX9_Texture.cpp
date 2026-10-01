////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "xrBackendDX9.h"
#include <d3dcommon.h>
////////////////////////////////////////////////////////////////////////////////
RHI_TextureHandle CRenderBackendDX9::AllocRHI_TextureHandle(DX9Texture* tex)
{
	uint32_t index;
	if(!m_FreeTextureIndices.empty())
	{
		index = m_FreeTextureIndices.top();
		m_FreeTextureIndices.pop();
		m_Textures[index] = tex;
	}
	else
	{
		index = static_cast<uint32_t>(m_Textures.size());
		m_Textures.push_back(tex);
	}
	return RHI_TextureHandle{index};
}

DX9Texture* CRenderBackendDX9::GetTexture(RHI_TextureHandle handle)
{
	if(!handle.IsValid() || handle.id >= m_Textures.size())
		return nullptr;
	return m_Textures[handle.id];
}

void CRenderBackendDX9::FreeRHI_TextureHandle(RHI_TextureHandle handle)
{
	if(!handle.IsValid() || handle.id >= m_Textures.size())
		return;
	DX9Texture* tex = m_Textures[handle.id];
	if(!tex)
	{
		Msg("! [DX9] Double free of RHI_TextureHandle(id=%u) detected, ignoring.", handle.id);
		return;
	}
	delete tex;
	m_Textures[handle.id] = nullptr;
	m_FreeTextureIndices.push(handle.id);
}

RHI_TextureHandle CRenderBackendDX9::CreateTexture(const RHI_TextureDesc& desc, const void* initialData)
{
	if (!m_pDevice) return {};

	if (desc.width == 0 || desc.height == 0)
	{
		Msg("! [DX9] CreateTexture: invalid dimensions (%ux%u)", desc.width, desc.height);
		return {};
	}

	if (desc.sampleCount > 1)
	{
		Msg("! [DX9] CreateTexture: MSAA textures not supported (sampleCount=%u)", desc.sampleCount);
		return {};
	}

	if (desc.arraySize > 1)
	{
		Msg("! [DX9] CreateTexture: texture arrays not supported (arraySize=%u)", desc.arraySize);
		return {};
	}

	const D3DFORMAT d3dFmt = RHIToD3DFormat(desc.format);
	if (d3dFmt == D3DFMT_UNKNOWN)
	{
		Msg("! [DX9] CreateTexture: unsupported format %u", (uint32_t)desc.format);
		return {};
	}

	DWORD usage = 0;
	D3DPOOL pool = D3DPOOL_DEFAULT;

	if (desc.usage & RHI_TexUsage_RenderTarget) usage |= D3DUSAGE_RENDERTARGET;
	if (desc.usage & RHI_TexUsage_DepthStencil) usage |= D3DUSAGE_DEPTHSTENCIL;
	if (desc.usage & RHI_TexUsage_GenerateMips) usage |= D3DUSAGE_AUTOGENMIPMAP;

	if (desc.usage & (RHI_TexUsage_CPUReadable | RHI_TexUsage_CPUWritable))
		R_ERRORF("! [DX9] CreateTexture: CPUReadable/CPUWritable flags are ignored in D3D9Ex " 
				 "(texture '%s'). Use staging workflow instead.", desc.debugName ? desc.debugName : "<unnamed>");

	// mipLevels == 0 => полная цепочка
	UINT mips = (desc.mipLevels == 0) ? 0 : desc.mipLevels;

	DX9Texture* impl = new DX9Texture;
	impl->format = desc.format;
	impl->dim = desc.dim;
	impl->width = desc.width;
	impl->height = desc.height;
	impl->depth = desc.depth;
	impl->mipLevels = desc.mipLevels;
	impl->arraySize = desc.arraySize;
	impl->sampleCount = desc.sampleCount;
	impl->usage = desc.usage;
	impl->isRenderTarget = (desc.usage & RHI_TexUsage_RenderTarget) != 0;
	impl->isDepthStencil = (desc.usage & RHI_TexUsage_DepthStencil) != 0;

	HRESULT hr = E_FAIL;
	IDirect3DBaseTexture9* base = nullptr;

	switch (desc.dim)
	{
	case RHI_TextureDim::Tex1D:
	case RHI_TextureDim::Tex2D:
	{
		IDirect3DTexture9* t = nullptr;
		hr = m_pDevice->CreateTexture(desc.width, desc.height, mips, usage, d3dFmt, pool, &t, nullptr);
		if (SUCCEEDED(hr)) { impl->tex2D = t; base = t; }
		break;
	}
	case RHI_TextureDim::Cube:
	{
		IDirect3DCubeTexture9* t = nullptr;
		hr = m_pDevice->CreateCubeTexture(desc.width, mips, usage, d3dFmt, pool, &t, nullptr);
		if (SUCCEEDED(hr)) { impl->texCube = t; base = t; }
		break;
	}
	case RHI_TextureDim::Tex3D:
	{
		IDirect3DVolumeTexture9* t = nullptr;
		hr = m_pDevice->CreateVolumeTexture(desc.width, desc.height, desc.depth, mips,
			usage, d3dFmt, pool, &t, nullptr);
		if (SUCCEEDED(hr)) { impl->tex3D = t; base = t; }
		break;
	}
	}

	if (FAILED(hr) || !base)
	{
		Msg("! [DX9] CreateTexture failed (hr=0x%08x, %ux%u dim=%u fmt=%u)", hr, desc.width, desc.height, (uint32_t)desc.dim, (uint32_t)desc.format);
		delete impl;
		return {};
	}

	// Debug name — D3D9 поддерживает D3DDebugObjectName через SetPrivateData.
	if (desc.debugName && desc.debugName[0])
	{
		base->SetPrivateData(WKPDID_D3DDebugObjectName, desc.debugName, (DWORD)strlen(desc.debugName), 0);
	}

	// --- Initial data upload (только для обычных 2D-текстур без RT/DS) ---
	if (initialData
		&& desc.dim == RHI_TextureDim::Tex2D
		&& !impl->isRenderTarget
		&& !impl->isDepthStencil
		&& pool == D3DPOOL_DEFAULT)
	{
		const size_t pixelSize = GetPixelSize(desc.format);
		if (pixelSize > 0)
		{
			IDirect3DTexture9* sysTex = nullptr;
			hr = m_pDevice->CreateTexture(desc.width, desc.height, desc.mipLevels,
				0, d3dFmt, D3DPOOL_SYSTEMMEM, &sysTex, nullptr);
			if (SUCCEEDED(hr))
			{
				D3DLOCKED_RECT locked{};
				if (SUCCEEDED(sysTex->LockRect(0, &locked, nullptr, 0)))
				{
					const uint8_t* src = static_cast<const uint8_t*>(initialData);
					uint8_t* dst = static_cast<uint8_t*>(locked.pBits);
					const size_t rowSize = size_t(desc.width) * pixelSize;

					for (uint32_t y = 0; y < desc.height; ++y)
					{
						memcpy(dst, src, rowSize);
						src += rowSize;
						dst += locked.Pitch;
					}
					sysTex->UnlockRect(0);
					hr = m_pDevice->UpdateTexture(sysTex, impl->tex2D);
					if (FAILED(hr))
						Msg("! [DX9] UpdateTexture failed (0x%08x)", hr);
				}
				sysTex->Release();
			}
		}
		else
		{
			Msg("! [DX9] CreateTexture: cannot upload initial data — unknown pixel size for fmt %u", (uint32_t)desc.format);
		}
	}

	return AllocRHI_TextureHandle(impl);
}

void CRenderBackendDX9::DestroyTexture(RHI_TextureHandle handle)
{
	DX9Texture* impl = GetTexture(handle);
	if (!impl)
	{
		Msg("! [DX9] DestroyTexture: invalid handle (id=%u)", handle.id);
		return;
	}
	if (impl->tex2D)   impl->tex2D->Release();
	if (impl->texCube) impl->texCube->Release();
	if (impl->tex3D)   impl->tex3D->Release();
	if (impl->surface) impl->surface->Release();
	FreeRHI_TextureHandle(handle);
}

bool CRenderBackendDX9::CheckFormatSupport(RHI_Format fmt, bool isRenderTarget, bool isDepthStencil, bool isCube)
{
	if(!m_pD3D)
		return false;

	if(fmt == RHI_Format::NULLRT)
		return true;

	D3DFORMAT d3dFmt = RHIToD3DFormat(fmt);
	if(d3dFmt == D3DFMT_UNKNOWN)
	{
		Msg("! [DX9] CheckFormatSupport: unknown RHI format %d", (int)fmt);
		return false;
	}

	DWORD usage = 0;
	if(isRenderTarget)
		usage |= D3DUSAGE_RENDERTARGET;
	if(isDepthStencil)
		usage |= D3DUSAGE_DEPTHSTENCIL;

	D3DRESOURCETYPE rtype = isCube ? D3DRTYPE_CUBETEXTURE : D3DRTYPE_TEXTURE;

	const D3DFORMAT adapterFmt = D3DFMT_X8R8G8B8;

	HRESULT hr = m_pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, adapterFmt, usage, rtype, d3dFmt);

	return SUCCEEDED(hr);
}

void* CRenderBackendDX9::GetTextureNativeHandle(RHI_TextureHandle handle)
{
	DX9Texture* tex = GetTexture(handle);
	if(!tex)
		return nullptr;
	return tex->tex2D ? (void*)tex->tex2D : (void*)tex->texCube;
}

bool CRenderBackendDX9::GetCubeMapFaceNative(RHI_TextureHandle handle, uint32_t face, uint32_t level, void** outSurface)
{
	if(!outSurface)
		return false;
	*outSurface = nullptr;
	DX9Texture* tex = GetTexture(handle);
	if(!tex || !tex->texCube)
		return false;
	IDirect3DSurface9* surf = nullptr;
	HRESULT hr = tex->texCube->GetCubeMapSurface((D3DCUBEMAP_FACES)face, level, &surf);
	if(FAILED(hr))
		return false;
	*outSurface = surf;
	return true;
}

void CRenderBackendDX9::SetShaderResource(uint32_t slot, RHI_TextureHandle tex)
{
	if (!m_pDevice) return;

	const DWORD stage = (slot < 16) ? slot : (D3DVERTEXTEXTURESAMPLER0 + (slot - 16));

	IDirect3DBaseTexture9* native = nullptr;
	if (tex.IsValid())
	{
		DX9Texture* t = GetTexture(tex);
		if (t) native = t->GetBase();
	}
	m_pDevice->SetTexture(stage, native);
}
////////////////////////////////////////////////////////////////////////////////
