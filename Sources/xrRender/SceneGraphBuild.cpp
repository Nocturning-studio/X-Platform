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

ICF float CalcSSA(float& distSQ, Fvector& C, IRender_Visual* V)
{
	float R = V->vis.sphere.R + 0;
	distSQ = Engine.RenderView.Position.distance_to_sqr(C) + EPS;
	return R / distSQ;
}
ICF float CalcSSA(float& distSQ, Fvector& C, float R)
{
	distSQ = Engine.RenderView.Position.distance_to_sqr(C) + EPS;
	return R / distSQ;
}

void CSceneGraph::EnqueueDynamic(IRender_Visual* pVisual, Fvector& Center)
{
	// Для доступа к методам CRender (например, rimp_select_sh_dynamic)
	CRender& RI = RenderImplementation;

	// 'm_traversal_marker' теперь член CSceneGraph, обращаемся напрямую
	if (pVisual->vis.m_traversal_marker == m_traversal_marker)
		return;
	pVisual->vis.m_traversal_marker = m_traversal_marker;

	float distSQ;
	float ScreenSpaceArea = CalcSSA(distSQ, Center, pVisual);
	if (ScreenSpaceArea <= r_ssaDISCARD)
		return;

	// Distortive geometry
	VERIFY(pVisual->shader._get());
	ShaderElement* sh_d = &*pVisual->shader->E[4];

	// pmask - член CSceneGraph
	if (sh_d && sh_d->flags.bDistort)
	{
		// Проверяем приоритет дисторшена (обычно он 1, но проверим честно)
		bool allowed = (sh_d->flags.iPriority / 2 == 0) ? m_fetch_config.fetch_priority_0 : m_fetch_config.fetch_priority_1;
		if (allowed)
		{
			mapSorted_Node* N = m_queue_distortion.insertInAnyWay(distSQ);
			N->val.ScreenSpaceArea = ScreenSpaceArea;
			// m_current_owner и m_current_transform - члены CSceneGraph
			N->val.pObject = m_current_owner;
			N->val.pVisual = pVisual;
			N->val.Matrix = *m_current_transform;
			N->val.se = sh_d; // 4=L_special
		}
	}

	// Select shader - метод остался в CRender
	ShaderElement* sh = RI.rimp_select_sh_dynamic(pVisual, distSQ);
	if (0 == sh)
		return;
	u32 priority = sh->flags.iPriority / 2;
	if (priority == 0 && !m_fetch_config.fetch_priority_0)
		return;
	if (priority == 1 && !m_fetch_config.fetch_priority_1)
		return;

	// Create common node
	// Invisible elements exist only in R1
	DynamicRenderNode item = {ScreenSpaceArea, m_current_owner, pVisual, *m_current_transform};

	// HUD rendering
	// m_is_hud_pass - член CSceneGraph
	if (m_is_hud_pass)
	{
		if (sh->flags.bStrictB2F)
		{
			mapSorted_Node* N = m_queue_transparent.insertInAnyWay(distSQ);
			N->val.ScreenSpaceArea = ScreenSpaceArea;
			N->val.pObject = m_current_owner;
			N->val.pVisual = pVisual;
			N->val.Matrix = *m_current_transform;
			N->val.se = sh;
			return;
		}
		else
		{
			mapHUD_Node* N = m_queue_hud.insertInAnyWay(distSQ);
			N->val.ScreenSpaceArea = ScreenSpaceArea;
			N->val.pObject = m_current_owner;
			N->val.pVisual = pVisual;
			N->val.Matrix = *m_current_transform;
			N->val.se = sh;
			return;
		}
	}

	// m_is_invisible_mode - член CSceneGraph
	if (m_is_invisible_mode)
		return;

	// strict-sorting selection
	if (sh->flags.bStrictB2F)
	{
		mapSorted_Node* N = m_queue_transparent.insertInAnyWay(distSQ);
		N->val.ScreenSpaceArea = ScreenSpaceArea;
		N->val.pObject = m_current_owner;
		N->val.pVisual = pVisual;
		N->val.Matrix = *m_current_transform;
		N->val.se = sh;
		return;
	}

	// Emissive geometry
	if (sh->flags.bEmissive)
	{
		mapSorted_Node* N = mapEmissive.insertInAnyWay(distSQ);
		N->val.ScreenSpaceArea = ScreenSpaceArea;
		N->val.pObject = m_current_owner;
		N->val.pVisual = pVisual;
		N->val.Matrix = *m_current_transform;
		N->val.se = &*pVisual->shader->E[4]; // 4=L_special
	}

	// pmask_wmark - член CSceneGraph
	if (sh->flags.bWmark && m_fetch_config.fetch_wallmarks)
	{
		mapSorted_Node* N = m_queue_wallmarks.insertInAnyWay(distSQ);
		N->val.ScreenSpaceArea = ScreenSpaceArea;
		N->val.pObject = m_current_owner;
		N->val.pVisual = pVisual;
		N->val.Matrix = *m_current_transform;
		N->val.se = sh;
		return;
	}

	// the most common node
	SPass& pass = *sh->passes.front();
	mapMatrix_T& map = m_queue_dynamic[sh->flags.iPriority / 2];
#ifdef USE_RESOURCE_DEBUGGER
	mapMatrixVS::TNode* Nvs = map.insert(pass.vs);
	mapMatrixPS::TNode* Nps = Nvs->val.insert(pass.ps);
#else
	mapMatrixVS::TNode* Nvs = map.insert(pass.vs->sh);
	mapMatrixPS::TNode* Nps = Nvs->val.insert(pass.ps->sh);
#endif
	mapMatrixCS::TNode* Ncs = Nps->val.insert(pass.constants._get());
	mapMatrixStates::TNode* Nstate = Ncs->val.insert(pass.state->state);
	mapMatrixTextures::TNode* Ntex = Nstate->val.insert(pass.T._get());
	mapMatrixItems& items = Ntex->val;
	items.push_back(item);

	// Need to sort for HZB efficient use
	if (ScreenSpaceArea > Ntex->val.ScreenSpaceArea)
	{
		Ntex->val.ScreenSpaceArea = ScreenSpaceArea;
		if (ScreenSpaceArea > Nstate->val.ScreenSpaceArea)
		{
			Nstate->val.ScreenSpaceArea = ScreenSpaceArea;
			if (ScreenSpaceArea > Ncs->val.ScreenSpaceArea)
			{
				Ncs->val.ScreenSpaceArea = ScreenSpaceArea;
				if (ScreenSpaceArea > Nps->val.ScreenSpaceArea)
				{
					Nps->val.ScreenSpaceArea = ScreenSpaceArea;
					if (ScreenSpaceArea > Nvs->val.ScreenSpaceArea)
					{
						Nvs->val.ScreenSpaceArea = ScreenSpaceArea;
					}
				}
			}
		}
	}

	// m_culling_bounds_recorder - член CSceneGraph
	if (m_culling_bounds_recorder)
	{
		Fbox3 temp;
		Fmatrix& xf = *m_current_transform;
		temp.transform(pVisual->vis.box, xf);
		m_culling_bounds_recorder->push_back(temp);
	}
}

