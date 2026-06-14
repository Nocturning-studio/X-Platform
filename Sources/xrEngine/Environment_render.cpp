#include "stdafx.h"
#pragma hdrstop

#include "Environment.h"
#ifndef _EDITOR
#include "render.h"
#endif
#include "xr_efflensflare.h"
#include "rain.h"
#include "thunderbolt.h"

#ifndef _EDITOR
#include "igame_level.h"
#endif

//////////////////////////////////////////////////////////////////////////
// half box def

/*
static	fvec3	hbox_verts[24]	=
{
	{-1.f,	-1.f,	-1.f}, {-1.f,	-1.01f,	-1.f},	// down
	{ 1.f,	-1.f,	-1.f}, { 1.f,	-1.01f,	-1.f},	// down
	{-1.f,	-1.f,	 1.f}, {-1.f,	-1.01f,	 1.f},	// down
	{ 1.f,	-1.f,	 1.f}, { 1.f,	-1.01f,	 1.f},	// down
	{-1.f,	 1.f,	-1.f}, {-1.f,	 1.f,	-1.f},
	{ 1.f,	 1.f,	-1.f}, { 1.f,	 1.f,	-1.f},
	{-1.f,	 1.f,	 1.f}, {-1.f,	 1.f,	 1.f},
	{ 1.f,	 1.f,	 1.f}, { 1.f,	 1.f,	 1.f},
	{-1.f,	 0.f,	-1.f}, {-1.f,	-1.f,	-1.f},	// half
	{ 1.f,	 0.f,	-1.f}, { 1.f,	-1.f,	-1.f},	// half
	{ 1.f,	 0.f,	 1.f}, { 1.f,	-1.f,	 1.f},	// half
	{-1.f,	 0.f,	 1.f}, {-1.f,	-1.f,	 1.f}	// half
};
*/

// SkyLoader: поднял скайбокс как в зп. Если не нужно, вернуть закомменченный код
static fvec3 hbox_verts[24] = {
	{-1.f, -1.f, -1.f}, {-1.f, -1.01f, -1.f}, // down
	{1.f, -1.f, -1.f},	{1.f, -1.01f, -1.f},  // down
	{-1.f, -1.f, 1.f},	{-1.f, -1.01f, 1.f},  // down
	{1.f, -1.f, 1.f},	{1.f, -1.01f, 1.f},	  // down
	{-1.f, 2.f, -1.f},	{-1.f, 1.f, -1.f},	  {1.f, 2.f, -1.f}, {1.f, 1.f, -1.f},  {-1.f, 2.f, 1.f},
	{-1.f, 1.f, 1.f},	{1.f, 2.f, 1.f},	  {1.f, 1.f, 1.f},	{-1.f, 0.f, -1.f}, {-1.f, -1.f, -1.f}, // half
	{1.f, 0.f, -1.f},	{1.f, -1.f, -1.f},															   // half
	{1.f, 0.f, 1.f},	{1.f, -1.f, 1.f},															   // half
	{-1.f, 0.f, 1.f},	{-1.f, -1.f, 1.f}															   // half
};
static u16 hbox_faces[20 * 3] = {0,	 2, 3,	3,	1, 0, 4,  5,  7, 7, 6, 4,  0,  1, 9,  9, 8, 0, 8,  9,
								 5,	 5, 4,	8,	1, 3, 10, 10, 9, 1, 9, 10, 7,  7, 5,  9, 3, 2, 11, 11,
								 10, 3, 10, 11, 6, 6, 7,  10, 2, 0, 8, 8,  11, 2, 11, 8, 4, 4, 6,  11};

#pragma pack(push, 1)
struct v_skybox
{
	fvec3 p;
	u32 color;
	fvec3 uv[2];

	void set(fvec3& _p, u32 _c, fvec3& _tc)
	{
		p = _p;
		color = _c;
		uv[0] = _tc;
		uv[1] = _tc;
	}
};
const u32 v_skybox_fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2 | D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEXCOORDSIZE3(1);
struct v_clouds
{
	fvec3 p;
	u32 color;
	u32 intensity;
	void set(fvec3& _p, u32 _c, u32 _i)
	{
		p = _p;
		color = _c;
		intensity = _i;
	}
};
const u32 v_clouds_fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR;
#pragma pack(pop)

