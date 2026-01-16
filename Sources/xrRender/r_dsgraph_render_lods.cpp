#include "stdafx.h"
#include "flod.h"

#ifdef _EDITOR
#include "igame_persistent.h"
#include "environment.h"
#else
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
#endif

extern float r_ssaLOD_A;
extern float r_ssaLOD_B;

// Предикат для сортировки граней (можно оставить глобальным или статическим)
static bool pred_dot_std(const std::pair<float, u32>& _1, const std::pair<float, u32>& _2)
{
	return _1.first < _2.first;
}

void CSceneGraph::r_dsgraph_render_lods(bool _setup_zb, bool _clear)
{
	PROFILE_FUNCTION();

	if (_setup_zb)
		mapLOD.getLR(lstLODs); // front-to-back
	else
		mapLOD.getRL(lstLODs); // back-to-front

	if (lstLODs.empty())
		return;

	// *** 1. Подготовка буфера и констант ***
	u32 shid = _setup_zb ? SE_R1_LMODELS : SE_R1_NORMAL_LQ;
	FLOD* firstV = (FLOD*)lstLODs[0].pVisual;

	u32 vOffset;
	// Блокируем память один раз для всех LODов
	FLOD::_hw* V_start = (FLOD::_hw*)RenderBackend.Vertex.Lock(lstLODs.size() * 4, firstV->geom->vb_stride, vOffset);

	float ssaRange = r_ssaLOD_A - r_ssaLOD_B;
	if (ssaRange < EPS_S)
		ssaRange = EPS_S;

	// Нужно захватить переменные для лямбды
	const float f_ssaLOD_B = r_ssaLOD_B;
	const Fvector vCameraPos = Device.vCameraPosition;

	// *** 2. ПАРАЛЛЕЛЬНЫЙ ПРОХОД: Генерация геометрии ***
	// Используем PPL для распараллеливания тяжелой математики
	concurrency::parallel_for(size_t(0), lstLODs.size(), [&](size_t i) {
		// Получаем указатель на 4 вершины, принадлежащие этому LOD-у
		FLOD::_hw* V = V_start + (i * 4);
		R_dsgraph::_LodItem& P = lstLODs[i];

		// calculate alpha
		float ssaDiff = P.ssa - f_ssaLOD_B;
		float scale = ssaDiff / ssaRange;
		int iA = iFloor((1.0f - scale) * 255.f);
		u32 uA = u32(clampr(iA, 0, 255));

		// calculate direction and shift
		FLOD* lodV = (FLOD*)P.pVisual;
		Fvector Ldir, shift;
		Ldir.sub(lodV->vis.sphere.P, vCameraPos).normalize();
		shift.mul(Ldir, -.5f * lodV->vis.sphere.R);

		// gen geometry
		FLOD::_face* facets = lodV->facets;

		// Используем локальный svector, это безопасно для потоков
		svector<std::pair<float, u32>, 8> selector;
		for (u32 s = 0; s < 8; s++)
			selector.push_back(mk_pair(Ldir.dotproduct(facets[s].N), s));

		// ВАЖНО: Используем std::sort, а не parallel_sort.
		// Запуск параллельной сортировки для 8 элементов внутри параллельного цикла убьет производительность.
		std::sort(selector.begin(), selector.end(), pred_dot_std);

		float dot_best = selector[selector.size() - 1].first;
		float dot_next = selector[selector.size() - 2].first;
		float dot_next_2 = selector[selector.size() - 3].first;
		u32 id_best = selector[selector.size() - 1].second;
		u32 id_next = selector[selector.size() - 2].second;

		// Now we have two "best" planes, calculate factor, and approx normal
		float fA = dot_best, fB = dot_next, fC = dot_next_2;
		float alpha = 0.5f + 0.5f * (1 - (fB - fC) / (fA - fC));
		int iF = iFloor(alpha * 255.5f);
		u32 uF = u32(clampr(iF, 0, 255));

		// Fill VB
		FLOD::_face& FA = facets[id_best];
		FLOD::_face& FB = facets[id_next];

		static const int vid[4] = {3, 0, 2, 1}; // const для безопасности

		for (u32 vit = 0; vit < 4; vit++)
		{
			int id = vid[vit];
			// Пишем прямо в память по вычисленному смещению
			V[vit].p0.add(FB.v[id].v, shift);
			V[vit].p1.add(FA.v[id].v, shift);
			V[vit].n0 = FB.N;
			V[vit].n1 = FA.N;
			V[vit].sun_af = color_rgba(FB.v[id].c_sun, FA.v[id].c_sun, uA, uF);
			V[vit].t0 = FB.v[id].t;
			V[vit].t1 = FA.v[id].t;
			V[vit].rgbh0 = FB.v[id].c_rgb_hemi;
			V[vit].rgbh1 = FA.v[id].c_rgb_hemi;
		}
	});

	// Разблокируем буфер — данные уже там
	RenderBackend.Vertex.Unlock(lstLODs.size() * 4, firstV->geom->vb_stride);

	// *** 3. ПОСЛЕДОВАТЕЛЬНЫЙ ПРОХОД: Группировка ***
	// Этот код выполняется очень быстро, параллелить его нет смысла (и опасно из-за порядка отрисовки)
	if (!lstLODs.empty())
	{
		ref_selement cur_S = lstLODs[0].pVisual->shader->E[shid];
		int cur_count = 0;

		for (u32 i = 0; i < lstLODs.size(); i++)
		{
			R_dsgraph::_LodItem& P = lstLODs[i];
			if (P.pVisual->shader->E[shid] == cur_S)
			{
				cur_count++;
			}
			else
			{
				lstLODgroups.push_back(cur_count);
				cur_S = P.pVisual->shader->E[shid];
				cur_count = 1;
			}
		}
		// Не забываем последнюю группу
		lstLODgroups.push_back(cur_count);
	}

	// *** 4. RENDER ***
	////OPTICK_EVENT("CSceneGraph::r_dsgraph_render_lods - render");

	int current = 0;
	RenderBackend.set_xform_world(Fidentity);

	for (u32 g = 0; g < lstLODgroups.size(); g++)
	{
		int p_count = lstLODgroups[g];

		// Проверка на 0, на всякий случай
		if (p_count > 0)
		{
			RenderBackend.set_Element(lstLODs[current].pVisual->shader->E[shid]);
			RenderBackend.set_Geometry(firstV->geom);
			RenderBackend.Render(D3DPT_TRIANGLELIST, vOffset, 0, 4 * p_count, 0, 2 * p_count);
			RenderBackend.stat.r.s_flora_lods.add(4 * p_count);

			current += p_count;
			vOffset += 4 * p_count;
		}
	}

	// *** 5. Cleanup ***
	lstLODs.clear();
	lstLODgroups.clear();

	if (_clear)
		mapLOD.clear();
}
