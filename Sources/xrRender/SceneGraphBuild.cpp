#include "stdafx.h"

#include "..\xrEngine\fhierrarhyvisual.h"
#include "..\xrEngine\SkeletonCustom.h"
#include "..\xrEngine\fmesh.h"
#include "..\xrEngine\irenderable.h"

#include "flod.h"
#include "particlegroup.h"
#include "FTreeVisual.h"

using namespace SceneGraphTypes;

CSceneGraph::CSceneGraph()
{
	// 1. Инициализация глобальных настроек
	m_feedback_interface = 0;
	val_feedback_breakp = 0;
	m_culling_bounds_recorder = 0;
	m_traversal_marker = 0;
	m_fetch_config = SceneGraphFetchConfig(true, true, false);
	b_loaded = FALSE;

	// 2. Счетчики
	counter_S = 0;
	counter_D = 0;

	// Примечание: m_packet и m_scratch инициализируются своими конструкторами по умолчанию
	// (std::vector и FixedMAP конструкторы сработают автоматически).
}

void CSceneGraph::destroy()
{
	// 1. Очистка рабочих буферов (Scratch Pad)
	// Это временные буферы для сортировки, очищаем память векторов.
	m_scratch.nrmVS.clear();
	m_scratch.nrmPS.clear();
	m_scratch.nrmCS.clear();
	m_scratch.nrmStates.clear();
	m_scratch.nrmTextures.clear();
	m_scratch.nrmTexturesTemp.clear();

	m_scratch.matVS.clear();
	m_scratch.matPS.clear();
	m_scratch.matCS.clear();
	m_scratch.matStates.clear();
	m_scratch.matTextures.clear();
	m_scratch.matTexturesTemp.clear();

	// 2. Очистка списков результатов (Packet Lists)
	m_packet.lstLODs.clear();
	m_packet.lstLODgroups.clear();
	m_packet.lstRenderables.clear();

	m_packet.m_visuals_static_visible.clear();
	m_packet.m_visuals_dynamic_visible.clear();

	// 3. Уничтожение Fixed Maps (Packet Queues)
	// Важно вызвать destroy(), чтобы освободить память аллокатора FixedMAP
	m_packet.queue_static[0].destroy();
	m_packet.queue_static[1].destroy();

	m_packet.queue_dynamic[0].destroy();
	m_packet.queue_dynamic[1].destroy();

	m_packet.queue_transparent.destroy();
	m_packet.queue_hud.destroy();

	m_packet.mapLOD.destroy();
	m_packet.queue_distortion.destroy();
	m_packet.queue_wallmarks.destroy();
	m_packet.mapEmissive.destroy();
}

