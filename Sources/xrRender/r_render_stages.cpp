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

void CRender::render_main(Fmatrix& view_projection, bool /*_use_portals*/)
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
	u32 current_marker = SceneGraph.m_traversal_marker.fetch_add(1) + 1;

	m_TraversalContext.frustum = &ViewBase;
	m_TraversalContext.traversal_marker_id = current_marker;

	CurrentRenderContext::Scope tls_scope(SceneGraph.m_packet, m_TraversalContext);

	// -------------------------------------------------------------------------
	// 1. Spatial Query
	// -------------------------------------------------------------------------
	g_SpatialSpace->q_frustum(SceneGraph.m_packet.m_spatial_query_results, ISpatial_DB::O_ORDERED,
							  STYPE_RENDERABLE | STYPE_LIGHTSOURCE, ViewBase);

	// -------------------------------------------------------------------------
	// 2. Sorting
	// -------------------------------------------------------------------------
	const Fvector camera_pos = Engine.RenderView.Position;
	auto sort_predicate = [camera_pos](ISpatial* a, ISpatial* b) {
		float dist_a = a->spatial.sphere.P.distance_to_sqr(camera_pos);
		float dist_b = b->spatial.sphere.P.distance_to_sqr(camera_pos);
		return dist_a < dist_b;
	};

	if (!SceneGraph.m_packet.m_spatial_query_results.empty())
	{
		concurrency::parallel_sort(SceneGraph.m_packet.m_spatial_query_results.begin(),
								   SceneGraph.m_packet.m_spatial_query_results.end(), sort_predicate);
	}

	// -------------------------------------------------------------------------
	// 3. Light Tracking
	// -------------------------------------------------------------------------
	set_Object(nullptr);
	if (active_phase() == PHASE_NORMAL)
	{
		uLastLTRACK++;
		size_t renderable_count = SceneGraph.m_packet.m_spatial_query_results.size();
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
			if (IRenderable* renderable =
					SceneGraph.m_packet.m_spatial_query_results[light_track_id]->dcast_Renderable())
			{
				if (CROS_impl* ros = (CROS_impl*)renderable->renderable_ROS())
					ros->update(renderable);
			}
		}
	}

	// -------------------------------------------------------------------------
	// 4. Portal Traversal
	// -------------------------------------------------------------------------
	SceneGraph.m_packet.portal_traverser.Traverse(pLastSector, ViewBase, Engine.RenderView.Position, view_projection,
												  CPortalTraverser::VQ_HOM | CPortalTraverser::VQ_SSA |
													  CPortalTraverser::VQ_FADE);

	// -------------------------------------------------------------------------
	// 5. Static Geometry
	// -------------------------------------------------------------------------
	const auto& visible_sectors = SceneGraph.m_packet.portal_traverser.GetVisibleSectors();

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

	HOM.MT_SYNC();

	// -------------------------------------------------------------------------
	// 6. Dynamic Geometry & Lights
	// -------------------------------------------------------------------------
	for (ISpatial* spatial : SceneGraph.m_packet.m_spatial_query_results)
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
				// Для света копирование не так страшно, но лучше тоже синхронизировать,
				// если light имеет свой vis_data. Обычно light->homdata это копия.
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
		// 2. HOM OCCLUSION CULLING (ИСПРАВЛЕНО)
		// =====================================================================
		// Получаем ссылку на оригинал
		vis_data& vis_orig = renderable->renderable.visual->vis;

		// Создаем временную копию для трансформации AABB в мировые координаты
		vis_data vis_temp = vis_orig;
		vis_temp.box.transform(renderable->renderable.transform);

		// Проверяем видимость
		BOOL bVisible = HOM.visible(vis_temp);

		// ВАЖНО: Возвращаем обновленные тайминги обратно в оригинал!
		// Иначе HOM будет проверять этот объект КАЖДЫЙ кадр, вызывая мерцание.
		vis_orig.hom_frame = vis_temp.hom_frame;
		vis_orig.hom_tested = vis_temp.hom_tested;

		if (bVisible)
		{
			// Обновляем копию для рендера (хотя это не строго обязательно для рендера, но полезно)
			// vis_orig = vis_temp; // <-- Нельзя, это перезапишет локальный AABB на мировой!

			// Рендерим
			m_TraversalContext.frustum = &ViewBase;
			set_Object(renderable);
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

	// 1. Конфигурация сбора (Fetch Config)
	SceneGraphFetchConfig GBufferPassFetchConfig;

	GBufferPassFetchConfig.fetch_priority_0 = true;
	GBufferPassFetchConfig.fetch_priority_1 = false;
	GBufferPassFetchConfig.fetch_wallmarks = true;

	SceneGraph.SetFetchConfig(GBufferPassFetchConfig);

	set_active_phase(PHASE_NORMAL);

	// 2. Конфигурация рекордера (Сбор баундов для теней)
	if (m_need_render_sun)
		SceneGraph.SetCullingBoundsCollector(&main_coarse_structure);
	else
		SceneGraph.SetCullingBoundsCollector(NULL);

	// 3. Фаза наполнения графа (Traverse & Cull)
	render_main(Engine.RenderView.ViewProjection, true); // Самый дорогой вызов - наполняет мапы SceneGraph

	// 4. Очистка состояния сбора (чтобы не повлиять на следующие этапы)
	SceneGraph.SetCullingBoundsCollector(NULL);

	// Сброс конфига на "дефолтный безопасный" (только Pri0)
	GBufferPassFetchConfig.fetch_wallmarks = false;

	SceneGraph.SetFetchConfig(GBufferPassFetchConfig);

	// 5. Фаза отрисовки (Render Backend)
	Engine.Statistic->RenderCALC_GBuffer.Begin();
	RenderBackend.enable_anisotropy_filtering();

	set_gbuffer();

	if (psDeviceFlags.test(rsWireframe))
		RenderBackend.SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	// Отрисовка собранного Opaque (Priority 0)
	SceneGraph.Render(SceneGraph.m_packet, SceneGraphRenderType::Opaque, 0);

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

	SceneGraph.m_packet.portal_traverser.RenderFade();

	RenderBackend.enable_anisotropy_filtering();

	set_gbuffer();

	if (psDeviceFlags.test(rsWireframe))
		RenderBackend.SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	RenderBackend.set_ZWriteEnable(FALSE);

	SceneGraph.Render(SceneGraph.m_packet, SceneGraphRenderType::LOD, 0, true, true);

	set_active_phase(PHASE_HUD);
	SceneGraph.Render(SceneGraph.m_packet, SceneGraphRenderType::HUD);
	set_active_phase(PHASE_NORMAL);

	if (psDeviceFlags.test(rsWireframe))
		RenderBackend.SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	RenderBackend.disable_anisotropy_filtering();
}

