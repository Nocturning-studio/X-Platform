#include "stdafx.h"
#include "r_portal_traverser.h"
#include "r_sector.h"
#include "r_portal.h"

// Глобальные настройки (SSA)
extern float r_ssaDISCARD;
extern float r_ssaLOD_A, r_ssaLOD_B;

CPortalTraverser::CPortalTraverser() : m_start_sector(nullptr), m_options(0)
{
}

void CPortalTraverser::CreateResources()
{
	if (!m_shader_fade)
		m_shader_fade.create("portal");
	if (!m_geom_fade)
		m_geom_fade.create(FVF::F_L, RenderBackendLegacy.Vertex.Buffer(), 0);
}

void CPortalTraverser::DestroyResources()
{
	m_shader_fade.destroy();
	m_geom_fade.destroy();
}

void CPortalTraverser::Reset()
{
	m_visible_sectors.clear();
	m_visited_portals.clear();
	m_fade_portals.clear();
}

CPortalTraverser::SectorVisibility& CPortalTraverser::GetOrAddSectorData(CSector* sector)
{
	// Линейный поиск (быстрее map на малых N)
	for (auto& vis : m_visible_sectors)
	{
		if (vis.sector == sector)
			return vis;
	}

	m_visible_sectors.emplace_back();
	SectorVisibility& vis = m_visible_sectors.back();
	vis.sector = sector;
	return vis;
}

void CPortalTraverser::Traverse(CSector* start, CFrustum& frustum, fvec3& view_pos, fmat4x4& xform, u32 options)
{
	VERIFY(start);
	Reset(); // Очистка перед запуском

	// Настройка контекста
	m_options = options;
	m_view_pos = view_pos;
	m_xform = xform;
	m_start_sector = start;

	// Матрица для Scissor теста (Projection -> Viewport 0..1)
	fmat4x4 m_viewport_01 = {0.5f, 0.0f, 0.0f, 0.0f, 0.0f, -0.5f, 0.0f, 0.0f,
							 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f,  0.0f, 1.0f};
	m_xform_proj.mul(m_viewport_01, m_xform);

	if (m_options & VQ_FADE)
		m_fade_portals.reserve(16);

	// Начальный Scissor (весь экран)
	ScissorRect start_scissor;
	start_scissor.set(0, 0, 1, 1);
	start_scissor.depth = 0.f;

	// Запуск рекурсии
	RecursiveTraverse(start, frustum, start_scissor);

	// Пост-обработка: Merge Scissors (если нужно для оптимизации)
	if (m_options & VQ_SCISSOR)
	{
		for (auto& sec_vis : m_visible_sectors)
		{
			sec_vis.merged_scissor.invalidate();
			sec_vis.merged_scissor.depth = flt_max;
			for (const auto& sc : sec_vis.scissors)
			{
				sec_vis.merged_scissor.merge(sc);
				if (sc.depth < sec_vis.merged_scissor.depth)
					sec_vis.merged_scissor.depth = sc.depth;
			}
		}
	}
}

