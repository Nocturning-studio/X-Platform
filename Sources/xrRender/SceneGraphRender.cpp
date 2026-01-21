#include "stdafx.h"
#include "SceneGraph.h"
#include "flod.h"
#include "render.h"

#include <ppl.h> // Для concurrency::parallel_for

// Глобальные переменные настроек (внешние)
extern float r_ssaGLOD_start, r_ssaGLOD_end;
extern float r_ssaHZBvsTEX;
extern float r_ssaLOD_A;
extern float r_ssaLOD_B;

using namespace SceneGraphTypes;

// ===============================================================================================
//  Internal Helpers & Predicates
// ===============================================================================================
namespace
{
// --- LOD Factor Calculation ---
ICF float CalculateLODFactor(float screen_space_area, float R)
{
	// Вычисляем коэффициент детализации на основе площади на экране
	return _sqrt(clampr((screen_space_area - r_ssaGLOD_end) / (r_ssaGLOD_start - r_ssaGLOD_end), 0.f, 1.f));
}

// --- LOD Sorting Predicate ---
static bool SortLodsByDotProduct(const std::pair<float, u32>& a, const std::pair<float, u32>& b)
{
	return a.first < b.first;
}

// --- Render Helper: Static Batches ---
static void RenderStaticBatch(SceneGraphTypes::mapNormalItems& batch)
{
	// Сортировка Front-to-Back по SSA для Early Z-Cull
	std::sort(batch.begin(), batch.end(),
			  [](const SceneGraphTypes::StaticRenderNode& a, const SceneGraphTypes::StaticRenderNode& b) {
				  return a.ScreenSpaceArea > b.ScreenSpaceArea;
			  });

	for (const auto& node : batch)
	{
		node.pVisual->Render(CalculateLODFactor(node.ScreenSpaceArea, node.pVisual->vis.sphere.R));
	}
}

// --- Render Helper: Dynamic Batches ---
static void RenderDynamicBatch(SceneGraphTypes::mapMatrixItems& batch)
{
	// Сортировка Front-to-Back
	std::sort(batch.begin(), batch.end(),
			  [](const SceneGraphTypes::DynamicRenderNode& a, const SceneGraphTypes::DynamicRenderNode& b) {
				  return a.ScreenSpaceArea > b.ScreenSpaceArea;
			  });

	for (const auto& node : batch)
	{
		RenderBackend.set_transform_world(node.Matrix);
		RenderImplementation.apply_object(node.pObject);
		RenderImplementation.apply_lmaterial();

		node.pVisual->Render(CalculateLODFactor(node.ScreenSpaceArea, node.pVisual->vis.sphere.R));
	}
	batch.clear();
}

// --- Render Callback: Sorted/Transparent Nodes ---
static void __fastcall RenderSortedNode(SceneGraphTypes::mapSorted_Node* node)
{
	VERIFY(node);
	IRender_Visual* pVisual = node->val.pVisual;
	VERIFY(pVisual && pVisual->shader._get());

	RenderBackend.set_Element(node->val.se);
	RenderBackend.set_transform_world(node->val.Matrix);
	RenderImplementation.apply_object(node->val.pObject);
	RenderImplementation.apply_lmaterial();

	pVisual->Render(CalculateLODFactor(node->key, pVisual->vis.sphere.R));
}

// --- Texture List Comparators (for State Optimization) ---
template <typename TNode> bool CompareTexturesLex2(TNode* N1, TNode* N2)
{
	STextureList* t1 = N1->key;
	STextureList* t2 = N2->key;
	if ((*t1)[0] < (*t2)[0])
		return true;
	if ((*t1)[0] > (*t2)[0])
		return false;
	if ((*t1)[1] < (*t2)[1])
		return true;
	return false;
}

template <typename TNode> bool CompareTexturesLex3(TNode* N1, TNode* N2)
{
	STextureList* t1 = N1->key;
	STextureList* t2 = N2->key;
	if ((*t1)[0] < (*t2)[0])
		return true;
	if ((*t1)[0] > (*t2)[0])
		return false;
	if ((*t1)[1] < (*t2)[1])
		return true;
	if ((*t1)[1] > (*t2)[1])
		return false;
	if ((*t1)[2] < (*t2)[2])
		return true;
	return false;
}

template <typename TNode> bool CompareTexturesLexN(TNode* N1, TNode* N2)
{
	STextureList* t1 = N1->key;
	STextureList* t2 = N2->key;
	return std::lexicographical_compare(t1->begin(), t1->end(), t2->begin(), t2->end());
}

template <typename TNode> bool CompareTexturesSSA(TNode* N1, TNode* N2)
{
	return (N1->val.ScreenSpaceArea > N2->val.ScreenSpaceArea);
}

// --- Texture List Sorting Logic ---
template <typename MapTextures, typename VecTypes>
void SortTextureList(VecTypes& list, VecTypes& temp_list, MapTextures& textures_map, BOOL bUseSSA)
{
	int texture_count = textures_map.begin()->key->size();

	if (bUseSSA)
	{
		if (texture_count <= 1)
		{
			textures_map.getANY_P(list);
			std::sort(list.begin(), list.end(), CompareTexturesSSA<typename MapTextures::TNode>);
		}
		else
		{
			// Разделяем на "близкие" (важные для HZB) и "дальние"
			for (auto it = textures_map.begin(); it != textures_map.end(); ++it)
			{
				if (it->val.ScreenSpaceArea > r_ssaHZBvsTEX)
					list.push_back(it);
				else
					temp_list.push_back(it);
			}

			// Близкие сортируем по SSA (для Z-Cull)
			std::sort(list.begin(), list.end(), CompareTexturesSSA<typename MapTextures::TNode>);

			// Дальние сортируем по текстурам (для минимизации переключений)
			if (2 == texture_count)
				std::sort(temp_list.begin(), temp_list.end(), CompareTexturesLex2<typename MapTextures::TNode>);
			else if (3 == texture_count)
				std::sort(temp_list.begin(), temp_list.end(), CompareTexturesLex3<typename MapTextures::TNode>);
			else
				std::sort(temp_list.begin(), temp_list.end(), CompareTexturesLexN<typename MapTextures::TNode>);

			list.insert(list.end(), temp_list.begin(), temp_list.end());
		}
	}
	else
	{
		textures_map.getANY_P(list);
		if (2 == texture_count)
			std::sort(list.begin(), list.end(), CompareTexturesLex2<typename MapTextures::TNode>);
		else if (3 == texture_count)
			std::sort(list.begin(), list.end(), CompareTexturesLex3<typename MapTextures::TNode>);
		else
			std::sort(list.begin(), list.end(), CompareTexturesLexN<typename MapTextures::TNode>);
	}
}
} // namespace

