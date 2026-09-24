///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
#include "blender_combine.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::combine_additional_postprocess()
{
	////OPTICK_EVENT("CRender::combine_additional_postprocess");

	RenderBackend.SetCullMode(CULL_DISABLE);
	RenderBackend.SetStencil(FALSE);

	RenderBackend.SetShaderElement(RenderTarget->s_combine->E[SE_COMBINE_POSTPROCESS]);
	RenderBackend.SetConstant("cas_params", ps_cas_contrast, ps_cas_sharpening, 0, 0);
	RenderBackend.SetConstant("bloom_parameters", ps_r_bloom_threshold,
							   ps_r_bloom_brightness,
							   ps_r_bloom_blades_threshold,
							   ps_r_bloom_blades_brightness);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic[0]);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::combine_sun_shafts()
{
	OPTICK_EVENT("CRender::combine_sun_shafts");

	RenderBackend.SetCullMode(CULL_DISABLE);
	RenderBackend.SetStencil(FALSE);

	RenderBackend.SetShaderElement(RenderTarget->s_combine->E[SE_COMBINE_VOLUMETRIC]);
	float sun_shafts_intensity = g_pGamePersistent->Environment().CurrentEnv->m_fSunShaftsIntensity;
	RenderBackend.SetConstant("sun_shafts_intensity", sun_shafts_intensity, 0, 0, 0);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic[1]);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_skybox()
{
	OPTICK_EVENT("CRender::render_skybox");

	RenderBackend.SetRenderTarget(RenderTarget->rt_Generic[1]);
	RenderBackend.SetDepthBuffer(NULL);
	RenderBackend.SetCullMode(CULL_DISABLE);
	RenderBackend.SetStencil(FALSE);
	RenderBackend.SetColorWriteEnable();

	// Draw full-screen quad textured with our scene image draw skybox
	RenderBackend.SetRenderState(D3DRS_ZENABLE, FALSE);
	g_pGamePersistent->Environment().RenderSky();
	RenderBackend.SetRenderState(D3DRS_ZENABLE, TRUE);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::precombine_scene()
{
	OPTICK_EVENT("CRender::combine_additional_postprocess");

	RenderBackend.SetCullMode(CULL_DISABLE);
	RenderBackend.SetStencil(FALSE);
	RenderBackend.SetColorWriteEnable();

	float additional_ambient = 0.0f;

	if(g_pGamePersistent && g_pGamePersistent->GetNightVisionState())
		additional_ambient = 0.5f;

	RenderBackend.SetShaderElement(RenderTarget->s_combine->E[SE_PRECOMBINE_SCENE]);
	RenderBackend.SetConstant("additional_ambient", additional_ambient);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic[0]);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::combine_scene_lighting()
{
	PROFILE_FUNCTION();

	float additional_ambient = 0.0f;

	if(g_pGamePersistent && g_pGamePersistent->GetNightVisionState())
		additional_ambient = 0.5f;

	CEnvDescriptorMixer* envdesc = g_pGamePersistent->Environment().CurrentEnv;

	const float minamb = 0.001f;
	fvec4 ambclr = {_max(sRgbToLinear(envdesc->ambient.x), minamb),
					_max(sRgbToLinear(envdesc->ambient.y), minamb),
					_max(sRgbToLinear(envdesc->ambient.z), minamb),
					ps_r_ao_brightness};

	fvec4 envclr = {sRgbToLinear(envdesc->hemi_color.x),
					sRgbToLinear(envdesc->hemi_color.y),
					sRgbToLinear(envdesc->hemi_color.z),
					envdesc->weight};

	IDirect3DBaseTexture9* e0 = nullptr;
	IDirect3DBaseTexture9* e1 = nullptr;

	if(envdesc->sky_irradiance_0)
		e0 = envdesc->sky_irradiance_0->surface_get();
	if(envdesc->sky_irradiance_1)
		e1 = envdesc->sky_irradiance_1->surface_get();

	if(e0)
	{
		RenderTarget->t_irradiance_map_0->surface_set(e0);
		_RELEASE(e0);
	}

	if(e1)
	{
		RenderTarget->t_irradiance_map_1->surface_set(e1);
		_RELEASE(e1);
	}

	RenderBackend.SetShaderElement(RenderTarget->s_combine->E[SE_COMBINE_SCENE]);
	RenderBackend.SetConstant("additional_ambient", additional_ambient);
	RenderBackend.SetConstant("debug_mode", ps_r_debug_render);
	RenderBackend.SetConstant("ambient_color", ambclr);
	RenderBackend.SetConstant("env_color", envclr);
	RenderBackend.SetCullMode(CULL_DISABLE);
	// stencil should be >= 1, we don't touch sky pixels
	RenderBackend.SetStencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic[1], RenderBackend.GetBaseZB());
	//
	// #ifdef DEBUG
	//	RenderBackend.SetCullMode(CULL_BACKFACE);
	//	static xr_vector<Fplane> saved_dbg_planes;
	//	if (bDebug)
	//		saved_dbg_planes = dbg_planes;
	//	else
	//		dbg_planes = saved_dbg_planes;
	//	if (1)
	//		for (u32 it = 0; it < dbg_planes.size(); it++)
	//		{
	//			Fplane& P = dbg_planes[it];
	//			fvec3 zero;
	//			zero.mul(P.n, P.d);
	//
	//			fvec3 L_dir, L_up = P.n, L_right;
	//			L_dir.set(0, 0, 1);
	//			if (_abs(L_up.dotproduct(L_dir)) > .99f)
	//				L_dir.set(1, 0, 0);
	//			L_right.crossproduct(L_up, L_dir);
	//			L_right.normalize();
	//			L_dir.crossproduct(L_right, L_up);
	//			L_dir.normalize();
	//
	//			fvec3 p0, p1, p2, p3;
	//			float sz = 100.f;
	//			p0.mad(zero, L_right, sz).mad(L_dir, sz);
	//			p1.mad(zero, L_right, sz).mad(L_dir, -sz);
	//			p2.mad(zero, L_right, -sz).mad(L_dir, -sz);
	//			p3.mad(zero, L_right, -sz).mad(L_dir, +sz);
	//			RenderBackend.dbg_DrawTRI(Fidentity, p0, p1, p2, 0xffffffff);
	//			RenderBackend.dbg_DrawTRI(Fidentity, p2, p3, p0, 0xffffffff);
	//		}
	//
	//	static xr_vector<dbg_line_t> saved_dbg_lines;
	//	if (bDebug)
	//		saved_dbg_lines = dbg_lines;
	//	else
	//		dbg_lines = saved_dbg_lines;
	//	if (1)
	//		for (u32 it = 0; it < dbg_lines.size(); it++)
	//		{
	//			RenderBackend.dbg_DrawLINE(Fidentity, dbg_lines[it].P0, dbg_lines[it].P1, dbg_lines[it].color);
	//		}
	//
	//	dbg_spheres.clear();
	//	dbg_lines.clear();
	//	dbg_planes.clear();
	// #endif
}
///////////////////////////////////////////////////////////////////////////////////
