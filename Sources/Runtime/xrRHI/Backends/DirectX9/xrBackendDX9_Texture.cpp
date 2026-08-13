#include "pch.h"
#include "xrBackendDX9.h"

RHI_BEGIN

TextureHandle CRenderBackendDX9::AllocTextureHandle(DX9Texture* tex)
{
	u32 index;
	if (!m_FreeTextureIndices.empty())
	{
		index = m_FreeTextureIndices.top();
		m_FreeTextureIndices.pop();
		m_Textures[index] = tex;
	}
	else
	{
		index = static_cast<u32>(m_Textures.size());
		m_Textures.push_back(tex);
	}
	return TextureHandle{index};
}

DX9Texture* CRenderBackendDX9::GetTexture(TextureHandle handle)
{
	if (!handle.IsValid() || handle.id >= m_Textures.size())
		return nullptr;
	return m_Textures[handle.id];
}

void CRenderBackendDX9::FreeTextureHandle(TextureHandle handle)
{
	if (!handle.IsValid() || handle.id >= m_Textures.size())
		return;
	DX9Texture* tex = m_Textures[handle.id];
	if (!tex)
	{
		Print("! [DX9] Double free of TextureHandle(id=%u) detected, ignoring.", handle.id);
		return;
	}
	delete tex;
	m_Textures[handle.id] = nullptr;
	m_FreeTextureIndices.push(handle.id);
}

TextureHandle CRenderBackendDX9::CreateTexture(const TextureDesc& desc, const void* initialData)
{
	if (!m_pDevice)
		return TextureHandle{};

	if (desc.width == 0 || desc.height == 0)
	{
		Print("! [DX9] CreateTexture: invalid dimensions (%ux%u)", desc.width, desc.height);
		return TextureHandle{};
	}

	D3DFORMAT d3dFmt = RHIToD3DFormat(desc.format);
	if (d3dFmt == D3DFMT_UNKNOWN)
	{
		Print("! [DX9] CreateTexture: unsupported format %d", (int)desc.format);
		return TextureHandle{};
	}

	DWORD usage = 0;
	if (desc.isRenderTarget)
		usage |= D3DUSAGE_RENDERTARGET;
	if (desc.isDepthStencil)
		usage |= D3DUSAGE_DEPTHSTENCIL;

	DX9Texture* impl = new DX9Texture;
	impl->format = desc.format;
	impl->width = desc.width;
	impl->height = desc.height;
	impl->isRenderTarget = desc.isRenderTarget;
	impl->isDepthStencil = desc.isDepthStencil;

	HRESULT hr;

	if (desc.isCubeMap)
	{
		IDirect3DCubeTexture9* cubeTex = nullptr;
		hr =
			m_pDevice->CreateCubeTexture(desc.width, desc.mipLevels, usage, d3dFmt, D3DPOOL_DEFAULT, &cubeTex, nullptr);

		if (FAILED(hr))
		{
			delete impl;
			Print("! [DX9] CreateCubeTexture failed (0x%08x) for format %d", hr, (int)desc.format);
			return TextureHandle{};
		}
		impl->texCube = cubeTex;
	}
	else
	{
		IDirect3DTexture9* tex2D = nullptr;
		hr = m_pDevice->CreateTexture(desc.width, desc.height, desc.mipLevels, usage, d3dFmt, D3DPOOL_DEFAULT, &tex2D,
									  nullptr);

		if (FAILED(hr))
		{
			delete impl;
			Print("! [DX9] CreateTexture failed (0x%08x) for format %d", hr, (int)desc.format);
			return TextureHandle{};
		}
		impl->tex2D = tex2D;

		if (initialData && !desc.isRenderTarget && !desc.isDepthStencil)
		{
			size_t pixelSize = GetPixelSize(desc.format);
			if (pixelSize > 0)
			{
				IDirect3DTexture9* sysTex = nullptr;
				hr = m_pDevice->CreateTexture(desc.width, desc.height, 1, 0, d3dFmt, D3DPOOL_SYSTEMMEM, &sysTex,
											  nullptr);

				if (SUCCEEDED(hr))
				{
					D3DLOCKED_RECT locked;
					if (SUCCEEDED(sysTex->LockRect(0, &locked, nullptr, 0)))
					{
						const BYTE* src = static_cast<const BYTE*>(initialData);
						BYTE* dst = static_cast<BYTE*>(locked.pBits);
						size_t rowSize = desc.width * pixelSize;

						for (u32 y = 0; y < desc.height; ++y)
						{
							memcpy(dst, src, rowSize);
							src += rowSize;
							dst += locked.Pitch;
						}
						sysTex->UnlockRect(0);

						hr = m_pDevice->UpdateTexture(sysTex, tex2D);
						if (FAILED(hr))
							Print("! [DX9] UpdateTexture failed (0x%08x)", hr);
					}
					sysTex->Release();
				}
				else
				{
					Print("! [DX9] Failed to create system memory texture for initial data");
				}
			}
			else
			{
				Print("! [DX9] Unknown pixel size for format %d, cannot upload initial data", (int)desc.format);
			}
		}
	}

	return AllocTextureHandle(impl);
}

