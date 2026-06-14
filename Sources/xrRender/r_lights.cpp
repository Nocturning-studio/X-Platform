#include "stdafx.h"

IC bool pred_area(light* _1, light* _2)
{
	u32 a0 = _1->TransformContext.ShadowContext.size;
	u32 a1 = _2->TransformContext.ShadowContext.size;
	return a0 > a1; // reverse -> descending
}

void CRender::render_lights(light_Package& LP)
{
    OPTICK_EVENT("render_lights");

    // ------------------------------------------------------------------------
    // 0. Базовая фильтрация нулевых указателей (как было)
    auto is_valid_light = [](light* L) {
        if (L == nullptr)
            return false;
        fvec3 zero = { 0, -1000, 0 };
        if (L->get_position().similar(zero, EPS_L))
            return false;
        return true;
    };

    LP.v_shadowed.erase(std::remove_if(LP.v_shadowed.begin(), LP.v_shadowed.end(),
        [&](light* L) { return !is_valid_light(L); }), LP.v_shadowed.end());
    LP.v_point.erase(std::remove_if(LP.v_point.begin(), LP.v_point.end(),
        [&](light* L) { return !is_valid_light(L); }), LP.v_point.end());
    LP.v_spot.erase(std::remove_if(LP.v_spot.begin(), LP.v_spot.end(),
        [&](light* L) { return !is_valid_light(L); }), LP.v_spot.end());

    // ------------------------------------------------------------------------
    // 1. ОБНОВЛЕНИЕ ВИДИМОСТИ И МАТРИЦ (без удаления)
    {
        OPTICK_EVENT("Update visibility and compute matrices");

        xr_vector<light*>& source = LP.v_shadowed;

        // Последовательное обновление видимости (occlusion queries не параллелятся)
        for (light* L : source)
        {
            L->vis_update(); // теперь неблокирующий!
        }

        // Параллельное вычисление матриц для ВИДИМЫХ источников (compute_xf_spot потокобезопасен)
        // Включайте только если уверены, что внутри нет глобального состояния.
        // Если сомневаетесь – оставьте последовательный цикл.
#if 1 // переключите на 0 для отключения параллелизма
        if (source.size() > 16)
        {
            concurrency::parallel_for_each(source.begin(), source.end(),
                [this](light* L) {
                    if (L->VisibilityData.visible)
                        LR.compute_xf_spot(L);
                });
        }
        else
#endif
        {
            for (light* L : source)
            {
                if (L->VisibilityData.visible)
                    LR.compute_xf_spot(L);
            }
        }
    }

    // ------------------------------------------------------------------------
    // 2. УДАЛЕНИЕ НЕВИДИМЫХ ИСТОЧНИКОВ (быстро, без вызовов vis_update)
    {
        OPTICK_EVENT("Remove invisible");

        LP.v_shadowed.erase(
            std::remove_if(LP.v_shadowed.begin(), LP.v_shadowed.end(),
                [](light* L) { return !L->VisibilityData.visible; }),
            LP.v_shadowed.end());
    }

    // ------------------------------------------------------------------------
    // 3. УПАКОВКА SHADOW MAPS (без изменений, кроме оптимизации сортировки)
    {
        OPTICK_EVENT("Pack shadow maps");

        xr_vector<light*>& source = LP.v_shadowed;
        if (source.empty())
            return;

        xr_vector<light*> refactored;
        refactored.reserve(source.size());

        // Сортировка по убыванию размера
        if (source.size() > 8)
            concurrency::parallel_sort(source.begin(), source.end(), pred_area);
        else
            std::sort(source.begin(), source.end(), pred_area);

        for (u16 smap_ID = 0; !source.empty(); smap_ID++)
        {
            LP_smap_pool.initialize(RenderImplementation.o.smapsize);

            for (auto it = source.begin(); it != source.end();)
            {
                light* L = *it;
                SMAP_Rect R;
                if (LP_smap_pool.push(R, L->TransformContext.ShadowContext.size))
                {
                    L->TransformContext.ShadowContext.posX = R.min.x;
                    L->TransformContext.ShadowContext.posY = R.min.y;
                    L->VisibilityData.smap_ID = smap_ID;
                    refactored.push_back(L);
                    it = source.erase(it);
                }
                else
                {
                    if (it == source.begin())
                        break;
                    ++it;
                }
            }
        }

        std::reverse(refactored.begin(), refactored.end());
        LP.v_shadowed = std::move(refactored);
    }

    // ------------------------------------------------------------------------
    // 4. РЕНДЕР ТЕНЕЙ (без изменений)
    HOM.Disable();

    while (!LP.v_shadowed.empty())
    {
        OPTICK_EVENT("Shadow map rendering");

        SceneGraphFetchConfig ShadowPassFetchConfig;
        ShadowPassFetchConfig.fetch_priority_0 = true;
        ShadowPassFetchConfig.fetch_priority_1 = false;
        ShadowPassFetchConfig.fetch_wallmarks = false;
        SceneGraph.SetFetchConfig(ShadowPassFetchConfig);

        stats.s_used++;
        clear_shadow_map_spot();

        xr_vector<light*> current_batch;
        xr_vector<light*>& source = LP.v_shadowed;
        u16 current_sid = source.back()->VisibilityData.smap_ID;

        while (!source.empty() && source.back()->VisibilityData.smap_ID == current_sid)
        {
            current_batch.push_back(source.back());
            source.pop_back();
        }
        Lights_LastFrame.insert(Lights_LastFrame.end(), current_batch.begin(), current_batch.end());

        set_active_phase(PHASE_SHADOW_DEPTH);
        for (light* L : current_batch)
        {
            L->get_smapvis().begin();
            SceneGraph.render_subspace(L->spatial.sector, L->TransformContext.ShadowContext.combine,
                L->get_position(), TRUE, FALSE, SceneGraph.m_packet);

            bool bNormal = SceneGraph.m_packet.queue_static[0].size() || SceneGraph.m_packet.queue_dynamic[0].size();
            bool bSpecial = SceneGraph.m_packet.queue_static[1].size() || SceneGraph.m_packet.queue_dynamic[1].size() ||
                SceneGraph.m_packet.queue_transparent.size();

            if (bNormal || bSpecial)
            {
                stats.s_merged++;
                render_shadow_map_spot(L);
                RenderBackendLegacy.set_transform_world(Fidentity);
                RenderBackendLegacy.set_transform_view(L->TransformContext.ShadowContext.view);
                RenderBackendLegacy.set_transform_project(L->TransformContext.ShadowContext.project);

                if (ps_r_lighting_flags.test(RFLAG_SUN_DETAILS))
                    Details->Render(DetailsRenderMode::DepthOnly, &L->TransformContext.ShadowContext.combine);

                SceneGraph.Render(SceneGraph.m_packet, SceneGraphRenderType::Opaque, 0);
                L->TransformContext.ShadowContext.transluent = FALSE;

                if (bSpecial)
                {
                    L->TransformContext.ShadowContext.transluent = TRUE;
                    render_shadow_map_spot_transluent(L);
                    SceneGraph.Render(SceneGraph.m_packet, SceneGraphRenderType::Opaque, 1);
                    SceneGraph.Render(SceneGraph.m_packet, SceneGraphRenderType::Transparent);
                }
            }
            else
            {
                stats.s_finalclip++;
            }
            L->get_smapvis().end();
        }

        // --------------------------------------------------------------------
        // 5. АККУМУЛЯЦИЯ СВЕТА (без изменений)
        {
            OPTICK_EVENT("Accumulation");
            set_light_accumulator();
            HOM.Disable();

            if (!LP.v_point.empty())
            {
                OPTICK_EVENT("Point");
                for (size_t i = 0; i < LP.v_point.size();)
                {
                    light* L = LP.v_point[i];
                    L->vis_update();
                    if (L->VisibilityData.visible)
                    {
                        accumulate_point_lights(L);
                        LP.v_point[i] = LP.v_point.back();
                        LP.v_point.pop_back();
                    }
                    else
                        i++;
                }
            }

            if (!LP.v_spot.empty())
            {
                OPTICK_EVENT("Spot");
                for (size_t i = 0; i < LP.v_spot.size();)
                {
                    light* L = LP.v_spot[i];
                    L->vis_update();
                    if (L->VisibilityData.visible)
                    {
                        LR.compute_xf_spot(L);
                        accumulate_spot_lights(L);
                        LP.v_spot[i] = LP.v_spot.back();
                        LP.v_spot.pop_back();
                    }
                    else
                        i++;
                }
            }

            if (!current_batch.empty())
            {
                OPTICK_EVENT("spot shadowed");
                for (light* L : current_batch)
                    accumulate_spot_lights(L);
                current_batch.clear();
            }
        }
    }

    // ------------------------------------------------------------------------
    // 6. ОСТАВШИЕСЯ ИСТОЧНИКИ
    ProcessRemainingLightsOptimized(LP);
}

