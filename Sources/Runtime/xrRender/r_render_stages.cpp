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
	// Configure
	m_need_render_sun = need_render_sun();

	ViewBase.CreateFromMatrix(Engine.RenderView.ViewProjection, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);
	View = 0;
}

void CRender::gather_visibility(fmat4x4& view_projection, SceneGraphPacket& dest)
{
	PROFILE_FUNCTION();

	// Сброс флагов контекста
	m_TraversalContext.is_invisible_mode = FALSE;
	m_TraversalContext.is_hud_pass = FALSE;

	// Если текущий сектор не определен, рисуем только HUD и выходим.
	if (!pLastSector)
	{
		set_Object(nullptr);
		if (g_pGameLevel && (active_phase() != PHASE_SHADOW_DEPTH))
			g_pGameLevel->pHUD->Render_Last();
		return;
	}

	// -------------------------------------------------------------------------
	// Настройка контекста (TLS)
	// -------------------------------------------------------------------------
	// Получаем уникальный маркер обхода для текущего вызова
	u32 current_marker = ++SceneGraph.m_traversal_marker;

	m_TraversalContext.frustum = &ViewBase;
	m_TraversalContext.traversal_marker_id = current_marker;
	m_TraversalContext.transform = &Fidentity;
	m_TraversalContext.render_phase = CRender::PHASE_NORMAL;

	CurrentRenderContext::Scope tls_scope(dest, m_TraversalContext);

	// -------------------------------------------------------------------------
	// Spatial Query
	// -------------------------------------------------------------------------
	g_SpatialSpace->q_frustum(dest.m_spatial_query_results, ISpatial_DB::O_ORDERED, STYPE_RENDERABLE | STYPE_LIGHTSOURCE, ViewBase);

	// -------------------------------------------------------------------------
	// Sorting
	// -------------------------------------------------------------------------
#if 0
	const fvec3 camera_pos = m_TraversalContext.RenderView.Position;
	auto sort_predicate = [camera_pos](ISpatial* a, ISpatial* b) {
		float dist_a = a->spatial.sphere.P.distance_to_sqr(camera_pos);
		float dist_b = b->spatial.sphere.P.distance_to_sqr(camera_pos);
		return dist_a < dist_b;
	};

	if (!dest.m_spatial_query_results.empty())
	{
		std::sort(dest.m_spatial_query_results.begin(), dest.m_spatial_query_results.end(), sort_predicate);
	}
#endif

	// -------------------------------------------------------------------------
	// Light Tracking
	// -------------------------------------------------------------------------
	set_Object(nullptr);
	if (active_phase() == PHASE_NORMAL)
	{
		uLastLTRACK++;
		// Используем результаты из dest
		size_t renderable_count = dest.m_spatial_query_results.size();
		size_t light_track_id = 0xffffffff;

		if (renderable_count)
			light_track_id = uLastLTRACK % renderable_count;

		if (CObject* current_entity = g_pGameLevel->CurrentViewEntity())
		{
			if (CROS_impl* ros = (CROS_impl*)current_entity->ROS())
				ros->update(current_entity);
		}

		if (renderable_count)
		{
			// Используем результаты из dest
			if (IRenderable* renderable = dest.m_spatial_query_results[light_track_id]->dcast_Renderable())
			{
				if (CROS_impl* ros = (CROS_impl*)renderable->renderable_ROS())
					ros->update(renderable);
			}
		}
	}

	// -------------------------------------------------------------------------
	// Portal Traversal (Траверсер внутри dest)
	// -------------------------------------------------------------------------
	// Используем траверсер, привязанный к конкретному пакету
	dest.portal_traverser.Traverse(pLastSector, ViewBase, m_TraversalContext.RenderView.Position, view_projection, CPortalTraverser::VQ_HOM | CPortalTraverser::VQ_SSA | CPortalTraverser::VQ_FADE);

	// -------------------------------------------------------------------------
	// Static Geometry
	// -------------------------------------------------------------------------
	const auto& visible_sectors = dest.portal_traverser.GetVisibleSectors();

	dest.visible_sectors_map.clear();
	for (const auto& sec_vis : dest.portal_traverser.GetVisibleSectors())
	{
		dest.visible_sectors_map[sec_vis.sector] = &sec_vis;
	}

	for (const auto& sec_vis : visible_sectors)
	{
		CSector* sector = sec_vis.sector;
		IRender_Visual* root_visual = sector->GetRootVisual();

		for (const auto& frustum : sec_vis.frustums)
		{
			set_Frustum((CFrustum*)&frustum);
			add_Geometry(root_visual);
		}
	}

	// -------------------------------------------------------------------------
	// Dynamic Geometry & Lights
	// -------------------------------------------------------------------------
	for (ISpatial* spatial : dest.m_spatial_query_results)
	{
		spatial->spatial_updatesector();
		CSector* sector = (CSector*)spatial->spatial.sector;

		// --- Источники света ---
		if (spatial->spatial.type & STYPE_LIGHTSOURCE)
		{
			light* pLight = (light*)(spatial->dcast_Light());
			VERIFY(pLight);

			if (pLight->get_LOD() > EPS_L)
			{
				if (HOM.visible(pLight->get_homdata()))
					dest.m_culled_lights.push_back(pLight);
			}
			continue;
		}

		// --- Динамика ---
		if (!(spatial->spatial.type & STYPE_RENDERABLE))
			continue;

		IRenderable* renderable = spatial->dcast_Renderable();
		if (!renderable)
			continue;

		auto it = dest.visible_sectors_map.find(sector);
		if (it == dest.visible_sectors_map.end())
			continue;

		const CPortalTraverser::SectorVisibility* active_vis_data = it->second;

		// Проверяем попадание объекта в подфрустумы сектора
		bool bInFrustum = false;
		for (const auto& frustum : active_vis_data->frustums)
		{
			if (frustum.testSphere_dirty(spatial->spatial.sphere.P, spatial->spatial.sphere.R))
			{
				bInFrustum = true;
				break;
			}
		}
		if (!bInFrustum) continue;

		// ФИЛЬТР HUD
		if (!sector)
		{
			float dist_sq = spatial->spatial.sphere.P.distance_to_sqr(m_TraversalContext.RenderView.Position);
			if (dist_sq < 2.25f)
				continue;
		}

		// =====================================================================
		// HOM OCCLUSION CULLING
		// =====================================================================
		vis_data& vis_orig = renderable->renderable.visual->vis;

		vis_data vis_temp = vis_orig;
		vis_temp.box.transform(renderable->renderable.transform);

		BOOL bVisible = HOM.visible(vis_temp);

		// Возвращаем обновленные тайминги обратно в оригинал
		vis_orig.hom_frame = vis_temp.hom_frame;
		vis_orig.hom_tested = vis_temp.hom_tested;

		if (bVisible)
			dest.m_culled_dynamics.push_back(renderable);
	}

	// Сброс контекста
	m_TraversalContext.frustum = nullptr;
}

