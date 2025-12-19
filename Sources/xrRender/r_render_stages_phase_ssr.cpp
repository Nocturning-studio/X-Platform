///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "blender_reflections.h"
///////////////////////////////////////////////////////////////////////////////////
bool ReflectionsTexIsCleared = false;
///////////////////////////////////////////////////////////////////////////////////
void CRender::clear_reflections()
{
	ReflectionsTexIsCleared = true;
	RenderBackend.ClearTexture(RenderTarget->rt_Reflections, color_argb(NULL, NULL, NULL, NULL));
	RenderBackend.ClearTexture(RenderTarget->rt_BackbufferMip, color_argb(NULL, NULL, NULL, NULL));
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::create_backbuffer_mip_chain()
{
	OPTICK_EVENT("CRender::downsample_scene_luminance");
	RenderBackend.set_CullMode(CULL_DISABLE);
	RenderBackend.set_Stencil(FALSE);

	ref_rt MipChain = RenderTarget->rt_BackbufferMip;

	// Начальная инициализация (копирование)
	RenderBackend.set_Element(RenderTarget->s_reflections->E[SE_SSR_GENERATE_MIP_CHAIN_PASS], 0);
	RenderBackend.RenderViewportSurface(MipChain->get_surface_level(0));

	// Генерация уровней (Blur pass)
	for (u32 i = 1; i < MipChain->get_levels_count(); i++)
	{
		RenderBackend.set_Element(RenderTarget->s_reflections->E[SE_SSR_GENERATE_MIP_CHAIN_PASS], 1);

		u32 prev_mip = i - 1, prev_mip_width, prev_mip_height;
		MipChain->get_level_desc(prev_mip, prev_mip_width, prev_mip_height);
		RenderBackend.set_Constant("mip_data", (float)prev_mip, 0.0f, 1.0f / prev_mip_width, 1.0f / prev_mip_height);
		MipChain->get_level_desc(i, prev_mip_width, prev_mip_height);

		RenderBackend.RenderViewportSurface(MipChain->get_surface_level(i));
	}
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_reflections()
{
	RenderBackend.set_CullMode(CULL_DISABLE);
	RenderBackend.set_Stencil(FALSE);

	RenderBackend.set_Element(RenderTarget->s_reflections->E[SE_SSR_RENDER_PASS]);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_1);

	RenderBackend.set_Element(RenderTarget->s_reflections->E[SE_SSR_DENOISE_PASS]);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Reflections);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_screen_space_reflections()
{
	OPTICK_EVENT("CRender::render_screen_space_reflections");

	clear_reflections();

	if (ps_r_postprocess_flags.test(RFLAG_REFLECTIONS) && (ps_r_shading_mode != 0))
	{
		ReflectionsTexIsCleared = false;
		create_backbuffer_mip_chain();
		render_reflections();
	}
}
///////////////////////////////////////////////////////////////////////////////////
