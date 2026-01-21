#include "stdafx.h"
#pragma hdrstop

#include "ResourceManager.h"

CRT::CRT()
{
	pSurface = NULL;
	pRT = NULL;
	dwWidth = 0;
	dwHeight = 0;
	fmt = D3DFMT_UNKNOWN;
}
CRT::~CRT()
{
	destroy();

	// release external reference
	Engine.ResourceManager->_DeleteRT(this);
}

void CRT::create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 levels)
{
	if (pSurface)
		return;

	R_ASSERT(HW.pDevice && Name && Name[0] && w && h);
	_order = CPU::GetCLK(); 

	HRESULT _hr;

	dwWidth = w;
	dwHeight = h;
	fmt = f;

	// Get caps
	D3DCAPS9 caps;
	R_CHK(HW.pDevice->GetDeviceCaps(&caps));

	// Pow2
	if (!btwIsPow2(w) || !btwIsPow2(h))
	{
		if (!HW.Caps.raster.bNonPow2)
		{
			Msg("!Resolution of RT(%s), %dx%d, %d is not can be devided by 2!!!", Name, w, h, levels);
			return;
		}
	}

	// Check width-and-height of render target surface
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
	
#pragma todo("NSDeathman to ALL: Разобраться с предупреждением")
#pragma warning(disable: 4063)
	// Select usage
	u32 usage = 0;
	switch (fmt)
	{
	case D3DFMT_D16_LOCKABLE:
	case D3DFMT_D32:
	case D3DFMT_D15S1:
	case D3DFMT_D24S8:
	case D3DFMT_D24X8:
	case D3DFMT_D24X4S4:
	case D3DFMT_D16:
	case D3DFMT_D32F_LOCKABLE:
	case D3DFMT_D24FS8:
	case MAKEFOURCC('D', 'F', '2', '4'): // fetch smap (ATI 9500)
	case MAKEFOURCC('D', 'F', '1', '6'): // fetch smap (ATI X1300)
	case MAKEFOURCC('R', 'A', 'W', 'Z'): // depth as texture (GeForce 6000-7000)
	case MAKEFOURCC('I', 'N', 'T', 'Z'): // depth as texture (GeForce 8000+, HD 4000+) 
	case MAKEFOURCC('R', 'E', 'S', 'Z'): // msaa depth as texture (HD 4000+)
		usage = D3DUSAGE_DEPTHSTENCIL;
		break;
	default:
		usage = D3DUSAGE_RENDERTARGET;
	}

	// Validate render-target usage
	_hr = HW.pD3D->CheckDeviceFormat(HW.DevAdapter, HW.DevT, HW.Caps.fTarget, usage, D3DRTYPE_TEXTURE, f);
	if (FAILED(_hr))
	{
		Msg("*!Can't create RT(%s), %dx%d, %d (CheckDeviceFormat)!!!", Name, w, h, levels);
		return;
	}

	// Try to create texture/surface
	Engine.ResourceManager->Evict();
	_hr = HW.pDevice->CreateTexture(w, h, levels, usage, f, D3DPOOL_DEFAULT, &pSurface, NULL);
	if (FAILED(_hr) || (0 == pSurface))
	{
		Msg("*!Can't create RT(%s), %dx%d, %d (CreateTexture)!!!", Name, w, h, levels);
		return;
	}

		// OK
//#ifdef DEBUG
	Msg("* created RT(%s), %dx%d, %d", Name, w, h, levels);
//#endif // DEBUG
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
void resptrcode_crt::create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 levels)
{
	_set(Engine.ResourceManager->_CreateRT(Name, w, h, f, levels));
}

//////////////////////////////////////////////////////////////////////////
CRTC::CRTC()
{
	if (pSurface)
		return;

	pSurface = NULL;
	pRT[0] = pRT[1] = pRT[2] = pRT[3] = pRT[4] = pRT[5] = NULL;
	dwSize = 0;
	fmt = D3DFMT_UNKNOWN;
}
CRTC::~CRTC()
{
	destroy();

	// release external reference
	Engine.ResourceManager->_DeleteRTC(this);
}

void CRTC::create(LPCSTR Name, u32 size, D3DFORMAT f, u32 levels)
{
	R_ASSERT(HW.pDevice && Name && Name[0] && size && btwIsPow2(size));
	_order = CPU::GetCLK(); 

	HRESULT _hr;

	dwSize = size;
	fmt = f;

	// Get caps
	D3DCAPS9 caps;
	R_CHK(HW.pDevice->GetDeviceCaps(&caps));

	// Check width-and-height of render target surface
	if (size > caps.MaxTextureWidth)
		return;
	if (size > caps.MaxTextureHeight)
		return;

	// Validate render-target usage
	_hr = HW.pD3D->CheckDeviceFormat(HW.DevAdapter, HW.DevT, HW.Caps.fTarget, D3DUSAGE_RENDERTARGET, D3DRTYPE_CUBETEXTURE, f);
	if (FAILED(_hr))
		return;

	// Try to create texture/surface
	Engine.ResourceManager->Evict();
	_hr = HW.pDevice->CreateCubeTexture(size, levels, D3DUSAGE_RENDERTARGET, f, D3DPOOL_DEFAULT, &pSurface, NULL);
	if (FAILED(_hr) || (0 == pSurface))
		return;

	// OK
	Msg("* created RTc(%s), 6(%d)", Name, size);
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

void resptrcode_crtc::create(LPCSTR Name, u32 size, D3DFORMAT f, u32 levels)
{
	_set(Engine.ResourceManager->_CreateRTC(Name, size, f, levels));
}
