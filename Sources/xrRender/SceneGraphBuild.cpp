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
	m_current_owner = NULL;
	m_current_transform = NULL;
	m_is_hud_pass = FALSE;
	m_is_invisible_mode = FALSE;
	m_record_multipass = FALSE;
	m_feedback_interface = 0;
	val_feedback_breakp = 0;
	m_culling_bounds_recorder = 0;
	m_traversal_marker = 0;
	m_fetch_config = SceneGraphFetchConfig(true, true, false); 
	b_loaded = FALSE;

	// Инициализация счетчиков (было в хедере, лучше здесь)
	counter_S = 0;
	counter_D = 0;
}

void CSceneGraph::destroy()
{
	// Очистка runtime структур
	nrmVS.clear();
	nrmPS.clear();
	nrmCS.clear();
	nrmStates.clear();
	nrmTextures.clear();
	nrmTexturesTemp.clear();

	matVS.clear();
	matPS.clear();
	matCS.clear();
	matStates.clear();
	matTextures.clear();
	matTexturesTemp.clear();

	lstLODs.clear();
	lstLODgroups.clear();
	lstRenderables.clear();
	lstSpatial.clear();
	lstVisuals.clear();

	lstRecorded.clear();

	// Очистка fixed maps
	m_queue_static[0].destroy();
	m_queue_static[1].destroy();
	m_queue_dynamic[0].destroy();
	m_queue_dynamic[1].destroy();
	m_queue_transparent.destroy();
	m_queue_hud.destroy();
	mapLOD.destroy();
	m_queue_distortion.destroy();
	m_queue_wallmarks.destroy();
	mapEmissive.destroy();
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

void CSceneGraph::EnqueueDynamic(IRender_Visual* pVisual, Fvector& object_center)
{
	// 1. Проверка на повторное добавление в этом кадре
	if (pVisual->vis.m_traversal_marker == m_traversal_marker)
		return;
	pVisual->vis.m_traversal_marker = m_traversal_marker;

	// 2. Вычисление метрики экрана (Screen Space Area) и дистанции
	float distance_sq;
	float screen_space_area = CalcScreenSpaceArea(distance_sq, object_center, pVisual);

	// Отсечение слишком мелких объектов
	if (screen_space_area <= r_ssaDISCARD)
		return;

	// 3. Обработка искажений (Distortion Geometry)
	// Шейдерный элемент с индексом 4 обычно отвечает за спецэффекты
	ShaderElement* shader_distortion = pVisual->shader->E[4]._get();

	if (shader_distortion && shader_distortion->flags.bDistort)
	{
		// Проверяем, разрешен ли этот приоритет в текущем проходе (Fetch Config)
		bool is_priority_allowed = (shader_distortion->flags.iPriority / 2 == 0) ? m_fetch_config.fetch_priority_0
																				 : m_fetch_config.fetch_priority_1;

		if (is_priority_allowed)
		{
			auto* node = m_queue_distortion.insertInAnyWay(distance_sq);
			node->val.ScreenSpaceArea = screen_space_area;
			node->val.pObject = m_current_owner;
			node->val.pVisual = pVisual;
			node->val.Matrix = *m_current_transform;
			node->val.se = shader_distortion;
		}
	}

	// 4. Выбор основного шейдера для рендеринга
	CRender& render_impl = RenderImplementation;
	ShaderElement* shader_element = render_impl.rimp_select_sh_dynamic(pVisual, distance_sq);

	if (!shader_element)
		return;

	// 5. Фильтрация по приоритету (G-Buffer Optimization)
	u32 priority = shader_element->flags.iPriority / 2;
	if (priority == 0 && !m_fetch_config.fetch_priority_0)
		return;
	if (priority == 1 && !m_fetch_config.fetch_priority_1)
		return;

	// Подготовка данных для узла
	// Invisible elements exist only in R1, so we skip check for m_is_invisible_mode logic logic here mostly
	if (m_is_invisible_mode)
		return;

	// 6. Маршрутизация по очередям (HUD / Sorted / Emissive / Wallmarks / Opaque)

	// --- HUD (Оружие и руки) ---
	if (m_is_hud_pass)
	{
		// Если шейдер требует строгой сортировки Back-to-Front (например, стекло на шлеме)
		if (shader_element->flags.bStrictB2F)
		{
			auto* node = m_queue_transparent.insertInAnyWay(distance_sq);
			node->val.ScreenSpaceArea = screen_space_area;
			node->val.pObject = m_current_owner;
			node->val.pVisual = pVisual;
			node->val.Matrix = *m_current_transform;
			node->val.se = shader_element;
		}
		else
		{
			auto* node = m_queue_hud.insertInAnyWay(distance_sq);
			node->val.ScreenSpaceArea = screen_space_area;
			node->val.pObject = m_current_owner;
			node->val.pVisual = pVisual;
			node->val.Matrix = *m_current_transform;
			node->val.se = shader_element;
		}
		return;
	}

	// --- Strict Sorting (Полупрозрачность) ---
	if (shader_element->flags.bStrictB2F)
	{
		auto* node = m_queue_transparent.insertInAnyWay(distance_sq);
		node->val.ScreenSpaceArea = screen_space_area;
		node->val.pObject = m_current_owner;
		node->val.pVisual = pVisual;
		node->val.Matrix = *m_current_transform;
		node->val.se = shader_element;
		return;
	}

	// --- Emissive (Светящиеся объекты) ---
	if (shader_element->flags.bEmissive)
	{
		auto* node = mapEmissive.insertInAnyWay(distance_sq);
		node->val.ScreenSpaceArea = screen_space_area;
		node->val.pObject = m_current_owner;
		node->val.pVisual = pVisual;
		node->val.Matrix = *m_current_transform;
		node->val.se = pVisual->shader->E[4]._get();
	}

	// --- Wallmarks (Следы на динамике) ---
	if (shader_element->flags.bWmark && m_fetch_config.fetch_wallmarks)
	{
		auto* node = m_queue_wallmarks.insertInAnyWay(distance_sq);
		node->val.ScreenSpaceArea = screen_space_area;
		node->val.pObject = m_current_owner;
		node->val.pVisual = pVisual;
		node->val.Matrix = *m_current_transform;
		node->val.se = shader_element;
		return;
	}

	// --- Opaque (Основная непрозрачная геометрия) ---
	// Самая сложная часть: вставка в иерархическую мапу для минимизации смены состояний

	DynamicRenderNode item = {screen_space_area, m_current_owner, pVisual, *m_current_transform};

	// Получаем первый проход шейдера (обычно самый важный)
	SPass& pass = *shader_element->passes.front();

	// Выбираем корневую мапу по приоритету
	auto& target_map = m_queue_dynamic[priority];

	// Иерархическая вставка: VS -> PS -> Constants -> States -> Textures
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

	// Добавляем сам объект в конечный список
	node_tex->val.push_back(item);

	// Обновляем SSA для всех уровней иерархии
	// Это нужно для сортировки групп: мы хотим рисовать группы с большими объектами (близкими) раньше,
	// чтобы работал Early Z-Cull.
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

	// Сбор данных для теней от солнца
	if (m_culling_bounds_recorder)
	{
		Fbox3 temp_box;
		temp_box.transform(pVisual->vis.box, *m_current_transform);
		m_culling_bounds_recorder->push_back(temp_box);
	}
}

void CSceneGraph::EnqueueStatic(IRender_Visual* pVisual)
{
	// 1. Проверка на повторное добавление (Traversal Marker)
	if (pVisual->vis.m_traversal_marker == m_traversal_marker)
		return;
	pVisual->vis.m_traversal_marker = m_traversal_marker;

	// 2. Вычисление метрики экрана (Screen Space Area)
	// Для статики берем позицию сферы напрямую, так как она уже в мировых координатах
	float distance_sq;
	float screen_space_area = CalcScreenSpaceArea(distance_sq, pVisual->vis.sphere.P, pVisual);

	if (screen_space_area <= r_ssaDISCARD)
		return;

	// 3. Обработка искажений (Distortion Geometry)
	ShaderElement* shader_distortion = pVisual->shader->E[4]._get();

	if (shader_distortion && shader_distortion->flags.bDistort)
	{
		// Проверяем, разрешен ли этот приоритет в текущем конфиге
		bool is_priority_allowed = (shader_distortion->flags.iPriority / 2 == 0) ? m_fetch_config.fetch_priority_0
																				 : m_fetch_config.fetch_priority_1;

		if (is_priority_allowed)
		{
			auto* node = m_queue_distortion.insertInAnyWay(distance_sq);
			node->val.ScreenSpaceArea = screen_space_area;
			node->val.pObject = nullptr; // Статика не имеет родительского объекта
			node->val.pVisual = pVisual;
			node->val.Matrix = Fidentity; // Статика всегда в мировых координатах (Identity)
			node->val.se = shader_distortion;
		}
	}

	// 4. Выбор основного шейдера (Static Shader Selection)
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

	// --- Strict Sorting (Полупрозрачность) ---
	if (shader_element->flags.bStrictB2F)
	{
		auto* node = m_queue_transparent.insertInAnyWay(distance_sq);
		node->val.ScreenSpaceArea = screen_space_area;
		node->val.pObject = nullptr;
		node->val.pVisual = pVisual;
		node->val.Matrix = Fidentity;
		node->val.se = shader_element;
		return;
	}

	// --- Emissive (Светящиеся объекты) ---
	if (shader_element->flags.bEmissive)
	{
		auto* node = mapEmissive.insertInAnyWay(distance_sq);
		node->val.ScreenSpaceArea = screen_space_area;
		node->val.pObject = nullptr;
		node->val.pVisual = pVisual;
		node->val.Matrix = Fidentity;
		// Для эмиссивов часто используется специальный проход из E[4]
		node->val.se = pVisual->shader->E[4]._get();
	}

	// --- Wallmarks (Следы на статике) ---
	if (shader_element->flags.bWmark && m_fetch_config.fetch_wallmarks)
	{
		auto* node = m_queue_wallmarks.insertInAnyWay(distance_sq);
		node->val.ScreenSpaceArea = screen_space_area;
		node->val.pObject = nullptr;
		node->val.pVisual = pVisual;
		node->val.Matrix = Fidentity;
		node->val.se = shader_element;
		return;
	}

	// 7. Обратная связь (Feedback)
	// Используется для стриминга или специфических запросов движка к рендеру
	if (m_feedback_interface && counter_S == val_feedback_breakp)
	{
		m_feedback_interface->rfeedback_static(pVisual);
	}

	// 8. Opaque (Основная статическая геометрия)
	counter_S++;

	// Получаем первый проход шейдера
	SPass& pass = *shader_element->passes.front();

	// Выбираем корневую мапу по приоритету
	auto& target_map = m_queue_static[priority];

	// Иерархическая вставка: VS -> PS -> Constants -> States -> Textures
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

	// Добавляем объект в конечный список
	// Используем StaticRenderNode (только SSA и Visual, без матрицы)
	StaticRenderNode item = {screen_space_area, pVisual};
	node_tex->val.push_back(item);

	// Обновляем SSA для всех уровней иерархии (для сортировки групп)
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

	// 9. Сбор данных для теней от солнца
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

// Теперь это метод класса CSceneGraph
bool CSceneGraph::ShouldRenderVisual(IRender_Visual* pVisual, bool isStatic, bool ignore_optimize)
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
		// Для динамики используем текущую матрицу трансформации графа
		Fvector pos;
		m_current_transform->transform_tiny(pos, pVisual->vis.sphere.P);
		adjusted_distance = GetDistFromCamera(pos);
	}

	// 2. Отсечение для Shadow Map (только для фазы теней)
	// Проверка active_phase должна происходить снаружи или мы можем проверить её здесь через CRender
	if (RenderImplementation.active_phase() == CRender::PHASE_SHADOW_DEPTH)
	{
		// Глобальное отсечение по дальности солнца для мелких объектов
		if (sphere_volume < 50000.f && adjusted_distance > ps_r_sun_far)
			return false;

		// Отсечение по уровням (используем High/Z компоненту как в оригинале для теней)
		// В оригинале для теней всегда использовались поля .z, независимо от настроек качества
		const int shadow_quality_idx = 2; // .z component

		const u32 static_count = sizeof(s_static_cull_levels) / sizeof(CullLevel);
		for (u32 i = 0; i < static_count; ++i)
		{
			const CullLevel& L = s_static_cull_levels[i];
			// В оригинале доступ к vec4: [0]=x, [1]=y, [2]=z, [3]=w
			if (sphere_volume < L.size[shadow_quality_idx] && adjusted_distance > L.dist[shadow_quality_idx])
				return false;
		}
	}

	// 3. Отсечение для основной геометрии
	const int q_idx = GetQualityIndex();

	if (isStatic)
	{
		const u32 count = sizeof(s_static_cull_levels) / sizeof(CullLevel);
		for (u32 i = 0; i < count; ++i)
		{
			const CullLevel& L = s_static_cull_levels[i];
			if (sphere_volume < L.size[q_idx] && adjusted_distance > L.dist[q_idx])
				return false;
		}
	}
	else // Dynamic
	{
		const u32 count = sizeof(s_dynamic_cull_levels) / sizeof(CullLevel);
		for (u32 i = 0; i < count; ++i)
		{
			const CullLevel& L = s_dynamic_cull_levels[i];
			if (sphere_volume < L.size[q_idx] && adjusted_distance > L.dist[q_idx])
				return false;
		}
	}

	return true;
}