// ===============================================================================================
//  CSceneGraph Implementation
// ===============================================================================================

void CSceneGraph::Render(SceneGraphRenderType type, u32 priority, bool clear, bool setup_zb)
{
	switch (type)
	{
	case SceneGraphRenderType::Opaque:
		_RenderOpaque(priority, clear);
		break;
	case SceneGraphRenderType::Transparent:
		_RenderTranslucent();
		break;
	case SceneGraphRenderType::HUD:
		_RenderHUD();
		break;
	case SceneGraphRenderType::LOD:
		_RenderLODs(setup_zb, clear);
		break;
	case SceneGraphRenderType::Emissive:
		_RenderEmissive();
		break;
	case SceneGraphRenderType::Wallmarks:
		_RenderWmarks();
		break;
	case SceneGraphRenderType::Distortion:
		_RenderDistortion();
		break;
	}
}

// ===============================================================================================
//  CSceneGraph Rendering Implementation (Updated)
// ===============================================================================================

void CSceneGraph::_RenderOpaque(u32 _priority, bool _clear)
{
	OPTICK_EVENT("RenderOpaque");
	Engine.Statistic->RenderDUMP.Begin();

	// -------------------------------------------------------------------------
	// PHASE 1: STATIC GEOMETRY (Level)
	// -------------------------------------------------------------------------
	{
		RenderBackend.set_transform_world(Fidentity);

		// Используем m_packet.queue_static
		mapNormalVS& map_vs = m_packet.queue_static[_priority];
		// Используем m_scratch.nrmVS
		map_vs.getANY_P(m_scratch.nrmVS);

		for (auto* node_vs : m_scratch.nrmVS)
		{
			RenderBackend.set_Vertex_Shader(node_vs->key);

			mapNormalPS& map_ps = node_vs->val;
			map_ps.ScreenSpaceArea = 0;
			// Используем m_scratch.nrmPS
			map_ps.getANY_P(m_scratch.nrmPS);

			for (auto* node_ps : m_scratch.nrmPS)
			{
				RenderBackend.set_Pixel_Shader(node_ps->key);

				mapNormalCS& map_cs = node_ps->val;
				map_cs.ScreenSpaceArea = 0;
				// Используем m_scratch.nrmCS
				map_cs.getANY_P(m_scratch.nrmCS);

				for (auto* node_cs : m_scratch.nrmCS)
				{
					RenderBackend.set_Constants(node_cs->key);

					mapNormalStates& map_states = node_cs->val;
					map_states.ScreenSpaceArea = 0;
					// Используем m_scratch.nrmStates
					map_states.getANY_P(m_scratch.nrmStates);

					for (auto* node_state : m_scratch.nrmStates)
					{
						RenderBackend.set_States(node_state->key);

						mapNormalTextures& map_tex = node_state->val;
						map_tex.ScreenSpaceArea = 0;

						// Используем m_scratch.nrmTextures и m_scratch.nrmTexturesTemp
						SortTextureList(m_scratch.nrmTextures, m_scratch.nrmTexturesTemp, map_tex, TRUE);

						for (auto* node_tex : m_scratch.nrmTextures)
						{
							RenderBackend.set_Textures(node_tex->key);
							RenderImplementation.apply_lmaterial();

							mapNormalItems& items = node_tex->val;
							items.ScreenSpaceArea = 0;

							RenderStaticBatch(items);

							if (_clear)
								items.clear();
						}

						m_scratch.nrmTextures.clear();
						m_scratch.nrmTexturesTemp.clear();
						if (_clear)
							map_tex.clear();
					}
					m_scratch.nrmStates.clear();
					if (_clear)
						map_states.clear();
				}
				m_scratch.nrmCS.clear();
				if (_clear)
					map_cs.clear();
			}
			m_scratch.nrmPS.clear();
			if (_clear)
				map_ps.clear();
		}
		m_scratch.nrmVS.clear();
		if (_clear)
			map_vs.clear();
	}

	// -------------------------------------------------------------------------
	// PHASE 2: DYNAMIC GEOMETRY (NPCs, Physics)
	// -------------------------------------------------------------------------
	{
		// Используем m_packet.queue_dynamic
		mapMatrixVS& map_vs = m_packet.queue_dynamic[_priority];
		// Используем m_scratch.matVS
		map_vs.getANY_P(m_scratch.matVS);

		for (auto* node_vs : m_scratch.matVS)
		{
			RenderBackend.set_Vertex_Shader(node_vs->key);

			mapMatrixPS& map_ps = node_vs->val;
			map_ps.ScreenSpaceArea = 0;
			// Используем m_scratch.matPS
			map_ps.getANY_P(m_scratch.matPS);

			for (auto* node_ps : m_scratch.matPS)
			{
				RenderBackend.set_Pixel_Shader(node_ps->key);

				mapMatrixCS& map_cs = node_ps->val;
				map_cs.ScreenSpaceArea = 0;
				// Используем m_scratch.matCS
				map_cs.getANY_P(m_scratch.matCS);

				for (auto* node_cs : m_scratch.matCS)
				{
					RenderBackend.set_Constants(node_cs->key);

					mapMatrixStates& map_states = node_cs->val;
					map_states.ScreenSpaceArea = 0;
					// Используем m_scratch.matStates
					map_states.getANY_P(m_scratch.matStates);

					for (auto* node_state : m_scratch.matStates)
					{
						RenderBackend.set_States(node_state->key);

						mapMatrixTextures& map_tex = node_state->val;
						map_tex.ScreenSpaceArea = 0;

						// Используем m_scratch.matTextures и m_scratch.matTexturesTemp
						SortTextureList(m_scratch.matTextures, m_scratch.matTexturesTemp, map_tex, TRUE);

						for (auto* node_tex : m_scratch.matTextures)
						{
							RenderBackend.set_Textures(node_tex->key);
							RenderImplementation.apply_lmaterial();

							mapMatrixItems& items = node_tex->val;
							items.ScreenSpaceArea = 0;

							RenderDynamicBatch(items);
						}

						m_scratch.matTextures.clear();
						m_scratch.matTexturesTemp.clear();
						if (_clear)
							map_tex.clear();
					}
					m_scratch.matStates.clear();
					if (_clear)
						map_states.clear();
				}
				m_scratch.matCS.clear();
				if (_clear)
					map_cs.clear();
			}
			m_scratch.matPS.clear();
			if (_clear)
				map_ps.clear();
		}
		m_scratch.matVS.clear();
		if (_clear)
			map_vs.clear();
	}

	Engine.Statistic->RenderDUMP.End();
}