void CSceneGraph::SetFetchConfig(const SceneGraphFetchConfig& config)
{
	m_fetch_config = config;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Scene graph actual insertion and sorting ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
float r_ssaDISCARD;
float r_ssaDONTSORT;
float r_ssaLOD_A, r_ssaLOD_B;
float r_ssaGLOD_start, r_ssaGLOD_end;
float r_ssaHZBvsTEX;

ICF float CalcScreenSpaceArea(float& distSQ, Fvector& C, IRender_Visual* V)
{
	float R = V->vis.sphere.R + 0;
	distSQ = Engine.RenderView.Position.distance_to_sqr(C) + EPS;
	return R / distSQ;
}
ICF float CalcScreenSpaceArea(float& distSQ, Fvector& C, float R)
{
	distSQ = Engine.RenderView.Position.distance_to_sqr(C) + EPS;
	return R / distSQ;
}

// Добавлен аргумент ctx
void CSceneGraph::EnqueueDynamic(IRender_Visual* pVisual, Fvector& object_center, const SceneTraversalContext& ctx)
{
	// 1. Проверка на повторное добавление в этом кадре
	if (pVisual->vis.m_traversal_marker == m_traversal_marker)
		return;
	pVisual->vis.m_traversal_marker = m_traversal_marker;

	// 2. Вычисление метрики экрана
	float distance_sq;
	float screen_space_area = CalcScreenSpaceArea(distance_sq, object_center, pVisual);

	if (screen_space_area <= r_ssaDISCARD)
		return;

	// 3. Обработка искажений
	ShaderElement* shader_distortion = pVisual->shader->E[4]._get();

	if (shader_distortion && shader_distortion->flags.bDistort)
	{
		bool is_priority_allowed = (shader_distortion->flags.iPriority / 2 == 0) ? m_fetch_config.fetch_priority_0
																				 : m_fetch_config.fetch_priority_1;

		if (is_priority_allowed)
		{
			auto* node = m_packet.queue_distortion.insertInAnyWay(distance_sq);

			node->val.ScreenSpaceArea = screen_space_area;
			// Используем ctx
			node->val.pObject = ctx.current_owner;
			node->val.pVisual = pVisual;
			node->val.Matrix = *ctx.current_transform;
			node->val.se = shader_distortion;
		}
	}

	// 4. Выбор шейдера
	CRender& render_impl = RenderImplementation;
	ShaderElement* shader_element = render_impl.rimp_select_sh_dynamic(pVisual, distance_sq);

	if (!shader_element)
		return;

	// 5. Фильтрация по приоритету
	u32 priority = shader_element->flags.iPriority / 2;
	if (priority == 0 && !m_fetch_config.fetch_priority_0)
		return;
	if (priority == 1 && !m_fetch_config.fetch_priority_1)
		return;

	// Используем ctx
	if (ctx.is_invisible_mode)
		return;

	// 6. Маршрутизация по очередям

	// --- HUD ---
	// Используем ctx
	if (ctx.is_hud_pass)
	{
		if (shader_element->flags.bStrictB2F)
		{
			auto* node = m_packet.queue_transparent.insertInAnyWay(distance_sq);
			node->val.ScreenSpaceArea = screen_space_area;
			// Используем ctx
			node->val.pObject = ctx.current_owner;
			node->val.pVisual = pVisual;
			node->val.Matrix = *ctx.current_transform;
			node->val.se = shader_element;
		}
		else
		{
			auto* node = m_packet.queue_hud.insertInAnyWay(distance_sq);
			node->val.ScreenSpaceArea = screen_space_area;
			// Используем ctx
			node->val.pObject = ctx.current_owner;
			node->val.pVisual = pVisual;
			node->val.Matrix = *ctx.current_transform;
			node->val.se = shader_element;
		}
		return;
	}

	// --- Strict Sorting ---
	if (shader_element->flags.bStrictB2F)
	{
		auto* node = m_packet.queue_transparent.insertInAnyWay(distance_sq);
		node->val.ScreenSpaceArea = screen_space_area;
		// Используем ctx
		node->val.pObject = ctx.current_owner;
		node->val.pVisual = pVisual;
		node->val.Matrix = *ctx.current_transform;
		node->val.se = shader_element;
		return;
	}

	// --- Emissive ---
	if (shader_element->flags.bEmissive)
	{
		auto* node = m_packet.mapEmissive.insertInAnyWay(distance_sq);
		node->val.ScreenSpaceArea = screen_space_area;
		// Используем ctx
		node->val.pObject = ctx.current_owner;
		node->val.pVisual = pVisual;
		node->val.Matrix = *ctx.current_transform;
		node->val.se = pVisual->shader->E[4]._get();
	}

	// --- Wallmarks ---
	if (shader_element->flags.bWmark && m_fetch_config.fetch_wallmarks)
	{
		auto* node = m_packet.queue_wallmarks.insertInAnyWay(distance_sq);
		node->val.ScreenSpaceArea = screen_space_area;
		// Используем ctx
		node->val.pObject = ctx.current_owner;
		node->val.pVisual = pVisual;
		node->val.Matrix = *ctx.current_transform;
		node->val.se = shader_element;
		return;
	}

	// --- Opaque ---

	// Используем ctx для создания ноды
	DynamicRenderNode item = {screen_space_area, ctx.current_owner, pVisual, *ctx.current_transform};

	SPass& pass = *shader_element->passes.front();
	auto& target_map = m_packet.queue_dynamic[priority];

#ifdef USE_RESOURCE_DEBUGGER
	auto* node_vs = target_map.insert(pass.vs);
	auto* node_ps = node_vs->val.insert(pass.ps);
#else
	auto* node_vs = target_map.insert(pass.vs->sh);
	auto* node_ps = node_vs->val.insert(pass.ps->sh);
#endif
	auto* node_cs = node_ps->val.insert(pass.constants._get());
	auto* node_state = node_cs->val.insert(pass.state->state);
	auto* node_tex = node_state->val.insert(pass.T._get());

	node_tex->val.push_back(item);

	// Обновление SSA
	if (screen_space_area > node_tex->val.ScreenSpaceArea)
	{
		node_tex->val.ScreenSpaceArea = screen_space_area;
		if (screen_space_area > node_state->val.ScreenSpaceArea)
		{
			node_state->val.ScreenSpaceArea = screen_space_area;
			if (screen_space_area > node_cs->val.ScreenSpaceArea)
			{
				node_cs->val.ScreenSpaceArea = screen_space_area;
				if (screen_space_area > node_ps->val.ScreenSpaceArea)
				{
					node_ps->val.ScreenSpaceArea = screen_space_area;
					if (screen_space_area > node_vs->val.ScreenSpaceArea)
					{
						node_vs->val.ScreenSpaceArea = screen_space_area;
					}
				}
			}
		}
	}

	if (m_culling_bounds_recorder)
	{
		Fbox3 temp_box;
		// Используем матрицу из ctx
		temp_box.transform(pVisual->vis.box, *ctx.current_transform);
		m_culling_bounds_recorder->push_back(temp_box);
	}
}

// Добавлен аргумент ctx
void CSceneGraph::EnqueueStatic(IRender_Visual* pVisual, const SceneTraversalContext& ctx)
{
	// 1. Проверка на повторное добавление
	if (pVisual->vis.m_traversal_marker == m_traversal_marker)
		return;
	pVisual->vis.m_traversal_marker = m_traversal_marker;

	// 2. Вычисление метрики экрана
	float distance_sq;
	float screen_space_area = CalcScreenSpaceArea(distance_sq, pVisual->vis.sphere.P, pVisual);

	if (screen_space_area <= r_ssaDISCARD)
		return;

	// 3. Обработка искажений
	ShaderElement* shader_distortion = pVisual->shader->E[4]._get();

	if (shader_distortion && shader_distortion->flags.bDistort)
	{
		bool is_priority_allowed = (shader_distortion->flags.iPriority / 2 == 0) ? m_fetch_config.fetch_priority_0
																				 : m_fetch_config.fetch_priority_1;

		if (is_priority_allowed)
		{
			auto* node = m_packet.queue_distortion.insertInAnyWay(distance_sq);
			node->val.ScreenSpaceArea = screen_space_area;
			node->val.pObject = nullptr;
			node->val.pVisual = pVisual;
			node->val.Matrix = Fidentity;
			node->val.se = shader_distortion;
		}
	}

	// 4. Выбор шейдера
	CRender& render_impl = RenderImplementation;
	ShaderElement* shader_element = render_impl.rimp_select_sh_static(pVisual, distance_sq);

	if (!shader_element)
		return;

	// 5. Фильтрация по приоритету
	u32 priority = shader_element->flags.iPriority / 2;
	if (priority == 0 && !m_fetch_config.fetch_priority_0)
		return;
	if (priority == 1 && !m_fetch_config.fetch_priority_1)
		return;

	// 6. Маршрутизация по очередям

	// --- Strict Sorting ---
	if (shader_element->flags.bStrictB2F)
	{
		auto* node = m_packet.queue_transparent.insertInAnyWay(distance_sq);
		node->val.ScreenSpaceArea = screen_space_area;
		node->val.pObject = nullptr;
		node->val.pVisual = pVisual;
		node->val.Matrix = Fidentity;
		node->val.se = shader_element;
		return;
	}

	// --- Emissive ---
	if (shader_element->flags.bEmissive)
	{
		auto* node = m_packet.mapEmissive.insertInAnyWay(distance_sq);
		node->val.ScreenSpaceArea = screen_space_area;
		node->val.pObject = nullptr;
		node->val.pVisual = pVisual;
		node->val.Matrix = Fidentity;
		node->val.se = pVisual->shader->E[4]._get();
	}

	// --- Wallmarks ---
	if (shader_element->flags.bWmark && m_fetch_config.fetch_wallmarks)
	{
		auto* node = m_packet.queue_wallmarks.insertInAnyWay(distance_sq);
		node->val.ScreenSpaceArea = screen_space_area;
		node->val.pObject = nullptr;
		node->val.pVisual = pVisual;
		node->val.Matrix = Fidentity;
		node->val.se = shader_element;
		return;
	}

	// 7. Обратная связь
	if (m_feedback_interface && counter_S == val_feedback_breakp)
	{
		m_feedback_interface->rfeedback_static(pVisual);
	}

	// 8. Opaque
	counter_S++;

	SPass& pass = *shader_element->passes.front();
	auto& target_map = m_packet.queue_static[priority];

#ifdef USE_RESOURCE_DEBUGGER
	auto* node_vs = target_map.insert(pass.vs);
	auto* node_ps = node_vs->val.insert(pass.ps);
#else
	auto* node_vs = target_map.insert(pass.vs->sh);
	auto* node_ps = node_vs->val.insert(pass.ps->sh);
#endif
	auto* node_cs = node_ps->val.insert(pass.constants._get());
	auto* node_state = node_cs->val.insert(pass.state->state);
	auto* node_tex = node_state->val.insert(pass.T._get());

	StaticRenderNode item = {screen_space_area, pVisual};
	node_tex->val.push_back(item);

	if (screen_space_area > node_tex->val.ScreenSpaceArea)
	{
		node_tex->val.ScreenSpaceArea = screen_space_area;
		if (screen_space_area > node_state->val.ScreenSpaceArea)
		{
			node_state->val.ScreenSpaceArea = screen_space_area;
			if (screen_space_area > node_cs->val.ScreenSpaceArea)
			{
				node_cs->val.ScreenSpaceArea = screen_space_area;
				if (screen_space_area > node_ps->val.ScreenSpaceArea)
				{
					node_ps->val.ScreenSpaceArea = screen_space_area;
					if (screen_space_area > node_vs->val.ScreenSpaceArea)
					{
						node_vs->val.ScreenSpaceArea = screen_space_area;
					}
				}
			}
		}
	}

	if (m_culling_bounds_recorder)
	{
		m_culling_bounds_recorder->push_back(pVisual->vis.box);
	}
}

// ===============================================================================================
//  Optimization Data & Constants (Anonymous Namespace)
// ===============================================================================================
namespace
{
// Значения для разных уровней качества (Low, Med, High, Ultra)
// x = Low, y = Med, z = High, w = Ultra

// Структура для хранения пары Dist/Size для одного уровня детализации
struct CullLevel
{
	Fvector4 dist;
	Fvector4 size;
};

// Массив уровней оптимизации для СТАТИКИ (12 уровней)
static const CullLevel s_static_cull_levels[] = {
	// Level 1
	{{150.f, 50.f, 50.f, 40.f}, {10.f, 25.f, 50.f, 50.f}},
	// Level 2
	{{200.f, 150.f, 150.f, 125.f}, {100.f, 200.f, 400.f, 500.f}},
	// Level 3
	{{250.f, 200.f, 200.f, 175.f}, {500.f, 1000.f, 1500.f, 1750.f}},
	// Level 4
	{{350.f, 300.f, 300.f, 250.f}, {2500.f, 2500.f, 5000.f, 5250.f}},
	// Level 5
	{{400.f, 400.f, 350.f, 300.f}, {7000.f, 7000.f, 20000.f, 25000.f}},
	// Level 6
	{{800.f, 750.f, 700.f, 600.f}, {12000.f, 15000.f, 30000.f, 40000.f}},
	// Level 7
	{{1000.f, 900.f, 900.f, 800.f}, {18000.f, 20000.f, 45000.f, 50000.f}},
	// Level 8
	{{1200.f, 1100.f, 1100.f, 1000.f}, {25000.f, 35000.f, 55000.f, 65000.f}},
	// Level 9
	{{1500.f, 1250.f, 1250.f, 1200.f}, {40000.f, 55000.f, 70000.f, 85000.f}},
	// Level 10
	{{1800.f, 1500.f, 1500.f, 1500.f}, {60000.f, 75000.f, 90000.f, 150000.f}},
	// Level 11
	{{2000.f, 1750.f, 1750.f, 1750.f}, {100000.f, 120000.f, 150000.f, 250000.f}},
	// Level 12
	{{2500.f, 2000.f, 2000.f, 2000.f}, {150000.f, 200000.f, 250000.f, 500000.f}}};

// Массив уровней оптимизации для ДИНАМИКИ (5 уровней)
static const CullLevel s_dynamic_cull_levels[] = {
	// Level 1
	{{80.f, 40.f, 30.f, 30.f}, {1.f, 2.f, 5.0f, 7.5f}},
	// Level 2
	{{150.f, 100.f, 80.f, 50.f}, {3.f, 4.f, 10.f, 15.f}},
	// Level 3
	{{250.f, 200.f, 150.f, 110.f}, {4000.f, 4000.f, 4000.f, 4000.f}},
	// Level 4
	{{500.f, 400.f, 300.f, 250.f}, {10000.f, 10000.f, 10000.f, 10000.f}},
	// Level 5
	{{750.f, 600.f, 500.f, 400.f}, {25000.f, 25000.f, 25000.f, 25000.f}}};

const float BASE_FOV = 67.f;

// Helper: Приблизительная дистанция с учетом FOV (для биноклей и прицелов)
IC float GetDistFromCamera(const Fvector& from_position)
{
	float distance = Engine.RenderView.Position.distance_to(from_position);
	// Защита от деления на ноль, если FOV экстремально мал (на всякий случай)
	float current_fov = (Engine.RenderView.Fov > EPS_S) ? Engine.RenderView.Fov : BASE_FOV;
	float fov_K = BASE_FOV / current_fov;
	return distance / fov_K;
}

// Helper: Выбор компонента вектора в зависимости от настроек качества
IC int GetQualityIndex()
{
	// В оригинальном коде:
	// mode == 2 -> .z (High)
	// mode == 1 -> .w (Ultra)
	// else      -> .x (Low)
	// Это немного странно, но сохраняем логику оригинала.

	switch (ps_geometry_quality_mode)
	{
	case 2:
		return 2; // .z (High)
	case 1:
		return 3; // .w (Ultra)
	default:
		return 0; // .x (Low) - используется как fallback для mode 3 и прочих
	}
}
} // namespace

// ===============================================================================================
//  CSceneGraph Implementation
// ===============================================================================================

bool CSceneGraph::ShouldRenderVisual(IRender_Visual* pVisual, bool isStatic, bool ignore_optimize,
									 const SceneTraversalContext& ctx)
{
	if (ignore_optimize)
		return true;

	// 1. Вычисляем параметры объекта
	float sphere_volume = pVisual->vis.sphere.volume();
	float adjusted_distance = 0.f;

	if (isStatic)
	{
		adjusted_distance = GetDistFromCamera(pVisual->vis.sphere.P);
	}
	else
	{
		// Для динамики используем текущую матрицу трансформации из переданного контекста
		Fvector pos;
		// Используем ctx.current_transform
		ctx.current_transform->transform_tiny(pos, pVisual->vis.sphere.P);
		adjusted_distance = GetDistFromCamera(pos);
	}

	// 2. Отсечение для Shadow Map
	if (RenderImplementation.active_phase() == CRender::PHASE_SHADOW_DEPTH)
	{
		if (sphere_volume < 50000.f && adjusted_distance > ps_r_sun_far)
			return false;

		const int shadow_quality_idx = 2; // .z component

		const u32 static_count = sizeof(s_static_cull_levels) / sizeof(CullLevel);
		for (u32 i = 0; i < static_count; ++i)
		{
			const CullLevel& L = s_static_cull_levels[i];
			if (sphere_volume < L.size[shadow_quality_idx] && adjusted_distance > L.dist[shadow_quality_idx])
				return false;
		}
	}

	// 3. Отсечение для основной геометрии
	const int q_idx = GetQualityIndex();

	const CullLevel* levels = isStatic ? s_static_cull_levels : s_dynamic_cull_levels;
	const u32 count = isStatic ? (sizeof(s_static_cull_levels) / sizeof(CullLevel))
							   : (sizeof(s_dynamic_cull_levels) / sizeof(CullLevel));

	for (u32 i = 0; i < count; ++i)
	{
		const CullLevel& L = levels[i];
		if (sphere_volume < L.size[q_idx] && adjusted_distance > L.dist[q_idx])
			return false;
	}

	return true;
}

// Добавлен аргумент ctx
void CSceneGraph::ProcessDynamicVisual(IRender_Visual* pVisual, const SceneTraversalContext& ctx)
{
	if (0 == pVisual)
		return;

	// Передаем ctx
	if (!ShouldRenderVisual(pVisual, false, RenderImplementation.active_phase() == CRender::PHASE_SHADOW_DEPTH, ctx))
		return;

	// Visual is 100% visible - simply add it
	xr_vector<IRender_Visual*>::iterator I, E;

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP: {
		PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;
		for (PS::CParticleGroup::SItemVecIt i_it = pG->items.begin(); i_it != pG->items.end(); i_it++)
		{
			PS::CParticleGroup::SItem& PE_It = *i_it;
			if (PE_It._effect)
				ProcessDynamicVisual(PE_It._effect, ctx);
			for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_related.begin();
				 pit != PE_It._children_related.end(); pit++)
				ProcessDynamicVisual(*pit, ctx);
			for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_free.begin();
				 pit != PE_It._children_free.end(); pit++)
				ProcessDynamicVisual(*pit, ctx);
		}
	}
		return;
	case MT_HIERRARHY: {
		FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
		I = pV->children.begin();
		E = pV->children.end();
		for (; I != E; I++)
			ProcessDynamicVisual(*I, ctx);
	}
		return;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID: {
		CKinematics* pV = (CKinematics*)pVisual;
		BOOL _use_lod = FALSE;
		if (pV->m_lod)
		{
			Fvector Tpos;
			float D;
			// Используем ctx.current_transform
			ctx.current_transform->transform_tiny(Tpos, pV->vis.sphere.P);
			float ScreenSpaceArea = CalcScreenSpaceArea(D, Tpos, pV->vis.sphere.R / 2.f);
			if (ScreenSpaceArea < r_ssaLOD_A)
				_use_lod = TRUE;
		}
		if (_use_lod)
		{
			ProcessDynamicVisual(pV->m_lod, ctx);
		}
		else
		{
			Fvector pos;
			// Используем ctx.current_transform
			ctx.current_transform->transform_tiny(pos, pVisual->vis.sphere.P);
			float adjusted_distane = GetDistFromCamera(pos);
			float switch_distance = 100.0f;

			switch (ps_geometry_quality_mode)
			{
			case 3:
				switch_distance = 100.0f;
				break;
			case 2:
				switch_distance = 50.0f;
				break;
			case 1:
				switch_distance = 25.0f;
				break;
			}

			if (adjusted_distane < switch_distance)
			{
				pV->CalculateBones(TRUE);
				pV->CalculateWallmarks();
			}

			I = pV->children.begin();
			E = pV->children.end();
			for (; I != E; I++)
				ProcessDynamicVisual(*I, ctx);
		}
	}
		return;
	default: {
		// General type of visual
		Fvector Tpos;
		// Используем ctx.current_transform
		ctx.current_transform->transform_tiny(Tpos, pVisual->vis.sphere.P);

		if (RenderImplementation.active_phase() == CRender::PHASE_NORMAL)
		{
			// Используем DReuseItem из m_packet
			SceneGraphPacket::DReuseItem item;
			item.visual = pVisual;
			// Используем ctx.current_transform
			item.matrix = *ctx.current_transform;
			m_packet.m_visuals_dynamic_visible.push_back(item);
		}

		// Передаем ctx
		EnqueueDynamic(pVisual, Tpos, ctx);
	}
		return;
	}
}

