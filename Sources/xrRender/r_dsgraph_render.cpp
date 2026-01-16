#include "stdafx.h"

#include "..\xrEngine\render.h"
#include "..\xrEngine\irenderable.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
#include "..\xrEngine\customhud.h"
#include "..\xrEngine\SkeletonCustom.h"

using namespace R_dsgraph;

extern float r_ssaDISCARD;
extern float r_ssaDONTSORT;
extern float r_ssaHZBvsTEX;
extern float r_ssaGLOD_start, r_ssaGLOD_end;

ICF float calcLOD(float ssa /*fDistSq*/, float R)
{
	return _sqrt(clampr((ssa - r_ssaGLOD_end) / (r_ssaGLOD_start - r_ssaGLOD_end), 0.f, 1.f));
}

// NORMAL
IC bool cmp_normal_items(const _NormalItem& N1, const _NormalItem& N2)
{
	return (N1.ssa > N2.ssa);
}

void __fastcall mapNormal_Render(mapNormalItems& N)
{
	// *** DIRECT ***
	std::sort(N.begin(), N.end(), cmp_normal_items);
	_NormalItem *I = &*N.begin(), *E = &*N.end();
	for (; I != E; I++)
	{
		_NormalItem& Ni = *I;
		Ni.pVisual->Render(calcLOD(Ni.ssa, Ni.pVisual->vis.sphere.R));
	}
}

// Matrix
IC bool cmp_matrix_items(const _MatrixItem& N1, const _MatrixItem& N2)
{
	return (N1.ssa > N2.ssa);
}

void __fastcall mapMatrix_Render(mapMatrixItems& N)
{
	// *** DIRECT ***
	std::sort(N.begin(), N.end(), cmp_matrix_items);
	_MatrixItem *I = &*N.begin(), *E = &*N.end();
	for (; I != E; I++)
	{
		_MatrixItem& Ni = *I;
		RenderBackend.set_xform_world(Ni.Matrix);
		RenderImplementation.apply_object(Ni.pObject);
		RenderImplementation.apply_lmaterial();
		Ni.pVisual->Render(calcLOD(Ni.ssa, Ni.pVisual->vis.sphere.R));
	}
	N.clear();
}

// ALPHA
void __fastcall sorted_L1(mapSorted_Node* N)
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

IC bool cmp_vs_nrm(mapNormalVS::TNode* N1, mapNormalVS::TNode* N2)
{
	return (N1->val.ssa > N2->val.ssa);
}
IC bool cmp_vs_mat(mapMatrixVS::TNode* N1, mapMatrixVS::TNode* N2)
{
	return (N1->val.ssa > N2->val.ssa);
}

IC bool cmp_ps_nrm(mapNormalPS::TNode* N1, mapNormalPS::TNode* N2)
{
	return (N1->val.ssa > N2->val.ssa);
}
IC bool cmp_ps_mat(mapMatrixPS::TNode* N1, mapMatrixPS::TNode* N2)
{
	return (N1->val.ssa > N2->val.ssa);
}

IC bool cmp_cs_nrm(mapNormalCS::TNode* N1, mapNormalCS::TNode* N2)
{
	return (N1->val.ssa > N2->val.ssa);
}
IC bool cmp_cs_mat(mapMatrixCS::TNode* N1, mapMatrixCS::TNode* N2)
{
	return (N1->val.ssa > N2->val.ssa);
}

IC bool cmp_states_nrm(mapNormalStates::TNode* N1, mapNormalStates::TNode* N2)
{
	return (N1->val.ssa > N2->val.ssa);
}
IC bool cmp_states_mat(mapMatrixStates::TNode* N1, mapMatrixStates::TNode* N2)
{
	return (N1->val.ssa > N2->val.ssa);
}

