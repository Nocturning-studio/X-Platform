#include "stdafx.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"

//////////////////////////////////////////////////////////////////////////
// tables to calculate view-frustum bounds in world space
// note: D3D uses [0..1] range for Z
static Fvector3 corners[8] = 
{
	{-1, -1, 0.7}, 
	{-1, -1, +1},	
	{-1, +1, +1}, 
	{-1, +1, 0.7},
	{+1, +1, +1},	 
	{+1, +1, 0.7}, 
	{+1, -1, +1}, 
	{+1, -1, 0.7}
};

static u16 facetable[16][3] = 
{
	{3, 2, 1}, 
	{3, 1, 0}, 
	{7, 6, 5}, 
	{5, 6, 4}, 
	{3, 5, 2}, 
	{4, 2, 5}, 
	{1, 6, 7}, 
	{7, 0, 1},
	{5, 3, 0}, 
	{7, 5, 0},
	{1, 4, 6}, 
	{2, 4, 1},
};

void CRender::accumulate_sun(u32 sub_phase, Fmatrix& xform, Fmatrix& xform_prev, float fBias)
{
	OPTICK_EVENT("CRender::accumulate_sun");

	// *** assume accumulator setted up ***
	light* sun = (light*)RenderImplementation.Lights.sun_adapted._get();

	// Common constants (light-related)
	Fvector L_dir, L_clr;
	float L_spec;
	L_clr.set(sun->get_color().r, sun->get_color().g, sun->get_color().b);
	Device.mView.transform_dir(L_dir, sun->get_direction());
	L_dir.normalize();

	// Perform masking (only once - on the first/near phase)
	if (SE_SUN_NEAR == sub_phase)
	{
		set_light_accumulator();
		RenderBackend.set_CullMode(CULL_DISABLE);

		// Use backend's viewport geometry setup
		u32 Offset = 0;
		RenderBackend.set_viewport_geometry(Offset);

		// Setup shader
		RenderBackend.set_Element(RenderTarget->s_accum_mask->E[SE_MASK_DIRECT]);
		RenderBackend.set_Constant("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0);

		// Stencil masking
		RenderBackend.set_ColorWriteEnable(FALSE);
		RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
		RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}

	// Setup lighting pass
	set_light_accumulator();
	RenderBackend.set_CullMode(CULL_BACKFACE);
	RenderBackend.set_ColorWriteEnable();

	// Texture adjustment matrix
	float fTexelOffs = (0.5f / float(RenderImplementation.o.smapsize));
	float fRange = 0.0f;

	switch (sub_phase)
	{
	case SE_SUN_NEAR:
		fBias = ps_r_sun_depth_near_bias;
		fRange = 1;
		break;
	case SE_SUN_MIDDLE:
		fBias = ps_r_sun_depth_middle_bias;
		fRange = 1;
		break;
	case SE_SUN_FAR:
		fBias = ps_r_sun_depth_far_bias;
		fRange = 1;
		break;
	}

	Fmatrix m_TexelAdjust = {0.5f,
							 0.0f,
							 0.0f,
							 0.0f,
							 0.0f,
							 -0.5f,
							 0.0f,
							 0.0f,
							 0.0f,
							 0.0f,
							 fRange,
							 0.0f,
							 0.5f + fTexelOffs,
							 0.5f + fTexelOffs,
							 fBias,
							 1.0f};

	// Compute shadow matrix
	FPU::m64r();
	Fmatrix xf_invview;
	xf_invview.invert(Device.mView);

	Fmatrix m_shadow;
	{
		Fmatrix xf_project;
		xf_project.mul(m_TexelAdjust, sun->X.D.combine);
		m_shadow.mul(xf_project, xf_invview);

		// TSM bias
		Fvector bias;
		bias.mul(L_dir, ps_r_sun_tsm_bias);
		Fmatrix bias_t;
		bias_t.translate(bias);
		m_shadow.mulB_44(bias_t);
	}
	FPU::m24r();

	float NormalBias = 0.0f;
	float DirectionalBias = 0.0f;

	switch (sub_phase)
	{
	case SE_SUN_NEAR:
		NormalBias = ps_r_sun_depth_near_normal_bias;
		DirectionalBias = ps_r_sun_depth_near_directional_bias;
		break;
	case SE_SUN_MIDDLE:
		NormalBias = ps_r_sun_depth_middle_normal_bias;
		DirectionalBias = ps_r_sun_depth_middle_directional_bias;
		break;
	case SE_SUN_FAR:
		NormalBias = ps_r_sun_depth_far_normal_bias;
		DirectionalBias = ps_r_sun_depth_far_directional_bias;
		break;
	}

	// Setup texgen
	Fmatrix m_Texgen;
	m_Texgen.identity();
	RenderBackend.xforms.set_W(m_Texgen);
	RenderBackend.xforms.set_V(Device.mView);
	RenderBackend.xforms.set_P(Device.mProject);
	RenderBackend.u_compute_texgen_screen(m_Texgen);

	// Setup geometry using backend
	u32 i_offset, v_offset;
	{
		// Lock indices
		u16* pib = RenderBackend.Index.Lock(sizeof(facetable) / sizeof(u16), i_offset);
		CopyMemory(pib, facetable, sizeof(facetable));
		RenderBackend.Index.Unlock(sizeof(facetable) / sizeof(u16));

		// Lock vertices
		u32 ver_count = sizeof(corners) / sizeof(Fvector3);
		FVF::L* pv = (FVF::L*)RenderBackend.Vertex.Lock(ver_count, RenderTarget->g_cuboid.stride(), v_offset);

		Fmatrix inv_XDcombine;
		if (sub_phase == SE_SUN_FAR)
			inv_XDcombine.invert(xform_prev);
		else
			inv_XDcombine.invert(xform);

		for (u32 i = 0; i < ver_count; ++i)
		{
			Fvector3 tmp_vec;
			inv_XDcombine.transform(tmp_vec, corners[i]);
			pv->set(tmp_vec, color_rgba(255, 255, 255, 255));
			pv++;
		}
		RenderBackend.Vertex.Unlock(ver_count, RenderTarget->g_cuboid.stride());
	}

	RenderBackend.set_Geometry(RenderTarget->g_cuboid);

	// Setup shader and constants
	RenderBackend.set_Element(RenderTarget->s_accum_direct_cascade->E[sub_phase]);
	RenderBackend.set_Constant("m_bias", NormalBias, DirectionalBias);
	RenderBackend.set_Constant("m_texgen", m_Texgen);
	RenderBackend.set_Constant("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0);
	RenderBackend.set_Constant("Ldynamic_color", sRgbToLinear(L_clr.x), sRgbToLinear(L_clr.y), sRgbToLinear(L_clr.z));
	RenderBackend.set_Constant("m_shadow", m_shadow);

	// Setup depth testing
	if ((SE_SUN_NEAR == sub_phase || SE_SUN_MIDDLE == sub_phase))
		HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
	else if (!ps_r_lighting_flags.is(RFLAGEXT_SUN_ZCULLING))
		HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
	else
		HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);

	// Setup stencil
	if (SE_SUN_NEAR == sub_phase || sub_phase == SE_SUN_MIDDLE)
		RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0xFE, D3DSTENCILOP_KEEP, D3DSTENCILOP_ZERO, D3DSTENCILOP_KEEP);
	else
		RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);

	// Render
	RenderBackend.Render(D3DPT_TRIANGLELIST, v_offset, 0, 8, i_offset, 16);

	// Render volumetric sun in separate pass
	accumulate_volumetric_sun(sub_phase, m_shadow, L_dir);
}

