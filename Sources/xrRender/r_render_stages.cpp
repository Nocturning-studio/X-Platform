////////////////////////////////////////////////////////////////////////////////
// Created: 19.03.2025
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_render_stages.h"
////////////////////////////////////////////////////////////////////////////////
void CRender::PrepareToRender()
{
	CalculateSceneVisibility();
}

void CRender::render_main(fmat4x4& view_projection, SceneGraphPacket& dest)
{
	PROFILE_FUNCTION();

	// Увеличиваем маркер кадра (легаси)
	SceneGraph.m_traversal_marker++;

	// Если текущий сектор не определен, рисуем только HUD и выходим.
	if (!pLastSector)
	{
		set_Object(nullptr);
		if (g_pGameLevel && (active_phase() != PHASE_SHADOW_DEPTH))
			g_pGameLevel->pHUD->Render_Last();
		return;
	}

	// -------------------------------------------------------------------------
	// 0. Настройка контекста (TLS)
	// -------------------------------------------------------------------------
	// Получаем уникальный маркер обхода для текущего вызова
	u32 current_marker = SceneGraph.m_traversal_marker.fetch_add(1) + 1;

	m_TraversalContext.frustum = &ViewBase;
	m_TraversalContext.traversal_marker_id = current_marker;
	m_TraversalContext.current_transform = &Fidentity;
	m_TraversalContext.render_phase = CRender::PHASE_NORMAL;

	// АКТИВИРУЕМ TLS:
	// Теперь все вызовы add_Visual/add_Geometry внутри этого скоупа
	// будут писать в переданный пакет 'dest'
	CurrentRenderContext::Scope tls_scope(dest, m_TraversalContext);

	// -------------------------------------------------------------------------
	// 1. Spatial Query (Пишем в dest)
	// -------------------------------------------------------------------------
	// Очистка не требуется, так как подразумевается, что dest.Clear() был вызван до render_main
	g_SpatialSpace->q_frustum(dest.m_spatial_query_results, ISpatial_DB::O_ORDERED,
							  STYPE_RENDERABLE | STYPE_LIGHTSOURCE, ViewBase);

	// -------------------------------------------------------------------------
	// 2. Sorting (Сортируем в dest)
	// -------------------------------------------------------------------------
	const fvec3 camera_pos = Engine.RenderView.Position;
	auto sort_predicate = [camera_pos](ISpatial* a, ISpatial* b) {
		float dist_a = a->spatial.sphere.P.distance_to_sqr(camera_pos);
		float dist_b = b->spatial.sphere.P.distance_to_sqr(camera_pos);
		return dist_a < dist_b;
	};

	if (!dest.m_spatial_query_results.empty())
	{
		concurrency::parallel_sort(dest.m_spatial_query_results.begin(), dest.m_spatial_query_results.end(),
								   sort_predicate);
	}

	// -------------------------------------------------------------------------
	// 3. Light Tracking
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
	// 4. Portal Traversal (Траверсер внутри dest)
	// -------------------------------------------------------------------------
	// Используем траверсер, привязанный к конкретному пакету
	dest.portal_traverser.Traverse(pLastSector, ViewBase, Engine.RenderView.Position, view_projection,
								   CPortalTraverser::VQ_HOM | CPortalTraverser::VQ_SSA | CPortalTraverser::VQ_FADE);

	// -------------------------------------------------------------------------
	// 5. Static Geometry (Берем из dest)
	// -------------------------------------------------------------------------
	const auto& visible_sectors = dest.portal_traverser.GetVisibleSectors();

	for (const auto& sec_vis : visible_sectors)
	{
		CSector* sector = sec_vis.sector;
		IRender_Visual* root_visual = sector->GetRootVisual();

		for (const auto& frustum : sec_vis.frustums)
		{
			set_Frustum((CFrustum*)&frustum);
			// add_Geometry сама возьмет dest из TLS (CurrentRenderContext)
			add_Geometry(root_visual);
		}
	}

	HOM.MT_SYNC();

	// -------------------------------------------------------------------------
	// 6. Dynamic Geometry & Lights (Берем из dest)
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
				// Примечание: Lights.add_light пишет в глобальный список.
				// При полном выносе в поток это место потребует защиты мьютексом
				// или своего буфера для lights. Пока оставляем как есть.
				if (HOM.visible(pLight->get_homdata()))
					Lights.add_light(pLight);
			}
			continue;
		}

		// --- Динамика ---
		if (!(spatial->spatial.type & STYPE_RENDERABLE))
			continue;

		IRenderable* renderable = spatial->dcast_Renderable();
		if (!renderable)
			continue;

		// 1. ФИЛЬТР HUD
		if (!sector)
		{
			float dist_sq = spatial->spatial.sphere.P.distance_to_sqr(Engine.RenderView.Position);
			if (dist_sq < 2.25f)
				continue;
		}

		// =====================================================================
		// 2. HOM OCCLUSION CULLING
		// =====================================================================
		vis_data& vis_orig = renderable->renderable.visual->vis;

		vis_data vis_temp = vis_orig;
		vis_temp.box.transform(renderable->renderable.transform);

		BOOL bVisible = HOM.visible(vis_temp);

		// Возвращаем обновленные тайминги обратно в оригинал
		vis_orig.hom_frame = vis_temp.hom_frame;
		vis_orig.hom_tested = vis_temp.hom_tested;

		if (bVisible)
		{
			// Рендерим
			m_TraversalContext.frustum = &ViewBase;
			set_Object(renderable);

			// add_Visual внутри вызовется с использованием dest из TLS
			renderable->renderable_Render();

			set_Object(nullptr);
		}
	}

	// Сброс контекста
	m_TraversalContext.frustum = nullptr;

	// 7. HUD Rendering
	if (g_pGameLevel && (active_phase() != PHASE_SHADOW_DEPTH))
		g_pGameLevel->pHUD->Render_Last();
}

