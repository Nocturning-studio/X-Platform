////////////////////////////////////////////////////////////////////////////////
// Created: 17.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
////////////////////////////////////////////////////////////////////////////////
//#define DEBUG_LIGHTS_CULLING
////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////