void CRender::MergeCulledLights(SceneGraphPacket& packet)
{
	if (packet.m_culled_lights.empty()) return;
	for (light* L : packet.m_culled_lights)
		Lights.add_light(L);
	packet.m_culled_lights.clear();
}

void CRender::calculate_scene_culling()
{
	PROFILE_FUNCTION();

	// Очищаем пакет перед новым сбором
	m_scene_data.Clear();

	if (!pLastSector)
	{
		// Если сектор не определён, собираем только HUD
		m_scene_data.view = Engine.RenderView.View;
		m_scene_data.projection = Engine.RenderView.Project;
		m_scene_data.view_projection = Engine.RenderView.ViewProjection;

		{
			SceneGraphFetchConfig hud_config(true, false, false);
			m_TraversalContext.fetch_config = hud_config;
			set_active_phase(PHASE_NORMAL);

			CurrentRenderContext::Scope tls_scope(m_scene_data.packet, m_TraversalContext);
			if (g_pGameLevel && (active_phase() != PHASE_SHADOW_DEPTH))
				g_pGameLevel->pHUD->Render_Last();
		}

		m_TraversalContext.fetch_config = SceneGraphFetchConfig(true, true, false);
		return;
	}

	// Сохраняем матрицы
	m_scene_data.view = Engine.RenderView.View;
	m_scene_data.projection = Engine.RenderView.Project;
	m_scene_data.view_projection = Engine.RenderView.ViewProjection;

	// Конфигурация: собираем все приоритеты и декали
	SceneGraphFetchConfig config;
	config.fetch_priority_0 = true;
	config.fetch_priority_1 = true;
	config.fetch_wallmarks = true;

	set_active_phase(PHASE_NORMAL);

	m_TraversalContext.RenderView = Engine.RenderView;
	m_TraversalContext.use_hom = true;
	m_TraversalContext.use_feedback = false;
	m_TraversalContext.fetch_config = SceneGraphFetchConfig(true, true, true);
	m_TraversalContext.culling_bounds = (m_need_render_sun) ? &main_coarse_structure : nullptr;

	// Обход сцены
	gather_visibility(m_scene_data.view_projection, m_scene_data.packet);
	SceneGraph.PrepareDynamicInstances(m_scene_data.packet, m_TraversalContext);
	MergeCulledLights(m_scene_data.packet);

	// HUD тоже попадает в этот пакет
	{
		CurrentRenderContext::Scope tls_scope(m_scene_data.packet, m_TraversalContext);
		if (g_pGameLevel && (active_phase() != PHASE_SHADOW_DEPTH))
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

	Fcolor sun_color = ((light*)Lights.sun_adapted._get())->get_color();
	return ps_r_lighting_flags.test(RFLAG_SUN) && (u_diffuse2s(sun_color.r, sun_color.g, sun_color.b) > EPS);
}

void CRender::render_gbuffer_primary()
{
	PROFILE_FUNCTION();

	MainSceneWorkItem& readItem = m_scene_data;

	Engine.Statistic->RenderCALC_GBuffer.Begin();
	RenderBackend.enable_anisotropy_filtering();

	set_gbuffer();

	if (psDeviceFlags.test(rsWireframe))
		RenderBackend.SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	SceneGraph.Render(readItem.packet, SceneGraphRenderType::Opaque, 0);

	if (Details)
		Details->Render(DetailsRenderMode::Default);

	if (psDeviceFlags.test(rsWireframe))
		RenderBackend.SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	RenderBackend.disable_anisotropy_filtering();
	Engine.Statistic->RenderCALC_GBuffer.End();
}

void CRender::render_gbuffer_secondary()
{
	PROFILE_FUNCTION();

	MainSceneWorkItem& readItem = m_scene_data;

	RenderBackend.enable_anisotropy_filtering();
	set_gbuffer();

	if (psDeviceFlags.test(rsWireframe))
		RenderBackend.SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	RenderBackend.set_ZWriteEnable(FALSE);

	SceneGraph.Render(readItem.packet, SceneGraphRenderType::LOD, 0, true, true);

	set_active_phase(PHASE_HUD);
	SceneGraph.Render(readItem.packet, SceneGraphRenderType::HUD);
	set_active_phase(PHASE_NORMAL);

	if (psDeviceFlags.test(rsWireframe))
		RenderBackend.SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	RenderBackend.disable_anisotropy_filtering();
}

void CRender::render_stage_forward()
{
	PROFILE_FUNCTION();

	// Берем данные из READ Item для Forward прохода
	MainSceneWorkItem& currentReadItem = m_scene_data;

	RenderBackend.set_Render_Target_Surface(RenderTarget->rt_Generic[1]);
	RenderBackend.set_Depth_Buffer(RenderBackend.GetBaseZB());
	RenderBackend.set_CullMode(CULL_BACKFACE);
	RenderBackend.set_Stencil(FALSE);

	// ============================================
	// PASS 1: Base Pass (Ambient + Texture + Hemi)
	// ============================================
	{
		RenderBackend.set_ColorWriteEnable();
		RenderBackend.set_ZWriteEnable(TRUE);

		set_active_phase(PHASE_NORMAL);

		// Рендерим собранное (Priority 1)
		SceneGraph.Render(currentReadItem.packet, SceneGraphRenderType::Opaque, 1);
		SceneGraph.Render(currentReadItem.packet, SceneGraphRenderType::Transparent);

		g_pGamePersistent->Environment().RenderThunderbolt();
		g_pGamePersistent->Environment().RenderRain();
	}

	// ============================================
	// PASS 3: Sun Light (Reuse)
	// ============================================
	/*
	// Смена фазы
	set_active_phase(PHASE_SUN_LIGHTING);

	RenderBackend.set_ColorWriteEnable();
	RenderBackend.SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
	RenderBackend.set_ZWriteEnable(FALSE);

	SceneTraversalContext reuse_ctx = m_TraversalContext;
	reuse_ctx.render_phase = PHASE_SUN_LIGHTING;

	SceneGraph.RenderFromCache(reuse_ctx, currentReadItem.packet);

	// DRAW
	SceneGraph.Render(currentReadItem.packet, SceneGraphRenderType::Opaque, 1);
	SceneGraph.Render(currentReadItem.packet, SceneGraphRenderType::Transparent);
	*/

	// ============================================
	// PASS 4: Debug
	// ============================================
	if (ps_r_debug_flags.test(RFLAG_DRAW_SUN_OCCLUDERS))
		m_SunOccluder->Render();

	if (ps_r_debug_flags.test(RFLAG_DRAW_HOM_OCCLUDERS))
		CPUOCC.DrawDebug();
}

void CRender::render_scene_to_gbuffer()
{
	PROFILE_FUNCTION();

	clear_gbuffer();

	//******* Main render :: PART-0	-- first
	render_gbuffer_primary();

	//******* Main render :: PART-1 (second)
	render_gbuffer_secondary();

	// Wall marks
	if (Wallmarks)
	{
		render_wallmarks();
		Wallmarks->Render(); // wallmarks has priority as normal geometry
	}
}

void CRender::render_sun()
{
	PROFILE_FUNCTION();

	if (!m_need_render_sun)
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
	render_lights(LP_normal);

	// Lighting, dependant on OCCQ
	render_lights(LP_pending);

	Engine.Statistic->RenderCALC_LIGHTS.End();
}

void CRender::render_postprocess()
{
	PROFILE_FUNCTION();

	Engine.Statistic->RenderCALC_POSTPROCESS.Begin();

	dummy_exposure();

	// Generic1 -> Generic0 -> Generic1
	if (ps_r_postprocess_flags.test(RFLAG_AUTOEXPOSURE))
		render_autoexposure();

	create_distortion_mask();

	render_distortion();

	render_bloom();

	// Generic1 -> Generic0 -> Generic1
	if (ps_r_postprocess_flags.test(RFLAG_DOF))
		render_depth_of_field();

	if (ps_render_flags.test(RFLAG_LENS_FLARES))
		g_pGamePersistent->Environment().RenderFlares();

	// Generic1 -> Generic0
	combine_additional_postprocess();

	//Radiation
	render_effectors_pass_generate_radiation_noise();

	//"Postprocess" params and colormapping (Generic_0 -> Generic_1)
	render_effectors_pass_combine();

	// Ceneric1 -> Generic1
	if (ps_r_postprocess_flags.test(RFLAG_MBLUR))
		render_motion_blur();

	//Generic_1 -> Generic_0
	render_effectors_pass_resolve_gamma();

	// Generic0 -> Generic1 -> Generic0
	if (ps_r_postprocess_flags.test(RFLAG_ANTI_ALIASING))
		render_antialiasing();

	//Generic_0 -> Generic_1
	render_effectors_pass_lut();

	// Ceneric1 -> Generic1
	if (ps_r_color_blind_mode)
		render_effectors_pass_color_blind_filter();

	// Generic1 -> Generic0
	render_screen_overlays();

	if (g_pGamePersistent)
		g_pGamePersistent->OnRenderPPUI_PP();

	Engine.Statistic->RenderCALC_POSTPROCESS.End();
}
////////////////////////////////////////////////////////////////////////////////
