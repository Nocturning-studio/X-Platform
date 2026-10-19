////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-RHI
////////////////////////////////////////////////////////////////////////////////
#include <source/pch.h>
#include "BackendDX9.h"
////////////////////////////////////////////////////////////////////////////////
namespace
{
	// "Обычный" depth-формат, для которого имеет смысл искать sampleable-аналог.
	inline bool IsStandardDepthFormat(RHI_Format f) noexcept
	{
		return f == RHI_Format::D24_UNORM_S8_UINT ||
			   f == RHI_Format::D16_UNORM;
	}

	// INTZ всегда 24-битный, DF24 - 24-битный, DF16 - 16-битный.
	inline bool IsShadowDepthD3DFormat(D3DFORMAT f) noexcept
	{
		return f == static_cast<D3DFORMAT>(MAKEFOURCC('I', 'N', 'T', 'Z')) ||
			   f == static_cast<D3DFORMAT>(MAKEFOURCC('D', 'F', '2', '4')) ||
			   f == static_cast<D3DFORMAT>(MAKEFOURCC('D', 'F', '1', '6'));
	}
} // namespace
////////////////////////////////////////////////////////////////////////////////
RHI_TextureHandle CRenderBackendDX9::CreateTexture(const RHI_TextureDesc& desc, const void* initialData)
{
	if(!m_pDevice)
		return {};

	if(desc.width == 0 || desc.height == 0)
	{
		Print("! [DX9] CreateTexture: invalid dimensions (%ux%u)", desc.width, desc.height);
		return {};
	}

	if(desc.sampleCount > 1)
	{
		Print("! [DX9] CreateTexture: MSAA textures not supported (sampleCount=%u)", desc.sampleCount);
		return {};
	}

	if(desc.arraySize > 1)
	{
		Print("! [DX9] CreateTexture: texture arrays not supported (arraySize=%u)", desc.arraySize);
		return {};
	}

	D3DFORMAT d3dFmt = RHIToD3DFormat(desc.format);
	if(d3dFmt == D3DFMT_UNKNOWN)
	{
		Print("! [DX9] CreateTexture: unsupported format %u", (uint32_t)desc.format);
		return {};
	}

	const bool wantsSampleableDepth = (desc.usage & RHI_TexUsage_DepthStencil) && 
									  (desc.usage & RHI_TexUsage_ShaderResource) && 
									  IsStandardDepthFormat(desc.format);

	bool usingShadowFormat = false;
	if (wantsSampleableDepth)
	{
		const D3DFORMAT shadow = SelectSampleableDepthFormat(desc.format);
		if (shadow != D3DFMT_UNKNOWN)
		{
			d3dFmt = shadow;
			usingShadowFormat = true;
		}
		else
		{
			Print("! [DX9] CreateTexture: sampleable depth requested, "
				"but no INTZ/DF24/DF16 support on this device "
				"(texture '%s'). Depth reads in shaders will not work.",
				desc.debugName ? desc.debugName : "<unnamed>");
		}
	}

	DWORD usage = 0;
	D3DPOOL pool = D3DPOOL_DEFAULT;

	if(desc.usage & RHI_TexUsage_RenderTarget)
		usage |= D3DUSAGE_RENDERTARGET;
	if(desc.usage & RHI_TexUsage_DepthStencil)
		usage |= D3DUSAGE_DEPTHSTENCIL;
	if(desc.usage & RHI_TexUsage_GenerateMips)
		usage |= D3DUSAGE_AUTOGENMIPMAP;

	if(desc.usage & (RHI_TexUsage_CPUReadable | RHI_TexUsage_CPUWritable))
		Print("! [DX9] CreateTexture: CPUReadable/CPUWritable flags are ignored in D3D9Ex "
				 "(texture '%s'). Use staging workflow instead.",
				 desc.debugName ? desc.debugName : "<unnamed>");

	// mipLevels == 0 => полная цепочка
	UINT mips = (desc.mipLevels == 0) ? 0 : desc.mipLevels;
	// Sampleable depth почти всегда mip=1 - INTZ не любит мипы
	if (usingShadowFormat && desc.mipLevels != 1)
	{
		Print("! [DX9] CreateTexture: INTZ/DFx with mipLevels=%u may not be supported " "by driver; forcing mipLevels=1", desc.mipLevels);
		mips = 1;
	}

	if (desc.usage & RHI_TexUsage_GenerateMips)
	{
		usage |= D3DUSAGE_AUTOGENMIPMAP;

		if (mips == 1)
		{
			Print("! [DX9] CreateTexture: GenerateMips + mipLevels=1 is invalid, "
				  "forcing full chain (texture '%s')",
				  desc.debugName ? desc.debugName : "<unnamed>");
			mips = 0;
		}
	}

	DX9Texture* impl = new DX9Texture;
	impl->format = desc.format;
	impl->dim = desc.dim;
	impl->width = desc.width;
	impl->height = desc.height;
	impl->depth = desc.depth;
	impl->mipLevels = mips;
	impl->arraySize = desc.arraySize;
	impl->sampleCount = desc.sampleCount;
	impl->usage = desc.usage;
	impl->isRenderTarget = (desc.usage & RHI_TexUsage_RenderTarget) != 0;
	impl->isDepthStencil = (desc.usage & RHI_TexUsage_DepthStencil) != 0;
	impl->isSampleableDepth = usingShadowFormat;
	impl->d3dFormat = d3dFmt;

	HRESULT hr = E_FAIL;
	IDirect3DBaseTexture9* base = nullptr;

	switch(desc.dim)
	{
	case RHI_TextureDim::Tex1D:
	case RHI_TextureDim::Tex2D:
	{
		IDirect3DTexture9* t = nullptr;
		hr = m_pDevice->CreateTexture(desc.width, desc.height, mips, usage, d3dFmt, pool, &t, nullptr);
		if(SUCCEEDED(hr))
		{
			impl->tex2D = t;
			base = t;
		}
		break;
	}
	case RHI_TextureDim::Cube:
	{
		IDirect3DCubeTexture9* t = nullptr;
		hr = m_pDevice->CreateCubeTexture(desc.width, mips, usage, d3dFmt, pool, &t, nullptr);
		if(SUCCEEDED(hr))
		{
			impl->texCube = t;
			base = t;
		}
		break;
	}
	case RHI_TextureDim::Tex3D:
	{
		IDirect3DVolumeTexture9* t = nullptr;
		hr = m_pDevice->CreateVolumeTexture(desc.width, desc.height, desc.depth, mips, usage, d3dFmt, pool, &t, nullptr);
		if(SUCCEEDED(hr))
		{
			impl->tex3D = t;
			base = t;
		}
		break;
	}
	}

	if(FAILED(hr) || !base)
	{
		Print("! [DX9] CreateTexture failed (hr=0x%08x, %ux%u dim=%u fmt=%u)", hr, desc.width, desc.height, (uint32_t)desc.dim, (uint32_t)desc.format);
		delete impl;
		return {};
	}

	// Debug name — D3D9 поддерживает D3DDebugObjectName через SetPrivateData.
	if(desc.debugName && desc.debugName[0])
		base->SetPrivateData(WKPDID_D3DDebugObjectName, desc.debugName, (DWORD)strlen(desc.debugName), 0);

	// --- Initial data upload (только для обычных 2D-текстур без RT/DS) ---
	if (initialData && desc.dim == RHI_TextureDim::Tex2D && !impl->isRenderTarget && !impl->isDepthStencil)
	{
		const size_t pixelSize = GetPixelSize(desc.format);
		if (pixelSize == 0)
		{
			Print("! [DX9] CreateTexture: cannot upload initial data — "
				"unknown pixel size for fmt %u", (uint32_t)desc.format);
		}
		else
		{
			// Реальное число mip-уровней у destination. desc.mipLevels может
			// быть 0 (full chain) — тогда фактическое значение известно только
			// после CreateTexture.
			const UINT destMips = impl->tex2D->GetLevelCount();

			// initialData содержит только mip 0. Если у текстуры есть ещё
			// мипы и НЕТ AUTOGENMIPMAP, заполнить их корректно нечем —
			// предупреждаем, но mip 0 всё равно загружаем.
			const bool hasAutoGen = (usage & D3DUSAGE_AUTOGENMIPMAP) != 0;
			if (destMips > 1 && !hasAutoGen)
			{
				Print("! [DX9] CreateTexture: initialData is mip-0 only, "
					"but texture has %u mips without AUTOGENMIPMAP. "
					"Mips 1+ will be undefined.", destMips);
			}

			IDirect3DTexture9* sysTex = nullptr;
			const HRESULT stageHr = m_pDevice->CreateTexture(desc.width, 
															 desc.height, 
															 destMips,
															 0,	// usage: staging
															 d3dFmt, 
															 D3DPOOL_SYSTEMMEM,
															 &sysTex, 
															 nullptr);

			if (FAILED(stageHr) || !sysTex)
			{
				Print("! [DX9] CreateTexture: staging texture creation failed (0x%08x)", stageHr);
			}
			else
			{
				D3DLOCKED_RECT locked{};
				if (FAILED(sysTex->LockRect(0, &locked, nullptr, 0)))
				{
					Print("! [DX9] CreateTexture: staging LockRect(0) failed");
				}
				else
				{
					const uint8_t* src = static_cast<const uint8_t*>(initialData);
					uint8_t* dst = static_cast<uint8_t*>(locked.pBits);
					const size_t   rowSize = size_t(desc.width) * pixelSize;

					for (uint32_t y = 0; y < desc.height; ++y)
					{
						memcpy(dst, src, rowSize);
						src += rowSize;
						dst += locked.Pitch;
					}
					sysTex->UnlockRect(0);

					const HRESULT upHr = m_pDevice->UpdateTexture(sysTex, impl->tex2D);
					if (FAILED(upHr))
						Print("! [DX9] CreateTexture: UpdateTexture failed (0x%08x)", upHr);
				}

				sysTex->Release();
			}
		}
	}

	return m_textures.Allocate(impl);
}