void CSceneGraph::_RenderHUD()
{
	OPTICK_EVENT("RenderHUD");
	ENGINE_API extern float psHUD_FOV;

	// Backup Projection
	Fmatrix ProjectOld = Engine.RenderView.Project;
	Fmatrix ViewProjectOld = Engine.RenderView.ViewProjection;

	// Create Custom HUD Projection
	Engine.RenderView.Project.build_projection(deg2rad(psHUD_FOV * Engine.RenderView.Fov), Engine.RenderView.Aspect,
											   VIEWPORT_NEAR_HUD,
											   g_pGamePersistent->Environment().CurrentEnv->far_plane);

	Engine.RenderView.ViewProjection.mul(Engine.RenderView.Project, Engine.RenderView.View);
	RenderBackend.set_transform_project(Engine.RenderView.Project);

	// Render
	RenderImplementation.set_render_mode(CRender::MODE_NEAR);
	// Используем m_packet.queue_hud
	m_packet.queue_hud.traverseLR(RenderSortedNode);
	m_packet.queue_hud.clear();
	RenderImplementation.set_render_mode(CRender::MODE_NORMAL);

	// Restore Projection
	Engine.RenderView.Project = ProjectOld;
	Engine.RenderView.ViewProjection = ViewProjectOld;
	RenderBackend.set_transform_project(Engine.RenderView.Project);
}

