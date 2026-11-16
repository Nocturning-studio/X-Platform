////////////////////////////////////////////////////////////////////////////////
// Created: 17.06.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
////////////////////////////////////////////////////////////////////////////////
// #define DEBUG_LIGHTS_CULLING
////////////////////////////////////////////////////////////////////////////////
void CRender::render_stage_lights_culling()
{
    PROFILE_FUNCTION();

    const u32 frame = Engine.TimeManager.GetFrameCount();
    const fvec3& cam_pos = Engine.RenderView.Position;
    const float max_dist_sq = ps_r_light_distance_cull * ps_r_light_distance_cull;

    CLight_DB& lights = Scene.GetLights();
    light_Package& LP_normal_pkg = Scene.GetNormalLights();
    light_Package& LP_pending_pkg = Scene.GetPendingLights();
    xr_vector<light*>& cpu_pending_lights = Scene.GetCpuOccPendingLights();

    light_Package& LP = lights.package;

    // ------------------------------------------------------------------
    // Хелпер: «видим, без запроса» + дистанционный куллинг.
    // frame2test намеренно = frame, чтобы на следующем кадре свет, вернувшийся
    // в диапазон, получил свежую проверку без задержки на 10..20 кадров.
    // ------------------------------------------------------------------
    auto distribute_visible_no_query = [&](xr_vector<light*>& src, xr_vector<light*>& dst)
    {
        for (light* L : src)
        {
            if (!L) continue;

            const float dist_sq = cam_pos.distance_to_sqr(L->spatial.sphere.P);
            if (dist_sq > max_dist_sq)
            {
                L->VisibilityData.visible = false;
                L->VisibilityData.pending = false;
                L->VisibilityData.frame2test = frame + ::Random.randI(2, 5);   // короткий recheck
                continue;
            }

            L->VisibilityData.visible = true;
            L->VisibilityData.pending = false;
            L->VisibilityData.frame2test = frame;
            dst.push_back(L);
        }
    };

    // =====================================================================
    // Fast-path #1: HOM выключен -> CPUOCC бесполезен (пустой depth-буфер).
    // Всё отдаём как видимое, иначе все запросы вернут 0 и откуллят сцену.
    // =====================================================================
    if (!CPUOCC.IsLoaded())
    {
        OPTICK_EVENT("lights_culling_no_hom");

        // Осушаем «зависший» запрос, чтобы CPUOCC не остался в pending-состоянии
        if (CPUOCC.IsQueryReady())
        {
            for (light* L : cpu_pending_lights)
            {
                if (!L) continue;
                L->VisibilityData.visible = true;
                L->VisibilityData.pending = false;
                L->VisibilityData.frame2test = frame;
            }
            CPUOCC.ResetPendingQuery();
            cpu_pending_lights.clear();
        }

        LP_normal_pkg.clear();
        LP_pending_pkg.clear();

        distribute_visible_no_query(LP.v_point, LP_normal_pkg.v_point);
        distribute_visible_no_query(LP.v_spot, LP_normal_pkg.v_spot);
        distribute_visible_no_query(LP.v_shadowed, LP_normal_pkg.v_shadowed);

        LP_normal_pkg.sort();
        return;
    }

    // =====================================================================
    // Fast-path #2: предыдущий запрос ещё в полёте.
    // Пересобираем пакеты из мастера (LP.v_*) по последнему известному
    // состоянию visible/pending — НЕ трогая in-flight query.
    // render_lights(…) в конце кадра опустошит LP_normal_pkg/LP_pending_pkg,
    // поэтому при возврате без пересборки кадр был бы пустым.
    //
    // Важно: не выходим здесь, если ждать нечего (cpu_pending_lights пуст) —
    // иначе BeginOcclusionQueries никогда не вызовется и IsQueryReady()
    // навсегда останется false (дедлок первого кадра).
    // =====================================================================
    if (!CPUOCC.IsQueryReady() && !cpu_pending_lights.empty())
    {
        OPTICK_EVENT("lights_culling_wait_query");

        LP_normal_pkg.clear();
        LP_pending_pkg.clear();

        // Читаем состояние, ничего не мутируем.
        auto rebuild = [&](xr_vector<light*>& src,
            xr_vector<light*>& dst_normal,
            xr_vector<light*>& dst_pending)
        {
            for (light* L : src)
            {
                if (!L) continue;

                // дистанционный куллинг — можно и нужно применять и здесь
                const float dist_sq = cam_pos.distance_to_sqr(L->spatial.sphere.P);
                if (dist_sq > max_dist_sq) continue;

                if (L->VisibilityData.pending)
                    dst_pending.push_back(L);         // в полёте -> рендерим как «pending»
                else if (L->VisibilityData.visible)
                    dst_normal.push_back(L);
            }
        };

        rebuild(LP.v_point, LP_normal_pkg.v_point, LP_pending_pkg.v_point);
        rebuild(LP.v_spot, LP_normal_pkg.v_spot, LP_pending_pkg.v_spot);
        rebuild(LP.v_shadowed, LP_normal_pkg.v_shadowed, LP_pending_pkg.v_shadowed);

        LP_normal_pkg.sort();
        LP_pending_pkg.sort();
        return;
    }

    // =====================================================================
    // 1. Сбор результатов предыдущего кадра
    // =====================================================================
    {
        OPTICK_EVENT("collect_cpu_query_results");

        for (light* L : cpu_pending_lights)
        {
            if (!L) continue;
            if (!L->VisibilityData.pending) continue;

            const u32 samples = CPUOCC.GetVisibleSamples(L->VisibilityData.query_id);

            if (samples == 0xfffffffe)
            {
                // Не должно случаться при IsQueryReady() == true, но
                // не застреваем в pending — временно visible.
                Msg("! [lights] query_id=%u not resolved despite IsQueryReady()", L->VisibilityData.query_id);
                L->VisibilityData.visible = true;
                L->VisibilityData.pending = false;
                L->VisibilityData.frame2test = frame + 1;
                continue;
            }

            const bool newVisible = (samples > ps_r_light_fragments_cull);
            L->VisibilityData.visible = newVisible;
            L->VisibilityData.pending = false;

            if (newVisible)
                L->VisibilityData.frame2test = frame + ::Random.randI(delay_large_min, delay_large_max);
            else
                L->VisibilityData.frame2test = frame + ::Random.randI(5, 10);

#ifdef DEBUG_LIGHTS_CULLING
            Msg("[CPU-OCC]   Light %p (type %d) qid=%u samples=%u -> %s",
                L, L->LightFlags.type, L->VisibilityData.query_id, samples,
                newVisible ? "VISIBLE" : "CULLED");
#endif
        }

        CPUOCC.ResetPendingQuery();
        cpu_pending_lights.clear();   // безопасно: только что прочли всё
    }

    // =====================================================================
    // 2. Очистка пакетов
    // =====================================================================
    {
        OPTICK_EVENT("clear_packages");
        LP_normal_pkg.clear();
        LP_pending_pkg.clear();
    }

    stats.l_shadowed = LP.v_shadowed.size();
    stats.l_unshadowed = LP.v_point.size() + LP.v_spot.size();
    stats.l_total = stats.l_shadowed + stats.l_unshadowed;

    // =====================================================================
    // 3. Начало нового батча CPUOCC
    // =====================================================================
    {
        OPTICK_EVENT("begin_query");

        SoftX::Viewport vp(0.0f, 0.0f, 512, 512, 0.0f, 1.0f);
        CPUOCC.BeginOcclusionQueries(Engine.RenderView.ViewProjection, vp);
    }

    // =====================================================================
    // 4. Подготовка (классификация)
    // =====================================================================
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
        if (!L)
            return { nullptr, false, false };

        const float dist_sq = cam_pos.distance_to_sqr(L->spatial.sphere.P);

        if (dist_sq > max_dist_sq)
        {
            L->VisibilityData.visible = false;
            L->VisibilityData.pending = false;
            L->VisibilityData.frame2test = frame + ::Random.randI(2, 5);
            return { L, false, false };
        }

        // Свет вернулся в диапазон после дистанционного куллинга (или был
        // occ-куллен, но уже прошёл положенный recheck) — сбрасываем
        // frame2test, чтобы vis_prepare() отработал в ЭТОМ кадре.
        if (!L->VisibilityData.visible && !L->VisibilityData.pending && L->VisibilityData.frame2test > frame)
        {
            L->VisibilityData.frame2test = frame;
        }

        const bool needs_q = L->vis_prepare(frame);
        return { L, needs_q, L->VisibilityData.pending };
    };

    const size_t total_lights = LP.v_point.size() + LP.v_spot.size() + LP.v_shadowed.size();
    {
        OPTICK_EVENT("Prepare lights (parallel/sequential)");
        if (total_lights > 64)
        {
            concurrency::parallel_invoke(
                [&]() { for (light* L : LP.v_point)    prepared_point.push_back(prepare_one(L));    },
                [&]() { for (light* L : LP.v_spot)     prepared_spot.push_back(prepare_one(L));     },
                [&]() { for (light* L : LP.v_shadowed) prepared_shadowed.push_back(prepare_one(L)); });
        }
        else
        {
            OPTICK_EVENT("prepare_sequential");
            for (light* L : LP.v_point)    prepared_point.push_back(prepare_one(L));
            for (light* L : LP.v_spot)     prepared_spot.push_back(prepare_one(L));
            for (light* L : LP.v_shadowed) prepared_shadowed.push_back(prepare_one(L));
        }
    }

    // =====================================================================
    // 5. Выдача CPU occlusion запросов
    // =====================================================================
    {
        OPTICK_EVENT("Issue CPU occlusion queries");

        auto issue_queries = [&](xr_vector<LightPrepareResult>& list)
        {
            for (auto& item : list)
            {
                if (!item.needs_query) continue;
                light* L = item.L;

                const u32 qid = (u32)CPUOCC.AddLightVolume(L);
                if (qid == UINT32_MAX)
                {
                    // Пул исчерпан / AddLightVolume не смог — не оставляем
                    // свет в pending навсегда: считаем видимым, перепроверим
                    // на следующем кадре.
                    L->VisibilityData.visible = true;
                    L->VisibilityData.pending = false;
                    L->VisibilityData.frame2test = frame + 1;
                    item.pending = false;
                    continue;
                }

                L->VisibilityData.query_id = qid;
            }
        };

        issue_queries(prepared_point);
        issue_queries(prepared_spot);
        issue_queries(prepared_shadowed);
    }

    // =====================================================================
    // 6. Завершение батча и сбор pending-источников
    // =====================================================================
    {
        OPTICK_EVENT("End occlusion queries");
        CPUOCC.EndOcclusionQueries();

        auto gather_pending = [&](const auto& src)
        {
            for (const auto& item : src)
                if (item.pending && item.L)
                    cpu_pending_lights.push_back(item.L);
        };
        gather_pending(prepared_point);
        gather_pending(prepared_spot);
        gather_pending(prepared_shadowed);
    }

    // =====================================================================
    // 7. Распределение normal / pending
    // =====================================================================
    {
        OPTICK_EVENT("Distribute to normal/pending");

        auto distribute = [](auto& prepared_list, auto& normal_list, auto& pending_list)
        {
            for (auto& item : prepared_list)
            {
                if (!item.L) continue;
                if (item.pending)
                    pending_list.push_back(item.L);
                else if (item.L->VisibilityData.visible)
                    normal_list.push_back(item.L);
            }
        };

        distribute(prepared_point, LP_normal_pkg.v_point, LP_pending_pkg.v_point);
        distribute(prepared_spot, LP_normal_pkg.v_spot, LP_pending_pkg.v_spot);
        distribute(prepared_shadowed, LP_normal_pkg.v_shadowed, LP_pending_pkg.v_shadowed);
    }

    // =====================================================================
    // 8. Сортировка
    // =====================================================================
    {
        OPTICK_EVENT("Sorting light arrays");
        LP_normal_pkg.sort();
        LP_pending_pkg.sort();
    }
}

void CRender::update_shadow_map_visibility()
{
    PROFILE_FUNCTION();

    auto process_list = [](xr_vector<light*>& list)
    {
        for (light* L : list)
        {
            if (!L) continue;
            try { L->get_smapvis().flushoccq(); }
            catch (...) { Msg("! Failed to flush-OCCq on light %p", L); }
        }
    };

    process_list(Scene.GetNormalLights().v_shadowed);
    process_list(Scene.GetPendingLights().v_shadowed);
}
////////////////////////////////////////////////////////////////////////////////