void CRenderBackendDX9::DestroyTexture(RHI_TextureHandle handle)
{
	DX9Texture* impl = m_textures.Release(handle);
	if (!impl)
	{
		if (handle.IsValid() && handle.id < m_textures.Data().size())
			Print("! [DX9] DestroyTexture: double free of RHI_TextureHandle(id=%u), ignoring", handle.id);
		return;
	}

	if (impl->tex2D)
		impl->tex2D->Release();
	if (impl->texCube)
		impl->texCube->Release();
	if (impl->tex3D)
		impl->tex3D->Release();
	delete impl;
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
		Print("! [DX9] CheckFormatSupport: unknown RHI format %d", (int)fmt);
		return false;
	}

	const bool isShadowFmt = d3dFmt == static_cast<D3DFORMAT>(MAKEFOURCC('I', 'N', 'T', 'Z')) ||
							 d3dFmt == static_cast<D3DFORMAT>(MAKEFOURCC('D', 'F', '2', '4')) ||
							 d3dFmt == static_cast<D3DFORMAT>(MAKEFOURCC('D', 'F', '1', '6'));

	D3DRESOURCETYPE rtype = (isCube && !isShadowFmt) ? D3DRTYPE_CUBETEXTURE : D3DRTYPE_TEXTURE;

	DWORD usage = 0;
	if(isRenderTarget)
		usage |= D3DUSAGE_RENDERTARGET;
	if(isDepthStencil || isShadowFmt)
		usage |= D3DUSAGE_DEPTHSTENCIL;

	const D3DFORMAT adapterFmt = D3DFMT_X8R8G8B8;

	HRESULT hr = m_pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, adapterFmt, usage, rtype, d3dFmt);

	return SUCCEEDED(hr);
}