void CPortalTraverser::RecursiveTraverse(CSector* current_sector, const CFrustum& cur_frustum,
										 const ScissorRect& cur_scissor)
{
	// 1. Регистрация видимости сектора
	SectorVisibility& sec_data = GetOrAddSectorData(current_sector);
	sec_data.frustums.push_back(cur_frustum);
	sec_data.scissors.push_back(cur_scissor);

	// 2. Обход порталов
	const auto& portals = current_sector->GetPortals();

	// Используем sPoly на стеке, чтобы избежать аллокаций
	sPoly poly_source, poly_clipped;

	for (CPortal* portal : portals)
	{
		// A. Проверка посещения (чтобы не ходить назад или кругами)
		bool already_visited = false;
		for (CPortal* p : m_visited_portals)
		{
			if (p == portal)
			{
				already_visited = true;
				break;
			}
		}
		if (already_visited)
			continue;

		// B. Определение целевого сектора
		CSector* target_sector = portal->GetOppositeSector(current_sector);

		// Anti-backtracking (не возвращаемся в тот, откуда начали, если это не dual render logic)
		// Если нужна логика Precise Portals, она должна быть реализована здесь.
		// Пока реализуем классическую логику: смотрим "сквозь" портал
		CSector* facing_sector = portal->GetSectorBack(m_view_pos);
		if (facing_sector == current_sector) // Мы смотрим в "спину" порталу?
			continue; // Пропускаем, если портал отвернут (Backface culling портала)

		if (target_sector == m_start_sector) // Не заходим обратно в стартовый
			continue;

		// Создаем копию сферы на стеке.
		fvec3 sphere_pos = portal->GetSphere().P;
		float sphere_rad = portal->GetSphere().R;

		if (!cur_frustum.testSphere_dirty(sphere_pos, sphere_rad))
			continue;

		// D. Screen Space Area Culling & Fade
		if (m_options & VQ_SSA)
		{
			fvec3 dir2portal;
			dir2portal.sub(portal->GetSphere().P, m_view_pos);
			float dist_sq = dir2portal.square_magnitude();
			float ssa = portal->GetSphere().R * portal->GetSphere().R / dist_sq;

			// Учитываем угол обзора
			dir2portal.div(_sqrt(dist_sq));
			ssa *= _abs(portal->GetPlane().n.dotproduct(dir2portal));

			if (ssa < r_ssaDISCARD)
				continue;

			if (m_options & VQ_FADE)
			{
				if (ssa < r_ssaLOD_A)
					m_fade_portals.push_back(std::make_pair(portal, ssa));
				if (ssa < r_ssaLOD_B)
					continue; // Слишком маленький, рисуем как Fade и не идем дальше
			}
		}

		// E. Geometric Clipping (Frustum)
		auto verts = portal->GetVertices();
		poly_source.assign(verts.begin(), verts.size());
		poly_clipped.clear();

		// ClipPoly возвращает указатель на результат или null
		sPoly* clipped_poly = cur_frustum.ClipPoly(poly_source, poly_clipped);

		if (!clipped_poly || clipped_poly->empty())
			continue;

		// F. Scissor Calculation
		ScissorRect next_scissor = cur_scissor;

		if (m_options & VQ_SCISSOR)
		{
			Fbox2 bb;
			bb.invalidate();
			float depth = flt_max;

			for (const fvec3& v : *clipped_poly)
			{
				fvec4 t;
				// Трансформация в Screen Space (0..1)
				t.x = v.x * m_xform_proj._11 + v.y * m_xform_proj._21 + v.z * m_xform_proj._31 + m_xform_proj._41;
				t.y = v.x * m_xform_proj._12 + v.y * m_xform_proj._22 + v.z * m_xform_proj._32 + m_xform_proj._42;
				t.z = v.x * m_xform_proj._13 + v.y * m_xform_proj._23 + v.z * m_xform_proj._33 + m_xform_proj._43;
				t.w = v.x * m_xform_proj._14 + v.y * m_xform_proj._24 + v.z * m_xform_proj._34 + m_xform_proj._44;

				if (t.w > EPS)
					t.mul(1.f / t.w);

				bb.min.x = _min(bb.min.x, t.x);
				bb.max.x = _max(bb.max.x, t.x);
				bb.min.y = _min(bb.min.y, t.y);
				bb.max.y = _max(bb.max.y, t.y);
				depth = _min(depth, t.z);
			}

			// Пересечение с предыдущим сциссором
			if (depth > EPS) // Если не за спиной
			{
				next_scissor.min.x = _max(cur_scissor.min.x, bb.min.x);
				next_scissor.max.x = _min(cur_scissor.max.x, bb.max.x);
				next_scissor.min.y = _max(cur_scissor.min.y, bb.min.y);
				next_scissor.max.y = _min(cur_scissor.max.y, bb.max.y);
				next_scissor.depth = depth;

				// Если область пустая - отсекаем
				if (next_scissor.min.x >= next_scissor.max.x || next_scissor.min.y >= next_scissor.max.y)
					continue;

				// HOM Culling (Быстрый тест по AABB сциссора)
				if ((m_options & VQ_HOM) && !RenderImplementation.HOM.visible(next_scissor, depth))
					continue;
			}
			else
			{
				// Если портал пересекает near plane, scissor не эффективен,
				// проверяем полигон целиком через HOM (медленно)
				if ((m_options & VQ_HOM) && !RenderImplementation.HOM.visible(*clipped_poly))
					continue;
			}
		}
		else
		{
			// Если сциссора нет, проверяем просто полигон
			if ((m_options & VQ_HOM) && !RenderImplementation.HOM.visible(*clipped_poly))
				continue;
		}

		// G. Рекурсия
		CFrustum next_frustum;
		// Создаем новый фрустум, ограниченный порталом
		fvec3 plane_n = portal->GetPlane().n;
		next_frustum.CreateFromPortal(clipped_poly, plane_n, m_view_pos, m_xform);

		m_visited_portals.push_back(portal); // Помечаем
		RecursiveTraverse(target_sector, next_frustum, next_scissor);
	}
}