// Добавлен аргумент ctx
void CSceneGraph::ProcessStaticVisual(IRender_Visual* pVisual, const SceneTraversalContext& ctx)
{
	if (!RenderImplementation.HOM.visible(pVisual->vis))
		return;

	// Передаем ctx
	if (!ShouldRenderVisual(pVisual, true, RenderImplementation.active_phase() == CRender::PHASE_SHADOW_DEPTH, ctx))
		return;

	if (RenderImplementation.active_phase() == CRender::PHASE_NORMAL)
	{
		m_packet.m_visuals_static_visible.push_back(pVisual);
	}

	xr_vector<IRender_Visual*>::iterator I, E;

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP: {
		PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;
		for (PS::CParticleGroup::SItemVecIt i_it = pG->items.begin(); i_it != pG->items.end(); i_it++)
		{
			PS::CParticleGroup::SItem& PE_It = *i_it;
			if (PE_It._effect)
				ProcessDynamicVisual(PE_It._effect, ctx);
			for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_related.begin();
				 pit != PE_It._children_related.end(); pit++)
				ProcessDynamicVisual(*pit, ctx);
			for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_free.begin();
				 pit != PE_It._children_free.end(); pit++)
				ProcessDynamicVisual(*pit, ctx);
		}
	}
		return;
	case MT_HIERRARHY: {
		FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
		I = pV->children.begin();
		E = pV->children.end();
		for (; I != E; I++)
			ProcessStaticVisual(*I, ctx);
	}
		return;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID: {
		Fvector pos;
		// Используем ctx.current_transform
		ctx.current_transform->transform_tiny(pos, pVisual->vis.sphere.P);
		float adjusted_distane = GetDistFromCamera(pos);
		float switch_distance = 100.0f;

		switch (ps_geometry_quality_mode)
		{
		case 3:
			switch_distance = 100.0f;
			break;
		case 2:
			switch_distance = 50.0f;
			break;
		case 1:
			switch_distance = 25.0f;
			break;
		}

		CKinematics* pV = (CKinematics*)pVisual;
		if (adjusted_distane < switch_distance)
			pV->CalculateBones(TRUE);
		I = pV->children.begin();
		E = pV->children.end();
		for (; I != E; I++)
			ProcessStaticVisual(*I, ctx);
	}
		return;
	case MT_LOD: {
		FLOD* pV = (FLOD*)pVisual;
		float D;
		float ScreenSpaceArea = CalcScreenSpaceArea(D, pV->vis.sphere.P, pV);
		ScreenSpaceArea *= pV->lod_factor;
		if (ScreenSpaceArea < r_ssaLOD_A)
		{
			if (ScreenSpaceArea < r_ssaDISCARD)
				return;

			auto* N = m_packet.mapLOD.insertInAnyWay(D);
			N->val.ScreenSpaceArea = ScreenSpaceArea;
			N->val.pVisual = pVisual;
		}
		if (ScreenSpaceArea > r_ssaLOD_B)
		{
			I = pV->children.begin();
			E = pV->children.end();
			for (; I != E; I++)
				ProcessStaticVisual(*I, ctx);
		}
	}
		return;
	case MT_TREE_PM:
	case MT_TREE_ST:
	default: {
		// Передаем ctx
		EnqueueStatic(pVisual, ctx);
	}
		return;
	}
}