bool CRenderBackendDX9::IsShadowDepthFormatSupported(D3DFORMAT fmt) const
{
	if (!m_pD3D)
		return false;

	// INTZ/DF24/DF16 - это текстуры, которые одновременно являются
	// depth-stencil поверхностями. Правильная проверка - именно
	// D3DUSAGE_DEPTHSTENCIL + D3DRTYPE_TEXTURE.
	return SUCCEEDED(m_pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT,
											   D3DDEVTYPE_HAL,
											   D3DFMT_X8R8G8B8,
											   D3DUSAGE_DEPTHSTENCIL,
											   D3DRTYPE_TEXTURE,
											   fmt));
}

D3DFORMAT CRenderBackendDX9::SelectSampleableDepthFormat(RHI_Format rhiFmt) const
{
	// INTZ - наиболее распространённый (NV + большинство AMD).
	const D3DFORMAT intz = static_cast<D3DFORMAT>(MAKEFOURCC('I', 'N', 'T', 'Z'));
	if (IsShadowDepthFormatSupported(intz))
		return intz;

	// DF24 - AMD-специфичный эквивалент.
	const D3DFORMAT df24 = static_cast<D3DFORMAT>(MAKEFOURCC('D', 'F', '2', '4'));
	if (IsShadowDepthFormatSupported(df24))
		return df24;

	// DF16 - 16-битный. Разумно использовать только если запрошен D16.
	if (rhiFmt == RHI_Format::D16_UNORM)
	{
		const D3DFORMAT df16 = static_cast<D3DFORMAT>(MAKEFOURCC('D', 'F', '1', '6'));
		if (IsShadowDepthFormatSupported(df16))
			return df16;
	}

	return D3DFMT_UNKNOWN;
}

void* CRenderBackendDX9::GetTextureNativeHandle(RHI_TextureHandle handle)
{
	DX9Texture* tex = GetTexture(handle);
	if(!tex)
		return nullptr;
	if (tex->tex2D)   return tex->tex2D;
	if (tex->texCube) return tex->texCube;
	if (tex->tex3D)   return tex->tex3D;
	return nullptr;
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
////////////////////////////////////////////////////////////////////////////////
