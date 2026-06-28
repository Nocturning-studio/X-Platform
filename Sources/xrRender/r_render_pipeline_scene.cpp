////////////////////////////////////////////////////////////////////////////////
// Created: 16.03.2025
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_render_pipeline.h"
////////////////////////////////////////////////////////////////////////////////
void CRender::RenderScene()
{
	PROFILE_FUNCTION();

	if (m_bFirstFrameAfterReset)
	{
		m_saved_viewproj.set(Engine.RenderView.ViewProjection);
		m_saved_invview.invert(Engine.RenderView.View);
		m_bFirstFrameAfterReset = false;
	}

	prepare_to_render();

	HOM.Enable();

	calculate_scene_culling();

	render_scene_to_gbuffer();

	HOM.Disable();

	// Directional light - sun
	render_sun();

	// Omni/Spot lights
	render_lights();

	render_ambient_occlusion();

	//if (ps_r_shading_mode == SHADING_MODE_PBR)
		//render_bent_normals();

	if ((ps_r_shading_mode == SHADING_MODE_PBR) && ps_r_postprocess_flags.test(RFLAG_REFLECTIONS))
	{
		create_hi_z_mip_chain();

		precombine_scene();

		render_screen_space_reflections();
	}

	render_skybox();

	combine_scene_lighting();

	render_stage_forward();

	//if (ps_r_lighting_flags.test(RFLAG_SUN_SHAFTS))
	combine_sun_shafts();

	render_postprocess();

	if (g_pGamePersistent)
		g_pGamePersistent->OnRenderPPUI_main();

	if(!m_bFirstFrameAfterReset)
		output_frame_to_screen();

	m_saved_viewproj.set(Engine.RenderView.ViewProjection);
	m_saved_invview.invert(Engine.RenderView.View);
}
////////////////////////////////////////////////////////////////////////////////
