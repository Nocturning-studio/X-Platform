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

IC bool pred_sp_sort(ISpatial* _1, ISpatial* _2)
{
	float d1 = _1->spatial.sphere.P.distance_to_sqr(Engine.RenderView.Position);
	float d2 = _2->spatial.sphere.P.distance_to_sqr(Engine.RenderView.Position);
	return d1 < d2;
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

bool CRender::is_dynamic_sun_enabled()
{
	return ps_r_lighting_flags.test(RFLAG_SUN);
}

void CRender::check_distort()
{
	if (!(SceneGraph.mapDistort.size() == 0))
	{
		Msg("! mapDistort isn't deleted correctly!");
	}
}

void CRender::render_main(Fmatrix& m_ViewProjection, bool _fportals)
{
	PROFILE_FUNCTION();

	// marker теперь в SceneGraph
	SceneGraph.marker++;

	// Calculate sector(s) and their objects
	if (pLastSector)
	{
		//!!!
		//!!! BECAUSE OF PARALLEL HOM RENDERING TRY TO DELAY ACCESS TO HOM AS MUCH AS POSSIBLE
		//!!!
		{
			// Traverse object database
			// lstRenderables теперь в SceneGraph
			g_SpatialSpace->q_frustum(SceneGraph.lstRenderables, ISpatial_DB::O_ORDERED,
									  STYPE_RENDERABLE + STYPE_LIGHTSOURCE, ViewBase);

			// (almost) Exact sorting order (front-to-back)
			concurrency::parallel_sort(SceneGraph.lstRenderables.begin(), SceneGraph.lstRenderables.end(),
									   pred_sp_sort);

			// Determine visibility for dynamic part of scene
			set_Object(0);
			u32 uID_LTRACK = 0xffffffff;
			if (active_phase() == PHASE_NORMAL)
			{
				uLastLTRACK++;
				if (SceneGraph.lstRenderables.size())
					uID_LTRACK = uLastLTRACK % SceneGraph.lstRenderables.size();

				// update light-vis for current entity / actor
				CObject* O = g_pGameLevel->CurrentViewEntity();
				if (O)
				{
					CROS_impl* R = (CROS_impl*)O->ROS();
					if (R)
						R->update(O);
				}

				// update light-vis for selected entity
				// track lighting environment
				if (SceneGraph.lstRenderables.size())
				{
					IRenderable* renderable = SceneGraph.lstRenderables[uID_LTRACK]->dcast_Renderable();
					if (renderable)
					{
						CROS_impl* T = (CROS_impl*)renderable->renderable_ROS();
						if (T)
							T->update(renderable);
					}
				}
			}
		}

		// Traverse sector/portal structure
		PortalTraverser.traverse(pLastSector, ViewBase, Engine.RenderView.Position, m_ViewProjection,
								 CPortalTraverser::VQ_HOM + CPortalTraverser::VQ_SSA + CPortalTraverser::VQ_FADE);

		// Determine visibility for static geometry hierrarhy
		for (u32 s_it = 0; s_it < PortalTraverser.r_sectors.size(); s_it++)
		{
			CSector* sector = (CSector*)PortalTraverser.r_sectors[s_it];
			IRender_Visual* root = sector->root();
			for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
			{
				set_Frustum(&(sector->r_frustums[v_it]));
				add_Geometry(root);
			}
		}

		// Traverse frustums
		for (u32 o_it = 0; o_it < SceneGraph.lstRenderables.size(); o_it++)
		{
			ISpatial* spatial = SceneGraph.lstRenderables[o_it];
			spatial->spatial_updatesector();
			CSector* sector = (CSector*)spatial->spatial.sector;
			if (0 == sector)
				continue; // disassociated from S/P structure

			if (spatial->spatial.type & STYPE_LIGHTSOURCE)
			{
				// lightsource
				light* L = (light*)(spatial->dcast_Light());
				VERIFY(L);
				float lod = L->get_LOD();
				if (lod > EPS_L)
				{
					vis_data& vis = L->get_homdata();
					if (HOM.visible(vis))
						Lights.add_light(L);
				}
				continue;
			}

			if (PortalTraverser.i_marker != sector->r_marker)
				continue; // inactive (untouched) sector

			for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
			{
				CFrustum& view = sector->r_frustums[v_it];

				if (!view.testSphere_dirty(spatial->spatial.sphere.P, spatial->spatial.sphere.R))
					continue;

				if (spatial->spatial.type & STYPE_RENDERABLE)
				{
					// renderable
					IRenderable* renderable = spatial->dcast_Renderable();
					VERIFY(renderable);

					// Occlusion
					vis_data& v_orig = renderable->renderable.visual->vis;
					vis_data v_copy = v_orig;
					v_copy.box.xform(renderable->renderable.xform);
					BOOL bVisible = HOM.visible(v_copy);
					v_orig.marker = v_copy.marker;
					v_orig.accept_frame = v_copy.accept_frame;
					v_orig.hom_frame = v_copy.hom_frame;
					v_orig.hom_tested = v_copy.hom_tested;
					if (!bVisible)
						break; // exit loop on frustums

					// Rendering
					set_Object(renderable);
					renderable->renderable_Render();
					set_Object(0);
				}
				break; // exit loop on frustums
			}
		}

		if (g_pGameLevel && !(active_phase() == PHASE_SHADOW_DEPTH))
			g_pGameLevel->pHUD->Render_Last(); // HUD
	}
	else
	{
		set_Object(0);

		if (g_pGameLevel && !(active_phase() == PHASE_SHADOW_DEPTH))
			g_pGameLevel->pHUD->Render_Last(); // HUD
	}
}
void CRender::query_wait()
{
	PROFILE_FUNCTION();

	Device.Statistic->RenderDUMP_Wait_S.Begin();

	CTimer Timer;
	Timer.Start();

	BOOL result = FALSE;
	HRESULT hr = S_FALSE;

	while ((hr = q_sync_point[q_sync_count]->GetData(&result, sizeof(result), D3DGETDATA_FLUSH)) == S_FALSE)
	{
		if (!SwitchToThread())
			Sleep(ps_r_thread_wait_sleep);

		if (Timer.GetElapsed_ms() > 500)
		{
			result = FALSE;
			break;
		}
	}

	Device.Statistic->RenderDUMP_Wait_S.End();

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

void CRender::render_depth_prepass()
{
	PROFILE_FUNCTION();

	SceneGraphFetchConfig DepthPrepassFetchConfig;

	DepthPrepassFetchConfig.fetch_priority_0 = true;
	DepthPrepassFetchConfig.fetch_priority_1 = false;
	DepthPrepassFetchConfig.fetch_wallmarks = false;

	SceneGraph.SetFetchConfig(DepthPrepassFetchConfig);

	SceneGraph.SetCullingBoundsCollector(NULL);

	set_active_phase(PHASE_DEPTH_PREPASS);

	render_main(Engine.RenderView.ViewProjection, false);

	RenderBackend.set_ColorWriteEnable(FALSE);
	RenderBackend.set_ZWriteEnable(TRUE);

	RenderBackend.SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);

	RenderBackend.enable_anisotropy_filtering();

	if (Details)
		Details->Render(DetailsRenderMode::DepthOnly);

	SceneGraph.Render(SceneGraphRenderType::HUD);
	SceneGraph.Render(SceneGraphRenderType::Opaque, 0);
	SceneGraph.Render(SceneGraphRenderType::LOD, 0, true, true);

	RenderBackend.disable_anisotropy_filtering();

	RenderBackend.set_ColorWriteEnable(TRUE);
	RenderBackend.set_ZWriteEnable(FALSE);
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

	// 2. Конфигурация рекордера (Сбор баундов для теней)
	if (m_need_render_sun)
		SceneGraph.SetCullingBoundsCollector(&main_coarse_structure);
	else
		SceneGraph.SetCullingBoundsCollector(NULL);

	// 3. Фаза наполнения графа (Traverse & Cull)
	set_active_phase(PHASE_NORMAL);
	render_main(Engine.RenderView.ViewProjection, true); // Самый дорогой вызов - наполняет мапы SceneGraph

	// 4. Очистка состояния сбора (чтобы не повлиять на следующие этапы)
	SceneGraph.SetCullingBoundsCollector(NULL);

	// Сброс конфига на "дефолтный безопасный" (только Pri0)
	GBufferPassFetchConfig.fetch_wallmarks = false;

	SceneGraph.SetFetchConfig(GBufferPassFetchConfig);

	// 5. Фаза отрисовки (Render Backend)
	Device.Statistic->RenderCALC_GBuffer.Begin();
	RenderBackend.enable_anisotropy_filtering();

	set_gbuffer();

	if (ps_r_ls_flags.test(RFLAG_Z_PREPASS))
		RenderBackend.SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);

	if (psDeviceFlags.test(rsWireframe))
		RenderBackend.SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	// Отрисовка собранного Opaque (Priority 0)
	SceneGraph.Render(SceneGraphRenderType::Opaque, 0);

	if (Details)
		Details->Render(DetailsRenderMode::Default);

	if (psDeviceFlags.test(rsWireframe))
		RenderBackend.SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	RenderBackend.disable_anisotropy_filtering();
	Device.Statistic->RenderCALC_GBuffer.End();
}

