#include "stdafx.h"

#include "..\xrEngine\fhierrarhyvisual.h"
#include "..\xrEngine\SkeletonCustom.h"
#include "..\xrEngine\fmesh.h"
#include "..\xrEngine\irenderable.h"

#include "flod.h"
#include "particlegroup.h"
#include "FTreeVisual.h"
#include <unordered_set>

using namespace SceneGraphTypes;

thread_local SceneGraphPacket* CurrentRenderContext::packet = nullptr;
thread_local SceneTraversalContext* CurrentRenderContext::context = nullptr;

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
	m_packet.m_spatial_query_results.clear();

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

ICF float CalcScreenSpaceArea(float& distSQ, fvec3& C, IRender_Visual* V)
{
	float R = V->vis.sphere.R + 0;
	distSQ = Engine.RenderView.Position.distance_to_sqr(C) + EPS;
	return R / distSQ;
}
ICF float CalcScreenSpaceArea(float& distSQ, fvec3& C, float R)
{
	distSQ = Engine.RenderView.Position.distance_to_sqr(C) + EPS;
	return R / distSQ;
}

float r_dtex_range = 50.f;

ShaderElement* SelectShaderElementForStaticVis(IRender_Visual* pVisual, float cdist_sq, const SceneTraversalContext& ctx)
{
	if (!pVisual)
	{
		return 0;
	}

	if (!pVisual->shader._get())
	{
		return 0;
	}

	int id = SE_R1_NORMAL_HQ;

	switch (ctx.render_phase)
	{
	case CRender::PHASE_HUD:	// HUD Forward Base Pass
	case CRender::PHASE_NORMAL: // Forward Base Pass
		id = ((std::sqrt(cdist_sq) - pVisual->vis.sphere.R) < r_dtex_range) ? SE_R1_NORMAL_HQ : SE_R1_NORMAL_LQ;
		break;
	case CRender::PHASE_POINT_LIGHTING: // Additive Point Light Pass
		id = SE_R1_LPOINT;
		break;
	case CRender::PHASE_SPOT_LIGHTING: // Additive Spot Light Pass
		id = SE_R1_LSPOT;
		break;
	case CRender::PHASE_SUN_LIGHTING: // Additive Sun Light Pass
		id = SE_R1_LSUN;
		break;
	case CRender::PHASE_SHADOW_DEPTH:
		id = SE_SHADOW_DEPTH;
		break;
	case CRender::PHASE_DEPTH_PREPASS:
		id = SE_DEPTH_PREPASS;
		break;
	}

	return pVisual->shader->E[id]._get();
}

ShaderElement* SelectShaderElementForDynamicVis(IRender_Visual* pVisual, float cdist_sq, const SceneTraversalContext& ctx)
{
	if (!pVisual)
	{
		return 0;
	}

	if (!pVisual->shader._get())
	{
		return 0;
	}

	int id = SE_R1_NORMAL_HQ;

	switch (ctx.render_phase)
	{
	case CRender::PHASE_HUD:	// HUD Forward Base Pass
	case CRender::PHASE_NORMAL: // Forward Base Pass
		id = ((std::sqrt(cdist_sq) - pVisual->vis.sphere.R) < r_dtex_range) ? SE_R1_NORMAL_HQ : SE_R1_NORMAL_LQ;
		break;
	case CRender::PHASE_POINT_LIGHTING: // Additive Point Light Pass
		id = SE_R1_LPOINT;
		break;
	case CRender::PHASE_SPOT_LIGHTING: // Additive Spot Light Pass
		id = SE_R1_LSPOT;
		break;
	case CRender::PHASE_SUN_LIGHTING: // Additive Sun Light Pass
		id = SE_R1_LSUN;
		break;
	case CRender::PHASE_SHADOW_DEPTH:
		id = SE_SHADOW_DEPTH;
		break;
	case CRender::PHASE_DEPTH_PREPASS:
		id = SE_DEPTH_PREPASS;
		break;
	}

	return pVisual->shader->E[id]._get();
}

