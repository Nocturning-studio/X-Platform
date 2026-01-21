#include "stdafx.h"

IC bool pred_area(light* _1, light* _2)
{
	u32 a0 = _1->X.S.size;
	u32 a1 = _2->X.S.size;
	return a0 > a1; // reverse -> descending
}

void CRender::render_lights(light_Package& LP)
{
	OPTICK_EVENT("render_lights");

	// Фильтрация нулевых указателей и невалидных источников
	auto is_valid_light = [](light* L) {
		if (L == nullptr)
			return false;

		// Проверка валидности позиции
		Fvector zero = {0, -1000, 0};
		if (L->get_position().similar(zero, EPS_L))
		{
			Msg("! [Warning] Found light with uninitialized position in render_lights");
			return false;
		}

		return true;
	};

	// Фильтрация для всех типов источников
	LP.v_shadowed.erase(
		std::remove_if(LP.v_shadowed.begin(), LP.v_shadowed.end(), [&](light* L) { return !is_valid_light(L); }),
		LP.v_shadowed.end());

	LP.v_point.erase(std::remove_if(LP.v_point.begin(), LP.v_point.end(), [&](light* L) { return !is_valid_light(L); }),
					 LP.v_point.end());

	LP.v_spot.erase(std::remove_if(LP.v_spot.begin(), LP.v_spot.end(), [&](light* L) { return !is_valid_light(L); }),
					LP.v_spot.end());

	//////////////////////////////////////////////////////////////////////////
	// 1. Оптимизированная фильтрация и подготовка теневых источников
	{
		OPTICK_EVENT("Refactor order based");

		xr_vector<light*>& source = LP.v_shadowed;

		// Более эффективное удаление невидимых источников
		source.erase(std::remove_if(source.begin(), source.end(),
									[this](light* L) {
										L->vis_update();
										if (!L->vis.visible)
											return true;

										LR.compute_xf_spot(L);
										return false;
									}),
					 source.end());
	}

	// 2. Оптимизированная упаковка shadow maps
	{
		OPTICK_EVENT("refactor");

		xr_vector<light*>& source = LP.v_shadowed;
		if (source.empty())
			return;

		xr_vector<light*> refactored;
		refactored.reserve(source.size());

		// Сортируем только один раз в начале
		if (source.size() > 8)
		{
			concurrency::parallel_sort(source.begin(), source.end(), pred_area);
		}
		else
		{
			std::sort(source.begin(), source.end(), pred_area);
		}

		for (u16 smap_ID = 0; !source.empty(); smap_ID++)
		{
			LP_smap_pool.initialize(RenderImplementation.o.smapsize);

			// Более эффективный алгоритм упаковки
			for (auto it = source.begin(); it != source.end();)
			{
				light* L = *it;
				SMAP_Rect R;
				if (LP_smap_pool.push(R, L->X.S.size))
				{
					L->X.S.posX = R.min.x;
					L->X.S.posY = R.min.y;
					L->vis.smap_ID = smap_ID;
					refactored.push_back(L);
					it = source.erase(it);
				}
				else
				{
					// Если не можем упаковать текущий (самый большой), переходим к следующему smap_ID
					if (it == source.begin())
						break;
					++it;
				}
			}
		}

		// save (lights are popped from back)
		std::reverse(refactored.begin(), refactored.end());
		LP.v_shadowed = std::move(refactored); // Используем move для эффективности
	}

	//////////////////////////////////////////////////////////////////////////
	// 3. Оптимизированный рендер теней
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

		// Группировка источников по smap_ID для batch обработки
		xr_vector<light*> current_batch;
		xr_vector<light*>& source = LP.v_shadowed;
		u16 current_sid = source.back()->vis.smap_ID;

		// Извлекаем всю группу с одинаковым smap_ID
		while (!source.empty() && source.back()->vis.smap_ID == current_sid)
		{
			current_batch.push_back(source.back());
			source.pop_back();
		}
		Lights_LastFrame.insert(Lights_LastFrame.end(), current_batch.begin(), current_batch.end());

		// Batch рендер shadow maps для всей группы
		set_active_phase(PHASE_SHADOW_DEPTH);

		for (light* L : current_batch)
		{
			L->get_smapvis().begin();
			// render_subspace уже работает с внутренним m_packet
			SceneGraph.render_subspace(L->spatial.sector, L->X.S.combine, L->get_position(), TRUE);

			// Проверяем очереди через m_packet
			bool bNormal = SceneGraph.m_packet.queue_static[0].size() || SceneGraph.m_packet.queue_dynamic[0].size();
			bool bSpecial = SceneGraph.m_packet.queue_static[1].size() || SceneGraph.m_packet.queue_dynamic[1].size() ||
							SceneGraph.m_packet.queue_transparent.size();

			if (bNormal || bSpecial)
			{
				stats.s_merged++;
				render_shadow_map_spot(L);
				RenderBackend.set_transform_world(Fidentity);
				RenderBackend.set_transform_view(L->X.S.view);
				RenderBackend.set_transform_project(L->X.S.project);

				if (ps_r_lighting_flags.test(RFLAG_SUN_DETAILS))
					Details->Render(DetailsRenderMode::DepthOnly, &L->X.S.combine);

				// SceneGraph.Render использует m_packet
				SceneGraph.Render(SceneGraphRenderType::Opaque, 0);
				L->X.S.transluent = FALSE;

				if (bSpecial)
				{
					L->X.S.transluent = TRUE;
					render_shadow_map_spot_transluent(L);
					SceneGraph.Render(SceneGraphRenderType::Opaque, 1);
					SceneGraph.Render(SceneGraphRenderType::Transparent);
				}
			}
			else
			{
				stats.s_finalclip++;
			}

			L->get_smapvis().end();
		}

		// 4. Оптимизированное накопление света
		{
			OPTICK_EVENT("Accumulation");

			set_light_accumulator();
			HOM.Disable();

			// Быстрое накопление point lights
			if (!LP.v_point.empty())
			{
				OPTICK_EVENT("Point");

				// Обрабатываем несколько источников за проход
				for (size_t i = 0; i < LP.v_point.size();)
				{
					light* L = LP.v_point[i];
					L->vis_update();

					if (L->vis.visible)
					{
						accumulate_point_lights(L);
						// Эффективное удаление
						LP.v_point[i] = LP.v_point.back();
						LP.v_point.pop_back();
					}
					else
					{
						i++;
					}
				}
			}

			// Быстрое накопление spot lights
			if (!LP.v_spot.empty())
			{
				OPTICK_EVENT("Spot");

				for (size_t i = 0; i < LP.v_spot.size();)
				{
					light* L = LP.v_spot[i];
					L->vis_update();

					if (L->vis.visible)
					{
						LR.compute_xf_spot(L);
						accumulate_spot_lights(L);
						LP.v_spot[i] = LP.v_spot.back();
						LP.v_spot.pop_back();
					}
					else
					{
						i++;
					}
				}
			}

			// Накопление теневых источников из текущей группы
			if (!current_batch.empty())
			{
				OPTICK_EVENT("spot shadowed");

				for (light* L : current_batch)
				{
					accumulate_spot_lights(L);
				}

				current_batch.clear();
			}
		}
	}

	// 5. Оптимизированная обработка оставшихся нетеевых источников
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
											if (L->vis.visible)
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
			if (L->vis.visible)
			{
				LR.compute_xf_spot(L);
			}
		}

		// Накопление
		LP.v_spot.erase(std::remove_if(LP.v_spot.begin(), LP.v_spot.end(),
									   [this](light* L) {
										   if (L->vis.visible)
										   {
											   accumulate_spot_lights(L);
											   return true;
										   }
										   return false;
									   }),
						LP.v_spot.end());
	}
}