IC bool cmp_textures_lex2_nrm(mapNormalTextures::TNode* N1, mapNormalTextures::TNode* N2)
{
	STextureList* t1 = N1->key;
	STextureList* t2 = N2->key;
	if ((*t1)[0] < (*t2)[0])
		return true;
	if ((*t1)[0] > (*t2)[0])
		return false;
	if ((*t1)[1] < (*t2)[1])
		return true;
	else
		return false;
}
IC bool cmp_textures_lex2_mat(mapMatrixTextures::TNode* N1, mapMatrixTextures::TNode* N2)
{
	STextureList* t1 = N1->key;
	STextureList* t2 = N2->key;
	if ((*t1)[0] < (*t2)[0])
		return true;
	if ((*t1)[0] > (*t2)[0])
		return false;
	if ((*t1)[1] < (*t2)[1])
		return true;
	else
		return false;
}
IC bool cmp_textures_lex3_nrm(mapNormalTextures::TNode* N1, mapNormalTextures::TNode* N2)
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
	else
		return false;
}
IC bool cmp_textures_lex3_mat(mapMatrixTextures::TNode* N1, mapMatrixTextures::TNode* N2)
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
	else
		return false;
}
IC bool cmp_textures_lexN_nrm(mapNormalTextures::TNode* N1, mapNormalTextures::TNode* N2)
{
	STextureList* t1 = N1->key;
	STextureList* t2 = N2->key;
	return std::lexicographical_compare(t1->begin(), t1->end(), t2->begin(), t2->end());
}
IC bool cmp_textures_lexN_mat(mapMatrixTextures::TNode* N1, mapMatrixTextures::TNode* N2)
{
	STextureList* t1 = N1->key;
	STextureList* t2 = N2->key;
	return std::lexicographical_compare(t1->begin(), t1->end(), t2->begin(), t2->end());
}
IC bool cmp_textures_ssa_nrm(mapNormalTextures::TNode* N1, mapNormalTextures::TNode* N2)
{
	return (N1->val.ssa > N2->val.ssa);
}
IC bool cmp_textures_ssa_mat(mapMatrixTextures::TNode* N1, mapMatrixTextures::TNode* N2)
{
	return (N1->val.ssa > N2->val.ssa);
}

void sort_tlist_nrm(xr_vector<mapNormalTextures::TNode*, render_alloc<mapNormalTextures::TNode*>>& lst,
					xr_vector<mapNormalTextures::TNode*, render_alloc<mapNormalTextures::TNode*>>& temp,
					mapNormalTextures& textures, BOOL bSSA)
{
	int amount = textures.begin()->key->size();
	if (bSSA)
	{
		if (amount <= 1)
		{
			// Just sort by SSA
			textures.getANY_P(lst);
			std::sort(lst.begin(), lst.end(), cmp_textures_ssa_nrm);
		}
		else
		{
			// Split into 2 parts
			mapNormalTextures::TNode* _it = textures.begin();
			mapNormalTextures::TNode* _end = textures.end();
			for (; _it != _end; _it++)
			{
				if (_it->val.ssa > r_ssaHZBvsTEX)
					lst.push_back(_it);
				else
					temp.push_back(_it);
			}

			// 1st - part - SSA, 2nd - lexicographically
			std::sort(lst.begin(), lst.end(), cmp_textures_ssa_nrm);
			if (2 == amount)
				std::sort(temp.begin(), temp.end(), cmp_textures_lex2_nrm);
			else if (3 == amount)
				std::sort(temp.begin(), temp.end(), cmp_textures_lex3_nrm);
			else
				std::sort(temp.begin(), temp.end(), cmp_textures_lexN_nrm);

			// merge lists
			lst.insert(lst.end(), temp.begin(), temp.end());
		}
	}
	else
	{
		textures.getANY_P(lst);
		if (2 == amount)
			std::sort(lst.begin(), lst.end(), cmp_textures_lex2_nrm);
		else if (3 == amount)
			std::sort(lst.begin(), lst.end(), cmp_textures_lex3_nrm);
		else
			std::sort(lst.begin(), lst.end(), cmp_textures_lexN_nrm);
	}
}

void sort_tlist_mat(xr_vector<mapMatrixTextures::TNode*, render_alloc<mapMatrixTextures::TNode*>>& lst,
					xr_vector<mapMatrixTextures::TNode*, render_alloc<mapMatrixTextures::TNode*>>& temp,
					mapMatrixTextures& textures, BOOL bSSA)
{
	int amount = textures.begin()->key->size();
	if (bSSA)
	{
		if (amount <= 1)
		{
			// Just sort by SSA
			textures.getANY_P(lst);
			std::sort(lst.begin(), lst.end(), cmp_textures_ssa_mat);
		}
		else
		{
			// Split into 2 parts
			mapMatrixTextures::TNode* _it = textures.begin();
			mapMatrixTextures::TNode* _end = textures.end();
			for (; _it != _end; _it++)
			{
				if (_it->val.ssa > r_ssaHZBvsTEX)
					lst.push_back(_it);
				else
					temp.push_back(_it);
			}

			// 1st - part - SSA, 2nd - lexicographically
			std::sort(lst.begin(), lst.end(), cmp_textures_ssa_mat);
			if (2 == amount)
				std::sort(temp.begin(), temp.end(), cmp_textures_lex2_mat);
			else if (3 == amount)
				std::sort(temp.begin(), temp.end(), cmp_textures_lex3_mat);
			else
				std::sort(temp.begin(), temp.end(), cmp_textures_lexN_mat);

			// merge lists
			lst.insert(lst.end(), temp.begin(), temp.end());
		}
	}
	else
	{
		textures.getANY_P(lst);
		if (2 == amount)
			std::sort(lst.begin(), lst.end(), cmp_textures_lex2_mat);
		else if (3 == amount)
			std::sort(lst.begin(), lst.end(), cmp_textures_lex3_mat);
		else
			std::sort(lst.begin(), lst.end(), cmp_textures_lexN_mat);
	}
}