// ===============================================================================================
//  Метод: EnqueueDynamic
//  Назначение: Добавление динамического объекта в очередь рендеринга.
//  Параметры:
//    pVisual       - Визуальный объект.
//    object_center - Центр объекта в мировых координатах (для сортировки).
//    ctx           - Текущий контекст обхода (матрицы, флаги, владелец).
//    dest          - Целевой пакет данных (куда записывать результат).
// ===============================================================================================
void CSceneGraph::EnqueueDynamic(IRender_Visual* pVisual, fvec3& object_center, const SceneTraversalContext& ctx, SceneGraphPacket& dest)
{
	if (!pVisual || !pVisual->shader._get())
		return;

	// -------------------------------------------------------------------------
	// Проверка уникальности (Traversal Marker)
	// -------------------------------------------------------------------------
	if (pVisual->vis.m_traversal_marker == ctx.traversal_marker_id)
		return;
	pVisual->vis.m_traversal_marker = ctx.traversal_marker_id;

	// -------------------------------------------------------------------------
	// Метрики (SSA & Distance)
	// -------------------------------------------------------------------------
	float distance_sq;
	// Вычисляем Screen Space Area для выбора LOD и отсечения.
	float screen_space_area = CalcScreenSpaceArea(distance_sq, object_center, pVisual);

	// Отсечение слишком мелких объектов (Small Object Culling)
	if (screen_space_area <= r_ssaDISCARD)
		return;

	// -------------------------------------------------------------------------
	// Выбор шейдера (Technique Selection)
	// -------------------------------------------------------------------------
	ShaderElement* shader_element = SelectShaderElementForDynamicVis(pVisual, distance_sq, ctx);

	if (!shader_element)
		return;

	// -------------------------------------------------------------------------
	// Фильтрация по приоритету
	// -------------------------------------------------------------------------
	u32 priority = shader_element->flags.iPriority / 2;
	if (priority == 0 && !m_fetch_config.fetch_priority_0)
		return;
	if (priority == 1 && !m_fetch_config.fetch_priority_1)
		return;

	if (priority > 1)
	{
#ifdef DEBUG
		Msg("! [SceneGraph] shader priority %d > 1, clamped to 1", priority);
#endif
		priority = 1;
	}

	if (ctx.render_phase == CRender::PHASE_NORMAL)
	{
		SceneGraphPacket::DReuseItem item = {pVisual, *ctx.current_transform};
		dest.m_visuals_dynamic_visible.push_back(item);

		for (int i = 0; i < pVisual->shader->elements_count; ++i)
		{
			ShaderElement* se = pVisual->shader->E[i]._get();
			if (se && se->flags.bDistort)
			{
				auto* node = dest.queue_distortion.insertInAnyWay(distance_sq);
				node->val.ScreenSpaceArea = screen_space_area;
				node->val.pObject = ctx.current_owner;
				node->val.pVisual = pVisual;
				node->val.pMatrix = ctx.current_transform;
				node->val.se = se;
			}
		}
	}

	// Проверка флага из контекста
	if (ctx.is_invisible_mode)
		return;

	// -------------------------------------------------------------------------
	// Маршрутизация (Routing)
	// -------------------------------------------------------------------------

	if (ctx.is_hud_pass)
	{
		if (shader_element->flags.bStrictB2F)
		{
			auto* node = dest.queue_transparent.insertInAnyWay(distance_sq);
			node->val.ScreenSpaceArea = screen_space_area;
			node->val.pObject = ctx.current_owner;
			node->val.pVisual = pVisual;
			node->val.pMatrix = ctx.current_transform;
			node->val.se = shader_element;
		}
		else
		{
			auto* node = dest.queue_hud.insertInAnyWay(distance_sq);
			node->val.ScreenSpaceArea = screen_space_area;
			node->val.pObject = ctx.current_owner;
			node->val.pVisual = pVisual;
			node->val.pMatrix = ctx.current_transform;
			node->val.se = shader_element;
		}
		return;
	}

	// --- B. Transparent (Alpha Blending) ---
	if (shader_element->flags.bStrictB2F)
	{
		auto* node = dest.queue_transparent.insertInAnyWay(distance_sq);
		node->val.ScreenSpaceArea = screen_space_area;
		node->val.pObject = ctx.current_owner;
		node->val.pVisual = pVisual;
		node->val.pMatrix = ctx.current_transform;
		node->val.se = shader_element;
		return;
	}

	if (ctx.render_phase == CRender::PHASE_NORMAL)
	{
		if (shader_element->flags.bEmissive)
		{
			auto* node = dest.mapEmissive.insertInAnyWay(distance_sq);
			node->val.ScreenSpaceArea = screen_space_area;
			node->val.pObject = ctx.current_owner;
			node->val.pVisual = pVisual;
			node->val.pMatrix = ctx.current_transform;
			node->val.se = pVisual->shader->E[4]._get();
		}

		if (shader_element->flags.bWmark && m_fetch_config.fetch_wallmarks)
		{
			auto* node = dest.queue_wallmarks.insertInAnyWay(distance_sq);
			node->val.ScreenSpaceArea = screen_space_area;
			node->val.pObject = ctx.current_owner;
			node->val.pVisual = pVisual;
			node->val.pMatrix = ctx.current_transform;
			node->val.se = shader_element;
			return;
		}
	}

	// -------------------------------------------------------------------------
	// Opaque Geometry (Основная геометрия)
	// -------------------------------------------------------------------------

	// Создаем узел, используя данные из ctx
	DynamicRenderNode item = {screen_space_area, ctx.current_owner, pVisual, ctx.current_transform};

	if (shader_element->passes.empty())
	{
#ifdef DEBUG
		Msg("! [SceneGraph] shader_element has no passes, skipping visual [%s]", pVisual->dbg_name.c_str());
#endif
		return;
	}

	SPass& pass = *shader_element->passes.front();

	// Выбираем очередь из переданного пакета dest
	auto& target_map = dest.queue_dynamic[priority];

	// Иерархическая вставка (State Sorting):
	// VS -> PS -> Constants -> States -> Textures -> Items
#ifdef USE_RESOURCE_DEBUGGER
	target_map.insert(pass.vs);
	auto* node_vs = target_map.find(pass.vs);
	VERIFY(node_vs);

	node_vs->val.insert(pass.ps);
	auto* node_ps = node_vs->val.find(pass.ps);
	VERIFY(node_ps);
#else
	target_map.insert(pass.vs->sh);
	auto* node_vs = target_map.find(pass.vs->sh);
	if (!node_vs)
	{
#ifdef DEBUG
		Msg("! [SceneGraph] Failed to find vertex shader node");
#endif
		return;
	}

	node_vs->val.insert(pass.ps->sh);
	auto* node_ps = node_vs->val.find(pass.ps->sh);
	if (!node_ps)
	{
#ifdef DEBUG
		Msg("! [SceneGraph] Failed to find pixel shader node");
#endif
		return;
	}
#endif

	node_ps->val.insert(pass.constants._get());
	auto* node_cs = node_ps->val.find(pass.constants._get());
	if (!node_cs)
		return;

	node_cs->val.insert(pass.state->state);
	auto* node_state = node_cs->val.find(pass.state->state);
	if (!node_state)
		return;

	node_state->val.insert(pass.T._get());
	auto* node_tex = node_state->val.find(pass.T._get());
	if (!node_tex)
		return;

	// Добавляем объект в конечный лист
	node_tex->val.push_back(item);

	// Пробрасываем максимальный SSA вверх по иерархии для сортировки групп (Early Z)
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

	// -------------------------------------------------------------------------
	// 8. Сбор данных для теней (Cascaded Shadow Maps Culling)
	// -------------------------------------------------------------------------
	if (m_culling_bounds_recorder)
	{
		Fbox3 temp_box;
		// Трансформируем AABB матрицей из ctx
		temp_box.transform(pVisual->vis.box, *ctx.current_transform);
		m_culling_bounds_recorder->push_back(temp_box);
	}
}

