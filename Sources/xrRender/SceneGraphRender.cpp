#include "stdafx.h"
#include "SceneGraph.h"
#include "flod.h"
#include "render.h"

#include <ppl.h> // Для concurrency::parallel_for

// Глобальные переменные (пока что)
extern float r_ssaGLOD_start, r_ssaGLOD_end;
extern float r_ssaHZBvsTEX;
extern float r_ssaLOD_A;
extern float r_ssaLOD_B;

using namespace SceneGraphTypes;

// ===============================================================================================
//  Internal Helpers & Predicates (Anonymous Namespace)
// ===============================================================================================
namespace
{
// --- LOD Calculation ---
ICF float calcLOD(float ssa /*fDistSq*/, float R)
{
	return _sqrt(clampr((ssa - r_ssaGLOD_end) / (r_ssaGLOD_start - r_ssaGLOD_end), 0.f, 1.f));
}

// --- LOD Sorting Helper ---
static bool pred_dot_std(const std::pair<float, u32>& _1, const std::pair<float, u32>& _2)
{
	return _1.first < _2.first;
}

// --- Normal Sorting Helper ---
static void mapNormal_Render(SceneGraphTypes::mapNormalItems& N)
{
	// Сортировка по SSA (screen space area)
	std::sort(N.begin(), N.end(),
			  [](const SceneGraphTypes::_NormalItem& N1, const SceneGraphTypes::_NormalItem& N2) { return (N1.ssa > N2.ssa); });

	for (auto& Ni : N)
	{
		Ni.pVisual->Render(calcLOD(Ni.ssa, Ni.pVisual->vis.sphere.R));
	}
}

// --- Matrix Sorting Helper ---
static void mapMatrix_Render(SceneGraphTypes::mapMatrixItems& N)
{
	std::sort(N.begin(), N.end(),
			  [](const SceneGraphTypes::_MatrixItem& N1, const SceneGraphTypes::_MatrixItem& N2) { return (N1.ssa > N2.ssa); });

	for (auto& Ni : N)
	{
		RenderBackend.set_xform_world(Ni.Matrix);
		RenderImplementation.apply_object(Ni.pObject);
		RenderImplementation.apply_lmaterial();
		Ni.pVisual->Render(calcLOD(Ni.ssa, Ni.pVisual->vis.sphere.R));
	}
	N.clear();
}

// --- Sorted Node Render Callback (for traversers) ---
static void __fastcall sorted_L1(SceneGraphTypes::mapSorted_Node* N)
{
	VERIFY(N);
	IRender_Visual* V = N->val.pVisual;
	VERIFY(V && V->shader._get());
	RenderBackend.set_Element(N->val.se);
	RenderBackend.set_xform_world(N->val.Matrix);
	RenderImplementation.apply_object(N->val.pObject);
	RenderImplementation.apply_lmaterial();
	V->Render(calcLOD(N->key, V->vis.sphere.R));
}

// --- Texture Sorting Predicates ---
template <typename TNode> bool cmp_textures_lex2(TNode* N1, TNode* N2)
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

template <typename TNode> bool cmp_textures_lex3(TNode* N1, TNode* N2)
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

template <typename TNode> bool cmp_textures_lexN(TNode* N1, TNode* N2)
{
	STextureList* t1 = N1->key;
	STextureList* t2 = N2->key;
	return std::lexicographical_compare(t1->begin(), t1->end(), t2->begin(), t2->end());
}

template <typename TNode> bool cmp_textures_ssa(TNode* N1, TNode* N2)
{
	return (N1->val.ssa > N2->val.ssa);
}

// --- Texture Sorting Logic ---
template <typename MapTextures, typename VecTypes>
void sort_tlist(VecTypes& lst, VecTypes& temp, MapTextures& textures, BOOL bSSA)
{
	int amount = textures.begin()->key->size();
	if (bSSA)
	{
		if (amount <= 1)
		{
			textures.getANY_P(lst);
			std::sort(lst.begin(), lst.end(), cmp_textures_ssa<typename MapTextures::TNode>);
		}
		else
		{
			auto _it = textures.begin();
			auto _end = textures.end();
			for (; _it != _end; _it++)
			{
				if (_it->val.ssa > r_ssaHZBvsTEX)
					lst.push_back(_it);
				else
					temp.push_back(_it);
			}

			std::sort(lst.begin(), lst.end(), cmp_textures_ssa<typename MapTextures::TNode>);

			if (2 == amount)
				std::sort(temp.begin(), temp.end(), cmp_textures_lex2<typename MapTextures::TNode>);
			else if (3 == amount)
				std::sort(temp.begin(), temp.end(), cmp_textures_lex3<typename MapTextures::TNode>);
			else
				std::sort(temp.begin(), temp.end(), cmp_textures_lexN<typename MapTextures::TNode>);

			lst.insert(lst.end(), temp.begin(), temp.end());
		}
	}
	else
	{
		textures.getANY_P(lst);
		if (2 == amount)
			std::sort(lst.begin(), lst.end(), cmp_textures_lex2<typename MapTextures::TNode>);
		else if (3 == amount)
			std::sort(lst.begin(), lst.end(), cmp_textures_lex3<typename MapTextures::TNode>);
		else
			std::sort(lst.begin(), lst.end(), cmp_textures_lexN<typename MapTextures::TNode>);
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

void CSceneGraph::_RenderOpaque(u32 _priority, bool _clear)
{
	OPTICK_EVENT("RenderOpaque");
	Device.Statistic->RenderDUMP.Begin();

	// **************************************************** NORMAL
	{
		// OPTICK_EVENT("NORMAL");
		RenderBackend.set_xform_world(Fidentity);

		mapNormalVS& vs = mapNormal[_priority];
		vs.getANY_P(nrmVS);

		for (u32 vs_id = 0; vs_id < nrmVS.size(); vs_id++)
		{
			mapNormalVS::TNode* Nvs = nrmVS[vs_id];
			RenderBackend.set_Vertex_Shader(Nvs->key);

			mapNormalPS& ps = Nvs->val;
			ps.ssa = 0;
			ps.getANY_P(nrmPS);
			for (u32 ps_id = 0; ps_id < nrmPS.size(); ps_id++)
			{
				mapNormalPS::TNode* Nps = nrmPS[ps_id];
				RenderBackend.set_Pixel_Shader(Nps->key);

				mapNormalCS& cs = Nps->val;
				cs.ssa = 0;
				cs.getANY_P(nrmCS);
				for (u32 cs_id = 0; cs_id < nrmCS.size(); cs_id++)
				{
					mapNormalCS::TNode* Ncs = nrmCS[cs_id];
					RenderBackend.set_Constants(Ncs->key);

					mapNormalStates& states = Ncs->val;
					states.ssa = 0;
					states.getANY_P(nrmStates);
					for (u32 state_id = 0; state_id < nrmStates.size(); state_id++)
					{
						mapNormalStates::TNode* Nstate = nrmStates[state_id];
						RenderBackend.set_States(Nstate->key);

						mapNormalTextures& tex = Nstate->val;
						tex.ssa = 0;

						sort_tlist(nrmTextures, nrmTexturesTemp, tex, TRUE);

						for (u32 tex_id = 0; tex_id < nrmTextures.size(); tex_id++)
						{
							mapNormalTextures::TNode* Ntex = nrmTextures[tex_id];
							RenderBackend.set_Textures(Ntex->key);
							RenderImplementation.apply_lmaterial();

							mapNormalItems& items = Ntex->val;
							items.ssa = 0;
							mapNormal_Render(items); // Local helper
							if (_clear)
								items.clear();
						}
						nrmTextures.clear();
						nrmTexturesTemp.clear();
						if (_clear)
							tex.clear();
					}
					nrmStates.clear();
					if (_clear)
						states.clear();
				}
				nrmCS.clear();
				if (_clear)
					cs.clear();
			}
			nrmPS.clear();
			if (_clear)
				ps.clear();
		}
		nrmVS.clear();
		if (_clear)
			vs.clear();
	}

	// **************************************************** MATRIX
	{
		// OPTICK_EVENT("MATRIX");
		mapMatrixVS& vs = mapMatrix[_priority];
		vs.getANY_P(matVS);

		for (u32 vs_id = 0; vs_id < matVS.size(); vs_id++)
		{
			mapMatrixVS::TNode* Nvs = matVS[vs_id];
			RenderBackend.set_Vertex_Shader(Nvs->key);

			mapMatrixPS& ps = Nvs->val;
			ps.ssa = 0;
			ps.getANY_P(matPS);

			for (u32 ps_id = 0; ps_id < matPS.size(); ps_id++)
			{
				mapMatrixPS::TNode* Nps = matPS[ps_id];
				RenderBackend.set_Pixel_Shader(Nps->key);

				mapMatrixCS& cs = Nps->val;
				cs.ssa = 0;
				cs.getANY_P(matCS);

				for (u32 cs_id = 0; cs_id < matCS.size(); cs_id++)
				{
					mapMatrixCS::TNode* Ncs = matCS[cs_id];
					RenderBackend.set_Constants(Ncs->key);

					mapMatrixStates& states = Ncs->val;
					states.ssa = 0;
					states.getANY_P(matStates);

					for (u32 state_id = 0; state_id < matStates.size(); state_id++)
					{
						mapMatrixStates::TNode* Nstate = matStates[state_id];
						RenderBackend.set_States(Nstate->key);

						mapMatrixTextures& tex = Nstate->val;
						tex.ssa = 0;

						sort_tlist(matTextures, matTexturesTemp, tex, TRUE);

						for (u32 tex_id = 0; tex_id < matTextures.size(); tex_id++)
						{
							mapMatrixTextures::TNode* Ntex = matTextures[tex_id];
							RenderBackend.set_Textures(Ntex->key);
							RenderImplementation.apply_lmaterial();

							mapMatrixItems& items = Ntex->val;
							items.ssa = 0;
							mapMatrix_Render(items);
						}
						matTextures.clear();
						matTexturesTemp.clear();
						if (_clear)
							tex.clear();
					}
					matStates.clear();
					if (_clear)
						states.clear();
				}
				matCS.clear();
				if (_clear)
					cs.clear();
			}
			matPS.clear();
			if (_clear)
				ps.clear();
		}
		matVS.clear();
		if (_clear)
			vs.clear();
	}

	Device.Statistic->RenderDUMP.End();
}

void CSceneGraph::_RenderHUD()
{
	OPTICK_EVENT("RenderHUD");
	ENGINE_API extern float psHUD_FOV;

	Fmatrix Pold = Device.mProject;
	Fmatrix FTold = Device.mFullTransform;
	Device.mProject.build_projection(deg2rad(psHUD_FOV * Device.fFOV), Device.fASPECT, VIEWPORT_NEAR_HUD,
									 g_pGamePersistent->Environment().CurrentEnv->far_plane);

	Device.mFullTransform.mul(Device.mProject, Device.mView);
	RenderBackend.set_xform_project(Device.mProject);

	RenderImplementation.set_render_mode(CRender::MODE_NEAR);
	mapHUD.traverseLR(sorted_L1); // Local helper
	mapHUD.clear();
	RenderImplementation.set_render_mode(CRender::MODE_NORMAL);

	Device.mProject = Pold;
	Device.mFullTransform = FTold;
	RenderBackend.set_xform_project(Device.mProject);
}

void CSceneGraph::_RenderTranslucent()
{
	OPTICK_EVENT("RenderTranslucent");
	mapSorted.traverseRL(sorted_L1);
	mapSorted.clear();
}

void CSceneGraph::_RenderEmissive()
{
	OPTICK_EVENT("RenderEmissive");
	mapEmissive.traverseLR(sorted_L1);
	mapEmissive.clear();
}

void CSceneGraph::_RenderWmarks()
{
	OPTICK_EVENT("RenderWmarks");
	mapWmark.traverseLR(sorted_L1);
	mapWmark.clear();
}

void CSceneGraph::_RenderDistortion()
{
	OPTICK_EVENT("RenderDistortion");
	mapDistort.traverseRL(sorted_L1);
	mapDistort.clear();
}

void CSceneGraph::_RenderLODs(bool _setup_zb, bool _clear)
{
	OPTICK_EVENT("RenderLODs");

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

	// Захват переменных для PPL
	const float f_ssaLOD_B = r_ssaLOD_B;
	const Fvector vCameraPos = Device.vCameraPosition;

	// *** 2. ПАРАЛЛЕЛЬНЫЙ ПРОХОД: Генерация геометрии ***
	concurrency::parallel_for(size_t(0), lstLODs.size(), [&](size_t i) {
		// Получаем указатель на 4 вершины, принадлежащие этому LOD-у
		FLOD::_hw* V = V_start + (i * 4);
		SceneGraphTypes::_LodItem& P = lstLODs[i];

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

		// Используем std::sort с локальным предикатом
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
	if (!lstLODs.empty())
	{
		ref_selement cur_S = lstLODs[0].pVisual->shader->E[shid];
		int cur_count = 0;

		for (u32 i = 0; i < lstLODs.size(); i++)
		{
			SceneGraphTypes::_LodItem& P = lstLODs[i];
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
		lstLODgroups.push_back(cur_count);
	}

	// *** 4. RENDER ***
	////OPTICK_EVENT("CSceneGraph::render_lods - render");

	int current = 0;
	RenderBackend.set_xform_world(Fidentity);

	for (u32 g = 0; g < lstLODgroups.size(); g++)
	{
		int p_count = lstLODgroups[g];

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

//////////////////////////////////////////////////////////////////////////
// sub-space rendering - shortcut to render with frustum extracted from matrix
void CSceneGraph::render_subspace(IRender_Sector* _sector, Fmatrix& mCombined, Fvector& _cop, BOOL _dynamic,
								  BOOL _precise_portals)
{
	OPTICK_EVENT("render_subspace - shortcut");

	CFrustum temp;
	temp.CreateFromMatrix(mCombined, FRUSTUM_P_ALL);
	render_subspace(_sector, &temp, mCombined, _cop, _dynamic, _precise_portals);
}

// sub-space rendering - main procedure
void CSceneGraph::render_subspace(IRender_Sector* _sector, CFrustum* _frustum, Fmatrix& mCombined, Fvector& _cop,
								  BOOL _dynamic, BOOL _precise_portals)
{
	OPTICK_EVENT("render_subspace - main");

	VERIFY(_sector);
	marker++; // !!! critical here

	// Save and build new frustum, disable HOM
	CFrustum ViewSave = RenderImplementation.ViewBase;
	RenderImplementation.ViewBase = *_frustum;
	RenderImplementation.View = &RenderImplementation.ViewBase;

	if (_precise_portals && RenderImplementation.rmPortals)
	{
		// Check if camera is too near to some portal - if so force DualRender
		Fvector box_radius;
		box_radius.set(EPS_L * 20, EPS_L * 20, EPS_L * 20);
		RenderImplementation.Sectors_xrc.box_options(CDB::OPT_FULL_TEST);
		RenderImplementation.Sectors_xrc.box_query(RenderImplementation.rmPortals, _cop, box_radius);
		for (int K = 0; K < RenderImplementation.Sectors_xrc.r_count(); K++)
		{
			CPortal* pPortal =
				(CPortal*)
					RenderImplementation.Portals[RenderImplementation.rmPortals
													 ->get_tris()[RenderImplementation.Sectors_xrc.r_begin()[K].id]
													 .dummy];
			pPortal->bDualRender = TRUE;
		}
	}

	// Traverse sector/portal structure
	PortalTraverser.traverse(_sector, RenderImplementation.ViewBase, _cop, mCombined, 0);

	// Determine visibility for static geometry hierrarhy
	for (u32 s_it = 0; s_it < PortalTraverser.r_sectors.size(); s_it++)
	{
		CSector* sector = (CSector*)PortalTraverser.r_sectors[s_it];
		IRender_Visual* root = sector->root();
		for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
		{
			RenderImplementation.set_Frustum(&(sector->r_frustums[v_it]));
			RenderImplementation.add_Geometry(root);
		}
	}

	if (_dynamic)
	{
		RenderImplementation.set_Object(0);

		// Traverse object database
		g_SpatialSpace->q_frustum(lstRenderables, ISpatial_DB::O_ORDERED, STYPE_RENDERABLE,
								  RenderImplementation.ViewBase);

		// Determine visibility for dynamic part of scene
		for (u32 o_it = 0; o_it < lstRenderables.size(); o_it++)
		{
			ISpatial* spatial = lstRenderables[o_it];
			CSector* sector = (CSector*)spatial->spatial.sector;
			if (0 == sector)
				continue; // disassociated from S/P structure
			if (PortalTraverser.i_marker != sector->r_marker)
				continue; // inactive (untouched) sector
			for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
			{
				RenderImplementation.set_Frustum(&(sector->r_frustums[v_it]));
				if (!RenderImplementation.View->testSphere_dirty(spatial->spatial.sphere.P, spatial->spatial.sphere.R))
					continue;

				// renderable
				IRenderable* renderable = spatial->dcast_Renderable();
				if (0 == renderable)
					continue; // unknown, but renderable object (r1_glow???)

				renderable->renderable_Render();
			}
		}
	}

	if (g_pGameLevel && (RenderImplementation.active_phase() == RenderImplementation.PHASE_SHADOW_DEPTH))
		g_pGameLevel->pHUD->Render_Actor_Shadow(); // ACtor Shadow

	// Restore
	RenderImplementation.ViewBase = ViewSave;
	RenderImplementation.View = 0;
}

void CSceneGraph::render_reuse()
{
	PROFILE_FUNCTION();

	// Статика
	for (IRender_Visual* V : m_visuals_static_visible)
	{
		// Вызываем добавление ЛИСТА.
		// Важно: мы не вызываем полную рекурсию add_Static, а сразу идем к листовой логике.
		// Но нам нужно, чтобы switch внутри add_leafs_Static отработал,
		// чтобы корректно раскидать LOD-ы, если они попали в список.

		// Оптимальный вариант - вызвать switch обработки типа из add_leafs_Static
		// Но чтобы не дублировать код, можно просто вызвать add_leafs_Static.
		// Да, там есть проверка HOM.visible, но она очень быстрая (это просто флаг после render_main).
		add_leafs_Static(V);
	}

	// Динамика
	for (auto& it : m_visuals_dynamic_visible)
	{
		RenderImplementation.set_Transform(&it.matrix); // Восстанавливаем матрицу
		add_leafs_Dynamic(it.visual);
	}
}