void CRender::render_gbuffer_secondary()
{
	PROFILE_FUNCTION();

	PortalTraverser.fade_render();

	RenderBackend.enable_anisotropy_filtering();

	set_gbuffer();

	if (psDeviceFlags.test(rsWireframe))
		RenderBackend.SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	if (ps_r_ls_flags.test(RFLAG_Z_PREPASS))
		RenderBackend.SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);

	RenderBackend.set_ZWriteEnable(FALSE);

	SceneGraph.Render(SceneGraphRenderType::LOD, 0, true, true);

	set_active_phase(PHASE_HUD);
	SceneGraph.Render(SceneGraphRenderType::HUD);
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

		L->xform_calc();
		Fvector L_pos = L->get_position();
		float L_range = L->get_range();
		float distSqToCam = Engine.RenderView.Position.distance_to_sqr(L_pos);
		if (distSqToCam > (L_range * L_range + 400.0f))
			continue;

		// =========================================================================
		// 1. ОТРИСОВКА МАСКИ СВЕТА
		// =========================================================================
		RenderBackend.set_xform_world(L->get_xform());
		RenderBackend.set_xform_view(Engine.RenderView.View);
		RenderBackend.set_xform_project(Engine.RenderView.Project);
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
		SceneGraph.marker++;

		for (IRender_Visual* V : SceneGraph.m_visuals_static_visible)
		{
			ShaderElement* E = rimp_select_sh_static(V, 0.0f);
			if (!E || E->passes.empty())
				continue;

			float R_sum = V->vis.sphere.R + L_range;
			if (V->vis.sphere.P.distance_to_sqr(L_pos) < (R_sum * R_sum))
				SceneGraph.add_leafs_Static(V);
		}

		for (auto& item : SceneGraph.m_visuals_dynamic_visible)
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
				SceneGraph.add_leafs_Dynamic(item.visual);
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

		SceneGraph.Render(SceneGraphRenderType::Opaque, 1);
		SceneGraph.Render(SceneGraphRenderType::Transparent);

		// CLEANUP
		dwLightMarkerID += 2;
		RenderBackend.SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		RenderBackend.set_Stencil(FALSE);
	}
}

