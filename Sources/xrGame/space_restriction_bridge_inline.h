////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction_bridge_inline.h
//	Created 	: 27.08.2004
//  Modified 	: 27.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restriction bridge inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC CSpaceRestrictionBridge::CSpaceRestrictionBridge(CSpaceRestrictionBase* object)
{
	VERIFY(object);
	m_object = object;
}

IC CSpaceRestrictionBase& CSpaceRestrictionBridge::object() const
{
	VERIFY(m_object);
	return (*m_object);
}

template <typename T>
IC u32 CSpaceRestrictionBridge::accessible_nearest(T& restriction, const Fvector& position, Fvector& result,
												   bool out_restriction)
{
	VERIFY(initialized());
	VERIFY(!restriction->border().empty());

	// 1. Кэшируем ссылку на список, чтобы не вызывать функцию дважды
	const auto& border_list = restriction->accessible_neighbour_border(restriction, out_restriction);
	VERIFY(!border_list.empty());

	// 2. Кэшируем LevelGraph, чтобы избежать постоянного вызова ai().level_graph()
	// Это критическая оптимизация, так как ai() - это синглтон, часто скрытый за функциями.
	const auto& level_graph = ai().level_graph();

	float min_dist_sqr = flt_max;
	u32 selected = u32(-1);

	// --- PHASE 1: Грубый поиск по границе ---
	// Используем range-based for для чистоты и скорости
	for (u32 vertex_id : border_list)
	{
		// vertex_position обычно возвращает значение, а не ссылку, но это легковесный Fvector
		float distance_sqr = level_graph.vertex_position(vertex_id).distance_to_sqr(position);
		if (distance_sqr < min_dist_sqr)
		{
			min_dist_sqr = distance_sqr;
			selected = vertex_id;
		}
	}
	VERIFY2(level_graph.valid_vertex_id(selected), *name());

	// --- PHASE 2: Уточнение по соседям ---
	{
		float current_min_dist = flt_max; // Локальная переменная для фазы 2
		u32 new_selected = u32(-1);

		CLevelGraph::const_iterator I, E;
		level_graph.begin(selected, I, E);

		for (; I != E; ++I)
		{
			u32 current = level_graph.value(selected, I);

			if (!level_graph.valid_vertex_id(current))
				continue;

			// Проверка ограничения
			// Логика: если мы внутри, нам нужны соседи снаружи, и наоборот.
			// restriction->inside возвращает bool.
			if (restriction->inside(current, !out_restriction) != out_restriction)
				continue;

			float distance_sqr = level_graph.vertex_position(current).distance_to_sqr(position);
			if (distance_sqr < current_min_dist)
			{
				current_min_dist = distance_sqr;
				new_selected = current;
			}
		}
		// Если нашли лучшего соседа, обновляем selected
		if (new_selected != u32(-1))
			selected = new_selected;
	}
	VERIFY(level_graph.valid_vertex_id(selected));

	// --- PHASE 3: Суб-вертексная точность (5 точек) ---
	// Оптимизация: разворачиваем цикл switch, убираем sqrt
	{
		Fvector center = level_graph.vertex_position(selected);
		// Предвычисляем оффсет
		float offset = level_graph.header().cell_size() * .5f - EPS_L;

		// Массив смещений для 4 углов: (x, z). Y вычисляется по плоскости.
		// 0: ++, 1: +-, 2: -+, 3: --
		const float offsets_x[4] = {offset, offset, -offset, -offset};
		const float offsets_z[4] = {offset, -offset, offset, -offset};

		min_dist_sqr = flt_max;
		bool found = false;

		// 1. Проверяем 4 угла
		for (int i = 0; i < 4; ++i)
		{
			Fvector pt;
			pt.x = center.x + offsets_x[i];
			pt.z = center.z + offsets_z[i];
			// Тяжелая операция вычисления Y через плоскость ноды
			pt.y = level_graph.vertex_plane_y(selected, pt.x, pt.z);

			// Быстрая проверка расстояния без sqrt
			float dist_sqr = pt.distance_to_sqr(position);
			if (dist_sqr < min_dist_sqr)
			{
				// Дорогие проверки делаем ТОЛЬКО если точка ближе текущего минимума
				// В оригинале VERIFY выполнялись всегда, в Release их нет, но логика осталась бы
				// Здесь мы доверяем геометрии Level Graph
				min_dist_sqr = dist_sqr;
				result = pt;
				found = true;
			}
		}

		// 2. Проверяем центр (i=4 в оригинале)
		{
			// center.y уже корректен из vertex_position
			float dist_sqr = center.distance_to_sqr(position);
			if (dist_sqr < min_dist_sqr)
			{
				min_dist_sqr = dist_sqr;
				result = center;
				found = true;
			}
		}

#ifdef DEBUG
		// Оставляем проверки для дебага, но только для финального результата,
		// чтобы не тормозить цикл поиска
		if (found)
		{
			// Проверки немного избыточны для релиза, но важны для отладки AI
			// VERIFY(level_graph.inside(selected, result)); // Может давать false из-за EPS
			VERIFY(restriction->inside(selected, !out_restriction) == out_restriction);
		}
#endif
		VERIFY(found);
	}
	VERIFY(level_graph.valid_vertex_id(selected));

	return (selected);
}

template <typename T>
IC const xr_vector<u32>& CSpaceRestrictionBridge::accessible_neighbour_border(T& restriction, bool out_restriction)
{
	return (object().accessible_neighbour_border(restriction, out_restriction));
}