//-----------------------------------------------------------------------------
// Environment render
//-----------------------------------------------------------------------------
extern ENGINE_API float psHUD_FOV;
void CEnvironment::RenderSky()
{
	// OPTICK_EVENT("CEnvironment::RenderSky");

#ifndef _EDITOR
	if (0 == g_pGameLevel)
		return;
#endif

	// Инициализация геометрии при необходимости
	if (bNeed_re_create_env)
	{
		sh_2sky.create(&m_b_skybox, "skybox_2t");
		sh_2geom.create(v_skybox_fvf, RenderBackendLegacy.Vertex.Buffer(), RenderBackendLegacy.Index.Buffer());
		clouds_sh.create("clouds", "null");
		clouds_geom.create(v_clouds_fvf, RenderBackendLegacy.Vertex.Buffer(), RenderBackendLegacy.Index.Buffer());
		bNeed_re_create_env = FALSE;
	}

	::Render->set_render_mode(::Render->MODE_FAR);

	// Матрица преобразования скайбокса
	fmat4x4 mSky;
	mSky.rotateY(CurrentEnv->sky_rotation);
	mSky.translate_over(Engine.RenderView.Position);

	// Вычисляем цвет скайбокса
	u32 i_offset, v_offset;
	u32 C = color_rgba(iFloor(CurrentEnv->sky_color.x * 255.f), iFloor(CurrentEnv->sky_color.y * 255.f),
					   iFloor(CurrentEnv->sky_color.z * 255.f), iFloor(CurrentEnv->weight * 255.f));

	// Заполняем индексный буфер
	u16* pib = RenderBackendLegacy.Index.Lock(20 * 3, i_offset);
	CopyMemory(pib, hbox_faces, 20 * 3 * 2);
	RenderBackendLegacy.Index.Unlock(20 * 3);

	// Заполняем вершинный буфер
	v_skybox* pv = (v_skybox*)RenderBackendLegacy.Vertex.Lock(12, sh_2geom.stride(), v_offset);
	for (u32 v = 0; v < 12; v++)
		pv[v].set(hbox_verts[v * 2], C, hbox_verts[v * 2 + 1]);
	RenderBackendLegacy.Vertex.Unlock(12, sh_2geom.stride());

	// Устанавливаем состояние рендера
	RenderBackendLegacy.set_transform_world(mSky);
	RenderBackendLegacy.set_Geometry(sh_2geom);
	RenderBackendLegacy.set_Shader(sh_2sky);

	// Рендерим скайбокс
	RenderBackendLegacy.Render(D3DPT_TRIANGLELIST, v_offset, 0, 12, i_offset, 20);

	// Сбрасываем режим рендера
	::Render->set_render_mode(::Render->MODE_NORMAL);

	// Рендерим солнце (линз флеры)
	eff_LensFlare->Render(TRUE, FALSE, FALSE);
}

void CEnvironment::RenderClouds()
{

}

void CEnvironment::RenderFlares()
{
#ifndef _EDITOR
	if (0 == g_pGameLevel)
		return;
#endif
	// 1
	eff_LensFlare->Render(FALSE, TRUE, TRUE);
}

float CEnvironment::GetFlaresBlendFactor()
{
	return eff_LensFlare->GetBlendFactor();
}

void CEnvironment::RenderThunderbolt()
{
	PROFILE_FUNCTION();

#ifndef _EDITOR
	if (0 == g_pGameLevel)
		return;
#endif
	// 2
	eff_Thunderbolt->Render();
}

void CEnvironment::RenderRain()
{
	PROFILE_FUNCTION();

#ifndef _EDITOR
	if (0 == g_pGameLevel)
		return;
#endif
	// 2
	eff_Rain->Render();
}

void CEnvironment::OnDeviceCreate()
{
	// Создаем шейдеры и геометрию
	sh_2sky.create(&m_b_skybox, "skybox_2t");
	sh_2geom.create(v_skybox_fvf, RenderBackendLegacy.Vertex.Buffer(), RenderBackendLegacy.Index.Buffer());
	clouds_sh.create("clouds", "null");
	clouds_geom.create(v_clouds_fvf, RenderBackendLegacy.Vertex.Buffer(), RenderBackendLegacy.Index.Buffer());

	// weathers
	{
		EnvsMapIt _I, _E;
		_I = WeatherCycles.begin();
		_E = WeatherCycles.end();
		for (; _I != _E; _I++)
			for (EnvIt it = _I->second.begin(); it != _I->second.end(); it++)
				(*it)->on_device_create();
	}
	// effects
	{
		EnvsMapIt _I, _E;
		_I = WeatherFXs.begin();
		_E = WeatherFXs.end();
		for (; _I != _E; _I++)
			for (EnvIt it = _I->second.begin(); it != _I->second.end(); it++)
				(*it)->on_device_create();
	}

	Invalidate();
	OnFrame();
}

void CEnvironment::OnDeviceDestroy()
{
	// OPTICK_EVENT("CEnvironment::OnDeviceDestroy");

	// Очищаем поверхности render targets
	tsky0->surface_set(NULL);
	tsky1->surface_set(NULL);

	tlut0->surface_set(NULL);
	tlut1->surface_set(NULL);

	// Уничтожаем шейдеры и геометрию
	sh_2sky.destroy();
	sh_2geom.destroy();
	clouds_sh.destroy();
	clouds_geom.destroy();

	// weathers
	{
		EnvsMapIt _I, _E;
		_I = WeatherCycles.begin();
		_E = WeatherCycles.end();
		for (; _I != _E; _I++)
			for (EnvIt it = _I->second.begin(); it != _I->second.end(); it++)
				(*it)->on_device_destroy();
	}
	// effects
	{
		EnvsMapIt _I, _E;
		_I = WeatherFXs.begin();
		_E = WeatherFXs.end();
		for (; _I != _E; _I++)
			for (EnvIt it = _I->second.begin(); it != _I->second.end(); it++)
				(*it)->on_device_destroy();
	}

	// Уничтожаем CurrentEnv
	if (CurrentEnv)
		CurrentEnv->destroy();
}

#ifdef _EDITOR
void CEnvironment::ED_Reload()
{
	OnDeviceDestroy();
	OnDeviceCreate();
}
#endif
