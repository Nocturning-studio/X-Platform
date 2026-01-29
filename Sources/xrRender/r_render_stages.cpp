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
}

void CRender::render_main(Fmatrix& view_projection, SceneGraphPacket& dest)
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
	const Fvector camera_pos = Engine.RenderView.Position;
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

	// 1. Управление буферами (SWAP)
	// Переключаем кадр. В будущем это будет в начале кадра.
	m_scene_write_ix = (m_scene_write_ix + 1) % 2;
	m_scene_read_ix = m_scene_write_ix; // Пока читаем то, что пишем

	// Получаем рабочие элементы
	MainSceneWorkItem& writeItem = GetGBufferWriteItem();
	MainSceneWorkItem& readItem = GetGBufferReadItem();

	writeItem.Clear();

	// Сохраняем матрицы для истории (чтобы Draw поток знал, как рисовать)
	writeItem.view = Engine.RenderView.View;
	writeItem.projection = Engine.RenderView.Project;
	writeItem.view_projection = Engine.RenderView.ViewProjection;

	// 2. GATHER PHASE (Пишем в writeItem)
	{
		// Конфигурация сбора
		SceneGraphFetchConfig GBufferPassFetchConfig;
		GBufferPassFetchConfig.fetch_priority_0 = true;
		GBufferPassFetchConfig.fetch_priority_1 = false;
		GBufferPassFetchConfig.fetch_wallmarks = true;
		SceneGraph.SetFetchConfig(GBufferPassFetchConfig);

		set_active_phase(PHASE_NORMAL);

		if (m_need_render_sun)
			SceneGraph.SetCullingBoundsCollector(&main_coarse_structure);
		else
			SceneGraph.SetCullingBoundsCollector(NULL);

		// ВЫЗЫВАЕМ ОБНОВЛЕННЫЙ render_main, передаем пакет
		render_main(writeItem.view_projection, writeItem.packet);

		SceneGraph.SetCullingBoundsCollector(NULL);

		// Сброс конфига
		GBufferPassFetchConfig.fetch_wallmarks = false;
		SceneGraph.SetFetchConfig(GBufferPassFetchConfig);
	}

	// Здесь может быть точка синхронизации, если Gather будет в другом потоке

	// 3. DRAW PHASE (Читаем из readItem)
	{
		Engine.Statistic->RenderCALC_GBuffer.Begin();
		RenderBackend.enable_anisotropy_filtering();

		set_gbuffer();

		if (psDeviceFlags.test(rsWireframe))
			RenderBackend.SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

		// Используем пакет из readItem
		SceneGraph.Render(readItem.packet, SceneGraphRenderType::Opaque, 0);

		if (Details)
			Details->Render(DetailsRenderMode::Default);

		if (psDeviceFlags.test(rsWireframe))
			RenderBackend.SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

		RenderBackend.disable_anisotropy_filtering();
		Engine.Statistic->RenderCALC_GBuffer.End();
	}
}

void CRender::render_gbuffer_secondary()
{
	PROFILE_FUNCTION();

	// Используем уже собранные данные из Read Item
	MainSceneWorkItem& readItem = GetGBufferReadItem();

	SceneGraph.m_packet.portal_traverser.RenderFade(); // Тут возможно надо брать traverser из readItem, если RenderFade зависит от него

	RenderBackend.enable_anisotropy_filtering();
	set_gbuffer();

	if (psDeviceFlags.test(rsWireframe))
		RenderBackend.SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	RenderBackend.set_ZWriteEnable(FALSE);

	// Рендерим из readItem.packet
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

	MainSceneWorkItem& writeItem = GetForwardWriteItem();
	MainSceneWorkItem& readItem = GetForwardReadItem();

	writeItem.Clear();
	writeItem.view_projection = Engine.RenderView.ViewProjection;

	// Используем m_packet.queue_distortion
	VERIFY(0 == SceneGraph.m_packet.queue_distortion.size());

	// Очищаем списки с прошлого кадра
	// Используем m_packet для списков Reuse
	SceneGraph.m_packet.m_visuals_static_visible.clear();
	SceneGraph.m_packet.m_visuals_dynamic_visible.clear();

	RenderBackend.set_Render_Target_Surface(RenderTarget->rt_Generic[1]);
	RenderBackend.set_Depth_Buffer(HW.pBaseZB);
	RenderBackend.set_CullMode(CULL_BACKFACE);
	RenderBackend.set_Stencil(FALSE);

	// ============================================
	// PASS 1: Base Pass (Ambient + Texture + Hemi)
	// ============================================
	{
		// Настраиваем состояния: Базовый проход ПИШЕТ цвет и Z
		RenderBackend.set_ColorWriteEnable();
		RenderBackend.set_ZWriteEnable(TRUE);

		SceneGraphFetchConfig ForwardPassFetchConfig;

		ForwardPassFetchConfig.fetch_priority_0 = false;
		ForwardPassFetchConfig.fetch_priority_1 = true;
		ForwardPassFetchConfig.fetch_wallmarks = false;

		SceneGraph.SetFetchConfig(ForwardPassFetchConfig);

		set_active_phase(PHASE_NORMAL);

		render_main(writeItem.view_projection, writeItem.packet);

		SceneGraph.Render(readItem.packet, SceneGraphRenderType::Opaque, 1);
		SceneGraph.Render(readItem.packet, SceneGraphRenderType::Transparent);

		g_pGamePersistent->Environment().RenderThunderbolt();
		g_pGamePersistent->Environment().RenderRain();
	}

	// ============================================
	// PASS 3: Sun Light
	// ============================================
	// Смена фазы
	set_active_phase(PHASE_SUN_LIGHTING);

	RenderBackend.set_ColorWriteEnable();

	RenderBackend.SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);

	// Z-Write должен быть FALSE для аддитивного солнца
	// Иначе оно будет перезаписывать глубину и "бороться" с базовой геометрией.
	// Шейдеры l_sun обычно используют blend one/one или dest_color/src_color
	RenderBackend.set_ZWriteEnable(FALSE);

	SceneGraph.render_reuse(m_TraversalContext, readItem.packet);

	// DRAW
	SceneGraph.Render(readItem.packet, SceneGraphRenderType::Opaque, 1);
	SceneGraph.Render(readItem.packet, SceneGraphRenderType::Transparent);

	// ============================================
	// PASS 4: Debug
	// ============================================

	if (m_SunOccluder && ps_r_debug_flags.test(RFLAG_DRAW_SUN_OCCLUDERS))
		m_SunOccluder->Render();
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