void CRender::render_forward_lights(xr_vector<light*>& lights, int phase)
{
	PROFILE_FUNCTION();

	if (lights.empty())
		return;

	dwLightMarkerID = 5;

	std::sort(lights.begin(), lights.end(), [](light* a, light* b) {
		return Engine.RenderView.Position.distance_to_sqr(a->get_position()) <
			   Engine.RenderView.Position.distance_to_sqr(b->get_position());
	});

	const size_t max_forward_lights = 100;
	size_t count = std::min(lights.size(), max_forward_lights);
	float smapsize = float(RenderImplementation.o.smapsize);

	for (size_t i = 0; i < count; ++i)
	{
		light* L = lights[i];

		if (!L->vis.visible || !L->flags.bActive)
			continue;

		L->transform_calc();
		Fvector L_pos = L->get_position();
		float L_range = L->get_range();
		float distSqToCam = Engine.RenderView.Position.distance_to_sqr(L_pos);
		if (distSqToCam > (L_range * L_range + 400.0f))
			continue;

		// =========================================================================
		// 1. ОТРИСОВКА МАСКИ СВЕТА
		// =========================================================================
		RenderBackend.set_transform_world(L->get_transform());
		RenderBackend.set_transform_view(Engine.RenderView.View);
		RenderBackend.set_transform_project(Engine.RenderView.Project);
		enable_scissor(L);

		u32 mask_id = (L->flags.type == IRender_Light::OMNIPART) ? SE_MASK_POINT : SE_MASK_SPOT;
		RenderBackend.set_Element(RenderTarget->s_accum_mask->E[mask_id]);

		// принудительное отключение цвета
		RenderBackend.SetRenderState(D3DRS_COLORWRITEENABLE, 0);
		RenderBackend.set_ZWriteEnable(FALSE);

		// Мы хотим ГАРАНТИРОВАННО пометить пиксели стенсилом, не завися от того,
		// что там нарисовало (или не нарисовало) Солнце.
		// Cull CW: Увеличиваем ref для пикселей ЗА объемом света
		RenderBackend.set_CullMode(CULL_FRONTFACE);
		RenderBackend.set_Stencil(TRUE, D3DCMP_ALWAYS, dwLightMarkerID, 0x01, 0xff, D3DSTENCILOP_KEEP,
								  D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE);
		draw_volume(L);

		// Cull CCW: Та же логика для передних граней
		RenderBackend.set_CullMode(CULL_BACKFACE);
		RenderBackend.set_Stencil(TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP,
								  D3DSTENCILOP_REPLACE);
		draw_volume(L);

		// =========================================================================
		// 2. ОТРИСОВКА ГЕОМЕТРИИ (СВЕТ НА ВОДЕ)
		// =========================================================================
		RenderImplementation.apply_lmaterial();

		Fvector L_pos_view;
		Engine.RenderView.View.transform_tiny(L_pos_view, L_pos);
		Fvector L_dir, L_dir_view;
		L_dir = L->get_direction();
		Engine.RenderView.View.transform_dir(L_dir_view, L_dir);
		L_dir_view.normalize();
		Fvector L_clr = {L->get_color().r, L->get_color().g, L->get_color().b};
		L_clr.mul(L->get_LOD());

		Fmatrix m_Shadow = Fidentity;
		Fmatrix m_Lmap = Fidentity;
		if (phase == PHASE_SPOT_LIGHTING || L->flags.type != IRender_Light::OMNIPART)
		{
			float fTexelOffs = (.5f / smapsize);
			float view_dim = float(L->X.S.size - 2) / smapsize;
			float view_sx = float(L->X.S.posX + 1) / smapsize;
			float view_sy = float(L->X.S.posY + 1) / smapsize;
			float fRange = float(1.f) * ps_r_ls_depth_scale;
			float fBias = ps_r_ls_depth_bias;

			Fmatrix m_TexelAdjust = {view_dim / 2.f,
									 0.0f,
									 0.0f,
									 0.0f,
									 0.0f,
									 -view_dim / 2.f,
									 0.0f,
									 0.0f,
									 0.0f,
									 0.0f,
									 fRange,
									 0.0f,
									 view_dim / 2.f + view_sx + fTexelOffs,
									 view_dim / 2.f + view_sy + fTexelOffs,
									 fBias,
									 1.0f};
			Fmatrix xf_inv_view;
			xf_inv_view.invert(Engine.RenderView.View);
			Fmatrix xf_project;
			xf_project.mul(m_TexelAdjust, L->X.S.project);
			m_Shadow.mul(L->X.S.view, xf_inv_view);
			m_Shadow.mulA_44(xf_project);

			float l_dim = 1.f;
			Fmatrix m_TexelAdjust2 = {l_dim / 2.f, 0.0f, 0.0f,	 0.0f, 0.0f,		-l_dim / 2.f, 0.0f,	 0.0f,
									  0.0f,		   0.0f, fRange, 0.0f, l_dim / 2.f, l_dim / 2.f,  fBias, 1.0f};
			xf_project.mul(m_TexelAdjust2, L->X.S.project);
			m_Lmap.mul(L->X.S.view, xf_inv_view);
			m_Lmap.mulA_44(xf_project);
			if (!L->flags.bShadow)
				m_Shadow = m_Lmap;
		}

		float att_R = L_range * .95f;
		float att_factor = 1.f / (att_R * att_R);
		RenderBackend.set_Constant("L_dynamic_pos", L_pos_view.x, L_pos_view.y, L_pos_view.z, att_factor);
		RenderBackend.set_Constant("L_dynamic_color", L_clr.x, L_clr.y, L_clr.z, 0.f);
		RenderBackend.set_Constant("L_dynamic_dir", L_dir_view.x, L_dir_view.y, L_dir_view.z, 0.f);
		if (phase == PHASE_SPOT_LIGHTING || L->flags.type != IRender_Light::OMNIPART)
		{
			float spot_cutoff = L->get_cone();
			float spot_inner = spot_cutoff * 0.8f;
			RenderBackend.set_Constant("Ldynamic_spot_att", cosf(spot_inner), cosf(spot_cutoff), L_range * L_range,
									   0.f);
		}
		else
		{
			RenderBackend.set_Constant("Ldynamic_spot_att", 0.f, 0.f, L_range * L_range, 0.f);
		}

		RenderBackend.set_Constant("m_shadow", m_Shadow);
		RenderBackend.set_Array_Constant("m_lmap", 0, m_Lmap._11, m_Lmap._21, m_Lmap._31, m_Lmap._41);
		RenderBackend.set_Array_Constant("m_lmap", 1, m_Lmap._12, m_Lmap._22, m_Lmap._32, m_Lmap._42);

		// --- RENDER GEOMETRY ---
		set_active_phase(phase);
		RenderImplementation.set_Transform(0);
		SceneGraph.m_traversal_marker++;

		// Используем m_packet.m_visuals_static_visible
		for (IRender_Visual* V : SceneGraph.m_packet.m_visuals_static_visible)
		{
			ShaderElement* E = rimp_select_sh_static(V, 0.0f);
			if (!E || E->passes.empty())
				continue;

			float R_sum = V->vis.sphere.R + L_range;
			if (V->vis.sphere.P.distance_to_sqr(L_pos) < (R_sum * R_sum))
				SceneGraph.ProcessStaticVisual(V, m_TraversalContext, SceneGraph.m_packet);
		}

		// Используем m_packet.m_visuals_dynamic_visible
		for (auto& item : SceneGraph.m_packet.m_visuals_dynamic_visible)
		{
			ShaderElement* E = rimp_select_sh_dynamic(item.visual, 0.0f);
			if (!E || E->passes.empty())
				continue;

			Fvector sphere_center_world;
			item.matrix.transform_tiny(sphere_center_world, item.visual->vis.sphere.P);
			float R_sum = item.visual->vis.sphere.R + L_range;
			if (sphere_center_world.distance_to_sqr(L_pos) < (R_sum * R_sum))
			{
				RenderImplementation.set_Transform(&item.matrix);
				SceneGraph.ProcessDynamicVisual(item.visual, m_TraversalContext, SceneGraph.m_packet);
			}
		}

		// Включаем обратно цвет
		RenderBackend.set_ColorWriteEnable(TRUE);
		RenderBackend.SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
		RenderBackend.set_ZWriteEnable(FALSE);

		// СТЕНСИЛ ТЕСТ для воды: рисуем только если маска == dwLightMarkerID
		RenderBackend.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID, 0xff, 0x00);

		RenderBackend.SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
		RenderBackend.set_CullMode(CULL_BACKFACE);

		SceneGraph.Render(SceneGraph.m_packet, SceneGraphRenderType::Opaque, 1);
		SceneGraph.Render(SceneGraph.m_packet, SceneGraphRenderType::Transparent);

		// CLEANUP
		dwLightMarkerID += 2;
		RenderBackend.SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		RenderBackend.set_Stencil(FALSE);
	}
}

