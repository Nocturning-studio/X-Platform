////////////////////////////////////////////////////////////////////////////////
// Created: 19.03.2025
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_render_stages.h"
////////////////////////////////////////////////////////////////////////////////
void CRender::prepare_to_render()
{
	m_need_render_sun = need_render_sun();

	Scene.GetFrustumBase().CreateFromMatrix(Engine.RenderView.ViewProjection, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);
}

////////////////////////////////////////////////////////////////////////////////
//  update_light_tracking
//  Раз в кадр обновляет ROS для камеры + для одного renderable из выборки
//  (round-robin). Это НЕ часть видимости — просто «размазанная» работа.
//  Вызывается после ComputeVisibility, чтобы использовать уже готовую
//  выборку m_spatial_query_results.
////////////////////////////////////////////////////////////////////////////////
void CRender::update_light_tracking(SceneGraphPacket& packet)
{
	if(active_phase() != PHASE_NORMAL)
		return;

	set_Object(nullptr);
	uLastLTRACK++;

	const size_t renderable_count = packet.m_spatial_query_results.size();
	if(!renderable_count)
		return;

	const size_t light_track_id = uLastLTRACK % renderable_count;

	if(CObject* current_entity = g_pGameLevel->CurrentViewEntity())
	{
		if(CROS_impl* ros = (CROS_impl*)current_entity->ROS())
			ros->update(current_entity);
	}

	if(IRenderable* renderable = packet.m_spatial_query_results[light_track_id]->dcast_Renderable())
	{
		if(CROS_impl* ros = (CROS_impl*)renderable->renderable_ROS())
			ros->update(renderable);
	}
}

////////////////////////////////////////////////////////////////////////////////
//  MergeCulledLights
//  Переносит light'ы, отобранные Scene.ComputeVisibility, в общий пул.
////////////////////////////////////////////////////////////////////////////////
void CRender::MergeCulledLights(SceneGraphPacket& packet)
{
	if(packet.m_culled_lights.empty())
		return;

	CLight_DB& lights = Scene.GetLights();
	for(light* L : packet.m_culled_lights)
		lights.add_light(L);
	packet.m_culled_lights.clear();
}

////////////////////////////////////////////////////////////////////////////////
//  calculate_scene_culling
//
//  1. Готовим запрос видимости (HUD-only или full).
//  2. Scene.ComputeVisibility — spatial query + portal traverse + collect.
//  3. Post: light tracking, dynamic instances, merge lights, HUD.
////////////////////////////////////////////////////////////////////////////////
void CRender::calculate_scene_culling()
{
	PROFILE_FUNCTION();

	const bool has_sector = (pLastSector != nullptr);

	SSceneVisibilityRequest req;
	req.render_view = Engine.RenderView;
	req.traversal_position = Engine.RenderView.Position;
	req.use_traversal_position = true;
	req.start_sector = pLastSector;
	req.use_hom = true;
	req.use_feedback = false;
	req.render_phase = PHASE_NORMAL;
	req.frustum_override = &Scene.GetFrustumBase();

	if (has_sector)
	{
		req.flags = SceneRenderPresets::GatherMainView;
		req.culling_bounds = m_need_render_sun ? &main_coarse_structure : nullptr;
	}
	else
	{
		req.flags = SceneRenderPresets::HUDOnly;
		req.culling_bounds = nullptr;
	}

	set_active_phase(PHASE_NORMAL);

	Scene.ComputeVisibility(req, m_scene_visibility_data);

	if(has_sector)
	{
		update_light_tracking(m_scene_visibility_data.packet);
		MergeCulledLights(m_scene_visibility_data.packet);
	}

	if(g_pGameLevel && (active_phase() != PHASE_SHADOW_DEPTH))
	{
		CurrentRenderContext::Scope tls_scope(m_scene_visibility_data.packet, m_scene_visibility_data.context);
		g_pGameLevel->pHUD->Render_Last();
	}
}

IC float u_diffuse2s(float x, float y, float z)
{
	float v = (x + y + z) / 3.f;
	return ((v < 1) ? powf(v, 2.f / 3.f) : v);
}

bool CRender::need_render_sun()
{
	if (!g_pGameLevel)
		return false;

	light* sun = (light*)Scene.GetLights().sun_adapted._get();
	if (!sun)
		return false;

	Fcolor sun_color = sun->get_color();
	return ps_r_lighting_flags.test(RFLAG_SUN) && (u_diffuse2s(sun_color.r, sun_color.g, sun_color.b) > EPS);
}