void CRender::render_stage_occlusion_culling()
{
	PROFILE_FUNCTION();

	phase_occq();

	LP_normal.clear();
	LP_pending.clear();

	{
		////OPTICK_EVENT("CRender::OcclusionCulling-Tests");

		light_Package& LP = Lights.package;

		// Быстрая статистика
		stats.l_shadowed = LP.v_shadowed.size();
		stats.l_unshadowed = LP.v_point.size() + LP.v_spot.size();
		stats.l_total = stats.l_shadowed + stats.l_unshadowed;

		// ВОССТАНАВЛИВАЕМ ПОСЛЕДОВАТЕЛЬНУЮ ОБРАБОТКУ для vis_prepare
		// т.к. vis_prepare работает с occlusion queries и не является потокобезопасной
		auto process_lights_safe = [&](auto& light_array, auto& pending_array, auto& normal_array) {
			const size_t count = light_array.size();

			// ТОЛЬКО последовательная обработка для vis_prepare
			for (size_t it = 0; it < count; it++)
			{
				light* L = light_array[it];
				L->vis_prepare(); // Эта операция должна быть последовательной!
				if (L->VisibilityData.pending)
					pending_array.push_back(L);
				else
					normal_array.push_back(L);
			}
		};

		// Обработка всех типов источников света (последовательно для безопасности)
		process_lights_safe(LP.v_point, LP_pending.v_point, LP_normal.v_point);
		process_lights_safe(LP.v_spot, LP_pending.v_spot, LP_normal.v_spot);
		process_lights_safe(LP.v_shadowed, LP_pending.v_shadowed, LP_normal.v_shadowed);
	}

	// Оптимизированная сортировка (может остаться параллельной)
	auto parallel_sort_if_large = [](auto& container) {
		if (container.size() > 20)
		{
			concurrency::parallel_sort(container.begin(), container.end());
		}
		else
		{
			std::sort(container.begin(), container.end());
		}
	};

	parallel_sort_if_large(LP_normal.v_point);
	parallel_sort_if_large(LP_normal.v_spot);
	parallel_sort_if_large(LP_normal.v_shadowed);
	parallel_sort_if_large(LP_pending.v_point);
	parallel_sort_if_large(LP_pending.v_spot);
	parallel_sort_if_large(LP_pending.v_shadowed);
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

void CRender::query_wait()
{
	PROFILE_FUNCTION();

	Engine.Statistic->RenderDUMP_Wait_S.Begin();

	CTimer Timer;
	Timer.Start();

	BOOL result = FALSE;
	HRESULT hr = S_FALSE;

	while ((hr = q_sync_point[q_sync_count]->GetData(&result, sizeof(result), D3DGETDATA_FLUSH)) == S_FALSE)
	{
		if (!SwitchToThread())
			Sleep(ps_r_thread_wait_sleep);

		if (Timer.GetElapsed_ms() > 100)
		{
			result = FALSE;
			break;
		}
	}

	Engine.Statistic->RenderDUMP_Wait_S.End();

	q_sync_count = (q_sync_count + 1) % HW.Caps.iGPUNum;
	CHK_DX(q_sync_point[q_sync_count]->Issue(D3DISSUE_END));
}

void CRender::update_shadow_map_visibility()
{
	PROFILE_FUNCTION();

	if (Lights_LastFrame.empty())
		return;

	// Безопасная обработка с проверкой валидности
	for (auto it = Lights_LastFrame.begin(); it != Lights_LastFrame.end();)
	{
		light* L = *it;
		if (L == nullptr)
		{
			it = Lights_LastFrame.erase(it);
			continue;
		}

		try
		{
			L->get_smapvis().flushoccq();
			++it;
		}
		catch (...)
		{
			Msg("! Failed to flush-OCCq on light %p", L);
			it = Lights_LastFrame.erase(it);
		}
	}
}

void CRender::render_lights()
{
	PROFILE_FUNCTION();

	Engine.Statistic->RenderCALC_LIGHTS.Begin();

	//******* Occlusion testing of volume-limited light-sources
	render_stage_occlusion_culling();

	// Update incremental shadowmap-visibility solver
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
