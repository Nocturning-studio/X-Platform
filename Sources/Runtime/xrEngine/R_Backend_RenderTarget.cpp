#include "stdafx.h"
#pragma hdrstop

#include "R_Backend_RenderTarget.h"
#include "ResourceManager.h"
#include "../xrRHI/xrRHI_Types.h"

using namespace xrRHI;

// Хелпер для определения Usage
static bool IsDepthStencilFormat(RHI_Format fmt)
{
	switch (fmt)
	{
	case RHI_Format::D16_UNORM:
	case RHI_Format::D15S1:
	case RHI_Format::D24_UNORM_S8_UINT:
	case RHI_Format::D24X8:
	case RHI_Format::D32_LOCKABLE:
	case RHI_Format::D32_FLOAT:
	case RHI_Format::D24S8_Shadow:
	case RHI_Format::D16_Shadow:
	case RHI_Format::D24X4S4:
		return true;
	default:
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////
// CRT Implementation
//////////////////////////////////////////////////////////////////////////

CRT::CRT()
{
	pSurface = NULL;
	pRT = NULL;
	dwWidth = 0;
	dwHeight = 0;
	d3dfmt = D3DFMT_UNKNOWN;
	fmt = RHI_Format::Unknown;
}

CRT::~CRT()
{
	destroy();

	// release external reference
	Engine.ResourceManager->_DeleteRT(this);
}

void CRT::create(LPCSTR Name, u32 w, u32 h, RHI_Format f, u32 levels)
{
	if (pSurface) return;

	R_ASSERT(RenderBackend.GetDevice() && Name && Name[0] && w && h);
	_order = CPU::GetCLK();

	dwWidth = w;
	dwHeight = h;
	fmt = f;

	xrRHI::IRenderBackend* RHI = ::RHI();
	const xrRHI::RHIDeviceCaps& caps = RHI->GetDeviceCaps();

	if (!btwIsPow2(w) || !btwIsPow2(h))
	{
		if (!caps.SupportsNonPow2Textures)
		{
			Msg("!Resolution of RT(%s), %dx%d, %d is not power of 2 and GPU doesn't support it!", Name, w, h, levels);
			return;
		}
	}

	if (w > caps.MaxTextureWidth)
	{
		Msg("*!Resolution of RT(%s), width %d exceeds max %d!", Name, w, caps.MaxTextureWidth);
		return;
	}
	if (h > caps.MaxTextureHeight)
	{
		Msg("*!Resolution of RT(%s), height %d exceeds max %d!", Name, h, caps.MaxTextureHeight);
		return;
	}

	bool isDepth = IsDepthStencilFormat(f);
	if (!RHI->CheckFormatSupport(f, !isDepth, isDepth))
	{
		Msg("*!GPU doesn't support format for RT(%s), %dx%d, %d!", Name, w, h, levels);
		return;
	}

	xrRHI::TextureDesc desc;
	desc.width = w;
	desc.height = h;
	desc.depth = 1;
	desc.mipLevels = levels;
	desc.format = f;
	desc.isRenderTarget = !isDepth;
	desc.isDepthStencil = isDepth;
	desc.isCubeMap = false;

	Engine.ResourceManager->Evict();
	xrRHI::TextureHandle handle = RHI->CreateTexture(desc);
	if (!handle.IsValid())
	{
		Msg("*!Can't create RT(%s), %dx%d, %d via RHI!", Name, w, h, levels);
		return;
	}

	pSurface = (IDirect3DTexture9*)RHI->GetTextureNativeHandle(handle);
	if (!pSurface)
	{
		RHI->DestroyTexture(handle);
		Msg("*!Can't get native texture for RT(%s)!", Name);
		return;
	}
	pSurface->AddRef(); // чтобы не уничтожился вместе с хендлом, если хендл удалится

	HRESULT hr = pSurface->GetSurfaceLevel(0, &pRT);
	if (FAILED(hr))
	{
		pSurface->Release();
		pSurface = nullptr;
		RHI->DestroyTexture(handle);
		Msg("*!Can't get surface level 0 for RT(%s)!", Name);
		return;
	}

	pTexture = Engine.ResourceManager->_CreateTexture(Name);
	pTexture->surface_set(pSurface);

	Msg("* created RT(%s), %dx%d, %d", Name, w, h, levels);
}

void CRT::destroy()
{
	if (pTexture._get())
	{
		pTexture->surface_set(0);
		pTexture = NULL;
	}
	_RELEASE(pRT);
	_RELEASE(pSurface);
}

void CRT::reset_begin()
{
	destroy();
}

void CRT::reset_end()
{
	create(*cName, dwWidth, dwHeight, fmt);
}

void resptrcode_crt::create(LPCSTR Name, u32 w, u32 h, RHI_Format f, u32 levels)
{
	_set(Engine.ResourceManager->_CreateRT(Name, w, h, f, levels));
}

//////////////////////////////////////////////////////////////////////////
// CRTC Implementation
//////////////////////////////////////////////////////////////////////////

CRTC::CRTC()
{
	if (pSurface)
		return;

	pSurface = NULL;
	pRT[0] = pRT[1] = pRT[2] = pRT[3] = pRT[4] = pRT[5] = NULL;
	dwSize = 0;
	d3dfmt = D3DFMT_UNKNOWN;
	fmt = RHI_Format::Unknown;
}

CRTC::~CRTC()
{
	destroy();
	Engine.ResourceManager->_DeleteRTC(this);
}

void CRTC::create(LPCSTR Name, u32 size, RHI_Format f, u32 levels)
{
	if (pSurface) return;

	R_ASSERT(RenderBackend.GetDevice() && Name && Name[0] && size && btwIsPow2(size));
	_order = CPU::GetCLK();

	dwSize = size;
	fmt = f;

	xrRHI::IRenderBackend* RHI = ::RHI();
	const xrRHI::RHIDeviceCaps& caps = RHI->GetDeviceCaps();

	if (size > caps.MaxTextureWidth || size > caps.MaxTextureHeight)
	{
		Msg("!Cubemap size %d exceeds max allowed for RTc(%s)", size, Name);
		return;
	}

	bool isDepth = IsDepthStencilFormat(f);
	if (!RHI->CheckFormatSupport(f, !isDepth, isDepth, true))
	{
		Msg("!GPU doesn't support format for RTc(%s)", Name);
		return;
	}

	xrRHI::TextureDesc desc;
	desc.width = size;
	desc.height = size;
	desc.depth = 1;
	desc.mipLevels = levels;
	desc.format = f;
	desc.isRenderTarget = !isDepth;
	desc.isDepthStencil = isDepth;
	desc.isCubeMap = true;

	Engine.ResourceManager->Evict();
	xrRHI::TextureHandle handle = RHI->CreateTexture(desc);
	if (!handle.IsValid())
	{
		Msg("!Failed to create RTc(%s) via RHI", Name);
		return;
	}

	pSurface = static_cast<IDirect3DCubeTexture9*>(RHI->GetTextureNativeHandle(handle));
	if (!pSurface)
	{
		RHI->DestroyTexture(handle);
		Msg("!Failed to get native cube texture for RTc(%s)", Name);
		return;
	}
	pSurface->AddRef();

	for (u32 face = 0; face < 6; face++)
	{
		IDirect3DSurface9* surf = nullptr;
		if (!RHI->GetCubeMapFaceNative(handle, face, 0, (void**)&surf) || !surf)
		{
			for (u32 j = 0; j < face; j++)
				if (pRT[j]) { pRT[j]->Release(); pRT[j] = nullptr; }
			pSurface->Release();
			pSurface = nullptr;
			RHI->DestroyTexture(handle);
			Msg("!Failed to get cube face %d for RTc(%s)", face, Name);
			return;
		}
		pRT[face] = surf;
	}

	pTexture = Engine.ResourceManager->_CreateTexture(Name);
	pTexture->surface_set(pSurface);

	Msg("* created RTc(%s), 6(%d)", Name, size);
}

void CRTC::destroy()
{
	pTexture->surface_set(0);
	pTexture = NULL;
	for (u32 face = 0; face < 6; face++)
		_RELEASE(pRT[face]);
	_RELEASE(pSurface);
}

void CRTC::reset_begin()
{
	destroy();
}

void CRTC::reset_end()
{
	create(*cName, dwSize, fmt);
}

void resptrcode_crtc::create(LPCSTR Name, u32 size, RHI_Format f, u32 levels)
{
	_set(Engine.ResourceManager->_CreateRTC(Name, size, f, levels));
}