void CSceneGraph::ProcessDynamicVisual(IRender_Visual* pVisual)
{
	if (0 == pVisual)
		return;

	// ShouldRenderVisual скорее всего осталась static/helper функцией в .cpp или стала private методом CSceneGraph.
	// Обращаемся к m_current_transform напрямую (это член CSceneGraph)
	// active_phase() - метод CRender, нужен глобальный доступ
	if (!ShouldRenderVisual(pVisual, false, RenderImplementation.active_phase() == CRender::PHASE_SHADOW_DEPTH))
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
				ProcessDynamicVisual(PE_It._effect); // Рекурсивный вызов внутри CSceneGraph
			for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_related.begin();
				 pit != PE_It._children_related.end(); pit++)
				ProcessDynamicVisual(*pit);
			for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_free.begin();
				 pit != PE_It._children_free.end(); pit++)
				ProcessDynamicVisual(*pit);
		}
	}
		return;
	case MT_HIERRARHY: {
		FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
		I = pV->children.begin();
		E = pV->children.end();
		for (; I != E; I++)
			ProcessDynamicVisual(*I);
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
			m_current_transform->transform_tiny(Tpos, pV->vis.sphere.P);
			float ScreenSpaceArea = CalcScreenSpaceArea(D, Tpos, pV->vis.sphere.R / 2.f);
			if (ScreenSpaceArea < r_ssaLOD_A)
				_use_lod = TRUE;
		}
		if (_use_lod)
		{
			ProcessDynamicVisual(pV->m_lod);
		}
		else
		{
#pragma todo(NSDeathman to NSDeathman - разобраться)
			Fvector pos;
			m_current_transform->transform_tiny(pos, pVisual->vis.sphere.P);
			float adjusted_distane = GetDistFromCamera(pos);
			float switch_distance = 100.0f;

			// Здесь нужна глобальная ps_geometry_quality_mode
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
				ProcessDynamicVisual(*I);
		}
	}
		return;
	default: {
		// General type of visual
		Fvector Tpos;
		m_current_transform->transform_tiny(Tpos, pVisual->vis.sphere.P);

		if (RenderImplementation.active_phase() == CRender::PHASE_NORMAL)
		{
			// DReuseItem определен внутри CSceneGraph (или R_dsgraph_structure)
			DReuseItem item;
			item.visual = pVisual;
			item.matrix = *m_current_transform;
			m_visuals_dynamic_visible.push_back(item);
		}

		EnqueueDynamic(pVisual, Tpos);
	}
		return;
	}
}