void CSceneGraph::EnqueueStatic(IRender_Visual* pVisual)
{
	CRender& RI = RenderImplementation;

	// 'm_traversal_marker' - член CSceneGraph
	if (pVisual->vis.m_traversal_marker == m_traversal_marker)
		return;
	pVisual->vis.m_traversal_marker = m_traversal_marker;

	float distSQ;
	float ScreenSpaceArea = CalcSSA(distSQ, pVisual->vis.sphere.P, pVisual);
	if (ScreenSpaceArea <= r_ssaDISCARD)
		return;

	// Distortive geometry
	VERIFY(pVisual->shader._get());
	ShaderElement* sh_d = &*pVisual->shader->E[4];

	// pmask - член CSceneGraph
	if (sh_d && sh_d->flags.bDistort)
	{
		bool allowed = (sh_d->flags.iPriority / 2 == 0) ? m_fetch_config.fetch_priority_0 : m_fetch_config.fetch_priority_1;
		if (allowed)
		{
			mapSorted_Node* N = m_queue_distortion.insertInAnyWay(distSQ);
			N->val.ScreenSpaceArea = ScreenSpaceArea;
			N->val.pObject = NULL;
			N->val.pVisual = pVisual;
			N->val.Matrix = Fidentity;
			N->val.se = &*pVisual->shader->E[4]; // 4=L_special
		}
	}

	// Select shader - вызываем метод CRender
	ShaderElement* sh = RI.rimp_select_sh_static(pVisual, distSQ);

	if (0 == sh)
		return;

	u32 priority = sh->flags.iPriority / 2;
	if (priority == 0 && !m_fetch_config.fetch_priority_0)
		return;
	if (priority == 1 && !m_fetch_config.fetch_priority_1)
		return;

	// strict-sorting selection
	if (sh->flags.bStrictB2F)
	{
		mapSorted_Node* N = m_queue_transparent.insertInAnyWay(distSQ);
		N->val.pObject = NULL;
		N->val.pVisual = pVisual;
		N->val.Matrix = Fidentity;
		N->val.se = sh;
		return;
	}

	// Emissive geometry
	if (sh->flags.bEmissive)
	{
		mapSorted_Node* N = mapEmissive.insertInAnyWay(distSQ);
		N->val.ScreenSpaceArea = ScreenSpaceArea;
		N->val.pObject = NULL;
		N->val.pVisual = pVisual;
		N->val.Matrix = Fidentity;
		N->val.se = &*pVisual->shader->E[4]; // 4=L_special
	}

	// pmask_wmark - член CSceneGraph
	if (sh->flags.bWmark && m_fetch_config.fetch_wallmarks)
	{

		mapSorted_Node* N = m_queue_wallmarks.insertInAnyWay(distSQ);
		N->val.ScreenSpaceArea = ScreenSpaceArea;
		N->val.pObject = NULL;
		N->val.pVisual = pVisual;
		N->val.Matrix = Fidentity;
		N->val.se = sh;
		return;
	}

	// m_feedback_interface, counter_S, val_feedback_breakp - члены CSceneGraph
	if (m_feedback_interface && counter_S == val_feedback_breakp)
		m_feedback_interface->rfeedback_static(pVisual);

	counter_S++;
	SPass& pass = *sh->passes.front();
	mapNormal_T& map = m_queue_static[sh->flags.iPriority / 2];
#ifdef USE_RESOURCE_DEBUGGER
	mapNormalVS::TNode* Nvs = map.insert(pass.vs);
	mapNormalPS::TNode* Nps = Nvs->val.insert(pass.ps);
#else
	mapNormalVS::TNode* Nvs = map.insert(pass.vs->sh);
	mapNormalPS::TNode* Nps = Nvs->val.insert(pass.ps->sh);
#endif
	mapNormalCS::TNode* Ncs = Nps->val.insert(pass.constants._get());
	mapNormalStates::TNode* Nstate = Ncs->val.insert(pass.state->state);
	mapNormalTextures::TNode* Ntex = Nstate->val.insert(pass.T._get());
	mapNormalItems& items = Ntex->val;
	StaticRenderNode item = {ScreenSpaceArea, pVisual};
	items.push_back(item);

	// Need to sort for HZB efficient use
	if (ScreenSpaceArea > Ntex->val.ScreenSpaceArea)
	{
		Ntex->val.ScreenSpaceArea = ScreenSpaceArea;
		if (ScreenSpaceArea > Nstate->val.ScreenSpaceArea)
		{
			Nstate->val.ScreenSpaceArea = ScreenSpaceArea;
			if (ScreenSpaceArea > Ncs->val.ScreenSpaceArea)
			{
				Ncs->val.ScreenSpaceArea = ScreenSpaceArea;
				if (ScreenSpaceArea > Nps->val.ScreenSpaceArea)
				{
					Nps->val.ScreenSpaceArea = ScreenSpaceArea;
					if (ScreenSpaceArea > Nvs->val.ScreenSpaceArea)
					{
						Nvs->val.ScreenSpaceArea = ScreenSpaceArea;
					}
				}
			}
		}
	}

	// m_culling_bounds_recorder - член CSceneGraph
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
			float ScreenSpaceArea = CalcSSA(D, Tpos, pV->vis.sphere.R / 2.f);
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
		float ScreenSpaceArea = CalcSSA(D, pV->vis.sphere.P, pV);
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
	// Check frustum visibility and calculate distance to visual's center
	Fvector Tpos; // transformed position
	EFC_Visible VIS;

	// m_current_transform теперь член CSceneGraph, обращаемся напрямую
	m_current_transform->transform_tiny(Tpos, pVisual->vis.sphere.P);

	// View и HOM остались в RenderImplementation (CRender)
	VIS = RenderImplementation.View->testSphere(Tpos, pVisual->vis.sphere.R, planes);
	if (fcvNone == VIS)
		return FALSE;

	// ShouldRenderVisual используем как внешнюю функцию (или метод, если перенесли)
	if (!ShouldRenderVisual(pVisual, false, RenderImplementation.active_phase() == CRender::PHASE_SHADOW_DEPTH))
		return FALSE;

	// If we get here visual is visible or partially visible
	xr_vector<IRender_Visual*>::iterator I, E;

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP: {
		PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;
		for (PS::CParticleGroup::SItemVecIt i_it = pG->items.begin(); i_it != pG->items.end(); i_it++)
		{
			PS::CParticleGroup::SItem& PE_It = *i_it;
			if (fcvPartial == VIS)
			{
				if (PE_It._effect)
					add_Dynamic(PE_It._effect, planes); // Рекурсия: вызов метода текущего объекта CSceneGraph
				for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_related.begin();
					 pit != PE_It._children_related.end(); pit++)
					add_Dynamic(*pit, planes);
				for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_free.begin();
					 pit != PE_It._children_free.end(); pit++)
					add_Dynamic(*pit, planes);
			}
			else
			{
				if (PE_It._effect)
					ProcessDynamicVisual(PE_It._effect); // Вызов метода текущего объекта
				for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_related.begin();
					 pit != PE_It._children_related.end(); pit++)
					ProcessDynamicVisual(*pit);
				for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_free.begin();
					 pit != PE_It._children_free.end(); pit++)
					ProcessDynamicVisual(*pit);
			}
		}
	}
	break;
	case MT_HIERRARHY: {
		FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
		I = pV->children.begin();
		E = pV->children.end();
		if (fcvPartial == VIS)
		{
			for (; I != E; I++)
				add_Dynamic(*I, planes);
		}
		else
		{
			for (; I != E; I++)
				ProcessDynamicVisual(*I);
		}
	}
	break;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID: {
		CKinematics* pV = (CKinematics*)pVisual;
		BOOL _use_lod = FALSE;
		if (pV->m_lod)
		{
			Fvector fTpos;
			float D;
			m_current_transform->transform_tiny(fTpos, pV->vis.sphere.P);
			float ScreenSpaceArea = CalcSSA(D, fTpos, pV->vis.sphere.R / 2.f);
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
			float adjusted_distance = GetDistFromCamera(pos);
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

			if (adjusted_distance < switch_distance)
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
	break;
	default: {
		// Вызываем метод вставки динамики (который мы ранее перенесли в CSceneGraph)
		EnqueueDynamic(pVisual, Tpos);
	}
	break;
	}
	return TRUE;
}

