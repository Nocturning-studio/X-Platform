// Texture.cpp: implementation of the CTexture class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop

#pragma warning(disable : 4995)
#include <d3dx9.h>
#pragma warning(default : 4995)

// #include "std_classes.h"
// #include "xr_avi.h"
#include "../xrEngine/ResourceManager.h"

int get_texture_load_lod(LPCSTR fn)
{
	CInifile::Sect& sect = pSettings->r_section("reduce_lod_texture_list");
	CInifile::SectCIt it_ = sect.Data.begin();
	CInifile::SectCIt it_e_ = sect.Data.end();

	CInifile::SectCIt it = it_;
	CInifile::SectCIt it_e = it_e_;

	for (; it != it_e; ++it)
	{
		if (strstr(fn, it->first.c_str()))
		{
			if (psTextureLOD < 1)
				return 0;
			else if (psTextureLOD < 3)
				return 1;
			else
				return 2;
		}
	}

	if (psTextureLOD < 2)
		return 0;
	else if (psTextureLOD < 4)
		return 1;
	else
		return 2;
}

u32 calc_texture_size(int lod, u32 mip_cnt, u32 orig_size)
{
	if (1 == mip_cnt)
		return orig_size;

	int _lod = lod;
	float res = float(orig_size);

	while (_lod > 0)
	{
		--_lod;
		res -= res / 1.333f;
	}
	return iFloor(res);
}

IDirect3DBaseTexture9* CRender::texture_load(LPCSTR fRName, u32& ret_msize)
{
	IDirect3DTexture9* pTexture2D = NULL;
	IDirect3DCubeTexture9* pTextureCUBE = NULL;
	string_path fn;
	u32 dwWidth, dwHeight;
	u32 img_size = 0;
	int img_loaded_lod = 0;
	D3DFORMAT fmt;
	u32 mip_cnt = u32(-1);
	// validation
	R_ASSERT(fRName);
	R_ASSERT(fRName[0]);

	// make file name
	string_path fname;
	strcpy(fname, fRName);
	Engine.ResourceManager->fix_texture_name(fname);
	IReader* S = NULL;

	// Try to find texture in multiple locations
	if (FS.exist(fn, "$level$", fname, ".dds"))
		goto _DDS;
	if (FS.exist(fn, "$game_textures$", fname, ".dds"))
		goto _DDS;
	if (FS.exist(fn, "$game_textures$", fname, ".hdr"))
		goto _DDS;
	if (FS.exist(fn, "$game_saves$", fname, ".dds"))
		goto _DDS;

#ifdef _EDITOR
	ELog.Msg(mtError, "Can't find texture '%s'", fname);
	return 0;
#else
	Msg("! Can't find texture '%s'", fname);
	R_ASSERT(FS.exist(fn, "$game_textures$", "ed\\ed_not_existing_texture", ".dds"));
	goto _DDS;
#endif

_DDS : {
	// Load and get header
	D3DXIMAGE_INFO IMG;
	S = FS.r_open(fn);
#ifdef DEBUG
	Msg("* Loaded: %s[%d]b", fn, S->length());
#endif // DEBUG
	img_size = S->length();
	R_ASSERT(S);
	HRESULT const result = D3DXGetImageInfoFromFileInMemory(S->pointer(), S->length(), &IMG);
	if (FAILED(result))
	{
		Msg("! Can't get image info for texture '%s'", fn);
		FS.r_close(S);
		string_path temp;
		R_ASSERT(FS.exist(temp, "$game_textures$", "ed\\ed_not_existing_texture", ".dds"));
		R_ASSERT(xr_strcmp(temp, fn));
		strcpy(fn, temp);
		goto _DDS;
	}

	if (IMG.ResourceType == D3DRTYPE_CUBETEXTURE)
		goto _DDS_CUBE;
	else
		goto _DDS_2D;

_DDS_CUBE : {
	HRESULT const result_cube = D3DXCreateCubeTextureFromFileInMemoryEx(
		HW.GetDevice(), S->pointer(), S->length(), D3DX_DEFAULT, IMG.MipLevels, 0, IMG.Format, D3DPOOL_DEFAULT,
		D3DX_DEFAULT, D3DX_DEFAULT, 0, &IMG, 0, &pTextureCUBE);
	FS.r_close(S);

	if (FAILED(result_cube))
	{
		Msg("! Can't load texture '%s'", fn);
		string_path temp;
		R_ASSERT(FS.exist(temp, "$game_textures$", "ed\\ed_not_existing_texture", ".dds"));
		R_ASSERT(xr_strcmp(temp, fn));
		strcpy(fn, temp);
		goto _DDS;
	}

	// OK
	dwWidth = IMG.Width;
	dwHeight = IMG.Height;
	fmt = IMG.Format;
	mip_cnt = pTextureCUBE->GetLevelCount();
	ret_msize = calc_texture_size(img_loaded_lod, mip_cnt, img_size);
	return pTextureCUBE;
}
_DDS_2D : {
	// Load texture directly with all mip levels using D3DPOOL_DEFAULT
	HRESULT const result_2D =
		D3DXCreateTextureFromFileInMemoryEx(HW.GetDevice(), S->pointer(), S->length(), D3DX_DEFAULT, D3DX_DEFAULT,
											D3DX_DEFAULT, // Use all mip levels
											0, IMG.Format,
											D3DPOOL_DEFAULT, // Use DEFAULT pool
											D3DX_DEFAULT, D3DX_DEFAULT, 0, &IMG, 0, &pTexture2D);
	FS.r_close(S);

	if (FAILED(result_2D))
	{
		Msg("! Can't load texture '%s'", fn);
		string_path temp;
		R_ASSERT(FS.exist(temp, "$game_textures$", "ed\\ed_not_existing_texture", ".dds"));
		xr_strlwr(temp);
		R_ASSERT(xr_strcmp(temp, fn));
		strcpy(fn, temp);
		goto _DDS;
	}

	// Get texture info
	D3DSURFACE_DESC desc;
	pTexture2D->GetLevelDesc(0, &desc);
	dwWidth = desc.Width;
	dwHeight = desc.Height;
	fmt = desc.Format;
	mip_cnt = pTexture2D->GetLevelCount();

	// Calculate size - use 0 for LOD since we want all mip levels
	ret_msize = calc_texture_size(0, mip_cnt, img_size);
	return pTexture2D;
}
}
}