bool bVolumetricSunTextureCleared = false;
void CRender::accumulate_volumetric_sun(u32 sub_phase, Fmatrix m_shadow, Fvector L_dir)
{
	OPTICK_EVENT("CRender::accumulate_volumetric_sun");

	if (!(g_pGamePersistent->Environment().CurrentEnv->m_fSunShaftsIntensity > 0.05f) || !ps_r_lighting_flags.test(RFLAG_SUN_SHAFTS))
	{
		if (!bVolumetricSunTextureCleared)
		{
			RenderBackend.ClearTexture(RenderTarget->rt_Volumetric_Sun, color_rgba(0, 0, 0, 0));
			bVolumetricSunTextureCleared = true;
		}
		return;
	}

	if (bVolumetricSunTextureCleared)
		bVolumetricSunTextureCleared = false;

	// Убираем ВСЕ ограничения для объемного света
	RenderBackend.set_Stencil(FALSE);
	RenderBackend.set_CullMode(CULL_DISABLE);
	RenderBackend.set_Depth_Buffer(NULL);
	RenderBackend.set_ColorWriteEnable();

	switch (sub_phase)
	{
	case SE_SUN_NEAR:
		sub_phase = SE_SUN_VOL_NEAR;
		break;
	case SE_SUN_MIDDLE:
		sub_phase = SE_SUN_VOL_MIDDLE;
		break;
	case SE_SUN_FAR:
		sub_phase = SE_SUN_VOL_FAR;
		break;
	}

	RenderBackend.set_Element(RenderTarget->s_accum_direct_cascade->E[sub_phase]);

	// Pass necessary constants
	float Weight = RenderTarget->rt_Volumetric_Sun->dwWidth;
	float Height = RenderTarget->rt_Volumetric_Sun->dwHeight;

	float sun_shafts_intensity = g_pGamePersistent->Environment().CurrentEnv->m_fSunShaftsIntensity;
	RenderBackend.set_Constant("image_resolution", Weight, Height, 1.0f / Weight, 1.0f / Height);
	RenderBackend.set_Constant("sun_shafts_intensity", sun_shafts_intensity, 0, 0, 0);
	RenderBackend.set_Constant("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0);
	RenderBackend.set_Constant("m_shadow", m_shadow);

	RenderBackend.RenderViewportSurface(Weight, Height, RenderTarget->rt_Volumetric_Sun);
}