// ===============================================================================================
//  Метод: EnqueueStatic
//  Назначение: Добавление статического объекта в очередь рендеринга.
//  Параметры:
//    pVisual - Визуальный объект.
//    ctx     - Контекст обхода (для статики важны флаги, но не матрица).
//    dest    - Целевой пакет данных.
// ===============================================================================================
void CSceneGraph::EnqueueStatic(IRender_Visual* pVisual, const SceneTraversalContext& ctx, SceneGraphPacket& dest)
{
	if (!pVisual || !pVisual->shader._get())
		return;

	// Проверка уникальности
	// Предотвращает дублирование объекта, если он виден через несколько порталов.
	// atomic_exchange возвращает старое значение.
	// Если старое значение уже равно текущему, значит другой поток успел нас опередить.
	if (pVisual->vis.m_traversal_marker == ctx.traversal_marker_id)
		return;
	pVisual->vis.m_traversal_marker = ctx.traversal_marker_id;

	// Метрики (позиция уже мировая)
	float distance_sq;
	float screen_space_area = CalcScreenSpaceArea(distance_sq, pVisual->vis.sphere.P, pVisual);

	if (screen_space_area <= r_ssaDISCARD)
		return;

	// Выбор шейдера
	ShaderElement* shader_element = SelectShaderElementForStaticVis(pVisual, distance_sq, ctx);

	if (!shader_element)
		return;

	// Фильтрация по приоритету
	u32 priority = shader_element->flags.iPriority / 2;
	if (priority == 0 && !m_fetch_config.fetch_priority_0)
		return;
	if (priority == 1 && !m_fetch_config.fetch_priority_1)
		return;

	if (priority > 1)
	{
#ifdef DEBUG
		Msg("! [SceneGraph] shader priority %d > 1, clamped to 1", priority);
#endif
		priority = 1;
	}

	if (ctx.render_phase == CRender::PHASE_NORMAL)
	{
		dest.m_visuals_static_visible.push_back(pVisual);

		for (int i = 0; i < pVisual->shader->elements_count; ++i)
		{
			ShaderElement* se = pVisual->shader->E[i]._get();
			if (se && se->flags.bDistort)
			{
				auto* node = dest.queue_distortion.insertInAnyWay(distance_sq);
				node->val.ScreenSpaceArea = screen_space_area;
				node->val.pObject = ctx.current_owner;
				node->val.pVisual = pVisual;
				node->val.pMatrix = ctx.current_transform;
				node->val.se = se;
			}
		}
	}

	// Маршрутизация

	// --- Strict Sorting ---
	if (shader_element->flags.bStrictB2F)
	{
		auto* node = dest.queue_transparent.insertInAnyWay(distance_sq);
		node->val.ScreenSpaceArea = screen_space_area;
		node->val.pObject = nullptr;
		node->val.pVisual = pVisual;
		node->val.pMatrix = &Fidentity;
		node->val.se = shader_element;
		return;
	}

	if (ctx.render_phase == CRender::PHASE_NORMAL)
	{
		if (shader_element->flags.bEmissive)
		{
			auto* node = dest.mapEmissive.insertInAnyWay(distance_sq);
			node->val.ScreenSpaceArea = screen_space_area;
			node->val.pObject = nullptr;
			node->val.pVisual = pVisual;
			node->val.pMatrix = &Fidentity;
			node->val.se = pVisual->shader->E[4]._get();
		}

		if (shader_element->flags.bWmark && m_fetch_config.fetch_wallmarks)
		{
			auto* node = dest.queue_wallmarks.insertInAnyWay(distance_sq);
			node->val.ScreenSpaceArea = screen_space_area;
			node->val.pObject = nullptr;
			node->val.pVisual = pVisual;
			node->val.pMatrix = &Fidentity;
			node->val.se = shader_element;
			return;
		}
	}

	// Обратная связь (Feedback)
	if (m_feedback_interface && counter_S == val_feedback_breakp)
	{
		m_feedback_interface->rfeedback_static(pVisual);
	}

	// Opaque Geometry
	counter_S++;

	if (shader_element->passes.empty())
	{
#ifdef DEBUG
		Msg("! [SceneGraph] shader_element has no passes, skipping visual [%s]", pVisual->dbg_name.c_str());
#endif
		return;
	}

	SPass& pass = *shader_element->passes.front();

	// Выбираем очередь из переданного пакета dest
	auto& target_map = dest.queue_static[priority];

	// Иерархическая вставка
#ifdef USE_RESOURCE_DEBUGGER
	target_map.insert(pass.vs);
	auto* node_vs = target_map.find(pass.vs);
	VERIFY(node_vs);

	node_vs->val.insert(pass.ps);
	auto* node_ps = node_vs->val.find(pass.ps);
	VERIFY(node_ps);
#else
	target_map.insert(pass.vs->sh);
	auto* node_vs = target_map.find(pass.vs->sh);
	if (!node_vs)
	{
#ifdef DEBUG
		Msg("! [SceneGraph] Failed to find vertex shader node");
#endif
		return;
	}

	node_vs->val.insert(pass.ps->sh);
	auto* node_ps = node_vs->val.find(pass.ps->sh);
	if (!node_ps)
	{
#ifdef DEBUG
		Msg("! [SceneGraph] Failed to find pixel shader node");
#endif
		return;
	}
#endif

	node_ps->val.insert(pass.constants._get());
	auto* node_cs = node_ps->val.find(pass.constants._get());
	if (!node_cs)
		return;

	node_cs->val.insert(pass.state->state);
	auto* node_state = node_cs->val.find(pass.state->state);
	if (!node_state)
		return;

	node_state->val.insert(pass.T._get());
	auto* node_tex = node_state->val.find(pass.T._get());
	if (!node_tex)
		return;

	StaticRenderNode item = {screen_space_area, pVisual};
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

	// 9. Сбор данных для теней
	if (m_culling_bounds_recorder)
	{
		// Для статики просто берем AABB, так как она не трансформируется
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
	fvec4 dist;
	fvec4 size;
};

// Массив уровней оптимизации для СТАТИКИ (12 уровней)
static const CullLevel s_static_cull_levels[] = 
{
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
static const CullLevel s_dynamic_cull_levels[] = 
{
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
IC float GetDistFromCamera(const fvec3& from_position)
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

bool CSceneGraph::ShouldRenderVisual(IRender_Visual* pVisual, bool isStatic, bool ignore_optimize, const SceneTraversalContext& ctx)
{
	if (ignore_optimize)
		return true;

	// Вычисляем параметры объекта
	float sphere_volume = pVisual->vis.sphere.volume();
	float adjusted_distance = 0.f;

	if (isStatic)
	{
		adjusted_distance = GetDistFromCamera(pVisual->vis.sphere.P);
	}
	else
	{
		// Для динамики используем текущую матрицу трансформации из переданного контекста
		fvec3 pos;
		// Используем ctx.current_transform
		ctx.current_transform->transform_tiny(pos, pVisual->vis.sphere.P);
		adjusted_distance = GetDistFromCamera(pos);
	}

	// Отсечение для Shadow Map
	if (ctx.render_phase == CRender::PHASE_SHADOW_DEPTH)
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

	// Отсечение для основной геометрии
	const int q_idx = GetQualityIndex();

	const CullLevel* levels = isStatic ? s_static_cull_levels : s_dynamic_cull_levels;
	const u32 count = isStatic ? (sizeof(s_static_cull_levels) / sizeof(CullLevel)) : (sizeof(s_dynamic_cull_levels) / sizeof(CullLevel));

	for (u32 i = 0; i < count; ++i)
	{
		const CullLevel& L = levels[i];
		if (sphere_volume < L.size[q_idx] && adjusted_distance > L.dist[q_idx])
			return false;
	}

	return true;
}

// ===============================================================================================
//  Метод: ProcessDynamicVisual
//  Назначение: Обработка динамического объекта, который гарантированно видим (или проверка не требуется).
//  Параметры:
//    pVisual - Визуальный объект.
//    ctx     - Контекст обхода (матрицы, флаги).
//    dest    - Целевой пакет данных.
// ===============================================================================================
void CSceneGraph::ProcessDynamicVisual(IRender_Visual* pVisual, const SceneTraversalContext& ctx, SceneGraphPacket& dest)
{
	if (!pVisual)
		return;

	// Проверка на значимость (Distance / Size Culling)
	// Несмотря на то, что объект "видим" по фрустуму, он может быть слишком маленьким.
	bool is_shadow_phase = (ctx.render_phase == CRender::PHASE_SHADOW_DEPTH);
	// Передаем ctx для корректного расчета дистанции
	if (!ShouldRenderVisual(pVisual, false, is_shadow_phase, ctx))
		return;

	// Итераторы для обхода детей
	xr_vector<IRender_Visual*>::iterator I, E;

	// Разбор типа объекта
	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP: 
	{
		PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;
		for (PS::CParticleGroup::SItemVecIt i_it = pG->items.begin(); i_it != pG->items.end(); i_it++)
		{
			PS::CParticleGroup::SItem& PE_It = *i_it;
			if (PE_It._effect)
				ProcessDynamicVisual(PE_It._effect, ctx, dest); // Рекурсия с ctx и dest
			for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_related.begin();
				 pit != PE_It._children_related.end(); pit++)
				ProcessDynamicVisual(*pit, ctx, dest); // Рекурсия с ctx и dest
			for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_free.begin();
				 pit != PE_It._children_free.end(); pit++)
				ProcessDynamicVisual(*pit, ctx, dest); // Рекурсия с ctx и dest
		}
	}
		return;

	case MT_HIERRARHY: 
	{
		FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
		I = pV->children.begin();
		E = pV->children.end();
		for (; I != E; I++)
			ProcessDynamicVisual(*I, ctx, dest); // Рекурсия с ctx и dest
	}
		return;

	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID: 
	{
		CKinematics* pV = (CKinematics*)pVisual;
		BOOL _use_lod = FALSE;

		// Проверка на использование LOD-модели
		if (pV->m_lod)
		{
			fvec3 Tpos;
			float D;
			// Используем матрицу из ctx для трансформации центра сферы
			ctx.current_transform->transform_tiny(Tpos, pV->vis.sphere.P);

			// Вычисляем SSA для переключения на LOD
			float ScreenSpaceArea = CalcScreenSpaceArea(D, Tpos, pV->vis.sphere.R / 2.f);
			if (ScreenSpaceArea < r_ssaLOD_A)
				_use_lod = TRUE;
		}

		if (_use_lod)
		{
			// Если объект далеко - рисуем упрощенную модель (LOD)
			ProcessDynamicVisual(pV->m_lod, ctx, dest); // Передаем ctx и dest
		}
		else
		{
			// Если объект близко - рисуем полную модель

			fvec3 pos;
			// Используем матрицу из ctx
			ctx.current_transform->transform_tiny(pos, pVisual->vis.sphere.P);
			float adjusted_distane = GetDistFromCamera(pos);
			float switch_distance = 100.0f;

			// Настройки качества геометрии
			switch (ps_geometry_quality_mode)
			{
			case 3:
				switch_distance = 100.0f;
				break; // Ultra
			case 2:
				switch_distance = 50.0f;
				break; // High
			case 1:
				switch_distance = 25.0f;
				break; // Low
			}

			BOOL bExact = (adjusted_distane < switch_distance);
			pV->CalculateBones(bExact);

			// Wallmarks считаем только вблизи и только для основного прохода (оптимизация)
			//    Для теней воллмарки обычно не нужны (они плоские).
			if (bExact && !ctx.is_invisible_mode)
			{
				if (ctx.render_phase == CRender::PHASE_NORMAL)
					pV->CalculateWallmarks();
			}

			// Рекурсивно обрабатываем части скелета (кости/меши)
			I = pV->children.begin();
			E = pV->children.end();
			for (; I != E; I++)
				ProcessDynamicVisual(*I, ctx, dest); // Передаем ctx и dest
		}
	}
		return;

	default: 
	{
		// Листовой узел (Mesh) - конечная геометрия

		fvec3 Tpos;
		// Трансформируем позицию используя матрицу из ctx
		ctx.current_transform->transform_tiny(Tpos, pVisual->vis.sphere.P);

		// Добавляем в очередь на отрисовку
		// Передаем ctx и dest
		EnqueueDynamic(pVisual, Tpos, ctx, dest);
	}
		return;
	}
}

