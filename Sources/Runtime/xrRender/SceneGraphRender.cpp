#include "stdafx.h"
#include "SceneGraph.h"
#include "flod.h"
#include "render.h"

#include <ppl.h>

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
	return std::sqrt(clampr((screen_space_area - r_ssaGLOD_end) / (r_ssaGLOD_start - r_ssaGLOD_end), 0.f, 1.f));
}

// --- LOD Sorting Predicate ---
static bool SortLodsByDotProduct(const std::pair<float, u32>& a, const std::pair<float, u32>& b)
{
	return a.first < b.first;
}

// --- Render Helper: Static Batches ---
static void RenderStaticBatch(SceneGraphTypes::mapNormalItems& batch)
{
	PROFILE_FUNCTION();

	// Сортировка Front-to-Back по SSA для Early Z-Cull
	std::sort(batch.begin(), batch.end(),
			  [](const SceneGraphTypes::StaticRenderNode& a, const SceneGraphTypes::StaticRenderNode& b) 
			  {
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
	PROFILE_FUNCTION();

	// Сортировка Front-to-Back
	std::sort(batch.begin(), batch.end(),
			  [](const SceneGraphTypes::DynamicRenderNode& a, const SceneGraphTypes::DynamicRenderNode& b) 
			  {
				  return a.ScreenSpaceArea > b.ScreenSpaceArea;
			  });

	for (const auto& node : batch)
	{
		RenderBackend.set_transform_world(*node.pMatrix);
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
	RenderBackend.set_transform_world(*node->val.pMatrix);
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
	if (textures_map.size() == 0)
		return;

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
	PROFILE_FUNCTION();

#ifdef DEBUG
	DebugCheckDuplicateVisuals(packet);
#endif

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
//  CSceneGraph Rendering Implementation
// ===============================================================================================

void CSceneGraph::_RenderOpaque(SceneGraphPacket& packet, u32 _priority, bool _clear)
{
	PROFILE_FUNCTION();
	Engine.Statistic->RenderDUMP.Begin();

	// -------------------------------------------------------------------------
	// PHASE 1: STATIC GEOMETRY (Level)
	// -------------------------------------------------------------------------
	{
		RenderBackend.set_transform_world(Fidentity);

		mapNormalVS& map_vs = packet.queue_static[_priority];

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

void CSceneGraph::RenderFromCache(const SceneTraversalContext& initial_ctx, SceneGraphPacket& packet)
{
	PROFILE_FUNCTION();

	SceneTraversalContext local_ctx = initial_ctx;
	local_ctx.traversal_marker_id = ++m_traversal_marker;

	auto static_visuals = packet.m_visuals_static_visible;
	for (IRender_Visual* V : static_visuals)
	{
		ProcessStaticVisual(V, local_ctx, packet);
	}

	auto dynamic_visuals = packet.m_visuals_dynamic_visible;
	for (auto& it : dynamic_visuals)
	{
		local_ctx.transform = &it.matrix;
		ProcessDynamicVisual(it.visual, local_ctx, packet);
	}
}

void CSceneGraph::_RenderHUD(SceneGraphPacket& packet)
{
	PROFILE_FUNCTION();
	ENGINE_API extern float psHUD_FOV;

	// Backup Projection
	fmat4x4 ProjectOld = Engine.RenderView.Project;
	fmat4x4 ViewProjectOld = Engine.RenderView.ViewProjection;

	// Create Custom HUD Projection
	Engine.RenderView.Project.build_projection(deg2rad(psHUD_FOV * Engine.RenderView.Fov), 
											   Engine.RenderView.Aspect,
											   VIEWPORT_NEAR_HUD,
											   g_pGamePersistent->Environment().CurrentEnv->far_plane);

	Engine.RenderView.ViewProjection.mul(Engine.RenderView.Project, Engine.RenderView.View);
	RenderBackend.set_transform_project(Engine.RenderView.Project);

	// Render
	RenderImplementation.set_render_mode(CRender::MODE_NEAR);
	packet.queue_hud.traverseLR(RenderSortedNode);
	packet.queue_hud.clear();
	RenderImplementation.set_render_mode(CRender::MODE_NORMAL);

	// Restore Projection
	Engine.RenderView.Project = ProjectOld;
	Engine.RenderView.ViewProjection = ViewProjectOld;
	RenderBackend.set_transform_project(Engine.RenderView.Project);
}

void CSceneGraph::_RenderTranslucent(SceneGraphPacket& packet)
{
	PROFILE_FUNCTION();
	packet.queue_transparent.traverseRL(RenderSortedNode);
	packet.queue_transparent.clear();
}

void CSceneGraph::_RenderEmissive(SceneGraphPacket& packet)
{
	PROFILE_FUNCTION();
	packet.mapEmissive.traverseLR(RenderSortedNode);
	packet.mapEmissive.clear();
}

void CSceneGraph::_RenderWmarks(SceneGraphPacket& packet)
{
	PROFILE_FUNCTION();
	packet.queue_wallmarks.traverseLR(RenderSortedNode);
	packet.queue_wallmarks.clear();
}

void CSceneGraph::_RenderDistortion(SceneGraphPacket& packet)
{
	PROFILE_FUNCTION();
	packet.queue_distortion.traverseRL(RenderSortedNode);
	packet.queue_distortion.clear();
}

void CSceneGraph::_RenderLODs(SceneGraphPacket& packet, bool _setup_zb, bool _clear)
{
	PROFILE_FUNCTION();

	// Сбор LOD-ов в плоский список
	if (_setup_zb)
		packet.mapLOD.getLR(packet.lstLODs); // front-to-back (для Z-buffer)
	else
		packet.mapLOD.getRL(packet.lstLODs); // back-to-front (для цвета)

	if (packet.lstLODs.empty())
		return;

	u32 shader_id = _setup_zb ? SE_R1_LMODELS : SE_R1_NORMAL_LQ;
	FLOD* first_visual = (FLOD*)packet.lstLODs[0].pVisual;

	u32 vb_offset;
	FLOD::_hw* VertexBuffer = (FLOD::_hw*)RenderBackend.Vertex.Lock(packet.lstLODs.size() * 4, first_visual->geom->vb_stride, vb_offset);

	float ssa_range = r_ssaLOD_A - r_ssaLOD_B;
	if (ssa_range < EPS_S)
		ssa_range = EPS_S;

	const float ssa_limit_b = r_ssaLOD_B;
	const fvec3 camera_pos = Engine.RenderView.Position;

	// *** Генерация геометрии ***
	concurrency::parallel_for(size_t(0), packet.lstLODs.size(), [&](size_t i) 
	{
		FLOD::_hw* V = VertexBuffer + (i * 4);
		SceneGraphTypes::LodRenderNode& Node = packet.lstLODs[i];
		FLOD* lod_visual = (FLOD*)Node.pVisual;

		// Вычисление Alpha
		float ssa_diff = Node.ScreenSpaceArea - ssa_limit_b;
		float scale = ssa_diff / ssa_range;
		int alpha_int = iFloor((1.0f - scale) * 255.f);
		u32 alpha_final = u32(clampr(alpha_int, 0, 255));

		// Вычисление направления
		fvec3 dir_to_camera, shift;
		dir_to_camera.sub(lod_visual->vis.sphere.P, camera_pos).normalize();
		shift.mul(dir_to_camera, -.5f * lod_visual->vis.sphere.R);

		// Выбор лучших плоскостей
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

		// Интерполяция
		float dot_a = dot_best, dot_b = dot_next, dot_c = dot_next_2;
		float alpha_factor = 0.5f + 0.5f * (1 - (dot_b - dot_c) / (dot_a - dot_c));
		int factor_int = iFloor(alpha_factor * 255.5f);
		u32 factor_final = u32(clampr(factor_int, 0, 255));

		// Заполнение буфера
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

	// *** Группировка по шейдерам ***
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
				packet.lstLODgroups.push_back(current_count);
				current_shader = Node.pVisual->shader->E[shader_id];
				current_count = 1;
			}
		}
		packet.lstLODgroups.push_back(current_count);
	}

	// *** RENDER ***
	int current_lod_index = 0;
	RenderBackend.set_transform_world(Fidentity);

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

	// *** Cleanup ***
	packet.lstLODs.clear();
	packet.lstLODgroups.clear();

	if (_clear)
		packet.mapLOD.clear();
}
