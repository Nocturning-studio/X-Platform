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

void CSceneGraph::Render(SceneGraphPacket& packet, SceneGraphRenderType type, u32 priority, bool clear, bool setup_zb)
{
	switch (type)
	{
	case SceneGraphRenderType::Opaque:
		_RenderOpaque(packet, priority, clear);
		break;
	case SceneGraphRenderType::Transparent:
		_RenderTranslucent(packet);
		break;
	case SceneGraphRenderType::HUD:
		_RenderHUD(packet);
		break;
	case SceneGraphRenderType::LOD:
		_RenderLODs(packet, setup_zb, clear);
		break;
	case SceneGraphRenderType::Emissive:
		_RenderEmissive(packet);
		break;
	case SceneGraphRenderType::Wallmarks:
		_RenderWmarks(packet);
		break;
	case SceneGraphRenderType::Distortion:
		_RenderDistortion(packet);
		break;
	}
}

// ===============================================================================================
//  CSceneGraph Rendering Implementation (Stateless Update)
// ===============================================================================================

// Добавлен аргумент packet
void CSceneGraph::_RenderOpaque(SceneGraphPacket& packet, u32 _priority, bool _clear)
{
	OPTICK_EVENT("RenderOpaque");
	Engine.Statistic->RenderDUMP.Begin();

	// -------------------------------------------------------------------------
	// PHASE 1: STATIC GEOMETRY (Level)
	// -------------------------------------------------------------------------
	{
		RenderBackend.set_transform_world(Fidentity);

		// Используем packet.queue_static
		mapNormalVS& map_vs = packet.queue_static[_priority];

		// m_scratch все еще берем из this->m_scratch, так как это рабочий буфер рендеринга.
		// В будущем scratch тоже можно будет передавать аргументом, если рендеринг будет параллельным.
		map_vs.getANY_P(m_scratch.nrmVS);

		for (auto* node_vs : m_scratch.nrmVS)
		{
			RenderBackend.set_Vertex_Shader(node_vs->key);

			mapNormalPS& map_ps = node_vs->val;
			map_ps.ScreenSpaceArea = 0;
			map_ps.getANY_P(m_scratch.nrmPS);

			for (auto* node_ps : m_scratch.nrmPS)
			{
				RenderBackend.set_Pixel_Shader(node_ps->key);

				mapNormalCS& map_cs = node_ps->val;
				map_cs.ScreenSpaceArea = 0;
				map_cs.getANY_P(m_scratch.nrmCS);

				for (auto* node_cs : m_scratch.nrmCS)
				{
					RenderBackend.set_Constants(node_cs->key);

					mapNormalStates& map_states = node_cs->val;
					map_states.ScreenSpaceArea = 0;
					map_states.getANY_P(m_scratch.nrmStates);

					for (auto* node_state : m_scratch.nrmStates)
					{
						RenderBackend.set_States(node_state->key);

						mapNormalTextures& map_tex = node_state->val;
						map_tex.ScreenSpaceArea = 0;

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
		// Используем packet.queue_dynamic
		mapMatrixVS& map_vs = packet.queue_dynamic[_priority];
		map_vs.getANY_P(m_scratch.matVS);

		for (auto* node_vs : m_scratch.matVS)
		{
			RenderBackend.set_Vertex_Shader(node_vs->key);

			mapMatrixPS& map_ps = node_vs->val;
			map_ps.ScreenSpaceArea = 0;
			map_ps.getANY_P(m_scratch.matPS);

			for (auto* node_ps : m_scratch.matPS)
			{
				RenderBackend.set_Pixel_Shader(node_ps->key);

				mapMatrixCS& map_cs = node_ps->val;
				map_cs.ScreenSpaceArea = 0;
				map_cs.getANY_P(m_scratch.matCS);

				for (auto* node_cs : m_scratch.matCS)
				{
					RenderBackend.set_Constants(node_cs->key);

					mapMatrixStates& map_states = node_cs->val;
					map_states.ScreenSpaceArea = 0;
					map_states.getANY_P(m_scratch.matStates);

					for (auto* node_state : m_scratch.matStates)
					{
						RenderBackend.set_States(node_state->key);

						mapMatrixTextures& map_tex = node_state->val;
						map_tex.ScreenSpaceArea = 0;

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

// Добавлен аргумент packet
void CSceneGraph::_RenderHUD(SceneGraphPacket& packet)
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
	// Используем packet.queue_hud
	packet.queue_hud.traverseLR(RenderSortedNode);
	packet.queue_hud.clear();
	RenderImplementation.set_render_mode(CRender::MODE_NORMAL);

	// Restore Projection
	Engine.RenderView.Project = ProjectOld;
	Engine.RenderView.ViewProjection = ViewProjectOld;
	RenderBackend.set_transform_project(Engine.RenderView.Project);
}

// Добавлен аргумент packet
void CSceneGraph::_RenderTranslucent(SceneGraphPacket& packet)
{
	OPTICK_EVENT("RenderTranslucent");
	// Используем packet.queue_transparent
	packet.queue_transparent.traverseRL(RenderSortedNode);
	packet.queue_transparent.clear();
}

// Добавлен аргумент packet
void CSceneGraph::_RenderEmissive(SceneGraphPacket& packet)
{
	OPTICK_EVENT("RenderEmissive");
	// Используем packet.mapEmissive
	packet.mapEmissive.traverseLR(RenderSortedNode);
	packet.mapEmissive.clear();
}

// Добавлен аргумент packet
void CSceneGraph::_RenderWmarks(SceneGraphPacket& packet)
{
	OPTICK_EVENT("RenderWmarks");
	// Используем packet.queue_wallmarks
	packet.queue_wallmarks.traverseLR(RenderSortedNode);
	packet.queue_wallmarks.clear();
}

// Добавлен аргумент packet
void CSceneGraph::_RenderDistortion(SceneGraphPacket& packet)
{
	OPTICK_EVENT("RenderDistortion");
	// Используем packet.queue_distortion
	packet.queue_distortion.traverseRL(RenderSortedNode);
	packet.queue_distortion.clear();
}

// Добавлен аргумент packet
void CSceneGraph::_RenderLODs(SceneGraphPacket& packet, bool _setup_zb, bool _clear)
{
	OPTICK_EVENT("RenderLODs");

	// Сбор LOD-ов в плоский список
	if (_setup_zb)
		// Используем packet.mapLOD и packet.lstLODs
		packet.mapLOD.getLR(packet.lstLODs); // front-to-back (для Z-buffer)
	else
		packet.mapLOD.getRL(packet.lstLODs); // back-to-front (для цвета)

	// Используем packet.lstLODs
	if (packet.lstLODs.empty())
		return;

	// *** 1. Подготовка буфера и констант ***
	u32 shader_id = _setup_zb ? SE_R1_LMODELS : SE_R1_NORMAL_LQ;
	// Используем packet.lstLODs
	FLOD* first_visual = (FLOD*)packet.lstLODs[0].pVisual;

	u32 vb_offset;
	// Используем packet.lstLODs.size()
	FLOD::_hw* VertexBuffer =
		(FLOD::_hw*)RenderBackend.Vertex.Lock(packet.lstLODs.size() * 4, first_visual->geom->vb_stride, vb_offset);

	float ssa_range = r_ssaLOD_A - r_ssaLOD_B;
	if (ssa_range < EPS_S)
		ssa_range = EPS_S;

	// Захват переменных для PPL
	const float ssa_limit_b = r_ssaLOD_B;
	const Fvector camera_pos = Engine.RenderView.Position;

	// *** 2. ПАРАЛЛЕЛЬНЫЙ ПРОХОД: Генерация геометрии ***
	// Используем packet.lstLODs
	concurrency::parallel_for(size_t(0), packet.lstLODs.size(), [&](size_t i) {
		FLOD::_hw* V = VertexBuffer + (i * 4);
		SceneGraphTypes::LodRenderNode& Node = packet.lstLODs[i];
		FLOD* lod_visual = (FLOD*)Node.pVisual;

		// 1. Вычисление Alpha
		float ssa_diff = Node.ScreenSpaceArea - ssa_limit_b;
		float scale = ssa_diff / ssa_range;
		int alpha_int = iFloor((1.0f - scale) * 255.f);
		u32 alpha_final = u32(clampr(alpha_int, 0, 255));

		// 2. Вычисление направления
		Fvector dir_to_camera, shift;
		dir_to_camera.sub(lod_visual->vis.sphere.P, camera_pos).normalize();
		shift.mul(dir_to_camera, -.5f * lod_visual->vis.sphere.R);

		// 3. Выбор лучших плоскостей
		FLOD::_face* facets = lod_visual->facets;

		svector<std::pair<float, u32>, 8> plane_selector;
		for (u32 s = 0; s < 8; s++)
			plane_selector.push_back(mk_pair(dir_to_camera.dotproduct(facets[s].N), s));

		std::sort(plane_selector.begin(), plane_selector.end(), SortLodsByDotProduct);

		float dot_best = plane_selector[plane_selector.size() - 1].first;
		float dot_next = plane_selector[plane_selector.size() - 2].first;
		float dot_next_2 = plane_selector[plane_selector.size() - 3].first;

		u32 id_best = plane_selector[plane_selector.size() - 1].second;
		u32 id_next = plane_selector[plane_selector.size() - 2].second;

		// 4. Интерполяция
		float dot_a = dot_best, dot_b = dot_next, dot_c = dot_next_2;
		float alpha_factor = 0.5f + 0.5f * (1 - (dot_b - dot_c) / (dot_a - dot_c));
		int factor_int = iFloor(alpha_factor * 255.5f);
		u32 factor_final = u32(clampr(factor_int, 0, 255));

		// 5. Заполнение буфера
		FLOD::_face& FaceA = facets[id_best];
		FLOD::_face& FaceB = facets[id_next];

		static const int vertex_indices[4] = {3, 0, 2, 1};

		for (u32 v_idx = 0; v_idx < 4; v_idx++)
		{
			int id = vertex_indices[v_idx];
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

	RenderBackend.Vertex.Unlock(packet.lstLODs.size() * 4, first_visual->geom->vb_stride);

	// *** 3. ПОСЛЕДОВАТЕЛЬНЫЙ ПРОХОД: Группировка по шейдерам ***
	// Используем packet.lstLODs
	if (!packet.lstLODs.empty())
	{
		ref_selement current_shader = packet.lstLODs[0].pVisual->shader->E[shader_id];
		int current_count = 0;

		for (u32 i = 0; i < packet.lstLODs.size(); i++)
		{
			SceneGraphTypes::LodRenderNode& Node = packet.lstLODs[i];
			if (Node.pVisual->shader->E[shader_id] == current_shader)
			{
				current_count++;
			}
			else
			{
				// Используем packet.lstLODgroups
				packet.lstLODgroups.push_back(current_count);
				current_shader = Node.pVisual->shader->E[shader_id];
				current_count = 1;
			}
		}
		packet.lstLODgroups.push_back(current_count);
	}

	// *** 4. RENDER ***
	int current_lod_index = 0;
	RenderBackend.set_transform_world(Fidentity);

	// Используем packet.lstLODgroups
	for (u32 g = 0; g < packet.lstLODgroups.size(); g++)
	{
		int primitive_count = packet.lstLODgroups[g];

		if (primitive_count > 0)
		{
			// Используем packet.lstLODs
			RenderBackend.set_Element(packet.lstLODs[current_lod_index].pVisual->shader->E[shader_id]);
			RenderBackend.set_Geometry(first_visual->geom);

			// Отрисовка батча (2 треугольника на 1 LOD)
			RenderBackend.Render(D3DPT_TRIANGLELIST, vb_offset, 0, 4 * primitive_count, 0, 2 * primitive_count);

			RenderBackend.stat.r.s_flora_lods.add(4 * primitive_count);

			current_lod_index += primitive_count;
			vb_offset += 4 * primitive_count;
		}
	}

	// *** 5. Cleanup ***
	// Очищаем packet
	packet.lstLODs.clear();
	packet.lstLODgroups.clear();

	if (_clear)
		packet.mapLOD.clear();
}


// ===============================================================================================
//  CSceneGraph::render_subspace
//  Назначение: Обход пространства (секторов и порталов) и сбор геометрии в указанный пакет.
// ===============================================================================================

// 1. Shortcut (создание фрустума из матрицы)
void CSceneGraph::render_subspace(IRender_Sector* _sector, Fmatrix& mCombined, Fvector& _cop, BOOL _dynamic,
								  BOOL _precise_portals, SceneGraphPacket& dest)
{
	OPTICK_EVENT("render_subspace - shortcut");

	CFrustum temp_frustum;
	temp_frustum.CreateFromMatrix(mCombined, FRUSTUM_P_ALL);
	render_subspace(_sector, &temp_frustum, mCombined, _cop, _dynamic, _precise_portals, dest);
}

// 2. Main Implementation (Основная логика)
void CSceneGraph::render_subspace(IRender_Sector* start_sector, CFrustum* view_frustum, Fmatrix& mCombined,
								  Fvector& camera_pos, BOOL render_dynamic, BOOL precise_portals,
								  SceneGraphPacket& dest)
{
	OPTICK_EVENT("render_subspace - main");

	VERIFY(start_sector);
	VERIFY(view_frustum);

	m_traversal_marker++; // Увеличиваем маркер, чтобы не обрабатывать один объект дважды за этот проход

	// -------------------------------------------------------------------------
	// Подготовка локального контекста
	// -------------------------------------------------------------------------
	SceneTraversalContext local_ctx;
	local_ctx.frustum = view_frustum; // Устанавливаем фрустум для этого прохода
	local_ctx.is_hud_pass = FALSE;	  // Для теней HUD обычно false
	local_ctx.is_invisible_mode = FALSE;
	local_ctx.current_owner = nullptr; // Сброс владельца
	local_ctx.current_transform = nullptr;

	// -------------------------------------------------------------------------
	// Precise Portals (Dual Render Force)
	// -------------------------------------------------------------------------
	// Если камера слишком близко к порталу, принудительно включаем рендеринг обоих секторов
	if (precise_portals && RenderImplementation.rmPortals)
	{
		Fvector box_radius;
		box_radius.set(EPS_L * 20, EPS_L * 20, EPS_L * 20);
		RenderImplementation.Sectors_xrc.box_options(CDB::OPT_FULL_TEST);
		RenderImplementation.Sectors_xrc.box_query(RenderImplementation.rmPortals, camera_pos, box_radius);

		for (int K = 0; K < RenderImplementation.Sectors_xrc.r_count(); K++)
		{
			u32 portal_id = RenderImplementation.rmPortals->get_tris()[RenderImplementation.Sectors_xrc.r_begin()[K].id].dummy;
			CPortal* pPortal = (CPortal*)RenderImplementation.Portals[portal_id];
			pPortal->bDualRender = TRUE;
		}
	}

	// -------------------------------------------------------------------------
	// Обход порталов (Portal Traversal)
	// -------------------------------------------------------------------------
	// Заполняет список видимых секторов (r_sectors) и фрустумов
	PortalTraverser.traverse(start_sector, *view_frustum, camera_pos, mCombined, 0);

	// -------------------------------------------------------------------------
	// Сбор СТАТИКИ (Static Geometry)
	// -------------------------------------------------------------------------
	for (u32 s_it = 0; s_it < PortalTraverser.r_sectors.size(); s_it++)
	{
		CSector* sector = (CSector*)PortalTraverser.r_sectors[s_it];
		IRender_Visual* root_visual = sector->root();

		for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
		{
			// Берем фрустум портала
			CFrustum& portal_frustum = sector->r_frustums[v_it];

			// Обновляем фрустум в контексте на более узкий (портальный)
			local_ctx.frustum = &portal_frustum;

			// Вызываем add_Static с ЛОКАЛЬНЫМ контекстом
			add_Static(root_visual, portal_frustum.getMask(), local_ctx, dest);
		}
	}

	local_ctx.frustum = view_frustum;

	// -------------------------------------------------------------------------
	// Сбор ДИНАМИКИ (Dynamic Geometry)
	// -------------------------------------------------------------------------
	if (render_dynamic)
	{
		// Запрос выполняется используя переданный view_frustum, а не глобальный
		g_SpatialSpace->q_frustum(dest.lstRenderables, ISpatial_DB::O_ORDERED, STYPE_RENDERABLE, *view_frustum);

		for (u32 o_it = 0; o_it < dest.lstRenderables.size(); o_it++)
		{
			ISpatial* spatial = dest.lstRenderables[o_it];
			CSector* sector = (CSector*)spatial->spatial.sector;

			if (0 == sector)
				continue;
			if (PortalTraverser.i_marker != sector->r_marker)
				continue;

			for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
			{
				CFrustum& portal_frustum = sector->r_frustums[v_it];

				// Используем локальный фрустум портала
				if (!portal_frustum.testSphere_dirty(spatial->spatial.sphere.P, spatial->spatial.sphere.R))
					continue;

				IRenderable* renderable = spatial->dcast_Renderable();
				if (0 == renderable)
					continue;

				// Настраиваем контекст
				local_ctx.frustum = &portal_frustum;
				local_ctx.current_owner = renderable;

				renderable->renderable_Render();
			}
		}
	}

	// -------------------------------------------------------------------------
	// F. Тень от актера (Actor Shadow Hack)
	// -------------------------------------------------------------------------
	if (g_pGameLevel && (RenderImplementation.active_phase() == RenderImplementation.PHASE_SHADOW_DEPTH))
	{
		// Этот метод внутри тоже вызывает add_Visual, который пойдет в пакет, настроенный в CRender
		g_pGameLevel->pHUD->Render_Actor_Shadow();
	}
}

// Frame Reuse Optimization
void CSceneGraph::render_reuse(const SceneTraversalContext& initial_ctx, SceneGraphPacket& packet)
{
	PROFILE_FUNCTION();

	// Статика (из packet)
	for (IRender_Visual* V : packet.m_visuals_static_visible)
	{
		ProcessStaticVisual(V, initial_ctx, packet);
	}

	// Динамика (из packet)
	SceneTraversalContext local_ctx = initial_ctx;
	for (auto& it : packet.m_visuals_dynamic_visible)
	{
		local_ctx.current_transform = &it.matrix;
		ProcessDynamicVisual(it.visual, local_ctx, packet);
	}
}
