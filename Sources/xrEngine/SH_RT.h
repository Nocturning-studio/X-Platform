#ifndef SH_RT_H
#define SH_RT_H
#pragma once

#include "../xrRHI/xrRHI_Types.h"

//////////////////////////////////////////////////////////////////////////
class ENGINE_API CRT : public xr_resource_named
{
  public:
	IDirect3DTexture9* pSurface;
	IDirect3DSurface9* pRT;
	ref_texture pTexture;

	u32 dwWidth;
	u32 dwHeight;
	xrRHI::RHI_Format fmt;
	D3DFORMAT d3dfmt;

	u64 _order;

	CRT();
	~CRT();

	void create(LPCSTR Name, u32 w, u32 h, xrRHI::RHI_Format f, u32 levels = 1);
	void destroy();
	void reset_begin();
	void reset_end();
	IC BOOL valid()
	{
		return !!pTexture;
	}

	// Получить описание (размеры) конкретного mip-уровня
	void get_level_desc(u32 level, u32& width, u32& height)
	{
		width = 0;
		height = 0;

		if (!pSurface)
			return;

		if (level >= get_levels_count())
			return;

		D3DSURFACE_DESC desc;
		HRESULT hr = pSurface->GetLevelDesc(level, &desc);

		if (SUCCEEDED(hr))
		{
			width = desc.Width;
			height = desc.Height;
		}
	}

	// Альтернативная версия, возвращающая структуру D3DSURFACE_DESC
	bool get_level_desc(u32 level, D3DSURFACE_DESC& desc)
	{
		Memory.mem_fill(&desc, 0, sizeof(desc));

		if (!pSurface)
			return false;

		if (level >= get_levels_count())
			return false;

		HRESULT hr = pSurface->GetLevelDesc(level, &desc);
		return SUCCEEDED(hr);
	}

	// Получить количество mip-уровней
	u32 get_levels_count()
	{
		if (!pSurface)
			return 0;
		return pSurface->GetLevelCount();
	}

	// Получить поверхность конкретного mip-уровня
	IDirect3DSurface9* get_surface_level(u32 level)
	{
		if (!pSurface || level >= get_levels_count())
			return NULL;

		IDirect3DSurface9* surface = NULL;
		HRESULT hr = pSurface->GetSurfaceLevel(level, &surface);

		if (FAILED(hr))
			return NULL;

		return surface;
	}
};
struct ENGINE_API resptrcode_crt : public resptr_base<CRT>
{
	void create(LPCSTR Name, u32 w, u32 h, xrRHI::RHI_Format f, u32 levels = 1);
	void destroy()
	{
		_set(NULL);
	}
};
typedef resptr_core<CRT, resptrcode_crt> ref_rt;

//////////////////////////////////////////////////////////////////////////
class ENGINE_API CRTC : public xr_resource_named
{
  public:
	IDirect3DCubeTexture9* pSurface;
	IDirect3DSurface9* pRT[6];
	ref_texture pTexture;

	u32 dwSize;
	D3DFORMAT fmt;

	u64 _order;

	CRTC();
	~CRTC();

	void create(LPCSTR name, u32 size, D3DFORMAT f, u32 levels = 1);
	void destroy();
	void reset_begin();
	void reset_end();
	IC BOOL valid()
	{
		return !pTexture;
	}
	void get_level_desc(u32 level, u32& size)
	{
		size = 0;

		if (!pSurface)
			return;

		if (level >= get_levels_count())
			return;

		D3DSURFACE_DESC desc;
		HRESULT hr = pSurface->GetLevelDesc(level, &desc);

		if (SUCCEEDED(hr))
		{
			size = desc.Width; // Для кубической текстуры Width == Height
		}
	}

	bool get_level_desc(u32 level, D3DSURFACE_DESC& desc)
	{
		Memory.mem_fill(&desc, 0, sizeof(desc));

		if (!pSurface)
			return false;

		if (level >= get_levels_count())
			return false;

		HRESULT hr = pSurface->GetLevelDesc(level, &desc);
		return SUCCEEDED(hr);
	}

	u32 get_levels_count()
	{
		if (!pSurface)
			return 0;
		return pSurface->GetLevelCount();
	}

	IDirect3DSurface9* get_surface_level(u32 face, u32 level)
	{
		if (!pSurface || level >= get_levels_count() || face >= 6)
			return NULL;

		IDirect3DSurface9* surface = NULL;
		HRESULT hr = pSurface->GetCubeMapSurface((D3DCUBEMAP_FACES)face, level, &surface);

		if (FAILED(hr))
			return NULL;

		return surface;
	}
};
struct ENGINE_API resptrcode_crtc : public resptr_base<CRTC>
{
	void create(LPCSTR Name, u32 size, D3DFORMAT f, u32 levels = 1);
	void destroy()
	{
		_set(NULL);
	}
};
typedef resptr_core<CRTC, resptrcode_crtc> ref_rtc;

#endif // SH_RT_H