void CSceneGraph::ProcessStaticVisual(IRender_Visual* pVisual)
{
	// HOM остался в RenderImplementation
	if (!RenderImplementation.HOM.visible(pVisual->vis))
		return;

	if (!ShouldRenderVisual(pVisual, true, RenderImplementation.active_phase() == CRender::PHASE_SHADOW_DEPTH))
		return;

	if (RenderImplementation.active_phase() == CRender::PHASE_NORMAL)
	{
		m_visuals_static_visible.push_back(pVisual);
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
				ProcessDynamicVisual(PE_It._effect); // Вызов метода CSceneGraph
			for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_related.begin();
				 pit != PE_It._children_related.end(); pit++)
				ProcessDynamicVisual(*pit);
			for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_free.begin();
				 pit != PE_It._children_free.end(); pit++)
				ProcessDynamicVisual(*pit);
		}
	}
		return;
	case MT_HIERRARHY: {
		FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
		I = pV->children.begin();
		E = pV->children.end();
		for (; I != E; I++)
			ProcessStaticVisual(*I); // Вызов метода CSceneGraph
	}
		return;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID: {

#pragma todo(NSDeathman to NSDeathman - разобраться)
		Fvector pos;
		m_current_transform->transform_tiny(pos, pVisual->vis.sphere.P);
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
			ProcessStaticVisual(*I);
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
			mapLOD_Node* N = mapLOD.insertInAnyWay(D);
			N->val.ScreenSpaceArea = ScreenSpaceArea;
			N->val.pVisual = pVisual;
		}
		if (ScreenSpaceArea > r_ssaLOD_B)
		{
			I = pV->children.begin();
			E = pV->children.end();
			for (; I != E; I++)
				ProcessStaticVisual(*I);
		}
	}
		return;
	case MT_TREE_PM:
	case MT_TREE_ST: {
		EnqueueStatic(pVisual);
	}
		return;
	default: {
		EnqueueStatic(pVisual);
	}
		return;
	}
}