void CSceneGraph::add_Static(IRender_Visual* pVisual, u32 planes)
{
	// Check frustum visibility and calculate distance to visual's center
	EFC_Visible VIS;
	vis_data& vis = pVisual->vis;
	// Используем View из RenderImplementation
	VIS = RenderImplementation.View->testSAABB(vis.sphere.P, vis.sphere.R, vis.box.data(), planes);
	if (fcvNone == VIS)
		return;
	// Используем HOM из RenderImplementation
	if (!RenderImplementation.HOM.visible(vis))
		return;

	// m_current_transform - член CSceneGraph
	if (!ShouldRenderVisual(pVisual, true, RenderImplementation.active_phase() == CRender::PHASE_SHADOW_DEPTH))
		return;

	// If we get here visual is visible or partially visible
	xr_vector<IRender_Visual*>::iterator I, E;

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP: {
		PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;
		for (PS::CParticleGroup::SItemVecIt i_it = pG->items.begin(); i_it != pG->items.end(); i_it++)
		{
			PS::CParticleGroup::SItem& PE_It = *i_it;
			if (fcvPartial == VIS)
			{
				if (PE_It._effect)
					add_Dynamic(PE_It._effect, planes);
				for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_related.begin();
					 pit != PE_It._children_related.end(); pit++)
					add_Dynamic(*pit, planes);
				for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_free.begin();
					 pit != PE_It._children_free.end(); pit++)
					add_Dynamic(*pit, planes);
			}
			else
			{
				if (PE_It._effect)
					ProcessDynamicVisual(PE_It._effect);
				for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_related.begin();
					 pit != PE_It._children_related.end(); pit++)
					ProcessDynamicVisual(*pit);
				for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_free.begin();
					 pit != PE_It._children_free.end(); pit++)
					ProcessDynamicVisual(*pit);
			}
		}
	}
	break;
	case MT_HIERRARHY: {
		FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
		I = pV->children.begin();
		E = pV->children.end();
		if (fcvPartial == VIS)
		{
			for (; I != E; I++)
				add_Static(*I, planes);
		}
		else
		{
			for (; I != E; I++)
				ProcessStaticVisual(*I);
		}
	}
	break;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID: {
#pragma todo(NSDeathman to NSDeathman - разобраться)
		Fvector pos;
		m_current_transform->transform_tiny(pos, pVisual->vis.sphere.P);
		float adjusted_distance = GetDistFromCamera(pos);
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

		if (adjusted_distance < switch_distance)
			pV->CalculateBones(TRUE);

		I = pV->children.begin();
		E = pV->children.end();
		if (fcvPartial == VIS)
		{
			for (; I != E; I++)
				add_Static(*I, planes);
		}
		else
		{
			for (; I != E; I++)
				ProcessStaticVisual(*I);
		}
	}
	break;
	case MT_LOD: {
		FLOD* pV = (FLOD*)pVisual;
		float D;
		float ScreenSpaceArea = CalcSSA(D, pV->vis.sphere.P, pV);
		ScreenSpaceArea *= pV->lod_factor;
		if (ScreenSpaceArea < r_ssaLOD_A)
		{
			if (ScreenSpaceArea < r_ssaDISCARD)
				return;
			// Вставка в локальный mapLOD
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
	break;
	case MT_TREE_ST:
	case MT_TREE_PM: {
		// Вызов метода через текущий объект
		EnqueueStatic(pVisual);
	}
		return;
	default: {
		// OPTICK_EVENT("default");
		EnqueueStatic(pVisual);
	}
	break;
	}
}

