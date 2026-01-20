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
	}

	PrepareToRender();

	// Configure
	m_need_render_sun = need_render_sun();

	// HOM
	render_hom();

	//*******
	// Sync point
	query_wait();

	clear_gbuffer();

	//******* Z-prefill calc - DEFERRER RENDERER
	if (ps_r_ls_flags.test(RFLAG_Z_PREPASS))
		render_depth_prepass();

	//******* Main render :: PART-0	-- first
	render_gbuffer_primary();

	//******* Main render :: PART-1 (second)
	render_gbuffer_secondary();

	if (m_bFirstFrameAfterReset)
	{
		m_bFirstFrameAfterReset = false;
		motion_blur_pass_save_depth();
		return;
	}

	// Wall marks
	if (Wallmarks)
	{
		render_wallmarks();
		Wallmarks->Render(); // wallmarks has priority as normal geometry
	}

	// Directional light - sun
	if (m_need_render_sun)
		render_sun();

	// Omni/Spot lights
	render_lights();

	HOM.Disable();

	render_ambient_occlusion();

	combine_scene();

	render_postprocess();

	if (g_pGamePersistent)
		g_pGamePersistent->OnRenderPPUI_main();

	output_frame_to_screen();

	m_saved_viewproj.set(Engine.RenderView.ViewProjection);
	m_saved_invview.invert(Engine.RenderView.View);

	Details->ClearVisible();
}
////////////////////////////////////////////////////////////////////////////////
