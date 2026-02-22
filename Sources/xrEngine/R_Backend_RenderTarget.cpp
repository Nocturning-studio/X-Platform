#include "stdafx.h"
#pragma hdrstop

#include "R_Backend_RenderTarget.h"
#include "ResourceManager.h"
#include "../xrRHI/xrRHI_Types.h"

using namespace xrRHI;

// Хелпер для конвертации в D3D9
static D3DFORMAT RHIFormat_To_D3D9(RHI_Format fmt)
{
	switch (fmt)
	{
	case RHI_Format::NULLRT:
		return (D3DFORMAT)MAKEFOURCC('N', 'U', 'L', 'L');

	case RHI_Format::RGBA8_UNORM:
		return D3DFMT_A8R8G8B8;
	case RHI_Format::A8_UNORM:
		return D3DFMT_A8;
	case RHI_Format::R8_UNORM:
		return D3DFMT_L8; // Часто используется как R8 в DX9

	case RHI_Format::RGBA16_FLOAT:
		return D3DFMT_A16B16G16R16F;
	case RHI_Format::RG16_FLOAT:
		return D3DFMT_G16R16F;
	case RHI_Format::R16_FLOAT:
		return D3DFMT_R16F;

	case RHI_Format::D16_UNORM:
		return D3DFMT_D16;
	case RHI_Format::D24_UNORM_S8_UINT:
		return D3DFMT_D24S8;
	case RHI_Format::D32_FLOAT:
		return D3DFMT_D32F_LOCKABLE;

	case RHI_Format::D15S1:
		return D3DFMT_D15S1;
	case RHI_Format::D24X8:
		return D3DFMT_D24X8;
	case RHI_Format::D32_LOCKABLE:
		return D3DFMT_D32;

	case RHI_Format::D24S8_Shadow:
		return (D3DFORMAT)MAKEFOURCC('I', 'N', 'T', 'Z');
	case RHI_Format::D16_Shadow:
		return (D3DFORMAT)MAKEFOURCC('D', 'F', '1', '6');
	case RHI_Format::D24X4S4:
		return D3DFMT_D24X4S4;

	default:
		return D3DFMT_UNKNOWN;
	}
}

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
	if (pSurface)
		return;

	R_ASSERT(HW.GetDevice() && Name && Name[0] && w && h);
	_order = CPU::GetCLK();

	HRESULT _hr;

	dwWidth = w;
	dwHeight = h;
	fmt = f;

	// Конвертируем формат для вызовов DX9
	d3dfmt = RHIFormat_To_D3D9(f);

	// Get caps
	D3DCAPS9 caps;
	R_CHK(HW.GetDevice()->GetDeviceCaps(&caps));

	// Pow2 check
	if (!btwIsPow2(w) || !btwIsPow2(h))
	{
		if (!HW.GetCaps().raster.bNonPow2)
		{
			Msg("!Resolution of RT(%s), %dx%d, %d is not can be devided by 2!!!", Name, w, h, levels);
			return;
		}
	}

	// Check width-and-height limits
	if (w > caps.MaxTextureWidth)
	{
		Msg("*!Resolution of RT(%s), %dx%d, %d is bigger the maximal!!!", Name, w, h, levels);
		return;
	}
	if (h > caps.MaxTextureHeight)
	{
		Msg("*!Resolution of RT(%s), %dx%d, %d is bigger the maximal!!!", Name, w, h, levels);
		return;
	}

	// Определяем Usage (рендерим в цвет или в глубину)
	u32 usage = D3DUSAGE_RENDERTARGET;
	if (IsDepthStencilFormat(f))
	{
		usage = D3DUSAGE_DEPTHSTENCIL;
	}

	// Validate render-target usage
	_hr = HW.GetD3D()->CheckDeviceFormat(HW.GetDevAdapter(), HW.GetDevT(), HW.GetCaps().fTarget, usage, D3DRTYPE_TEXTURE, d3dfmt);
	if (FAILED(_hr))
	{
		Msg("*!Can't create RT(%s), %dx%d, %d (CheckDeviceFormat)!!!", Name, w, h, levels);
		return;
	}

	// Try to create texture/surface
	Engine.ResourceManager->Evict();

	_hr = HW.GetDevice()->CreateTexture(w, h, levels, usage, d3dfmt, D3DPOOL_DEFAULT, &pSurface, NULL);

	if (FAILED(_hr) || (0 == pSurface))
	{
		Msg("*!Can't create RT(%s), %dx%d, %d (CreateTexture)!!!", Name, w, h, levels);
		return;
	}

	// OK
	Msg("* created RT(%s), %dx%d, %d", Name, w, h, levels);
	R_CHK(pSurface->GetSurfaceLevel(0, &pRT));
	pTexture = Engine.ResourceManager->_CreateTexture(Name);
	pTexture->surface_set(pSurface);
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

	// release external reference
	Engine.ResourceManager->_DeleteRTC(this);
}

void CRTC::create(LPCSTR Name, u32 size, RHI_Format f, u32 levels)
{
	if (pSurface)
		return;

	R_ASSERT(HW.GetDevice() && Name && Name[0] && size && btwIsPow2(size));
	_order = CPU::GetCLK();

	HRESULT _hr;

	dwSize = size;
	fmt = f;

	// 1. Конвертируем формат для DX9
	d3dfmt = RHIFormat_To_D3D9(f);

	// Get caps
	D3DCAPS9 caps;
	R_CHK(HW.GetDevice()->GetDeviceCaps(&caps));

	// Check size (cubemaps are usually square power of 2)
	if (size > caps.MaxTextureWidth || size > caps.MaxTextureHeight)
		return;

	// 2. Определяем Usage (рендерим в цвет или в глубину)
	u32 usage = D3DUSAGE_RENDERTARGET;
	if (IsDepthStencilFormat(f))
	{
		usage = D3DUSAGE_DEPTHSTENCIL;
	}

	// Validate render-target usage
	// Используем D3DRTYPE_CUBETEXTURE
	_hr = HW.GetD3D()->CheckDeviceFormat(HW.GetDevAdapter(), HW.GetDevT(), HW.GetCaps().fTarget, usage, D3DRTYPE_CUBETEXTURE, d3dfmt);
	if (FAILED(_hr))
		return;

	// Try to create texture/surface
	Engine.ResourceManager->Evict();

	_hr = HW.GetDevice()->CreateCubeTexture(size, levels, usage, d3dfmt, D3DPOOL_DEFAULT, &pSurface, NULL);

	if (FAILED(_hr) || (0 == pSurface))
		return;

	// OK
	Msg("* created RTc(%s), 6(%d)", Name, size);

	// Получаем поверхности для каждой грани
	for (u32 face = 0; face < 6; face++)
		R_CHK(pSurface->GetCubeMapSurface((D3DCUBEMAP_FACES)face, 0, pRT + face));

	pTexture = Engine.ResourceManager->_CreateTexture(Name);
	pTexture->surface_set(pSurface);
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