void CRender::CalculateSceneVisibility()
{
	PROFILE_FUNCTION();

	// 1. Управление буферами (SWAP)
	// Переключаем индексы буферов один раз за кадр здесь.
	// Write - куда пишем сейчас. Read - откуда будем читать в фазах рендеринга.
	m_scene_write_ix = (m_scene_write_ix + 1) % 2;

	// В синхронном режиме (пока нет многопоточности) читаем из того же буфера, в который пишем.
	// При параллельном исполнении здесь будет: m_scene_read_ix = (m_scene_write_ix + 1) % 2;
	m_scene_read_ix = m_scene_write_ix;

	// -------------------------------------------------------------------------
	// PHASE A: GBuffer Gather (Priority 0 + Wallmarks)
	// -------------------------------------------------------------------------
	{
		MainSceneWorkItem& item = GetGBufferWriteItem();
		item.Clear();

		// Сохраняем матрицы для истории (чтобы Draw поток знал, как рисовать)
		item.view = Engine.RenderView.View;
		item.projection = Engine.RenderView.Project;
		item.view_projection = Engine.RenderView.ViewProjection;

		// Конфигурация сбора для GBuffer
		SceneGraphFetchConfig GBufferPassFetchConfig;
		GBufferPassFetchConfig.fetch_priority_0 = true;	 // Обычная геометрия
		GBufferPassFetchConfig.fetch_priority_1 = false; // Сложные материалы (потом)
		GBufferPassFetchConfig.fetch_wallmarks = true;	 // Воллмарки
		SceneGraph.SetFetchConfig(GBufferPassFetchConfig);

		// Устанавливаем фазу (влияет на выбор шейдеров в rimp_select_sh_...)
		set_active_phase(PHASE_NORMAL);

		// Сбор баундов для теней (только в основном проходе)
		if (m_need_render_sun)
			SceneGraph.SetCullingBoundsCollector(&main_coarse_structure);
		else
			SceneGraph.SetCullingBoundsCollector(NULL);

		// Сбор данных (запись в item.packet)
		render_main(item.view_projection, item.packet);

		// Очистка состояния
		SceneGraph.SetCullingBoundsCollector(NULL);
	}

	// -------------------------------------------------------------------------
	// PHASE B: Forward Gather (Priority 1 + Transparents)
	// -------------------------------------------------------------------------
	{
		MainSceneWorkItem& item = GetForwardWriteItem();
		item.Clear();

		// Сохраняем матрицы
		item.view = Engine.RenderView.View;
		item.projection = Engine.RenderView.Project;
		item.view_projection = Engine.RenderView.ViewProjection;

		// Конфигурация сбора для Forward
		SceneGraphFetchConfig ForwardPassFetchConfig;
		ForwardPassFetchConfig.fetch_priority_0 = false; // Уже отрисовали в GBuffer
		ForwardPassFetchConfig.fetch_priority_1 = true; // Геометрия со сложными шейдерами (Forward)
		ForwardPassFetchConfig.fetch_wallmarks = false; // Уже собрали (или не нужны здесь)
		SceneGraph.SetFetchConfig(ForwardPassFetchConfig);

		set_active_phase(PHASE_NORMAL);

		// Сбор данных (запись в item.packet)
		render_main(item.view_projection, item.packet);
	}

	// Восстанавливаем дефолтный конфиг на всякий случай
	SceneGraph.SetFetchConfig(SceneGraphFetchConfig(true, true, false));
}