void CRenderBackendDX9::DestroyTexture(TextureHandle handle)
{
	DX9Texture* impl = GetTexture(handle);
	if (!impl)
	{
		Print("! [DX9] DestroyTexture: invalid or already destroyed handle (id=%u).", handle.id);
		return;
	}
	if (impl->tex2D)
		impl->tex2D->Release();
	if (impl->texCube)
		impl->texCube->Release();
	if (impl->surface)
		impl->surface->Release();
	FreeTextureHandle(handle);
}

void CRenderBackendDX9::SetTexture(u32 slot, TextureHandle texture, SamplerHandle sampler)
{
	if (!m_pDevice)
		return;

	if (sampler.IsValid())
	{
		DX9Sampler* samp = GetSampler(sampler);
		if (samp)
		{
			ApplySampler(slot, samp->desc);
		}
		else
		{
			ApplyDefaultSampler(slot);
		}
	}
	else
	{
		ApplyDefaultSampler(slot);
	}

	IDirect3DBaseTexture9* d3dTex = nullptr;
	if (texture.IsValid())
	{
		DX9Texture* tex = GetTexture(texture);
		if (tex)
		{
			d3dTex = tex->tex2D;
		}
	}
	m_pDevice->SetTexture(slot, d3dTex);
}

bool CRenderBackendDX9::CheckFormatSupport(RHI_Format fmt, bool isRenderTarget, bool isDepthStencil, bool isCube)
{
	if (!m_pD3D)
		return false;

	if (fmt == RHI_Format::NULLRT)
		return true;

	D3DFORMAT d3dFmt = RHIToD3DFormat(fmt);
	if (d3dFmt == D3DFMT_UNKNOWN)
	{
		Print("! [DX9] CheckFormatSupport: unknown RHI format %d", (int)fmt);
		return false;
	}

	DWORD usage = 0;
	if (isRenderTarget)
		usage |= D3DUSAGE_RENDERTARGET;
	if (isDepthStencil)
		usage |= D3DUSAGE_DEPTHSTENCIL;

	D3DRESOURCETYPE rtype = isCube ? D3DRTYPE_CUBETEXTURE : D3DRTYPE_TEXTURE;

	const D3DFORMAT adapterFmt = D3DFMT_X8R8G8B8;

	HRESULT hr = m_pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, adapterFmt, usage, rtype, d3dFmt);

	return SUCCEEDED(hr);
}

void* CRenderBackendDX9::GetTextureNativeHandle(TextureHandle handle)
{
	DX9Texture* tex = GetTexture(handle);
	if (!tex)
		return nullptr;
	return tex->tex2D ? (void*)tex->tex2D : (void*)tex->texCube;
}

bool CRenderBackendDX9::GetCubeMapFaceNative(TextureHandle handle, u32 face, u32 level, void** outSurface)
{
	if (!outSurface)
		return false;
	*outSurface = nullptr;
	DX9Texture* tex = GetTexture(handle);
	if (!tex || !tex->texCube)
		return false;
	IDirect3DSurface9* surf = nullptr;
	HRESULT hr = tex->texCube->GetCubeMapSurface((D3DCUBEMAP_FACES)face, level, &surf);
	if (FAILED(hr))
		return false;
	*outSurface = surf;
	return true;
}

RHI_END