void CRender::render_gbuffer()
{
	PROFILE_FUNCTION();

	Engine.Statistic->RenderCALC_GBuffer.Begin();
	RenderBackend.EnableAnisotropyFiltering();
	set_gbuffer();

	if (psDeviceFlags.test(rsWireframe))
		RenderBackend.SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	RenderBackend.SetDepthWriteEnable(TRUE);

	Scene.Render(m_scene_visibility_data, SceneRenderPresets::Opaque, true, true);

	if (Scene.GetDetails())
		Scene.RenderDetails(DetailsRenderMode::Default);

	set_active_phase(PHASE_HUD);
	Scene.Render(m_scene_visibility_data, SceneRenderPresets::HUDOnly);
	set_active_phase(PHASE_NORMAL);

	if (Scene.GetWallmarks())
	{
		RenderBackend.SetDepthWriteEnable(FALSE);
		render_wallmarks();
		Scene.RenderWallmarks();
	}

	if (psDeviceFlags.test(rsWireframe))
		RenderBackend.SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	RenderBackend.DisableAnisotropyFiltering();
	Engine.Statistic->RenderCALC_GBuffer.End();
}

void CRender::render_stage_forward()
{
	PROFILE_FUNCTION();

	RenderBackend.SetRenderTarget(RenderTarget->rt_Generic[1]);
	RenderBackend.SetDepthBuffer(RenderBackend.GetBaseZB());
	RenderBackend.SetCullMode(CULL_BACKFACE);
	RenderBackend.SetStencil(FALSE);

	RenderBackend.SetColorWriteEnable();
	RenderBackend.SetDepthWriteEnable(TRUE);

	set_active_phase(PHASE_NORMAL);

	Scene.Render(m_scene_visibility_data, SceneRenderPresets::RenderForwardStage);

	g_pGamePersistent->Environment().RenderThunderbolt();
	g_pGamePersistent->Environment().RenderRain();

	// Debug passes
	if (ps_r_debug_flags.test(RFLAG_DRAW_SUN_OCCLUDERS))
	{
		if (CSunOccluder* occ = Scene.GetSunOccluder())
			occ->Render();
	}

	if (ps_r_debug_flags.test(RFLAG_DRAW_HOM_OCCLUDERS))
		CPUOCC.DrawDebug();
}

void CRender::render_scene_to_gbuffer()
{
	PROFILE_FUNCTION();

	clear_gbuffer();

	render_gbuffer();
}

void CRender::render_sun()
{
	PROFILE_FUNCTION();

	if(!m_need_render_sun)
		return;

	Engine.Statistic->RenderCALC_SUN.Begin();

	RenderImplementation.stats.l_visible++;
	render_sun_cascades();
	dwLightMarkerID += 2;

	Engine.Statistic->RenderCALC_SUN.End();
}

void CRender::render_lights()
{
	PROFILE_FUNCTION();

	Engine.Statistic->RenderCALC_LIGHTS.Begin();

	//******* Occlusion testing of volume-limited light-sources
	render_stage_lights_culling();

	// Incremental shadow map visibility
	update_shadow_map_visibility();

	// Set render targets
	set_light_accumulator();

	// Lighting, non dependant on OCCQ
	render_lights(Scene.GetNormalLights());

	// Lighting, dependant on OCCQ
	render_lights(Scene.GetPendingLights());

	Engine.Statistic->RenderCALC_LIGHTS.End();
}

void CRender::render_postprocess()
{
	PROFILE_FUNCTION();

	Engine.Statistic->RenderCALC_POSTPROCESS.Begin();

	dummy_exposure();

	// Generic1 -> Generic0 -> Generic1
	if(ps_r_postprocess_flags.test(RFLAG_AUTOEXPOSURE))
		render_autoexposure();

	create_distortion_mask();

	render_distortion();

	render_bloom();

	// Generic1 -> Generic0 -> Generic1
	if(ps_r_postprocess_flags.test(RFLAG_DOF))
		render_depth_of_field();

	if(ps_render_flags.test(RFLAG_LENS_FLARES))
		g_pGamePersistent->Environment().RenderFlares();

	// Generic1 -> Generic0
	combine_additional_postprocess();

	// Radiation
	render_effectors_pass_generate_radiation_noise();

	//"Postprocess" params and colormapping (Generic_0 -> Generic_1)
	render_effectors_pass_combine();

	// Ceneric1 -> Generic1
	if(ps_r_postprocess_flags.test(RFLAG_MBLUR))
		render_motion_blur();

	// Generic_1 -> Generic_0
	render_effectors_pass_resolve_gamma();

	// Generic0 -> Generic1 -> Generic0
	if(ps_r_postprocess_flags.test(RFLAG_ANTI_ALIASING))
		render_antialiasing();

	// Generic_0 -> Generic_1
	render_effectors_pass_lut();

	// Ceneric1 -> Generic1
	if(ps_r_color_blind_mode)
		render_effectors_pass_color_blind_filter();

	// Generic1 -> Generic0
	render_screen_overlays();

	if(g_pGamePersistent)
		g_pGamePersistent->OnRenderPPUI_PP();

	Engine.Statistic->RenderCALC_POSTPROCESS.End();
}
////////////////////////////////////////////////////////////////////////////////