IC float u_diffuse2s(float x, float y, float z)
{
	float v = (x + y + z) / 3.f;
	return ((v < 1) ? powf(v, 2.f / 3.f) : v);
}

bool CRender::need_render_sun()
{
	Fcolor sun_color = ((light*)Lights.sun_adapted._get())->get_color();
	return ps_r_lighting_flags.test(RFLAG_SUN) && (u_diffuse2s(sun_color.r, sun_color.g, sun_color.b) > EPS);
}

void CRender::render_gbuffer_primary()
{
	PROFILE_FUNCTION();

	// 1. Получаем данные для отрисовки (READ Item)
	// Данные уже были собраны в CalculateSceneVisibility
	MainSceneWorkItem& readItem = GetGBufferReadItem();

	// 2. DRAW PHASE
	{
		Engine.Statistic->RenderCALC_GBuffer.Begin();
		RenderBackendLegacy.enable_anisotropy_filtering();

		set_gbuffer();

		if (psDeviceFlags.test(rsWireframe))
			RenderBackendLegacy.SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

		// Используем пакет из readItem
		SceneGraph.Render(readItem.packet, SceneGraphRenderType::Opaque, 0);

		if (Details)
			Details->Render(DetailsRenderMode::Default);

		if (psDeviceFlags.test(rsWireframe))
			RenderBackendLegacy.SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

		RenderBackendLegacy.disable_anisotropy_filtering();
		Engine.Statistic->RenderCALC_GBuffer.End();
	}
}

void CRender::render_gbuffer_secondary()
{
	PROFILE_FUNCTION();

	// Используем уже собранные данные из Read Item (GBuffer packet)
	MainSceneWorkItem& readItem = GetGBufferReadItem();

	// Примечание: Portal Traverser обновляется в render_main,
	// поэтому он находится внутри readItem.packet.
	readItem.packet.portal_traverser.RenderFade();

	RenderBackendLegacy.enable_anisotropy_filtering();
	set_gbuffer();

	if (psDeviceFlags.test(rsWireframe))
		RenderBackendLegacy.SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	RenderBackendLegacy.set_ZWriteEnable(FALSE);

	// Рендерим из readItem.packet
	// LODs
	SceneGraph.Render(readItem.packet, SceneGraphRenderType::LOD, 0, true, true);

	// HUD (оружие) обычно рисуется поверх или в GBuffer, если нужно
	set_active_phase(PHASE_HUD);
	SceneGraph.Render(readItem.packet, SceneGraphRenderType::HUD);
	set_active_phase(PHASE_NORMAL);

	if (psDeviceFlags.test(rsWireframe))
		RenderBackendLegacy.SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	RenderBackendLegacy.disable_anisotropy_filtering();
}