void CRender::render_stage_forward()
{
	PROFILE_FUNCTION();

	VERIFY(0 == mapDistort.size());

	// Очищаем списки с прошлого кадра
	SceneGraph.m_visuals_static_visible.clear();
	SceneGraph.m_visuals_dynamic_visible.clear();

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
		// И ЗАПОЛНИТ наши списки m_visuals_... благодаря правкам в add_leafs
		render_main(Engine.RenderView.ViewProjection, false);

		SceneGraph.Render(SceneGraphRenderType::Opaque, 1);
		SceneGraph.Render(SceneGraphRenderType::Transparent);

		g_pGamePersistent->Environment().RenderThunderbolt();
		g_pGamePersistent->Environment().RenderRain();
	}

	// ============================================
	// PASS 2: Dynamic Lighting Passes (Lights)
	// ============================================

	//render_forward_lights(Lights.package.v_point, PHASE_POINT_LIGHTING);
	//render_forward_lights(Lights.package.v_shadowed, PHASE_POINT_LIGHTING);
	//render_forward_lights(Lights.package.v_spot, PHASE_SPOT_LIGHTING);

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
	//render_main(Engine.RenderView.ViewProjection, false);
	SceneGraph.render_reuse();
	SceneGraph.Render(SceneGraphRenderType::Opaque, 1);
	SceneGraph.Render(SceneGraphRenderType::Transparent);

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

	Device.Statistic->RenderCALC_SUN.Begin();

	RenderImplementation.stats.l_visible++;
	render_sun_cascades();
	dwLightMarkerID += 2;

	Device.Statistic->RenderCALC_SUN.End();
}

void CRender::render_lights()
{
	PROFILE_FUNCTION();

	Device.Statistic->RenderCALC_LIGHTS.Begin();

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

	Device.Statistic->RenderCALC_LIGHTS.End();
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

	Device.Statistic->RenderCALC_POSTPROCESS.Begin();

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

	Device.Statistic->RenderCALC_POSTPROCESS.End();
}
////////////////////////////////////////////////////////////////////////////////
