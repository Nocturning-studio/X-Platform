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

static ref_shader g_Shader = nullptr;
void CRender::debug_draw_light_volumes(const light_Package& package, const fmat4x4& VP)
{
	// Отключаем запись в глубину и тест глубины, чтобы видеть сквозь стены
	RenderBackendLegacy.set_ZWriteEnable(FALSE);
	RenderBackendLegacy.SetRenderState(D3DRS_ZENABLE, FALSE);
	RenderBackendLegacy.set_CullMode(CULL_DISABLE);

	if(g_Shader == nullptr)
		g_Shader.create("sun_occluder");

	RenderBackendLegacy.set_Shader(g_Shader);

	// Собираем все источники из Lights_LastFrame (или из Lights.package)
	auto draw_list = [&](const xr_vector<light*>& lights)
	{
		for (light* L : lights)
		{
			if (!L) continue;

			RenderBackendLegacy.set_transform_world(L->get_transform());
			RenderBackendLegacy.set_transform_view(Engine.RenderView.View);
			RenderBackendLegacy.set_transform_project(Engine.RenderView.Project);

			draw_volume(L);
		}
	};

	draw_list(package.v_point);
	draw_list(package.v_spot);
	draw_list(package.v_shadowed);

	RenderBackendLegacy.set_ZWriteEnable(TRUE);
	RenderBackendLegacy.SetRenderState(D3DRS_ZENABLE, TRUE);
	RenderBackendLegacy.set_CullMode(CULL_BACKFACE);
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

//#define DEBUG_LIGHTS_CULLING

void CRender::render_stage_occlusion_culling()
{
	OPTICK_EVENT("render_stage_occlusion_culling");
	PROFILE_FUNCTION();

	const u32 frame = Engine.TimeManager.GetFrameCount();
	const fvec3& cam_pos = Engine.RenderView.Position;
	const float max_dist_sq = ps_r_light_distance_cull * ps_r_light_distance_cull;

#ifdef DEBUG_LIGHTS_CULLING
	Msg("[CPU-OCC] === Frame %d BEGIN ===", frame);
	Msg("[CPU-OCC] Lights in package: point=%d spot=%d shadowed=%d",
		Lights.package.v_point.size(), Lights.package.v_spot.size(), Lights.package.v_shadowed.size());
#endif

	// --------------------------------------------------------------------
	// 0. Сбор результатов предыдущего кадра (CPU occlusion)
	// --------------------------------------------------------------------
	{
		OPTICK_EVENT("collect_cpu_query_results");
		if (CPUOCC.IsQueryReady())
		{
#ifdef DEBUG_LIGHTS_CULLING
			Msg("[CPU-OCC] Query ready, processing %d pending lights", m_cpu_occ_pending_lights.size());
#endif
			for (light* L : m_cpu_occ_pending_lights)
			{
				if (!L || !L->VisibilityData.pending)
					continue;

				u32 samples = CPUOCC.GetVisibleSamples(L->VisibilityData.query_id);
				if (samples != 0xfffffffe)
				{
					bool newVisible = (samples > ps_r_light_fragments_cull);
					L->VisibilityData.visible = newVisible;
					L->VisibilityData.pending = false;

					if (newVisible)
						L->VisibilityData.frame2test = frame + ::Random.randI(delay_large_min, delay_large_max);
					else
						L->VisibilityData.frame2test = frame + ::Random.randI(5, 10);

#ifdef DEBUG_LIGHTS_CULLING
					Msg("[CPU-OCC]   Light %p (type %d) query_id=%u samples=%u -> %s",
						L, L->LightFlags.type, L->VisibilityData.query_id, samples,
						newVisible ? "VISIBLE" : "CULLED");
#endif
				}
			}
			CPUOCC.ResetPendingQuery();
		}
		m_cpu_occ_pending_lights.clear();
	}

	// --------------------------------------------------------------------
	// 1. Очистка пакетов
	// --------------------------------------------------------------------
	{
		OPTICK_EVENT("clear_packages");
		LP_normal.clear();
		LP_pending.clear();
	}

	light_Package& LP = Lights.package;

	stats.l_shadowed = LP.v_shadowed.size();
	stats.l_unshadowed = LP.v_point.size() + LP.v_spot.size();
	stats.l_total = stats.l_shadowed + stats.l_unshadowed;

	// --------------------------------------------------------------------
	// 2. Заполнение depth buffer и начало нового occlusion query
	// --------------------------------------------------------------------
	{
		OPTICK_EVENT("build_depth_and_begin_query");

		// Ожидание готовности depth-буфера и свап
		CPUOCC.WaitForBuildAndSwap();

		// Запускаем новое заполнение для следующего кадра
		CPUOCC.BuildDepthBuffer(Engine.RenderView.ViewProjection);

		// Теперь начинаем запросы (буфер уже актуален)
		SoftX::Viewport vp(0.0f, 0.0f, 512, 512, 0.0f, 1.0f);
		CPUOCC.BeginOcclusionQueries(Engine.RenderView.ViewProjection, vp);
	}

	// --------------------------------------------------------------------
	// 3. Подготовка источников (классификация)
	// --------------------------------------------------------------------
	struct LightPrepareResult
	{
		light* L;
		bool needs_query;
		bool pending;
	};

	xr_vector<LightPrepareResult> prepared_point, prepared_spot, prepared_shadowed;
	prepared_point.reserve(LP.v_point.size());
	prepared_spot.reserve(LP.v_spot.size());
	prepared_shadowed.reserve(LP.v_shadowed.size());

	auto prepare_one = [&](light* L) -> LightPrepareResult
	{
		float dist_sq = cam_pos.distance_to_sqr(L->spatial.sphere.P);
		if (dist_sq > max_dist_sq)
		{
			L->VisibilityData.visible = false;
			L->VisibilityData.pending = false;
			L->VisibilityData.frame2test = frame + ::Random.randI(10, 20);
#ifdef DEBUG_LIGHTS_CULLING
			Msg("[CPU-OCC] PREP: light %p culled by distance (dist %.1f > max %.1f)",
				L, sqrtf(dist_sq), sqrtf(max_dist_sq));
#endif
			return { L, false, false };
		}

#ifdef DEBUG_LIGHTS_CULLING
		Msg("[CPU-OCC] PREP: light %p type %d dist %.1f frame2test %d (curr %d) shadow=%d",
			L, L->LightFlags.type, sqrtf(dist_sq), L->VisibilityData.frame2test, frame, L->LightFlags.bShadow);
#endif

		bool needs_q = L->vis_prepare(frame);

#ifdef DEBUG_LIGHTS_CULLING
		Msg("[CPU-OCC]   -> needs_q=%d pending=%d visible=%d", needs_q, L->VisibilityData.pending, L->VisibilityData.visible);
#endif
		return { L, needs_q, L->VisibilityData.pending };
	};

	const size_t total_lights = LP.v_point.size() + LP.v_spot.size() + LP.v_shadowed.size();
	{
		OPTICK_EVENT("Prepare lights (parallel/sequential)");
		if (total_lights > 64)
		{
			concurrency::parallel_invoke(
				[&]() {
					OPTICK_EVENT("prepare_point");
					for (light* L : LP.v_point) prepared_point.push_back(prepare_one(L));
				},
				[&]() {
					OPTICK_EVENT("prepare_spot");
					for (light* L : LP.v_spot) prepared_spot.push_back(prepare_one(L));
				},
					[&]() {
					OPTICK_EVENT("prepare_shadowed");
					for (light* L : LP.v_shadowed) prepared_shadowed.push_back(prepare_one(L));
				}
				);
		}
		else
		{
			OPTICK_EVENT("prepare_sequential");
			for (light* L : LP.v_point)    prepared_point.push_back(prepare_one(L));
			for (light* L : LP.v_spot)     prepared_spot.push_back(prepare_one(L));
			for (light* L : LP.v_shadowed) prepared_shadowed.push_back(prepare_one(L));
		}
	}

	// --------------------------------------------------------------------
	// 4. Выдача CPU occlusion запросов
	// --------------------------------------------------------------------
	{
		OPTICK_EVENT("Issue CPU occlusion queries");

#ifdef DEBUG_LIGHTS_CULLING
		u32 total_issued = 0;
#endif

		auto issue_queries = [&](xr_vector<LightPrepareResult>& prepared_list, const char* name)
		{
#ifdef DEBUG_LIGHTS_CULLING
			u32 issued = 0, fallback = 0;
#endif
			for (auto& item : prepared_list)
			{
				if (!item.needs_query)
					continue;

				light* L = item.L;
				u32 qid = (u32)CPUOCC.AddLightVolume(L);
				L->VisibilityData.query_id = qid;
#ifdef DEBUG_LIGHTS_CULLING
				++issued;
				if (qid == UINT32_MAX) ++fallback;
#endif
			}
#ifdef DEBUG_LIGHTS_CULLING
			Msg("[CPU-OCC]   %s: issued=%d, fallback=%d", name, issued, fallback);
			total_issued += issued;
#endif
		};

		issue_queries(prepared_point, "point");
		issue_queries(prepared_spot, "spot");
		issue_queries(prepared_shadowed, "shadowed");

#ifdef DEBUG_LIGHTS_CULLING
		Msg("[CPU-OCC] Total queries issued: %d", total_issued);
#endif
	}

	// --------------------------------------------------------------------
	// 5. Завершение запроса и сохранение pending-источников
	// --------------------------------------------------------------------
	{
		OPTICK_EVENT("End occlusion queries");
		CPUOCC.EndOcclusionQueries();

		m_cpu_occ_pending_lights.clear();
		auto gather_pending = [&](const auto& src) {
			for (auto& item : src)
				if (item.pending)
					m_cpu_occ_pending_lights.push_back(item.L);
		};
		gather_pending(prepared_point);
		gather_pending(prepared_spot);
		gather_pending(prepared_shadowed);
	}

	// --------------------------------------------------------------------
	// 6. Распределение по LP_normal / LP_pending
	// --------------------------------------------------------------------
	{
		OPTICK_EVENT("Distribute to normal/pending");

		auto distribute = [](auto& prepared_list, auto& normal_list, auto& pending_list)
		{
			for (auto& item : prepared_list)
			{
				if (item.pending)
					pending_list.push_back(item.L);
				else if (item.L->VisibilityData.visible)
					normal_list.push_back(item.L);
			}
		};

		distribute(prepared_point, LP_normal.v_point, LP_pending.v_point);
		distribute(prepared_spot, LP_normal.v_spot, LP_pending.v_spot);
		distribute(prepared_shadowed, LP_normal.v_shadowed, LP_pending.v_shadowed);
	}

#ifdef DEBUG_LIGHTS_CULLING
	{
		u32 total_visible = LP_normal.v_point.size() + LP_normal.v_spot.size() + LP_normal.v_shadowed.size();
		u32 total_pending = LP_pending.v_point.size() + LP_pending.v_spot.size() + LP_pending.v_shadowed.size();
		u32 total_prepared = prepared_point.size() + prepared_spot.size() + prepared_shadowed.size();
		u32 total_invisible = total_prepared - total_visible - total_pending;
		Msg("[CPU-OCC] END FRAME %d: visible=%d pending=%d culled=%d",
			frame, total_visible, total_pending, total_invisible);
	}
#endif

	// --------------------------------------------------------------------
	// 7. Сортировка (используем штатный метод пакета)
	// --------------------------------------------------------------------
	{
		OPTICK_EVENT("Sorting light arrays");
		LP_normal.sort();
		LP_pending.sort();
	}
}

void CRender::update_shadow_map_visibility()
{
	PROFILE_FUNCTION();

	auto process_list = [](xr_vector<light*>& list)
	{
		for (light* L : list)
		{
			if (!L)
				continue;
			try
			{
				L->get_smapvis().flushoccq();
			}
			catch (...)
			{
				Msg("! Failed to flush-OCCq on light %p", L);
			}
		}
	};

	process_list(LP_normal.v_shadowed);
	process_list(LP_pending.v_shadowed);
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