// Добавлен аргумент ctx
BOOL CSceneGraph::add_Dynamic(IRender_Visual* pVisual, u32 planes, const SceneTraversalContext& ctx)
{
	// 1. Трансформация позиции в мировые координаты
	Fvector world_position;
	// Используем ctx.current_transform
	ctx.current_transform->transform_tiny(world_position, pVisual->vis.sphere.P);

	// 2. Frustum Culling
	EFC_Visible visibility_status =
		RenderImplementation.View->testSphere(world_position, pVisual->vis.sphere.R, planes);

	if (visibility_status == fcvNone)
		return FALSE;

	// 3. Проверка на значимость
	// Передаем ctx
	bool is_shadow_phase = (RenderImplementation.active_phase() == CRender::PHASE_SHADOW_DEPTH);
	if (!ShouldRenderVisual(pVisual, false, is_shadow_phase, ctx))
		return FALSE;

	// 4. Разбор типа объекта
	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP: {
		PS::CParticleGroup* pGroup = (PS::CParticleGroup*)pVisual;

		if (visibility_status == fcvPartial)
		{
			for (PS::CParticleGroup::SItem& item : pGroup->items)
			{
				if (item._effect)
					add_Dynamic(item._effect, planes, ctx);
				for (IRender_Visual* child : item._children_related)
					add_Dynamic(child, planes, ctx);
				for (IRender_Visual* child : item._children_free)
					add_Dynamic(child, planes, ctx);
			}
		}
		else
		{
			for (PS::CParticleGroup::SItem& item : pGroup->items)
			{
				if (item._effect)
					ProcessDynamicVisual(item._effect, ctx);
				for (IRender_Visual* child : item._children_related)
					ProcessDynamicVisual(child, ctx);
				for (IRender_Visual* child : item._children_free)
					ProcessDynamicVisual(child, ctx);
			}
		}
	}
	break;

	case MT_HIERRARHY: {
		FHierrarhyVisual* pHierarchy = (FHierrarhyVisual*)pVisual;

		if (visibility_status == fcvPartial)
		{
			for (IRender_Visual* child : pHierarchy->children)
				add_Dynamic(child, planes, ctx);
		}
		else
		{
			for (IRender_Visual* child : pHierarchy->children)
				ProcessDynamicVisual(child, ctx);
		}
	}
	break;

	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID: {
		CKinematics* pKinematics = (CKinematics*)pVisual;

		bool use_lod = false;
		if (pKinematics->m_lod)
		{
			float dist_sq;
			// Используем world_position, которая уже посчитана с помощью ctx
			// Note: CalcScreenSpaceArea использует камеру, но ей нужен world pos.
			// В оригинале мы считали world_position заново внутри ProcessDynamicVisual,
			// здесь она уже есть. Но для чистоты вызова ProcessDynamicVisual(lod) мы просто передаем ctx.

			// Проверка SSA (используем world_position, вычисленную выше)
			float screen_space_area = CalcScreenSpaceArea(dist_sq, world_position, pVisual->vis.sphere.R / 2.f);

			if (screen_space_area < r_ssaLOD_A)
				use_lod = true;
		}

		if (use_lod)
		{
			ProcessDynamicVisual(pKinematics->m_lod, ctx);
		}
		else
		{
			float dist_from_camera = GetDistFromCamera(world_position);
			float switch_distance = 100.0f;

			switch (ps_geometry_quality_mode)
			{
			case 3:
				switch_distance = 100.0f;
				break;
			case 2:
				switch_distance = 50.0f;
				break;
			case 1:
				switch_distance = 25.0f;
				break;
			}

			if (dist_from_camera < switch_distance)
			{
				pKinematics->CalculateBones(TRUE);
				pKinematics->CalculateWallmarks();
			}

			for (IRender_Visual* child : pKinematics->children)
				ProcessDynamicVisual(child, ctx);
		}
	}
	break;

	default: {
		// Передаем ctx
		EnqueueDynamic(pVisual, world_position, ctx);
	}
	break;
	}

	return TRUE;
}