// ===============================================================================================
//  Метод: ProcessStaticVisual
//  Назначение: Обработка статического объекта (часть уровня), который гарантированно видим.
//  Параметры:
//    pVisual - Визуальный объект.
//    ctx     - Контекст обхода.
//    dest    - Целевой пакет данных.
// ===============================================================================================
void CSceneGraph::ProcessStaticVisual(IRender_Visual* pVisual, const SceneTraversalContext& ctx, SceneGraphPacket& dest)
{
	if (!pVisual)
		return;

	// Проверка на значимость
	bool is_shadow_phase = (ctx.render_phase == CRender::PHASE_SHADOW_DEPTH);
	// Передаем ctx
	if (!ShouldRenderVisual(pVisual, true, is_shadow_phase, ctx))
		return;

	xr_vector<IRender_Visual*>::iterator I, E;

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP: 
	{
		PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;
		for (PS::CParticleGroup::SItemVecIt i_it = pG->items.begin(); i_it != pG->items.end(); i_it++)
		{
			PS::CParticleGroup::SItem& PE_It = *i_it;
			if (PE_It._effect)
				ProcessDynamicVisual(PE_It._effect, ctx, dest); // Рекурсия с ctx и dest
			for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_related.begin();
				 pit != PE_It._children_related.end(); pit++)
				ProcessDynamicVisual(*pit, ctx, dest); // Рекурсия с ctx и dest
			for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_free.begin();
				 pit != PE_It._children_free.end(); pit++)
				ProcessDynamicVisual(*pit, ctx, dest); // Рекурсия с ctx и dest
		}
	}
		return;

	case MT_HIERRARHY: 
	{
		FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
		I = pV->children.begin();
		E = pV->children.end();
		for (; I != E; I++)
			ProcessStaticVisual(*I, ctx, dest); // Рекурсия с ctx и dest
	}
		return;

	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID: 
	{
		// Скелетная статика (трупы, декорации)
		fvec3 pos;
		// Используем матрицу из ctx (для статики это обычно Identity, но для универсальности берем из контекста)
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
			pV->CalculateBones(TRUE); // Обновляем кости, если близко

		I = pV->children.begin();
		E = pV->children.end();
		for (; I != E; I++)
			ProcessStaticVisual(*I, ctx, dest); // Рекурсия с ctx и dest
	}
		return;

	case MT_LOD: 
	{
		// Статические деревья и объекты с билборд-LODами
		FLOD* pV = (FLOD*)pVisual;
		float D;
		float ScreenSpaceArea = CalcScreenSpaceArea(D, pV->vis.sphere.P, pV);

		// Учитываем коэффициент качества LOD
		ScreenSpaceArea *= pV->lod_factor;

		// Если далеко - добавляем в список LOD-ов (билбордов)
		if (ScreenSpaceArea < r_ssaLOD_A)
		{
			if (pVisual->vis.m_traversal_marker == ctx.traversal_marker_id)
				return;
			pVisual->vis.m_traversal_marker = ctx.traversal_marker_id;

			if (ScreenSpaceArea < r_ssaDISCARD)
				return;

			if (ctx.render_phase == CRender::PHASE_NORMAL)
			{
				dest.m_visuals_static_visible.push_back(pVisual);

				auto* N = dest.mapLOD.insertInAnyWay(D);
				N->val.ScreenSpaceArea = ScreenSpaceArea;
				N->val.pVisual = pVisual;
			}
		}

		// Если близко - рендерим детальную геометрию (детей)
		if (ScreenSpaceArea > r_ssaLOD_B)
		{
			I = pV->children.begin();
			E = pV->children.end();
			for (; I != E; I++)
				ProcessStaticVisual(*I, ctx, dest); // Рекурсия с ctx и dest
		}
	}
		return;

	case MT_TREE_PM:
	case MT_TREE_ST: 
	{
		// Вычисляем позицию для сортировки
		fvec3 Tpos;
		ctx.current_transform->transform_tiny(Tpos, pVisual->vis.sphere.P);

		// Отправляем в ДИНАМИЧЕСКУЮ очередь.
		// Это сохранит ctx.current_transform и передаст его в шейдер как m_W.
		EnqueueDynamic(pVisual, Tpos, ctx, dest);
	}
	break;

	default: 
	{
		// Обычная геометрия (стены, террейн) - Identity матрица ок
		EnqueueStatic(pVisual, ctx, dest);
	}
	break;
	}
}