// Вспомогательная функция для обработки оставшихся источников
void CRender::ProcessRemainingLightsOptimized(light_Package& LP)
{
	OPTICK_EVENT("ProcessRemainingLightsOptimized");

	// Point lights
	if (!LP.v_point.empty())
	{
		OPTICK_EVENT("remaining point");

		// Пакетное обновление видимости
		for (light* L : LP.v_point)
		{
			L->vis_update();
		}

		// Фильтрация и накопление в одном проходе
		LP.v_point.erase(std::remove_if(LP.v_point.begin(), LP.v_point.end(),
										[this](light* L) {
											if (L->VisibilityData.visible)
											{
												accumulate_point_lights(L);
												return true;
											}
											return false;
										}),
						 LP.v_point.end());
	}

	// Spot lights
	if (!LP.v_spot.empty())
	{
		OPTICK_EVENT("remaining spot");

		// Предварительное вычисление матриц для видимых источников
		for (light* L : LP.v_spot)
		{
			L->vis_update();
			if (L->VisibilityData.visible)
			{
				LR.compute_xf_spot(L);
			}
		}

		// Накопление
		LP.v_spot.erase(std::remove_if(LP.v_spot.begin(), LP.v_spot.end(),
									   [this](light* L) {
										   if (L->VisibilityData.visible)
										   {
											   accumulate_spot_lights(L);
											   return true;
										   }
										   return false;
									   }),
						LP.v_spot.end());
	}
}