void CSceneGraph::_RenderTranslucent()
{
	OPTICK_EVENT("RenderTranslucent");
	// Используем m_packet.queue_transparent
	m_packet.queue_transparent.traverseRL(RenderSortedNode);
	m_packet.queue_transparent.clear();
}

void CSceneGraph::_RenderEmissive()
{
	OPTICK_EVENT("RenderEmissive");
	// Используем m_packet.mapEmissive
	m_packet.mapEmissive.traverseLR(RenderSortedNode);
	m_packet.mapEmissive.clear();
}

void CSceneGraph::_RenderWmarks()
{
	OPTICK_EVENT("RenderWmarks");
	// Используем m_packet.queue_wallmarks
	m_packet.queue_wallmarks.traverseLR(RenderSortedNode);
	m_packet.queue_wallmarks.clear();
}

void CSceneGraph::_RenderDistortion()
{
	OPTICK_EVENT("RenderDistortion");
	// Используем m_packet.queue_distortion
	m_packet.queue_distortion.traverseRL(RenderSortedNode);
	m_packet.queue_distortion.clear();
}

void CSceneGraph::_RenderLODs(bool _setup_zb, bool _clear)
{
	OPTICK_EVENT("RenderLODs");

	// Сбор LOD-ов в плоский список
	if (_setup_zb)
		// Используем m_packet.mapLOD и m_packet.lstLODs
		m_packet.mapLOD.getLR(m_packet.lstLODs); // front-to-back (для Z-buffer)
	else
		m_packet.mapLOD.getRL(m_packet.lstLODs); // back-to-front (для цвета)

	// Используем m_packet.lstLODs
	if (m_packet.lstLODs.empty())
		return;

	// *** 1. Подготовка буфера и констант ***
	u32 shader_id = _setup_zb ? SE_R1_LMODELS : SE_R1_NORMAL_LQ;
	// Используем m_packet.lstLODs
	FLOD* first_visual = (FLOD*)m_packet.lstLODs[0].pVisual;

	u32 vb_offset;
	// Блокируем память один раз для всех LODов (по 4 вершины на LOD)
	// Используем m_packet.lstLODs.size()
	FLOD::_hw* VertexBuffer =
		(FLOD::_hw*)RenderBackend.Vertex.Lock(m_packet.lstLODs.size() * 4, first_visual->geom->vb_stride, vb_offset);

	float ssa_range = r_ssaLOD_A - r_ssaLOD_B;
	if (ssa_range < EPS_S)
		ssa_range = EPS_S;

	// Захват переменных для PPL
	const float ssa_limit_b = r_ssaLOD_B;
	const Fvector camera_pos = Engine.RenderView.Position;

	// *** 2. ПАРАЛЛЕЛЬНЫЙ ПРОХОД: Генерация геометрии ***
	// Вычисляем поворот билбордов и смешивание текстур в параллель
	// Используем m_packet.lstLODs
	concurrency::parallel_for(size_t(0), m_packet.lstLODs.size(), [&](size_t i) {
		FLOD::_hw* V = VertexBuffer + (i * 4);
		SceneGraphTypes::LodRenderNode& Node = m_packet.lstLODs[i];
		FLOD* lod_visual = (FLOD*)Node.pVisual;

		// 1. Вычисление Alpha (Fade In/Out)
		float ssa_diff = Node.ScreenSpaceArea - ssa_limit_b;
		float scale = ssa_diff / ssa_range;
		int alpha_int = iFloor((1.0f - scale) * 255.f);
		u32 alpha_final = u32(clampr(alpha_int, 0, 255));

		// 2. Вычисление направления на камеру
		Fvector dir_to_camera, shift;
		dir_to_camera.sub(lod_visual->vis.sphere.P, camera_pos).normalize();
		shift.mul(dir_to_camera, -.5f * lod_visual->vis.sphere.R);

		// 3. Выбор лучших плоскостей (Facet Selection)
		FLOD::_face* facets = lod_visual->facets;

		// Локальный вектор для сортировки (безопасно для потоков)
		svector<std::pair<float, u32>, 8> plane_selector;
		for (u32 s = 0; s < 8; s++)
			plane_selector.push_back(mk_pair(dir_to_camera.dotproduct(facets[s].N), s));

		std::sort(plane_selector.begin(), plane_selector.end(), SortLodsByDotProduct);

		float dot_best = plane_selector[plane_selector.size() - 1].first;
		float dot_next = plane_selector[plane_selector.size() - 2].first;
		float dot_next_2 = plane_selector[plane_selector.size() - 3].first;

		u32 id_best = plane_selector[plane_selector.size() - 1].second;
		u32 id_next = plane_selector[plane_selector.size() - 2].second;

		// 4. Интерполяция между двумя плоскостями
		float dot_a = dot_best, dot_b = dot_next, dot_c = dot_next_2;
		float alpha_factor = 0.5f + 0.5f * (1 - (dot_b - dot_c) / (dot_a - dot_c));
		int factor_int = iFloor(alpha_factor * 255.5f);
		u32 factor_final = u32(clampr(factor_int, 0, 255));

		// 5. Заполнение вершинного буфера
		FLOD::_face& FaceA = facets[id_best];
		FLOD::_face& FaceB = facets[id_next];

		static const int vertex_indices[4] = {3, 0, 2, 1};

		for (u32 v_idx = 0; v_idx < 4; v_idx++)
		{
			int id = vertex_indices[v_idx];
			// Пишем прямо в память по вычисленному смещению
			V[v_idx].p0.add(FaceB.v[id].v, shift);
			V[v_idx].p1.add(FaceA.v[id].v, shift);
			V[v_idx].n0 = FaceB.N;
			V[v_idx].n1 = FaceA.N;
			V[v_idx].sun_af = color_rgba(FaceB.v[id].c_sun, FaceA.v[id].c_sun, alpha_final, factor_final);
			V[v_idx].t0 = FaceB.v[id].t;
			V[v_idx].t1 = FaceA.v[id].t;
			V[v_idx].rgbh0 = FaceB.v[id].c_rgb_hemi;
			V[v_idx].rgbh1 = FaceA.v[id].c_rgb_hemi;
		}
	});

	// Используем m_packet.lstLODs.size()
	RenderBackend.Vertex.Unlock(m_packet.lstLODs.size() * 4, first_visual->geom->vb_stride);

	// *** 3. ПОСЛЕДОВАТЕЛЬНЫЙ ПРОХОД: Группировка по шейдерам ***
	// Чтобы минимизировать смену стейтов при отрисовке батча
	// Используем m_packet.lstLODs
	if (!m_packet.lstLODs.empty())
	{
		ref_selement current_shader = m_packet.lstLODs[0].pVisual->shader->E[shader_id];
		int current_count = 0;

		for (u32 i = 0; i < m_packet.lstLODs.size(); i++)
		{
			SceneGraphTypes::LodRenderNode& Node = m_packet.lstLODs[i];
			if (Node.pVisual->shader->E[shader_id] == current_shader)
			{
				current_count++;
			}
			else
			{
				// Используем m_packet.lstLODgroups
				m_packet.lstLODgroups.push_back(current_count);
				current_shader = Node.pVisual->shader->E[shader_id];
				current_count = 1;
			}
		}
		// Используем m_packet.lstLODgroups
		m_packet.lstLODgroups.push_back(current_count);
	}

	// *** 4. RENDER ***
	int current_lod_index = 0;
	RenderBackend.set_transform_world(Fidentity);

	// Используем m_packet.lstLODgroups
	for (u32 g = 0; g < m_packet.lstLODgroups.size(); g++)
	{
		int primitive_count = m_packet.lstLODgroups[g];

		if (primitive_count > 0)
		{
			// Используем m_packet.lstLODs
			RenderBackend.set_Element(m_packet.lstLODs[current_lod_index].pVisual->shader->E[shader_id]);
			RenderBackend.set_Geometry(first_visual->geom);

			// Отрисовка батча (2 треугольника на 1 LOD)
			RenderBackend.Render(D3DPT_TRIANGLELIST, vb_offset, 0, 4 * primitive_count, 0, 2 * primitive_count);

			RenderBackend.stat.r.s_flora_lods.add(4 * primitive_count);

			current_lod_index += primitive_count;
			vb_offset += 4 * primitive_count;
		}
	}

	// *** 5. Cleanup ***
	// Очищаем списки из m_packet
	m_packet.lstLODs.clear();
	m_packet.lstLODgroups.clear();

	if (_clear)
		// Очищаем mapLOD из m_packet
		m_packet.mapLOD.clear();
}