// ===============================================================================================
//  Метод: add_Dynamic
//  Назначение: Добавление динамического объекта с проверкой видимости (Frustum Culling).
//  Параметры:
//    pVisual - Визуальный объект.
//    planes  - Маска плоскостей фрустума (для оптимизации проверки дочерних объектов).
//    ctx     - Контекст обхода (текущая матрица трансформации и флаги).
//    dest    - Целевой пакет для записи (Thread-Local или Global).
// ===============================================================================================
BOOL CSceneGraph::add_Dynamic(IRender_Visual* pVisual, u32 planes, const SceneTraversalContext& ctx, SceneGraphPacket& dest)
{
	// Трансформация позиции в мировые координаты
	// Используем матрицу из переданного контекста, а не this->m_current_transform
	fvec3 world_position;
	ctx.current_transform->transform_tiny(world_position, pVisual->vis.sphere.P);

	// Frustum Culling (Отсечение по пирамиде видимости)
	// Проверяем сферу объекта в мировых координатах
	VERIFY(ctx.frustum);
	EFC_Visible visibility_status = ctx.frustum->testSphere(world_position, pVisual->vis.sphere.R, planes);

	// Если объект полностью вне экрана - выходим
	if (visibility_status == fcvNone)
		return FALSE;

	// Проверка на значимость (Distance / Size Culling)
	// ShouldRenderVisual теперь тоже принимает ctx
	bool is_shadow_phase = (ctx.render_phase == CRender::PHASE_SHADOW_DEPTH);
	if (!ShouldRenderVisual(pVisual, false, is_shadow_phase, ctx))
		return FALSE;

	// Разбор типа объекта и рекурсия
	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP: 
	{
		PS::CParticleGroup* pGroup = (PS::CParticleGroup*)pVisual;

		// Если родитель виден частично (fcvPartial), нужно проверять фрустум для детей (add_Dynamic).
		// Если родитель виден полностью (fcvFully), детей можно добавлять без проверки (ProcessDynamicVisual).

		if (visibility_status == fcvPartial)
		{
			for (PS::CParticleGroup::SItem& item : pGroup->items)
			{
				if (item._effect)
					add_Dynamic(item._effect, planes, ctx, dest); // Рекурсия с проверкой
				for (IRender_Visual* child : item._children_related)
					add_Dynamic(child, planes, ctx, dest);
				for (IRender_Visual* child : item._children_free)
					add_Dynamic(child, planes, ctx, dest);
			}
		}
		else // fcvFully
		{
			for (PS::CParticleGroup::SItem& item : pGroup->items)
			{
				if (item._effect)
					ProcessDynamicVisual(item._effect, ctx, dest); // Быстрое добавление
				for (IRender_Visual* child : item._children_related)
					ProcessDynamicVisual(child, ctx, dest);
				for (IRender_Visual* child : item._children_free)
					ProcessDynamicVisual(child, ctx, dest);
			}
		}
	}
	break;

	case MT_HIERRARHY: 
	{
		FHierrarhyVisual* pHierarchy = (FHierrarhyVisual*)pVisual;

		if (visibility_status == fcvPartial)
		{
			for (IRender_Visual* child : pHierarchy->children)
				add_Dynamic(child, planes, ctx, dest); // Рекурсия с проверкой
		}
		else
		{
			for (IRender_Visual* child : pHierarchy->children)
				ProcessDynamicVisual(child, ctx, dest); // Быстрое добавление
		}
	}
	break;

	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID: 
	{
		CKinematics* pKinematics = (CKinematics*)pVisual;

		// Логика LOD для скелетов
		bool use_lod = false;
		if (pKinematics->m_lod)
		{
			float dist_sq;
			// Используем уже вычисленную world_position
			float screen_space_area = CalcScreenSpaceArea(dist_sq, world_position, pVisual->vis.sphere.R / 2.f);

			if (screen_space_area < r_ssaLOD_A)
				use_lod = true;
		}

		if (use_lod)
		{
			// Рендерим LOD вместо реальной геометрии
			ProcessDynamicVisual(pKinematics->m_lod, ctx, dest);
		}
		else
		{
			// Расчет дистанции для переключения качества анимаций
			float dist_from_camera = GetDistFromCamera(world_position);
			float switch_distance = 100.0f;

			switch (ps_geometry_quality_mode)
			{
			case 3:
				switch_distance = 100.0f;
				break; // Ultra
			case 2:
				switch_distance = 50.0f;
				break; // High
			case 1:
				switch_distance = 25.0f;
				break; // Low
			}

			// Если близко - обновляем кости (Software Skinning / Wallmarks update)
			// Примечание: Это изменяет состояние объекта, что в идеале должно быть вынесено из фазы сбора,
			// но для legacy поддержки оставляем здесь. В многопотоке это место требует внимания (мьютекс в Kinematics).
			if (dist_from_camera < switch_distance)
			{
				pKinematics->CalculateBones(TRUE);

				if (ctx.render_phase != CRender::PHASE_SHADOW_DEPTH)
					pKinematics->CalculateWallmarks();
			}

			// Скелеты всегда добавляем через Process, так как части (children) обычно внутри AABB родителя
			for (IRender_Visual* child : pKinematics->children)
				ProcessDynamicVisual(child, ctx, dest);
		}
	}
	break;

	default: 
	{
		// Листовой объект (Mesh) - отправляем в низкоуровневую очередь
		EnqueueDynamic(pVisual, world_position, ctx, dest);
	}
	break;
	}

	return TRUE;
}