void CRender::render_stage_forward()
{
	PROFILE_FUNCTION();

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

		// !!! ИСПРАВЛЕНИЕ: Базовая фаза должна быть NORMAL, чтобы заполнился кэш !!!
		set_active_phase(PHASE_NORMAL);

		// Этот вызов заполнит граф геометрией с шейдерами normal_hq/lq
		// И ЗАПОЛНИТ наши списки m_packet.m_visuals_... благодаря правкам в add_leafs
		render_main(Engine.RenderView.ViewProjection, false);

		SceneGraph.Render(SceneGraph.m_packet, SceneGraphRenderType::Opaque, 1);
		SceneGraph.Render(SceneGraph.m_packet, SceneGraphRenderType::Transparent);

		g_pGamePersistent->Environment().RenderThunderbolt();
		g_pGamePersistent->Environment().RenderRain();
	}

	// ============================================
	// PASS 2: Dynamic Lighting Passes (Lights)
	// ============================================

	// render_forward_lights(Lights.package.v_point, PHASE_POINT_LIGHTING);
	// render_forward_lights(Lights.package.v_shadowed, PHASE_POINT_LIGHTING);
	// render_forward_lights(Lights.package.v_spot, PHASE_SPOT_LIGHTING);

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

	// Заново наполняем граф из кэша.
	// render_main(Engine.RenderView.ViewProjection, false);
	SceneGraph.render_reuse(m_TraversalContext, SceneGraph.m_packet);
	SceneGraph.Render(SceneGraph.m_packet, SceneGraphRenderType::Opaque, 1);
	SceneGraph.Render(SceneGraph.m_packet, SceneGraphRenderType::Transparent);

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
				if (L->vis.pending)
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