BOOL CSceneGraph::add_Dynamic(IRender_Visual* pVisual, u32 planes)
{
	// 1. Трансформация позиции в мировые координаты
	Fvector world_position;
	m_current_transform->transform_tiny(world_position, pVisual->vis.sphere.P);

	// 2. Frustum Culling (Проверка попадания в камеру)
	EFC_Visible visibility_status =
		RenderImplementation.View->testSphere(world_position, pVisual->vis.sphere.R, planes);

	if (visibility_status == fcvNone)
		return FALSE;

	// 3. Проверка на значимость (Distance / Size Culling)
	bool is_shadow_phase = (RenderImplementation.active_phase() == CRender::PHASE_SHADOW_DEPTH);
	if (!ShouldRenderVisual(pVisual, false, is_shadow_phase))
		return FALSE;

	// 4. Разбор типа объекта
	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP: {
		PS::CParticleGroup* pGroup = (PS::CParticleGroup*)pVisual;

		// Логика разделяется в зависимости от статуса видимости родителя
		if (visibility_status == fcvPartial)
		{
			for (PS::CParticleGroup::SItem& item : pGroup->items)
			{
				if (item._effect)
					add_Dynamic(item._effect, planes); // Рекурсия с проверкой фрустума

				for (IRender_Visual* child : item._children_related)
					add_Dynamic(child, planes);

				for (IRender_Visual* child : item._children_free)
					add_Dynamic(child, planes);
			}
		}
		else // fcvFully - объект полностью в кадре, проверки фрустума детям не нужны
		{
			for (PS::CParticleGroup::SItem& item : pGroup->items)
			{
				if (item._effect)
					ProcessDynamicVisual(item._effect); // Быстрое добавление

				for (IRender_Visual* child : item._children_related)
					ProcessDynamicVisual(child);

				for (IRender_Visual* child : item._children_free)
					ProcessDynamicVisual(child);
			}
		}
	}
	break;

	case MT_HIERRARHY: {
		FHierrarhyVisual* pHierarchy = (FHierrarhyVisual*)pVisual;

		if (visibility_status == fcvPartial)
		{
			for (IRender_Visual* child : pHierarchy->children)
				add_Dynamic(child, planes);
		}
		else
		{
			for (IRender_Visual* child : pHierarchy->children)
				ProcessDynamicVisual(child);
		}
	}
	break;

	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID: {
		CKinematics* pKinematics = (CKinematics*)pVisual;

		// Проверка LOD для скелета
		bool use_lod = false;
		if (pKinematics->m_lod)
		{
			float dist_sq;
			// Используем уже вычисленную world_position, но для SSA нужен радиус
			// Примечание: CalcScreenSpaceArea использует позицию камеры из Engine.RenderView
			float screen_space_area = CalcScreenSpaceArea(dist_sq, world_position, pVisual->vis.sphere.R / 2.f);

			if (screen_space_area < r_ssaLOD_A)
				use_lod = true;
		}

		if (use_lod)
		{
			ProcessDynamicVisual(pKinematics->m_lod);
		}
		else
		{
			// Расчет дистанции для переключения качества анимаций/стенсилов
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

			// Если близко - обновляем кости и следы
			if (dist_from_camera < switch_distance)
			{
				pKinematics->CalculateBones(TRUE);
				pKinematics->CalculateWallmarks();
			}

			// Скелеты всегда добавляем как Process, так как их части (children)
			// обычно находятся внутри баундинг бокса родителя.
			for (IRender_Visual* child : pKinematics->children)
				ProcessDynamicVisual(child);
		}
	}
	break;

	default: {
		// Листовой объект (Mesh) - отправляем в очередь
		EnqueueDynamic(pVisual, world_position);
	}
	break;
	}

	return TRUE;
}