// ===============================================================================================
//  Метод: add_Static
//  Назначение: Добавление статического объекта с проверкой видимости (Frustum + HOM).
//  Параметры:
//    pVisual - Визуальный объект.
//    planes  - Маска плоскостей фрустума.
//    ctx     - Контекст обхода.
//    dest    - Целевой пакет.
// ===============================================================================================
void CSceneGraph::add_Static(IRender_Visual* pVisual, u32 planes, const SceneTraversalContext& ctx, SceneGraphPacket& dest)
{
	// Frustum Culling (Sphere + AABB Test)
	// Для статики позиции вершин уже в мировом пространстве, трансформация не нужна (обычно Identity).
	vis_data& vis_data = pVisual->vis;

	VERIFY(ctx.frustum);
	EFC_Visible visibility_status =
		ctx.frustum->testSAABB(vis_data.sphere.P, vis_data.sphere.R, vis_data.box.data(), planes);

	if (visibility_status == fcvNone)
		return;

	// Occlusion Culling (HOM - Hierarchical Occlusion Maps)
	// Пропускаем невидимые за стенами/холмами объекты
	if (!RenderImplementation.HOM.visible(vis_data))
		return;

	// Проверка на значимость (Distance / Size Culling)
	bool is_shadow_phase = (ctx.render_phase == CRender::PHASE_SHADOW_DEPTH);
	// Передаем ctx для корректного расчета дистанции
	if (!ShouldRenderVisual(pVisual, true, is_shadow_phase, ctx))
		return;

	// Разбор типа объекта
	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP: 
	{
		PS::CParticleGroup* pGroup = (PS::CParticleGroup*)pVisual;

		if (visibility_status == fcvPartial)
		{
			for (PS::CParticleGroup::SItem& item : pGroup->items)
			{
				if (item._effect)
					add_Dynamic(item._effect, planes, ctx, dest); // Статика может содержать динамические эффекты
				for (auto* c : item._children_related)
					add_Dynamic(c, planes, ctx, dest);
				for (auto* c : item._children_free)
					add_Dynamic(c, planes, ctx, dest);
			}
		}
		else
		{
			for (PS::CParticleGroup::SItem& item : pGroup->items)
			{
				if (item._effect)
					ProcessDynamicVisual(item._effect, ctx, dest);
				for (auto* c : item._children_related)
					ProcessDynamicVisual(c, ctx, dest);
				for (auto* c : item._children_free)
					ProcessDynamicVisual(c, ctx, dest);
			}
		}
	}
	break;

	case MT_HIERRARHY: 
	{
		FHierrarhyVisual* pHierarchy = (FHierrarhyVisual*)pVisual;

		if (visibility_status == fcvPartial)
		{
			for (IRender_Visual* child : pHierarchy->children)
				add_Static(child, planes, ctx, dest); // Рекурсия
		}
		else
		{
			for (IRender_Visual* child : pHierarchy->children)
				ProcessStaticVisual(child, ctx, dest); // Быстрое добавление
		}
	}
	break;

	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID: 
	{
		// Скелетная статика (трупы как часть уровня и т.д.)
		fvec3 object_pos;
		// Используем трансформацию из контекста (даже если это Identity, важно соблюдать контракт)
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
				add_Static(child, planes, ctx, dest);
		}
		else
		{
			for (IRender_Visual* child : pKinematics->children)
				ProcessStaticVisual(child, ctx, dest);
		}
	}
	break;

	case MT_LOD: 
	{
		// Обработка деревьев и крупных объектов с LOD-ами
		FLOD* pLod = (FLOD*)pVisual;
		float dist_unused;
		float screen_space_area = CalcScreenSpaceArea(dist_unused, pLod->vis.sphere.P, pLod);

		screen_space_area *= pLod->lod_factor;

		// Если объект далеко - рисуем его как Imposter (LOD, билборд)
		if (screen_space_area < r_ssaLOD_A)
		{
			if (screen_space_area < r_ssaDISCARD)
				return;

			// Вставляем в mapLOD целевого пакета
			auto* node = dest.mapLOD.insertInAnyWay(dist_unused);
			node->val.ScreenSpaceArea = screen_space_area;
			node->val.pVisual = pVisual;
		}
		else if (screen_space_area > r_ssaLOD_B) // Если объект близко - рисуем его детальную геометрию (детей)
		{
			for (IRender_Visual* child : pLod->children)
				ProcessStaticVisual(child, ctx, dest);
		}
	}
	break;

	case MT_TREE_ST:
	case MT_TREE_PM: 
	{
		// Получаем мировую позицию
		fvec3 world_pos;
		ctx.current_transform->transform_tiny(world_pos, pVisual->vis.sphere.P);

		// Используем EnqueueDynamic, чтобы сохранить матрицу трансформации
		EnqueueDynamic(pVisual, world_pos, ctx, dest);
	}
	break;

	default: 
	{
		// Обычная статика
		EnqueueStatic(pVisual, ctx, dest);
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

void CSceneGraph::PrepareDynamicInstances(SceneGraphPacket& packet, const SceneTraversalContext& gather_ctx)
{
	SceneTraversalContext ctx = gather_ctx;
	ctx.current_transform = nullptr;
	ctx.frustum = nullptr;

	CurrentRenderContext::Scope tls_scope(packet, ctx);

	for (IRenderable* renderable : packet.m_culled_dynamics)
	{
		if (!renderable)
			continue;

		ctx.current_owner = renderable;
		renderable->renderable_Render();
	}
	packet.m_culled_dynamics.clear();
}

void CSceneGraph::DebugCheckDuplicateVisuals(SceneGraphPacket& packet)
{
	// Вспомогательная лямбда для проверки вектора узлов
	auto check_batch = [](auto& batch, const char* context) {
		std::unordered_set<IRender_Visual*> unique_set;
		for (const auto& node : batch)
		{
			IRender_Visual* v = node.pVisual;
			if (!unique_set.insert(v).second)
			{
				Msg("[DUPLICATE] visual 0x%p in %s", v, context);
				R_ASSERT2(false, "Duplicate visual detected in render packet");
			}
		}
	};

	// --- Статическая геометрия (queue_static[0] и queue_static[1]) ---
	for (int priority = 0; priority < 2; ++priority)
	{
		auto& mapVS = packet.queue_static[priority];
		for (auto itVS = mapVS.begin(); itVS != mapVS.end(); ++itVS)
		{
			auto& mapPS = itVS->val;
			for (auto itPS = mapPS.begin(); itPS != mapPS.end(); ++itPS)
			{
				auto& mapCS = itPS->val;
				for (auto itCS = mapCS.begin(); itCS != mapCS.end(); ++itCS)
				{
					auto& mapState = itCS->val;
					for (auto itState = mapState.begin(); itState != mapState.end(); ++itState)
					{
						auto& mapTex = itState->val;
						for (auto itTex = mapTex.begin(); itTex != mapTex.end(); ++itTex)
						{
							check_batch(itTex->val, "static opaque batch");
						}
					}
				}
			}
		}
	}

	// --- Динамическая геометрия (queue_dynamic[0] и queue_dynamic[1]) ---
	for (int priority = 0; priority < 2; ++priority)
	{
		auto& mapVS = packet.queue_dynamic[priority];
		for (auto itVS = mapVS.begin(); itVS != mapVS.end(); ++itVS)
		{
			auto& mapPS = itVS->val;
			for (auto itPS = mapPS.begin(); itPS != mapPS.end(); ++itPS)
			{
				auto& mapCS = itPS->val;
				for (auto itCS = mapCS.begin(); itCS != mapCS.end(); ++itCS)
				{
					auto& mapState = itCS->val;
					for (auto itState = mapState.begin(); itState != mapState.end(); ++itState)
					{
						auto& mapTex = itState->val;
						for (auto itTex = mapTex.begin(); itTex != mapTex.end(); ++itTex)
						{
							check_batch(itTex->val, "dynamic opaque batch");
						}
					}
				}
			}
		}
	}

	// --- Sorted-контейнеры (transparent, distortion, wallmarks, emissive, HUD) ---
	auto check_sorted = [](auto& sortedMap, const char* context) 
	{
		std::unordered_set<IRender_Visual*> unique_set;
		for (auto it = sortedMap.begin(); it != sortedMap.end(); ++it)
		{
			IRender_Visual* v = it->val.pVisual;
			if (!unique_set.insert(v).second)
			{
				Msg("[DUPLICATE] visual 0x%p in %s", v, context);
				R_ASSERT2(false, "Duplicate visual detected in render packet");
			}
		}
	};

	check_sorted(packet.queue_transparent, "transparent");
	check_sorted(packet.queue_distortion, "distortion");
	check_sorted(packet.queue_wallmarks, "wallmarks");
	check_sorted(packet.mapEmissive, "emissive");
	check_sorted(packet.queue_hud, "HUD");

	// --- LOD map ---
	{
		std::unordered_set<IRender_Visual*> unique_set;
		for (auto it = packet.mapLOD.begin(); it != packet.mapLOD.end(); ++it)
		{
			IRender_Visual* v = it->val.pVisual;
			if (!unique_set.insert(v).second)
			{
				Msg("[DUPLICATE] visual 0x%p in LOD map", v);
				R_ASSERT2(false, "Duplicate visual detected in render packet");
			}
		}
	}
}

// ===============================================================================================
//  CSceneGraph::BuildScene
//  Назначение: Обход пространства (секторов и порталов) и сбор геометрии в указанный пакет.
// ===============================================================================================

// Shortcut (создание фрустума из матрицы)
void CSceneGraph::BuildScene(IRender_Sector* _sector, 
							 fmat4x4& mCombined, 
							 fvec3& _cop, 
							 BOOL _dynamic, 
							 BOOL _precise_portals, 
							 SceneGraphPacket& dest)
{
	OPTICK_EVENT("BuildScene - shortcut");

	CFrustum temp_frustum;
	temp_frustum.CreateFromMatrix(mCombined, FRUSTUM_P_ALL);
	BuildScene(_sector, &temp_frustum, mCombined, _cop, _dynamic, _precise_portals, dest);
}

// Main Implementation (Основная логика)
void CSceneGraph::BuildScene(IRender_Sector* start_sector,
							 CFrustum* view_frustum, 
							 fmat4x4& mCombined,
							 fvec3& camera_pos, 
							 BOOL render_dynamic, 
							 BOOL precise_portals,
							 SceneGraphPacket& dest)
{
	OPTICK_EVENT("BuildScene - main");

	VERIFY(start_sector);
	VERIFY(view_frustum);

	dest.Clear();

	// Увеличиваем маркер
	m_traversal_marker++;

	// -------------------------------------------------------------------------
	// Подготовка локального контекста (TLS)
	// -------------------------------------------------------------------------
	SceneTraversalContext local_ctx;
	local_ctx.frustum = view_frustum; // Базовый фрустум
	local_ctx.is_hud_pass = FALSE;
	local_ctx.is_invisible_mode = FALSE;
	local_ctx.current_owner = nullptr;
	local_ctx.current_transform = &Fidentity;
	local_ctx.traversal_marker_id = ++m_traversal_marker;
	local_ctx.render_phase = CRender::PHASE_SHADOW_DEPTH;

	CurrentRenderContext::Scope tls_scope(dest, local_ctx);

	// -------------------------------------------------------------------------
	// Precise Portals (Внимание: Потенциально небезопасно в MT)
	// -------------------------------------------------------------------------
	// Если precise_portals=TRUE передается в параллельных потоках,
	// запись в pPortal->bDualRender может вызвать гонку данных.
	// Обычно для теней (cascades) это FALSE.
	// Поле bDualRender удалено, так как оно нарушает потокобезопасность.
	/*
	if (precise_portals && RenderImplementation.rmPortals)
	{
		fvec3 box_radius;
		box_radius.set(EPS_L * 20, EPS_L * 20, EPS_L * 20);
		RenderImplementation.Sectors_xrc.box_options(CDB::OPT_FULL_TEST);
		RenderImplementation.Sectors_xrc.box_query(RenderImplementation.rmPortals, camera_pos, box_radius);

		for (int K = 0; K < RenderImplementation.Sectors_xrc.r_count(); K++)
		{
			u32 portal_id =
				RenderImplementation.rmPortals->get_tris()[RenderImplementation.Sectors_xrc.r_begin()[K].id].dummy;
			CPortal* pPortal = (CPortal*)RenderImplementation.Portals[portal_id];
			pPortal->bDualRender = TRUE;
		}
	}
	*/

	// -------------------------------------------------------------------------
	// Обход порталов (Traverse)
	// -------------------------------------------------------------------------
	dest.portal_traverser.Traverse((CSector*)start_sector, *view_frustum, camera_pos, mCombined, 0);

	const auto& visible_sectors = dest.portal_traverser.GetVisibleSectors();

	dest.visible_sectors_map.clear();
	for (const auto& sec_vis : dest.portal_traverser.GetVisibleSectors())
	{
		dest.visible_sectors_map[sec_vis.sector] = &sec_vis;
	}

	// -------------------------------------------------------------------------
	// Сбор СТАТИКИ (Static Geometry)
	// -------------------------------------------------------------------------
	// Проходим по результатам обхода
	for (const auto& sec_vis : visible_sectors)
	{
		CSector* sector = sec_vis.sector;
		IRender_Visual* root_visual = sector->GetRootVisual();
		add_Static(root_visual, view_frustum->getMask(), local_ctx, dest);
	}

	// Возвращаем общий фрустум в контекст
	local_ctx.frustum = view_frustum;

	// -------------------------------------------------------------------------
	// Сбор ДИНАМИКИ (Dynamic Geometry)
	// -------------------------------------------------------------------------
	if (render_dynamic)
	{
		// Делаем запрос к пространственному дереву, используя ОБЩИЙ фрустум каскада
		// Результат пишется в dest.m_spatial_query_results
		g_SpatialSpace->q_frustum(dest.m_spatial_query_results, ISpatial_DB::O_ORDERED, STYPE_RENDERABLE, *view_frustum);

		for (u32 o_it = 0; o_it < dest.m_spatial_query_results.size(); o_it++)
		{
			ISpatial* spatial = dest.m_spatial_query_results[o_it];
			CSector* sector = (CSector*)spatial->spatial.sector;

			if (0 == sector)
				continue;

			// --- ПРОВЕРКА ВИДИМОСТИ СЕКТОРА ---
			// Раньше мы проверяли маркер: if (sector->r_marker != ...)
			// Теперь сектор не хранит маркер текущего прохода.
			// Мы должны найти этот сектор в списке visible_sectors нашего траверсера.

			auto it = dest.visible_sectors_map.find(sector);
			if (it == dest.visible_sectors_map.end()) 
				continue;

			const auto* active_vis_data = it->second;
			if (!active_vis_data)
				continue;

			// --- ПРОВЕРКА ПО ФРУСТУМАМ СЕКТОРА ---
			// Берем фрустумы из найденной структуры данных
			for (const auto& frustum : active_vis_data->frustums)
			{
				// Быстрый тест сферы с конкретным фрустумом
				if (!frustum.testSphere_dirty(spatial->spatial.sphere.P, spatial->spatial.sphere.R))
					continue;

				IRenderable* renderable = spatial->dcast_Renderable();
				if (0 == renderable)
					continue;

				// Настраиваем контекст для отрисовки
				local_ctx.frustum = &frustum;
				local_ctx.current_owner = renderable;

				// Вызываем рендер объекта.
				// Благодаря TLS, внутри вызовется add_Visual, который запишет в 'dest'.
				renderable->renderable_Render();

				// Если объект прошел проверку хотя бы одного фрустума - мы его добавили.
				// Прерываем цикл по фрустумам, чтобы не добавлять дубликаты.
				break;
			}
		}
	}
}