void CPortalTraverser::RenderFade()
{
	// 1. Проверка наличия данных
	if (m_fade_portals.empty())
		return;

	// 2. Сортировка Back-to-Front (для корректного Alpha Blending)
	// Используем лямбду с захватом позиции камеры (m_view_pos)
	fvec3 camera_pos = m_view_pos;

	std::sort(m_fade_portals.begin(), m_fade_portals.end(),
			  [camera_pos](const std::pair<CPortal*, float>& a, const std::pair<CPortal*, float>& b) {
				  // Сравниваем квадрат дистанции до центров порталов
				  float d1 = camera_pos.distance_to_sqr(a.first->GetSphere().P);
				  float d2 = camera_pos.distance_to_sqr(b.first->GetSphere().P);
				  return d2 > d1; // По убыванию (от дальнего к ближнему)
			  });

	// 3. Расчет необходимого размера буфера
	u32 poly_count = 0;
	for (const auto& item : m_fade_portals)
	{
		// Портал — это выпуклый многоугольник. Триангуляция "веером" (Triangle Fan).
		// Количество треугольников = кол-во вершин - 2.
		poly_count += item.first->GetVertices().size() - 2;
	}

	if (poly_count == 0)
		return;

	// 4. Блокировка вершинного буфера
	u32 v_offset = 0;
	// Используем формат FVF::L (Point + Color)
	FVF::L* v_ptr = (FVF::L*)RenderBackendLegacy.Vertex.Lock(poly_count * 3, m_geom_fade.stride(), v_offset);

	// 5. Подготовка констант цвета
	float ssa_range = r_ssaLOD_A - r_ssaLOD_B;
	if (ssa_range < EPS)
		ssa_range = EPS;

	// Получаем текущий ambient цвет из окружения
	fvec3 ambient_f = g_pGamePersistent->Environment().CurrentEnv->ambient;
	u32 ambient_clr = color_rgba_f(ambient_f.x, ambient_f.y, ambient_f.z, 0);

	// 6. Заполнение геометрии
	for (const auto& item : m_fade_portals)
	{
		CPortal* portal = item.first;
		float ssa = item.second;

		// Вычисление альфы на основе Screen Space Area
		// Чем меньше SSA (дальше портал), тем плотнее "туман"
		float ssa_diff = ssa - r_ssaLOD_B;
		float ssa_scale = ssa_diff / ssa_range;
		int alpha = iFloor((1.0f - ssa_scale) * 255.5f);
		clamp(alpha, 0, 255);

		// Подмешиваем альфу в цвет эмбиента
		u32 final_clr = subst_alpha(ambient_clr, u32(alpha));

		// Триангуляция многоугольника портала
		const auto& verts = portal->GetVertices();
		u32 tri_count = verts.size() - 2;

		// Строим Triangle List из Triangle Fan (v[0], v[i+1], v[i+2])
		for (u32 k = 0; k < tri_count; ++k)
		{
			v_ptr->set(verts[0], final_clr);
			v_ptr++;
			v_ptr->set(verts[k + 1], final_clr);
			v_ptr++;
			v_ptr->set(verts[k + 2], final_clr);
			v_ptr++;
		}
	}

	RenderBackendLegacy.Vertex.Unlock(poly_count * 3, m_geom_fade.stride());

	// 7. Отрисовка
	RenderBackendLegacy.set_transform_world(Fidentity);
	RenderBackendLegacy.set_Shader(m_shader_fade);
	RenderBackendLegacy.set_Geometry(m_geom_fade);

	// Отключаем отсечение задних граней, чтобы "туман" был виден с любой стороны портала
	RenderBackendLegacy.set_CullMode(CULL_DISABLE);

	RenderBackendLegacy.Render(D3DPT_TRIANGLELIST, v_offset, poly_count);

	// Восстанавливаем Cull Mode
	RenderBackendLegacy.set_CullMode(CULL_BACKFACE);

	// 8. Очистка списка (данные устаревают каждый кадр)
	m_fade_portals.clear();
}