void CSceneGraph::add_Static(IRender_Visual* pVisual, u32 planes)
{
	// 1. Frustum Culling (Sphere + AABB Test)
	vis_data& vis_data = pVisual->vis;
	EFC_Visible visibility_status =
		RenderImplementation.View->testSAABB(vis_data.sphere.P, vis_data.sphere.R, vis_data.box.data(), planes);

	if (visibility_status == fcvNone)
		return;

	// 2. Occlusion Culling (HOM - Hierarchical Occlusion Maps)
	// Пропускаем невидимые за стенами объекты
	if (!RenderImplementation.HOM.visible(vis_data))
		return;

	// 3. Проверка на значимость (Distance / Size Culling)
	bool is_shadow_phase = (RenderImplementation.active_phase() == CRender::PHASE_SHADOW_DEPTH);
	if (!ShouldRenderVisual(pVisual, true, is_shadow_phase))
		return;

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
					add_Dynamic(item._effect, planes);
				for (auto* c : item._children_related)
					add_Dynamic(c, planes);
				for (auto* c : item._children_free)
					add_Dynamic(c, planes);
			}
		}
		else
		{
			for (PS::CParticleGroup::SItem& item : pGroup->items)
			{
				if (item._effect)
					ProcessDynamicVisual(item._effect);
				for (auto* c : item._children_related)
					ProcessDynamicVisual(c);
				for (auto* c : item._children_free)
					ProcessDynamicVisual(c);
			}
		}
	}
	break;

	case MT_HIERRARHY: {
		FHierrarhyVisual* pHierarchy = (FHierrarhyVisual*)pVisual;

		if (visibility_status == fcvPartial)
		{
			for (IRender_Visual* child : pHierarchy->children)
				add_Static(child, planes);
		}
		else
		{
			for (IRender_Visual* child : pHierarchy->children)
				ProcessStaticVisual(child);
		}
	}
	break;

	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID: {
		// Скелетная статика (например, трупы, ставшие частью уровня, или сложные механизмы)
		Fvector object_pos;
		m_current_transform->transform_tiny(object_pos, pVisual->vis.sphere.P);

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
				add_Static(child, planes);
		}
		else
		{
			for (IRender_Visual* child : pKinematics->children)
				ProcessStaticVisual(child);
		}
	}
	break;

	case MT_LOD: {
		FLOD* pLod = (FLOD*)pVisual;
		float dist_unused;
		float screen_space_area = CalcScreenSpaceArea(dist_unused, pLod->vis.sphere.P, pLod);

		screen_space_area *= pLod->lod_factor;

		// Если объект далеко - рисуем его как LOD (билборд)
		if (screen_space_area < r_ssaLOD_A)
		{
			if (screen_space_area < r_ssaDISCARD)
				return;

			// Вставляем в очередь LOD-ов
			mapLOD_Node* node = mapLOD.insertInAnyWay(dist_unused);
			node->val.ScreenSpaceArea = screen_space_area;
			node->val.pVisual = pVisual;
		}

		// Если объект близко - рисуем его детальную геометрию (детей)
		if (screen_space_area > r_ssaLOD_B)
		{
			for (IRender_Visual* child : pLod->children)
				ProcessStaticVisual(child);
		}
	}
	break;

	case MT_TREE_ST:
	case MT_TREE_PM:
	default: {
		// Обычная статика - отправляем в очередь
		EnqueueStatic(pVisual);
	}
	break;
	}
}