void CSceneGraph::r_dsgraph_render_graph(u32 _priority, bool _clear)
{
	PROFILE_FUNCTION_FULL();

	Device.Statistic->RenderDUMP.Begin();

	// **************************************************** NORMAL
	// Perform sorting based on ScreenSpaceArea
	// Sorting by SSA and changes minimizations
	{
		//OPTICK_EVENT("NORMAL");

		RenderBackend.set_xform_world(Fidentity);
		mapNormalVS& vs = mapNormal[_priority];
		vs.getANY_P(nrmVS);
		//std::sort(nrmVS.begin(), nrmVS.end(), cmp_vs_nrm);
		for (u32 vs_id = 0; vs_id < nrmVS.size(); vs_id++)
		{
			mapNormalVS::TNode* Nvs = nrmVS[vs_id];
			RenderBackend.set_Vertex_Shader(Nvs->key);

			mapNormalPS& ps = Nvs->val;
			ps.ssa = 0;
			ps.getANY_P(nrmPS);
			//std::sort(nrmPS.begin(), nrmPS.end(), cmp_ps_nrm);
			for (u32 ps_id = 0; ps_id < nrmPS.size(); ps_id++)
			{
				mapNormalPS::TNode* Nps = nrmPS[ps_id];
				RenderBackend.set_Pixel_Shader(Nps->key);

				mapNormalCS& cs = Nps->val;
				cs.ssa = 0;
				cs.getANY_P(nrmCS);
				//std::sort(nrmCS.begin(), nrmCS.end(), cmp_cs_nrm);
				for (u32 cs_id = 0; cs_id < nrmCS.size(); cs_id++)
				{
					mapNormalCS::TNode* Ncs = nrmCS[cs_id];
					RenderBackend.set_Constants(Ncs->key);

					mapNormalStates& states = Ncs->val;
					states.ssa = 0;
					states.getANY_P(nrmStates);
					//std::sort(nrmStates.begin(), nrmStates.end(), cmp_states_nrm);
					for (u32 state_id = 0; state_id < nrmStates.size(); state_id++)
					{
						mapNormalStates::TNode* Nstate = nrmStates[state_id];
						RenderBackend.set_States(Nstate->key);

						mapNormalTextures& tex = Nstate->val;
						tex.ssa = 0;
						sort_tlist_nrm(nrmTextures, nrmTexturesTemp, tex, true);
						for (u32 tex_id = 0; tex_id < nrmTextures.size(); tex_id++)
						{
							mapNormalTextures::TNode* Ntex = nrmTextures[tex_id];
							RenderBackend.set_Textures(Ntex->key);
							RenderImplementation.apply_lmaterial();

							mapNormalItems& items = Ntex->val;
							items.ssa = 0;
							mapNormal_Render(items);
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
	// Perform sorting based on ScreenSpaceArea
	// Sorting by SSA and changes minimizations
	{
		//OPTICK_EVENT("MATRIX");

		mapMatrixVS& vs = mapMatrix[_priority];
		vs.getANY_P(matVS);
		//std::sort(matVS.begin(), matVS.end(), cmp_vs_mat);
		for (u32 vs_id = 0; vs_id < matVS.size(); vs_id++)
		{
			mapMatrixVS::TNode* Nvs = matVS[vs_id];
			RenderBackend.set_Vertex_Shader(Nvs->key);

			mapMatrixPS& ps = Nvs->val;
			ps.ssa = 0;
			ps.getANY_P(matPS);
			//std::sort(matPS.begin(), matPS.end(), cmp_ps_mat);
			for (u32 ps_id = 0; ps_id < matPS.size(); ps_id++)
			{
				mapMatrixPS::TNode* Nps = matPS[ps_id];
				RenderBackend.set_Pixel_Shader(Nps->key);

				mapMatrixCS& cs = Nps->val;
				cs.ssa = 0;
				cs.getANY_P(matCS);
				//std::sort(matCS.begin(), matCS.end(), cmp_cs_mat);
				for (u32 cs_id = 0; cs_id < matCS.size(); cs_id++)
				{
					mapMatrixCS::TNode* Ncs = matCS[cs_id];
					RenderBackend.set_Constants(Ncs->key);

					mapMatrixStates& states = Ncs->val;
					states.ssa = 0;
					states.getANY_P(matStates);
					//std::sort(matStates.begin(), matStates.end(), cmp_states_mat);
					for (u32 state_id = 0; state_id < matStates.size(); state_id++)
					{
						mapMatrixStates::TNode* Nstate = matStates[state_id];
						RenderBackend.set_States(Nstate->key);

						mapMatrixTextures& tex = Nstate->val;
						tex.ssa = 0;
						sort_tlist_mat(matTextures, matTexturesTemp, tex, true);
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
//////////////////////////////////////////////////////////////////////////
// HUD render
void CSceneGraph::r_dsgraph_render_hud()
{
	PROFILE_FUNCTION();

	ENGINE_API extern float psHUD_FOV;

	// Change projection
	Fmatrix Pold = Device.mProject;
	Fmatrix FTold = Device.mFullTransform;
	Device.mProject.build_projection(deg2rad(psHUD_FOV * Device.fFOV), Device.fASPECT, VIEWPORT_NEAR_HUD, g_pGamePersistent->Environment().CurrentEnv->far_plane);

	Device.mFullTransform.mul(Device.mProject, Device.mView);
	RenderBackend.set_xform_project(Device.mProject);

	// Rendering
	RenderImplementation.set_render_mode(CRender::MODE_NEAR);
	mapHUD.traverseLR(sorted_L1);
	mapHUD.clear();
	RenderImplementation.set_render_mode(CRender::MODE_NORMAL);

	// Restore projection
	Device.mProject = Pold;
	Device.mFullTransform = FTold;
	RenderBackend.set_xform_project(Device.mProject);
}
//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void CSceneGraph::r_dsgraph_render_sorted()
{
	PROFILE_FUNCTION();

	// Sorted (back to front)
	mapSorted.traverseRL(sorted_L1);
	mapSorted.clear();
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void CSceneGraph::r_dsgraph_render_emissive()
{
	PROFILE_FUNCTION();

	// Sorted (back to front)
	mapEmissive.traverseLR(sorted_L1);
	mapEmissive.clear();
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void CSceneGraph::r_dsgraph_render_wmarks()
{
	PROFILE_FUNCTION();

	// Sorted (back to front)
	mapWmark.traverseLR(sorted_L1);
	mapWmark.clear();
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void CSceneGraph::r_dsgraph_render_distort()
{
	PROFILE_FUNCTION();

	// Sorted (back to front)
	mapDistort.traverseRL(sorted_L1);
	mapDistort.clear();
}

//////////////////////////////////////////////////////////////////////////
// sub-space rendering - shortcut to render with frustum extracted from matrix
void CSceneGraph::r_dsgraph_render_subspace(IRender_Sector* _sector, Fmatrix& mCombined, Fvector& _cop,
													BOOL _dynamic, BOOL _precise_portals)
{
	PROFILE_FUNCTION_FULL();

	CFrustum temp;
	temp.CreateFromMatrix(mCombined, FRUSTUM_P_ALL);
	r_dsgraph_render_subspace(_sector, &temp, mCombined, _cop, _dynamic, _precise_portals);
}

// sub-space rendering - main procedure
void CSceneGraph::r_dsgraph_render_subspace(IRender_Sector* _sector, CFrustum* _frustum, Fmatrix& mCombined,
													Fvector& _cop, BOOL _dynamic, BOOL _precise_portals)
{
	PROFILE_FUNCTION();

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
			CPortal* pPortal = (CPortal*)RenderImplementation.Portals[RenderImplementation.rmPortals->get_tris()[RenderImplementation.Sectors_xrc.r_begin()[K].id].dummy];
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

void CSceneGraph::r_dsgraph_render_reuse()
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