void CRender::render_stage_forward()
{
	PROFILE_FUNCTION();

	// Берем данные из READ Item для Forward прохода
	MainSceneWorkItem& readItem =
		GetForwardWriteItem(); // ВНИМАНИЕ: Тут была ошибка в именовании в оригинале, должно быть GetForwardReadItem()

	// Исправляем на ReadItem:
	MainSceneWorkItem& currentReadItem = GetForwardReadItem();

	// Используем queue_distortion из текущего пакета, проверка на пустоту (ассерт)
	VERIFY(0 == currentReadItem.packet.queue_distortion.size());

	// Reuse списки очищаются внутри SceneGraph::Render или вручную, если нужно,
	// но здесь мы просто читаем.

	RenderBackendLegacy.set_Render_Target_Surface(RenderTarget->rt_Generic[1]);
	RenderBackendLegacy.set_Depth_Buffer(HW.GetBaseZB());
	RenderBackendLegacy.set_CullMode(CULL_BACKFACE);
	RenderBackendLegacy.set_Stencil(FALSE);

	// ============================================
	// PASS 1: Base Pass (Ambient + Texture + Hemi)
	// ============================================
	{
		RenderBackendLegacy.set_ColorWriteEnable();
		RenderBackendLegacy.set_ZWriteEnable(TRUE);

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
	// Смена фазы
	set_active_phase(PHASE_SUN_LIGHTING);

	RenderBackendLegacy.set_ColorWriteEnable();
	RenderBackendLegacy.SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
	RenderBackendLegacy.set_ZWriteEnable(FALSE);

	SceneTraversalContext reuse_ctx = m_TraversalContext;
	reuse_ctx.render_phase = PHASE_SUN_LIGHTING;

	SceneGraph.render_reuse(reuse_ctx, currentReadItem.packet);

	// DRAW
	SceneGraph.Render(currentReadItem.packet, SceneGraphRenderType::Opaque, 1);
	SceneGraph.Render(currentReadItem.packet, SceneGraphRenderType::Transparent);

	// ============================================
	// PASS 4: Debug
	// ============================================
	if (m_SunOccluder && ps_r_debug_flags.test(RFLAG_DRAW_SUN_OCCLUDERS))
		m_SunOccluder->Render();

	//debug_draw_light_volumes(Lights.package, Engine.RenderView.ViewProjection);
	//CPUOCC.DrawDebug();
	//CPUOCC.DebugRenderLightVolumes(Lights.package, Engine.RenderView.ViewProjection);
}

void CRender::render_sun()
{
	PROFILE_FUNCTION();

	Engine.Statistic->RenderCALC_SUN.Begin();

	RenderImplementation.stats.l_visible++;
	render_sun_cascades();
	dwLightMarkerID += 2;

	Engine.Statistic->RenderCALC_SUN.End();
}

void CRender::render_hom()
{
	PROFILE_FUNCTION();

	ViewBase.CreateFromMatrix(Engine.RenderView.ViewProjection, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);
	View = 0;

	if (!ps_render_flags.test(RFLAG_EXP_MT_CALC))
	{
		HOM.Enable();
		HOM.Render(ViewBase);
	}
}

void CRender::render_lights()
{
	PROFILE_FUNCTION();

	Engine.Statistic->RenderCALC_LIGHTS.Begin();

	//******* Occlusion testing of volume-limited light-sources
	render_stage_occlusion_culling();

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

void CRender::combine_scene()
{
	PROFILE_FUNCTION();

	if (ps_r_shading_mode == 1)
		render_bent_normals();

	if ((ps_r_shading_mode == 1) && ps_r_postprocess_flags.test(RFLAG_REFLECTIONS))
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
}

void CRender::render_postprocess()
{
	PROFILE_FUNCTION();

	Engine.Statistic->RenderCALC_POSTPROCESS.Begin();

	dummy_exposure();

	// Generic1 -> Generic0 -> Generic1
	//if (ps_r_postprocess_flags.test(RFLAG_AUTOEXPOSURE))
	//	render_autoexposure();

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