// Добавлен аргумент ctx
void CSceneGraph::add_Static(IRender_Visual* pVisual, u32 planes, const SceneTraversalContext& ctx)
{
	vis_data& vis_data = pVisual->vis;
	EFC_Visible visibility_status =
		RenderImplementation.View->testSAABB(vis_data.sphere.P, vis_data.sphere.R, vis_data.box.data(), planes);

	if (visibility_status == fcvNone)
		return;

	if (!RenderImplementation.HOM.visible(vis_data))
		return;

	bool is_shadow_phase = (RenderImplementation.active_phase() == CRender::PHASE_SHADOW_DEPTH);
	// Передаем ctx
	if (!ShouldRenderVisual(pVisual, true, is_shadow_phase, ctx))
		return;

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP: {
		PS::CParticleGroup* pGroup = (PS::CParticleGroup*)pVisual;

		if (visibility_status == fcvPartial)
		{
			for (PS::CParticleGroup::SItem& item : pGroup->items)
			{
				if (item._effect)
					add_Dynamic(item._effect, planes, ctx);
				for (auto* c : item._children_related)
					add_Dynamic(c, planes, ctx);
				for (auto* c : item._children_free)
					add_Dynamic(c, planes, ctx);
			}
		}
		else
		{
			for (PS::CParticleGroup::SItem& item : pGroup->items)
			{
				if (item._effect)
					ProcessDynamicVisual(item._effect, ctx);
				for (auto* c : item._children_related)
					ProcessDynamicVisual(c, ctx);
				for (auto* c : item._children_free)
					ProcessDynamicVisual(c, ctx);
			}
		}
	}
	break;

	case MT_HIERRARHY: {
		FHierrarhyVisual* pHierarchy = (FHierrarhyVisual*)pVisual;

		if (visibility_status == fcvPartial)
		{
			for (IRender_Visual* child : pHierarchy->children)
				add_Static(child, planes, ctx);
		}
		else
		{
			for (IRender_Visual* child : pHierarchy->children)
				ProcessStaticVisual(child, ctx);
		}
	}
	break;

	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID: {
		Fvector object_pos;
		// Используем ctx.current_transform
		ctx.current_transform->transform_tiny(object_pos, pVisual->vis.sphere.P);

		float dist_from_camera = GetDistFromCamera(object_pos);
		float switch_distance = 100.0f;

		switch (ps_geometry_quality_mode)
		{
		case 3:
			switch_distance = 100.0f;
			break;
		case 2:
			switch_distance = 50.0f;
			break;
		case 1:
			switch_distance = 25.0f;
			break;
		}

		CKinematics* pKinematics = (CKinematics*)pVisual;

		if (dist_from_camera < switch_distance)
			pKinematics->CalculateBones(TRUE);

		if (visibility_status == fcvPartial)
		{
			for (IRender_Visual* child : pKinematics->children)
				add_Static(child, planes, ctx);
		}
		else
		{
			for (IRender_Visual* child : pKinematics->children)
				ProcessStaticVisual(child, ctx);
		}
	}
	break;

	case MT_LOD: {
		FLOD* pLod = (FLOD*)pVisual;
		float dist_unused;
		float screen_space_area = CalcScreenSpaceArea(dist_unused, pLod->vis.sphere.P, pLod);

		screen_space_area *= pLod->lod_factor;

		if (screen_space_area < r_ssaLOD_A)
		{
			if (screen_space_area < r_ssaDISCARD)
				return;

			// Используем m_packet.mapLOD
			auto* node = m_packet.mapLOD.insertInAnyWay(dist_unused);
			node->val.ScreenSpaceArea = screen_space_area;
			node->val.pVisual = pVisual;
		}

		if (screen_space_area > r_ssaLOD_B)
		{
			for (IRender_Visual* child : pLod->children)
				ProcessStaticVisual(child, ctx);
		}
	}
	break;

	case MT_TREE_ST:
	case MT_TREE_PM:
	default: {
		// Передаем ctx
		EnqueueStatic(pVisual, ctx);
	}
	break;
	}
}

void CSceneGraph::SetCullingBoundsCollector(xr_vector<Fbox3, render_alloc<Fbox3>>* dest)
{
	m_culling_bounds_recorder = dest;

	if (m_culling_bounds_recorder)
		m_culling_bounds_recorder->clear();
}