//////////////////////////////////////////////////////////////////////////
// Subspace Rendering (Traversal)
// Обход секторов и порталов
//////////////////////////////////////////////////////////////////////////

// Shortcut: Create frustum from matrix
void CSceneGraph::render_subspace(IRender_Sector* _sector, Fmatrix& mCombined, Fvector& _cop, BOOL _dynamic,
								  BOOL _precise_portals)
{
	OPTICK_EVENT("render_subspace - shortcut");

	CFrustum temp_frustum;
	temp_frustum.CreateFromMatrix(mCombined, FRUSTUM_P_ALL);
	render_subspace(_sector, &temp_frustum, mCombined, _cop, _dynamic, _precise_portals);
}

// ===============================================================================================
//  CSceneGraph Implementation (Updated Methods)
// ===============================================================================================

// Main procedure
void CSceneGraph::render_subspace(IRender_Sector* start_sector, CFrustum* view_frustum, Fmatrix& mCombined,
								  Fvector& camera_pos, BOOL render_dynamic, BOOL precise_portals)
{
	OPTICK_EVENT("render_subspace - main");

	VERIFY(start_sector);
	m_traversal_marker++; // Important: New traversal ID

	// Save context
	CFrustum ViewSave = RenderImplementation.ViewBase;
	RenderImplementation.ViewBase = *view_frustum;
	RenderImplementation.View = &RenderImplementation.ViewBase;

	// Portal Precision Check (Dual Render Force)
	if (precise_portals && RenderImplementation.rmPortals)
	{
		Fvector box_radius;
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

	// Traverse sector/portal structure
	PortalTraverser.traverse(start_sector, RenderImplementation.ViewBase, camera_pos, mCombined, 0);

	// 1. Collect STATIC Geometry (Hierarchical)
	for (u32 s_it = 0; s_it < PortalTraverser.r_sectors.size(); s_it++)
	{
		CSector* sector = (CSector*)PortalTraverser.r_sectors[s_it];
		IRender_Visual* root_visual = sector->root();

		for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
		{
			RenderImplementation.set_Frustum(&(sector->r_frustums[v_it]));
			RenderImplementation.add_Geometry(root_visual);
		}
	}

	// 2. Collect DYNAMIC Geometry (Spatial DB)
	if (render_dynamic)
	{
		RenderImplementation.set_Object(0); // Это обнуляет m_ctx.current_owner через вызов set_Object

		// Traverse object database
		// Используем m_packet.lstRenderables вместо локального вектора
		g_SpatialSpace->q_frustum(m_packet.lstRenderables, ISpatial_DB::O_ORDERED, STYPE_RENDERABLE,
								  RenderImplementation.ViewBase);

		// Determine visibility for dynamic part of scene
		// Итерируемся по m_packet.lstRenderables
		for (u32 o_it = 0; o_it < m_packet.lstRenderables.size(); o_it++)
		{
			ISpatial* spatial = m_packet.lstRenderables[o_it];
			CSector* sector = (CSector*)spatial->spatial.sector;

			if (0 == sector)
				continue; // Object is lost (no sector)
			if (PortalTraverser.i_marker != sector->r_marker)
				continue; // Object is in invisible sector

			for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
			{
				RenderImplementation.set_Frustum(&(sector->r_frustums[v_it]));

				if (!RenderImplementation.View->testSphere_dirty(spatial->spatial.sphere.P, spatial->spatial.sphere.R))
					continue;

				// Is it renderable?
				IRenderable* renderable = spatial->dcast_Renderable();
				if (0 == renderable)
					continue;

				// Это вызовет CSceneGraph::add_Dynamic (или ProcessDynamicVisual),
				// который уже обновлен для использования m_packet и m_ctx
				renderable->renderable_Render();
			}
		}
	}

	// Actor Shadow (Specific hack for shadow pass)
	if (g_pGameLevel && (RenderImplementation.active_phase() == RenderImplementation.PHASE_SHADOW_DEPTH))
		g_pGameLevel->pHUD->Render_Actor_Shadow();

	// Restore context
	RenderImplementation.ViewBase = ViewSave;
	RenderImplementation.View = 0;
}

// Frame Reuse Optimization
void CSceneGraph::render_reuse()
{
	PROFILE_FUNCTION();

	// Статика (Reuse List)
	// Используем список из m_packet
	for (IRender_Visual* V : m_packet.m_visuals_static_visible)
	{
		// Используем ProcessStaticVisual, чтобы корректно обработать LOD-ы
		// если они были сохранены в список
		ProcessStaticVisual(V);
	}

	// Динамика (Reuse List)
	// Используем список из m_packet
	for (auto& it : m_packet.m_visuals_dynamic_visible)
	{
		// Вызываем локальный метод set_Transform, чтобы обновить m_ctx.current_transform
		set_Transform(&it.matrix);
		ProcessDynamicVisual(it.visual);
	}
}
