#include "stdafx.h"

#include "..\xrEngine\fhierrarhyvisual.h"
#include "..\xrEngine\SkeletonCustom.h"
#include "..\xrEngine\fmesh.h"
#include "..\xrEngine\irenderable.h"

#include "flod.h"
#include "particlegroup.h"
#include "FTreeVisual.h"

using namespace R_dsgraph;

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
	distSQ = Device.vCameraPosition.distance_to_sqr(C) + EPS;
	return R / distSQ;
}
ICF float CalcSSA(float& distSQ, Fvector& C, float R)
{
	distSQ = Device.vCameraPosition.distance_to_sqr(C) + EPS;
	return R / distSQ;
}

void R_dsgraph_structure::r_dsgraph_insert_dynamic(IRender_Visual* pVisual, Fvector& Center)
{
	CRender& RI = RenderImplementation;

	if (pVisual->vis.marker == RI.marker)
		return;
	pVisual->vis.marker = RI.marker;

	float distSQ;
	float SSA = CalcSSA(distSQ, Center, pVisual);
	if (SSA <= r_ssaDISCARD)
		return;

	// Distortive geometry should be marked and R2 special-cases it
	// a) Allow to optimize RT order
	// b) Should be rendered to special distort buffer in another pass
	VERIFY(pVisual->shader._get());
	ShaderElement* sh_d = &*pVisual->shader->E[4];
	if (sh_d && sh_d->flags.bDistort && pmask[sh_d->flags.iPriority / 2])
	{
		mapSorted_Node* N = mapDistort.insertInAnyWay(distSQ);
		N->val.ssa = SSA;
		N->val.pObject = RI.val_pObject;
		N->val.pVisual = pVisual;
		N->val.Matrix = *RI.val_pTransform;
		N->val.se = sh_d; // 4=L_special
	}

	// Select shader
	ShaderElement* sh = RenderImplementation.rimp_select_sh_dynamic(pVisual, distSQ);
	if (0 == sh)
		return;
	if (!pmask[sh->flags.iPriority / 2])
		return;

	// Create common node
	// NOTE: Invisible elements exist only in R1
	_MatrixItem item = {SSA, RI.val_pObject, pVisual, *RI.val_pTransform};

	// HUD rendering
	if (RI.val_bHUD)
	{
		if (sh->flags.bStrictB2F)
		{
			mapSorted_Node* N = mapSorted.insertInAnyWay(distSQ);
			N->val.ssa = SSA;
			N->val.pObject = RI.val_pObject;
			N->val.pVisual = pVisual;
			N->val.Matrix = *RI.val_pTransform;
			N->val.se = sh;
			return;
		}
		else
		{
			mapHUD_Node* N = mapHUD.insertInAnyWay(distSQ);
			N->val.ssa = SSA;
			N->val.pObject = RI.val_pObject;
			N->val.pVisual = pVisual;
			N->val.Matrix = *RI.val_pTransform;
			N->val.se = sh;
			return;
		}
	}

	if (RI.val_bInvisible)
		return;

	// strict-sorting selection
	if (sh->flags.bStrictB2F)
	{
		mapSorted_Node* N = mapSorted.insertInAnyWay(distSQ);
		N->val.ssa = SSA;
		N->val.pObject = RI.val_pObject;
		N->val.pVisual = pVisual;
		N->val.Matrix = *RI.val_pTransform;
		N->val.se = sh;
		return;
	}

	// Emissive geometry should be marked and R2 special-cases it
	// a) Allow to skeep already lit pixels
	// b) Allow to make them 100% lit and really bright
	// c) Should not cast shadows
	// d) Should be rendered to accumulation buffer in the second pass
	if (sh->flags.bEmissive)
	{
		mapSorted_Node* N = mapEmissive.insertInAnyWay(distSQ);
		N->val.ssa = SSA;
		N->val.pObject = RI.val_pObject;
		N->val.pVisual = pVisual;
		N->val.Matrix = *RI.val_pTransform;
		N->val.se = &*pVisual->shader->E[4]; // 4=L_special
	}
	if (sh->flags.bWmark && pmask_wmark)
	{
		mapSorted_Node* N = mapWmark.insertInAnyWay(distSQ);
		N->val.ssa = SSA;
		N->val.pObject = RI.val_pObject;
		N->val.pVisual = pVisual;
		N->val.Matrix = *RI.val_pTransform;
		N->val.se = sh;
		return;
	}

	// the most common node
	SPass& pass = *sh->passes.front();
	mapMatrix_T& map = mapMatrix[sh->flags.iPriority / 2];
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
	if (SSA > Ntex->val.ssa)
	{
		Ntex->val.ssa = SSA;
		if (SSA > Nstate->val.ssa)
		{
			Nstate->val.ssa = SSA;
			if (SSA > Ncs->val.ssa)
			{
				Ncs->val.ssa = SSA;
				if (SSA > Nps->val.ssa)
				{
					Nps->val.ssa = SSA;
					if (SSA > Nvs->val.ssa)
					{
						Nvs->val.ssa = SSA;
					}
				}
			}
		}
	}

	if (val_recorder)
	{
		Fbox3 temp;
		Fmatrix& xf = *RI.val_pTransform;
		temp.xform(pVisual->vis.box, xf);
		val_recorder->push_back(temp);
	}
}

void R_dsgraph_structure::r_dsgraph_insert_static(IRender_Visual* pVisual)
{
	CRender& RI = RenderImplementation;

	if (pVisual->vis.marker == RI.marker)
		return;
	pVisual->vis.marker = RI.marker;

	float distSQ;
	float SSA = CalcSSA(distSQ, pVisual->vis.sphere.P, pVisual);
	if (SSA <= r_ssaDISCARD)
		return;

	// Distortive geometry should be marked and R2 special-cases it
	// a) Allow to optimize RT order
	// b) Should be rendered to special distort buffer in another pass
	VERIFY(pVisual->shader._get());
	ShaderElement* sh_d = &*pVisual->shader->E[4];
	if (sh_d && sh_d->flags.bDistort && pmask[sh_d->flags.iPriority / 2])
	{
		//////OPTICK_EVENT("R_dsgraph_structure::r_dsgraph_insert_static - Distort");

		mapSorted_Node* N = mapDistort.insertInAnyWay(distSQ);
		N->val.ssa = SSA;
		N->val.pObject = NULL;
		N->val.pVisual = pVisual;
		N->val.Matrix = Fidentity;
		N->val.se = &*pVisual->shader->E[4]; // 4=L_special
	}

	// Select shader
	ShaderElement* sh = RenderImplementation.rimp_select_sh_static(pVisual, distSQ);

	if (0 == sh)
		return;

	if (!pmask[sh->flags.iPriority / 2])
		return;

	// strict-sorting selection
	if (sh->flags.bStrictB2F)
	{
		//////OPTICK_EVENT("R_dsgraph_structure::r_dsgraph_insert_static - bStrictB2F");

		mapSorted_Node* N = mapSorted.insertInAnyWay(distSQ);
		N->val.pObject = NULL;
		N->val.pVisual = pVisual;
		N->val.Matrix = Fidentity;
		N->val.se = sh;
		return;
	}

	// Emissive geometry should be marked and R2 special-cases it
	// a) Allow to skeep already lit pixels
	// b) Allow to make them 100% lit and really bright
	// c) Should not cast shadows
	// d) Should be rendered to accumulation buffer in the second pass
	if (sh->flags.bEmissive)
	{
		//////OPTICK_EVENT("R_dsgraph_structure::r_dsgraph_insert_static - Emissive");

		mapSorted_Node* N = mapEmissive.insertInAnyWay(distSQ);
		N->val.ssa = SSA;
		N->val.pObject = NULL;
		N->val.pVisual = pVisual;
		N->val.Matrix = Fidentity;
		N->val.se = &*pVisual->shader->E[4]; // 4=L_special
	}

	if (sh->flags.bWmark && pmask_wmark)
	{
		//////OPTICK_EVENT("R_dsgraph_structure::r_dsgraph_insert_static - Wmark");

		mapSorted_Node* N = mapWmark.insertInAnyWay(distSQ);
		N->val.ssa = SSA;
		N->val.pObject = NULL;
		N->val.pVisual = pVisual;
		N->val.Matrix = Fidentity;
		N->val.se = sh;
		return;
	}

	if (val_feedback && counter_S == val_feedback_breakp)
		val_feedback->rfeedback_static(pVisual);

	counter_S++;
	//////OPTICK_EVENT("R_dsgraph_structure::r_dsgraph_insert_static - insert");
	SPass& pass = *sh->passes.front();
	mapNormal_T& map = mapNormal[sh->flags.iPriority / 2];
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
	_NormalItem item = {SSA, pVisual};
	items.push_back(item);

	// Need to sort for HZB efficient use
	//////OPTICK_EVENT("R_dsgraph_structure::r_dsgraph_insert_static - Sort");
	if (SSA > Ntex->val.ssa)
	{
		Ntex->val.ssa = SSA;
		if (SSA > Nstate->val.ssa)
		{
			Nstate->val.ssa = SSA;
			if (SSA > Ncs->val.ssa)
			{
				Ncs->val.ssa = SSA;
				if (SSA > Nps->val.ssa)
				{
					Nps->val.ssa = SSA;
					if (SSA > Nvs->val.ssa)
					{
						Nvs->val.ssa = SSA;
					}
				}
			}
		}
	}

	if (val_recorder)
	{
		val_recorder->push_back(pVisual->vis.box);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Static geometry optimization
#define O_S_L1_S_LOW 10.f  // geometry 3d volume size
#define O_S_L1_D_LOW 150.f // distance, after which it is not rendered
#define O_S_L2_S_LOW 100.f
#define O_S_L2_D_LOW 200.f
#define O_S_L3_S_LOW 500.f
#define O_S_L3_D_LOW 250.f
#define O_S_L4_S_LOW 2500.f
#define O_S_L4_D_LOW 350.f
#define O_S_L5_S_LOW 7000.f
#define O_S_L5_D_LOW 400.f
#define O_S_L6_S_LOW 12000.f
#define O_S_L6_D_LOW 800.f
#define O_S_L7_S_LOW 18000.f
#define O_S_L7_D_LOW 1000.f
#define O_S_L8_S_LOW 25000.f
#define O_S_L8_D_LOW 1200.f
#define O_S_L9_S_LOW 40000.f
#define O_S_L9_D_LOW 1500.f
#define O_S_L10_S_LOW 60000.f
#define O_S_L10_D_LOW 1800.f
#define O_S_L11_S_LOW 100000.f
#define O_S_L11_D_LOW 2000.f
#define O_S_L12_S_LOW 150000.f
#define O_S_L12_D_LOW 2500.f

#define O_S_L1_S_MED 25.f
#define O_S_L1_D_MED 50.f
#define O_S_L2_S_MED 200.f
#define O_S_L2_D_MED 150.f
#define O_S_L3_S_MED 1000.f
#define O_S_L3_D_MED 200.f
#define O_S_L4_S_MED 2500.f
#define O_S_L4_D_MED 300.f
#define O_S_L5_S_MED 7000.f
#define O_S_L5_D_MED 400.f
#define O_S_L6_S_MED 15000.f
#define O_S_L6_D_MED 750.f
#define O_S_L7_S_MED 20000.f
#define O_S_L7_D_MED 900.f
#define O_S_L8_S_MED 35000.f
#define O_S_L8_D_MED 1100.f
#define O_S_L9_S_MED 55000.f
#define O_S_L9_D_MED 1250.f
#define O_S_L10_S_MED 75000.f
#define O_S_L10_D_MED 1500.f
#define O_S_L11_S_MED 120000.f
#define O_S_L11_D_MED 1750.f
#define O_S_L12_S_MED 200000.f
#define O_S_L12_D_MED 2000.f

#define O_S_L1_S_HII 50.f
#define O_S_L1_D_HII 50.f
#define O_S_L2_S_HII 400.f
#define O_S_L2_D_HII 150.f
#define O_S_L3_S_HII 1500.f
#define O_S_L3_D_HII 200.f
#define O_S_L4_S_HII 5000.f
#define O_S_L4_D_HII 300.f
#define O_S_L5_S_HII 20000.f
#define O_S_L5_D_HII 350.f
#define O_S_L6_S_HII 30000.f
#define O_S_L6_D_HII 700.f
#define O_S_L7_S_HII 45000.f
#define O_S_L7_D_HII 900.f
#define O_S_L8_S_HII 55000.f
#define O_S_L8_D_HII 1100.f
#define O_S_L9_S_HII 70000.f
#define O_S_L9_D_HII 1250.f
#define O_S_L10_S_HII 90000.f
#define O_S_L10_D_HII 1500.f
#define O_S_L11_S_HII 150000.f
#define O_S_L11_D_HII 1750.f
#define O_S_L12_S_HII 250000.f
#define O_S_L12_D_HII 2000.f

#define O_S_L1_S_ULT 50.f
#define O_S_L1_D_ULT 40.f
#define O_S_L2_S_ULT 500.f
#define O_S_L2_D_ULT 125.f
#define O_S_L3_S_ULT 1750.f
#define O_S_L3_D_ULT 175.f
#define O_S_L4_S_ULT 5250.f
#define O_S_L4_D_ULT 250.f
#define O_S_L5_S_ULT 25000.f
#define O_S_L5_D_ULT 300.f
#define O_S_L6_S_ULT 40000.f
#define O_S_L6_D_ULT 600.f
#define O_S_L7_S_ULT 50000.f
#define O_S_L7_D_ULT 800.f
#define O_S_L8_S_ULT 65000.f
#define O_S_L8_D_ULT 1000.f
#define O_S_L9_S_ULT 85000.f
#define O_S_L9_D_ULT 1200.f
#define O_S_L10_S_ULT 150000.f
#define O_S_L10_D_ULT 1500.f
#define O_S_L11_S_ULT 250000.f
#define O_S_L11_D_ULT 1750.f
#define O_S_L12_S_ULT 500000.f
#define O_S_L12_D_ULT 2000.f

// Dyn geometry optimization
#define O_D_L1_S_LOW 1.f  // geometry 3d volume size
#define O_D_L1_D_LOW 80.f // distance, after which it is not rendered
#define O_D_L2_S_LOW 3.f
#define O_D_L2_D_LOW 150.f
#define O_D_L3_S_LOW 4000.f
#define O_D_L3_D_LOW 250.f
#define O_D_L4_S_LOW 10000.f
#define O_D_L4_D_LOW 500.f
#define O_D_L5_S_LOW 25000.f
#define O_D_L5_D_LOW 750.f

#define O_D_L1_S_MED 2.f
#define O_D_L1_D_MED 40.f
#define O_D_L2_S_MED 4.f
#define O_D_L2_D_MED 100.f
#define O_D_L3_S_MED 4000.f
#define O_D_L3_D_MED 200.f
#define O_D_L4_S_MED 10000.f
#define O_D_L4_D_MED 400.f
#define O_D_L5_S_MED 25000.f
#define O_D_L5_D_MED 600.f

#define O_D_L1_S_HII 5.0f
#define O_D_L1_D_HII 30.f
#define O_D_L2_S_HII 10.f
#define O_D_L2_D_HII 80.f
#define O_D_L3_S_HII 4000.f
#define O_D_L3_D_HII 150.f
#define O_D_L4_S_HII 10000.f
#define O_D_L4_D_HII 300.f
#define O_D_L5_S_HII 25000.f
#define O_D_L5_D_HII 500.f

#define O_D_L1_S_ULT 7.5f
#define O_D_L1_D_ULT 30.f
#define O_D_L2_S_ULT 15.f
#define O_D_L2_D_ULT 50.f
#define O_D_L3_S_ULT 4000.f
#define O_D_L3_D_ULT 110.f
#define O_D_L4_S_ULT 10000.f
#define O_D_L4_D_ULT 250.f
#define O_D_L5_S_ULT 25000.f
#define O_D_L5_D_ULT 400.f

Fvector4 o_optimize_static_l1_dist = {O_S_L1_D_LOW, O_S_L1_D_MED, O_S_L1_D_HII, O_S_L1_D_ULT};
Fvector4 o_optimize_static_l1_size = {O_S_L1_S_LOW, O_S_L1_S_MED, O_S_L1_S_HII, O_S_L1_S_ULT};
Fvector4 o_optimize_static_l2_dist = {O_S_L2_D_LOW, O_S_L2_D_MED, O_S_L2_D_HII, O_S_L2_D_ULT};
Fvector4 o_optimize_static_l2_size = {O_S_L2_S_LOW, O_S_L2_S_MED, O_S_L2_S_HII, O_S_L2_S_ULT};
Fvector4 o_optimize_static_l3_dist = {O_S_L3_D_LOW, O_S_L3_D_MED, O_S_L3_D_HII, O_S_L3_D_ULT};
Fvector4 o_optimize_static_l3_size = {O_S_L3_S_LOW, O_S_L3_S_MED, O_S_L3_S_HII, O_S_L3_S_ULT};
Fvector4 o_optimize_static_l4_dist = {O_S_L4_D_LOW, O_S_L4_D_MED, O_S_L4_D_HII, O_S_L4_D_ULT};
Fvector4 o_optimize_static_l4_size = {O_S_L4_S_LOW, O_S_L4_S_MED, O_S_L4_S_HII, O_S_L4_S_ULT};
Fvector4 o_optimize_static_l5_dist = {O_S_L5_D_LOW, O_S_L5_D_MED, O_S_L5_D_HII, O_S_L5_D_ULT};
Fvector4 o_optimize_static_l5_size = {O_S_L5_S_LOW, O_S_L5_S_MED, O_S_L5_S_HII, O_S_L5_S_ULT};
Fvector4 o_optimize_static_l6_dist = {O_S_L6_D_LOW, O_S_L6_D_MED, O_S_L6_D_HII, O_S_L6_D_ULT};
Fvector4 o_optimize_static_l6_size = {O_S_L6_S_LOW, O_S_L6_S_MED, O_S_L6_S_HII, O_S_L6_S_ULT};
Fvector4 o_optimize_static_l7_dist = {O_S_L7_D_LOW, O_S_L7_D_MED, O_S_L7_D_HII, O_S_L7_D_ULT};
Fvector4 o_optimize_static_l7_size = {O_S_L7_S_LOW, O_S_L7_S_MED, O_S_L7_S_HII, O_S_L7_S_ULT};
Fvector4 o_optimize_static_l8_dist = {O_S_L8_D_LOW, O_S_L8_D_MED, O_S_L8_D_HII, O_S_L8_D_ULT};
Fvector4 o_optimize_static_l8_size = {O_S_L8_S_LOW, O_S_L8_S_MED, O_S_L8_S_HII, O_S_L8_S_ULT};
Fvector4 o_optimize_static_l9_dist = {O_S_L9_D_LOW, O_S_L9_D_MED, O_S_L9_D_HII, O_S_L9_D_ULT};
Fvector4 o_optimize_static_l9_size = {O_S_L9_S_LOW, O_S_L9_S_MED, O_S_L9_S_HII, O_S_L9_S_ULT};
Fvector4 o_optimize_static_l10_dist = {O_S_L10_D_LOW, O_S_L10_D_MED, O_S_L10_D_HII, O_S_L10_D_ULT};
Fvector4 o_optimize_static_l10_size = {O_S_L10_S_LOW, O_S_L10_S_MED, O_S_L10_S_HII, O_S_L10_S_ULT};
Fvector4 o_optimize_static_l11_dist = {O_S_L11_D_LOW, O_S_L11_D_MED, O_S_L11_D_HII, O_S_L11_D_ULT};
Fvector4 o_optimize_static_l11_size = {O_S_L11_S_LOW, O_S_L11_S_MED, O_S_L11_S_HII, O_S_L11_S_ULT};
Fvector4 o_optimize_static_l12_dist = {O_S_L12_D_LOW, O_S_L12_D_MED, O_S_L12_D_HII, O_S_L12_D_ULT};
Fvector4 o_optimize_static_l12_size = {O_S_L12_S_LOW, O_S_L12_S_MED, O_S_L12_S_HII, O_S_L12_S_ULT};

Fvector4 o_optimize_dynamic_l1_dist = {O_D_L1_D_LOW, O_D_L1_D_MED, O_D_L1_D_HII, O_D_L1_D_ULT};
Fvector4 o_optimize_dynamic_l1_size = {O_D_L1_S_LOW, O_D_L1_S_MED, O_D_L1_S_HII, O_D_L1_S_ULT};
Fvector4 o_optimize_dynamic_l2_dist = {O_D_L2_D_LOW, O_D_L2_D_MED, O_D_L2_D_HII, O_D_L2_D_ULT};
Fvector4 o_optimize_dynamic_l2_size = {O_D_L2_S_LOW, O_D_L2_S_MED, O_D_L2_S_HII, O_D_L2_S_ULT};
Fvector4 o_optimize_dynamic_l3_dist = {O_D_L3_D_LOW, O_D_L3_D_MED, O_D_L3_D_HII, O_D_L3_D_ULT};
Fvector4 o_optimize_dynamic_l3_size = {O_D_L3_S_LOW, O_D_L3_S_MED, O_D_L3_S_HII, O_D_L3_S_ULT};
Fvector4 o_optimize_dynamic_l4_dist = {O_D_L4_D_LOW, O_D_L4_D_MED, O_D_L4_D_HII, O_D_L4_D_ULT};
Fvector4 o_optimize_dynamic_l4_size = {O_D_L4_S_LOW, O_D_L4_S_MED, O_D_L4_S_HII, O_D_L4_S_ULT};
Fvector4 o_optimize_dynamic_l5_dist = {O_D_L5_D_LOW, O_D_L5_D_MED, O_D_L5_D_HII, O_D_L5_D_ULT};
Fvector4 o_optimize_dynamic_l5_size = {O_D_L5_S_LOW, O_D_L5_S_MED, O_D_L5_S_HII, O_D_L5_S_ULT};

#define BASE_FOV 67.f

IC float GetDistFromCamera(const Fvector& from_position)
// Aproximate, adjusted by fov, distance from camera to position (For right work when looking though binoculars and
// scopes)
{
	float distance = Device.vCameraPosition.distance_to(from_position);
	float fov_K = BASE_FOV / Device.fFOV;
	float adjusted_distane = distance / fov_K;

	return adjusted_distane;
}

IC bool IsValuableToRender(IRender_Visual* pVisual, bool isStatic, bool sm, Fmatrix& transform_matrix,
						   bool ignore_optimize = false)
{
	if (ignore_optimize)
		return true;

	if (true)
	{
		float sphere_volume = pVisual->vis.sphere.volume();

		float adjusted_distane = 0;

		if (isStatic)
			adjusted_distane = GetDistFromCamera(pVisual->vis.sphere.P);
		else
		// dynamic geometry position needs to be transformed by transform matrix, to get world coordinates, dont forget
		// ;)
		{
			Fvector pos;
			transform_matrix.transform_tiny(pos, pVisual->vis.sphere.P);

			adjusted_distane = GetDistFromCamera(pos);
		}

		if (sm) // Highest cut off for shadow map
		{
			if (sphere_volume < 50000.f && adjusted_distane > ps_r_sun_far)
				// don't need geometry behind the farest sun shadow cascade
				return false;

			if ((sphere_volume < o_optimize_static_l1_size.z) && (adjusted_distane > o_optimize_static_l1_dist.z))
				return false;
			else if ((sphere_volume < o_optimize_static_l2_size.z) && (adjusted_distane > o_optimize_static_l2_dist.z))
				return false;
			else if ((sphere_volume < o_optimize_static_l3_size.z) && (adjusted_distane > o_optimize_static_l3_dist.z))
				return false;
			else if ((sphere_volume < o_optimize_static_l4_size.z) && (adjusted_distane > o_optimize_static_l4_dist.z))
				return false;
			else if ((sphere_volume < o_optimize_static_l5_size.z) && (adjusted_distane > o_optimize_static_l5_dist.z))
				return false;
			else if ((sphere_volume < o_optimize_static_l6_size.z) && (adjusted_distane > o_optimize_static_l6_dist.z))
				return false;
			else if ((sphere_volume < o_optimize_static_l7_size.z) && (adjusted_distane > o_optimize_static_l7_dist.z))
				return false;
			else if ((sphere_volume < o_optimize_static_l8_size.z) && (adjusted_distane > o_optimize_static_l8_dist.z))
				return false;
			else if ((sphere_volume < o_optimize_static_l9_size.z) && (adjusted_distane > o_optimize_static_l9_dist.z))
				return false;
			else if ((sphere_volume < o_optimize_static_l10_size.z) &&
					 (adjusted_distane > o_optimize_static_l10_dist.z))
				return false;
			else if ((sphere_volume < o_optimize_static_l11_size.z) &&
					 (adjusted_distane > o_optimize_static_l11_dist.z))
				return false;
			else if ((sphere_volume < o_optimize_static_l12_size.z) &&
					 (adjusted_distane > o_optimize_static_l12_dist.z))
				return false;
		}

		if (isStatic)
		{
			/*if (ps_geometry_quality_mode == 3)
			{
				if ((sphere_volume < o_optimize_static_l1_size.y) && (adjusted_distane > o_optimize_static_l1_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_static_l2_size.y) &&
						 (adjusted_distane > o_optimize_static_l2_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_static_l3_size.y) &&
						 (adjusted_distane > o_optimize_static_l3_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_static_l4_size.y) &&
						 (adjusted_distane > o_optimize_static_l4_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_static_l5_size.y) &&
						 (adjusted_distane > o_optimize_static_l5_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_static_l6_size.y) &&
						 (adjusted_distane > o_optimize_static_l6_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_static_l7_size.y) &&
						 (adjusted_distane > o_optimize_static_l7_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_static_l8_size.y) &&
						 (adjusted_distane > o_optimize_static_l8_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_static_l9_size.y) &&
						 (adjusted_distane > o_optimize_static_l9_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_static_l10_size.y) &&
						 (adjusted_distane > o_optimize_static_l10_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_static_l11_size.y) &&
						 (adjusted_distane > o_optimize_static_l11_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_static_l12_size.y) &&
						 (adjusted_distane > o_optimize_static_l12_dist.y))
					return false;
			}
			else*/  if (ps_geometry_quality_mode == 2)
			{
				if ((sphere_volume < o_optimize_static_l1_size.z) && (adjusted_distane > o_optimize_static_l1_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_static_l2_size.z) &&
						 (adjusted_distane > o_optimize_static_l2_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_static_l3_size.z) &&
						 (adjusted_distane > o_optimize_static_l3_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_static_l4_size.z) &&
						 (adjusted_distane > o_optimize_static_l4_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_static_l5_size.z) &&
						 (adjusted_distane > o_optimize_static_l5_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_static_l6_size.z) &&
						 (adjusted_distane > o_optimize_static_l6_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_static_l7_size.z) &&
						 (adjusted_distane > o_optimize_static_l7_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_static_l8_size.z) &&
						 (adjusted_distane > o_optimize_static_l8_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_static_l9_size.z) &&
						 (adjusted_distane > o_optimize_static_l9_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_static_l10_size.z) &&
						 (adjusted_distane > o_optimize_static_l10_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_static_l11_size.z) &&
						 (adjusted_distane > o_optimize_static_l11_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_static_l12_size.z) &&
						 (adjusted_distane > o_optimize_static_l12_dist.z))
					return false;
			}
			else if (ps_geometry_quality_mode == 1)
			{
				if ((sphere_volume < o_optimize_static_l1_size.w) && (adjusted_distane > o_optimize_static_l1_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_static_l2_size.w) &&
						 (adjusted_distane > o_optimize_static_l2_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_static_l3_size.w) &&
						 (adjusted_distane > o_optimize_static_l3_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_static_l4_size.w) &&
						 (adjusted_distane > o_optimize_static_l4_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_static_l5_size.w) &&
						 (adjusted_distane > o_optimize_static_l5_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_static_l6_size.w) &&
						 (adjusted_distane > o_optimize_static_l6_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_static_l7_size.w) &&
						 (adjusted_distane > o_optimize_static_l7_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_static_l8_size.w) &&
						 (adjusted_distane > o_optimize_static_l8_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_static_l9_size.w) &&
						 (adjusted_distane > o_optimize_static_l9_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_static_l10_size.w) &&
						 (adjusted_distane > o_optimize_static_l10_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_static_l11_size.w) &&
						 (adjusted_distane > o_optimize_static_l11_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_static_l12_size.w) &&
						 (adjusted_distane > o_optimize_static_l12_dist.w))
					return false;
			}
			/*else
			{
				if ((sphere_volume < o_optimize_static_l1_size.x) && (adjusted_distane > o_optimize_static_l1_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_static_l2_size.x) &&
						 (adjusted_distane > o_optimize_static_l2_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_static_l3_size.x) &&
						 (adjusted_distane > o_optimize_static_l3_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_static_l4_size.x) &&
						 (adjusted_distane > o_optimize_static_l4_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_static_l5_size.x) &&
						 (adjusted_distane > o_optimize_static_l5_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_static_l6_size.x) &&
						 (adjusted_distane > o_optimize_static_l6_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_static_l7_size.x) &&
						 (adjusted_distane > o_optimize_static_l7_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_static_l8_size.x) &&
						 (adjusted_distane > o_optimize_static_l8_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_static_l9_size.x) &&
						 (adjusted_distane > o_optimize_static_l9_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_static_l10_size.x) &&
						 (adjusted_distane > o_optimize_static_l10_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_static_l11_size.x) &&
						 (adjusted_distane > o_optimize_static_l11_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_static_l12_size.x) &&
						 (adjusted_distane > o_optimize_static_l12_dist.x))
					return false;
			}*/
		}
		else
		{
			/*if (ps_geometry_quality_mode == 3)
			{
				if ((sphere_volume < o_optimize_dynamic_l1_size.y) && (adjusted_distane > o_optimize_dynamic_l1_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l2_size.y) &&
						 (adjusted_distane > o_optimize_dynamic_l2_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l3_size.y) &&
						 (adjusted_distane > o_optimize_dynamic_l3_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l4_size.y) &&
						 (adjusted_distane > o_optimize_dynamic_l4_dist.y))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l5_size.y) &&
						 (adjusted_distane > o_optimize_dynamic_l5_dist.y))
					return false;
			}
			else*/  if (ps_geometry_quality_mode == 2)
			{
				if ((sphere_volume < o_optimize_dynamic_l1_size.z) && (adjusted_distane > o_optimize_dynamic_l1_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l2_size.z) &&
						 (adjusted_distane > o_optimize_dynamic_l2_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l3_size.z) &&
						 (adjusted_distane > o_optimize_dynamic_l3_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l4_size.z) &&
						 (adjusted_distane > o_optimize_dynamic_l4_dist.z))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l5_size.z) &&
						 (adjusted_distane > o_optimize_dynamic_l5_dist.z))
					return false;
			}
			else if (ps_geometry_quality_mode == 1)
			{
				if ((sphere_volume < o_optimize_dynamic_l1_size.w) && (adjusted_distane > o_optimize_dynamic_l1_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l2_size.w) &&
						 (adjusted_distane > o_optimize_dynamic_l2_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l3_size.w) &&
						 (adjusted_distane > o_optimize_dynamic_l3_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l4_size.w) &&
						 (adjusted_distane > o_optimize_dynamic_l4_dist.w))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l5_size.w) &&
						 (adjusted_distane > o_optimize_dynamic_l5_dist.w))
					return false;
			}
			/*else
			{
				if ((sphere_volume < o_optimize_dynamic_l1_size.x) && (adjusted_distane > o_optimize_dynamic_l1_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l2_size.x) &&
						 (adjusted_distane > o_optimize_dynamic_l2_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l3_size.x) &&
						 (adjusted_distane > o_optimize_dynamic_l3_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l4_size.x) &&
						 (adjusted_distane > o_optimize_dynamic_l4_dist.x))
					return false;
				else if ((sphere_volume < o_optimize_dynamic_l5_size.x) &&
						 (adjusted_distane > o_optimize_dynamic_l5_dist.x))
					return false;
			}*/
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CRender::add_leafs_Dynamic(IRender_Visual* pVisual)
{
	//////OPTICK_EVENT("R_dsgraph_structure::add_leafs_Dynamic");

	if (0 == pVisual)
		return;

	if (!IsValuableToRender(pVisual, false, RenderImplementation.active_phase() == RenderImplementation.PHASE_SHADOW_DEPTH, *val_pTransform, false))
		return;

	// Visual is 100% visible - simply add it
	xr_vector<IRender_Visual*>::iterator I, E; // it may be useful for 'hierrarhy' visual

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP: {
		//////OPTICK_EVENT("R_dsgraph_structure::add_leafs_Dynamic - Particle group");
		// Add all children, doesn't perform any tests
		PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;
		for (PS::CParticleGroup::SItemVecIt i_it = pG->items.begin(); i_it != pG->items.end(); i_it++)
		{
			PS::CParticleGroup::SItem& PE_It = *i_it;
			if (PE_It._effect)
				add_leafs_Dynamic(PE_It._effect);
			for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_related.begin();
				 pit != PE_It._children_related.end(); pit++)
				add_leafs_Dynamic(*pit);
			for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_free.begin();
				 pit != PE_It._children_free.end();
				 pit++)
				add_leafs_Dynamic(*pit);
		}
	}
		return;
	case MT_HIERRARHY: {
		//////OPTICK_EVENT("R_dsgraph_structure::add_leafs_Dynamic - hierrarhy");
		// Add all children, doesn't perform any tests
		FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
		I = pV->children.begin();
		E = pV->children.end();
		for (; I != E; I++)
			add_leafs_Dynamic(*I);
	}
		return;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID: {
		//////OPTICK_EVENT("R_dsgraph_structure::add_leafs_Dynamic - skeleton");
		// Add all children, doesn't perform any tests
		CKinematics* pV = (CKinematics*)pVisual;
		BOOL _use_lod = FALSE;
		if (pV->m_lod)
		{
			Fvector Tpos;
			float D;
			val_pTransform->transform_tiny(Tpos, pV->vis.sphere.P);
			float ssa = CalcSSA(D, Tpos, pV->vis.sphere.R / 2.f); // assume dynamics never consume full sphere
			if (ssa < r_ssaLOD_A)
				_use_lod = TRUE;
		}
		if (_use_lod)
		{
			add_leafs_Dynamic(pV->m_lod);
		}
		else
		{
#pragma todo(NSDeathman to NSDeathman - разобратьс€)
			Fvector pos;
			val_pTransform->transform_tiny(pos, pVisual->vis.sphere.P);
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
				pV->CalculateWallmarks(); //. bug?
			}

			I = pV->children.begin();
			E = pV->children.end();
			for (; I != E; I++)
				add_leafs_Dynamic(*I);
		}
	}
		return;
	default: {
		// General type of visual
		// Calculate distance to it's center
		Fvector Tpos;
		val_pTransform->transform_tiny(Tpos, pVisual->vis.sphere.P);

		if (active_phase() == PHASE_NORMAL)
		{
			DReuseItem item;
			item.visual = pVisual;
			item.matrix = *val_pTransform; //  опируем текущую матрицу
			m_visuals_dynamic_visible.push_back(item);
		}

		r_dsgraph_insert_dynamic(pVisual, Tpos);
	}

		return;
	}
}

void CRender::add_leafs_Static(IRender_Visual* pVisual)
{
	//////OPTICK_EVENT("R_dsgraph_structure::add_leafs_Static");

	if (!HOM.visible(pVisual->vis))
		return;

	if (!IsValuableToRender(pVisual, true, RenderImplementation.active_phase() == RenderImplementation.PHASE_SHADOW_DEPTH, *val_pTransform, false))
		return;

	if (active_phase() == PHASE_NORMAL)
	{
		m_visuals_static_visible.push_back(pVisual);
	}

	// Visual is 100% visible - simply add it
	xr_vector<IRender_Visual*>::iterator I, E; // it may be usefull for 'hierrarhy' visuals

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP: {
		//////OPTICK_EVENT("R_dsgraph_structure::add_leafs_Static - Particle group");
		// Add all children, doesn't perform any tests
		PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;
		for (PS::CParticleGroup::SItemVecIt i_it = pG->items.begin(); i_it != pG->items.end(); i_it++)
		{
			PS::CParticleGroup::SItem& PE_It = *i_it;
			if (PE_It._effect)
				add_leafs_Dynamic(PE_It._effect);
			for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_related.begin();
				 pit != PE_It._children_related.end(); pit++)
				add_leafs_Dynamic(*pit);
			for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_free.begin();
				 pit != PE_It._children_free.end();
				 pit++)
				add_leafs_Dynamic(*pit);
		}
	}
		return;
	case MT_HIERRARHY: {
		//////OPTICK_EVENT("R_dsgraph_structure::add_leafs_Static - Hierrarhy");
		// Add all children, doesn't perform any tests
		FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
		I = pV->children.begin();
		E = pV->children.end();
		for (; I != E; I++)
			add_leafs_Static(*I);
	}
		return;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID: {
		//////OPTICK_EVENT("R_dsgraph_structure::add_leafs_Static - Skeleton");

		#pragma todo(NSDeathman to NSDeathman - разобратьс€)
		Fvector pos;
		val_pTransform->transform_tiny(pos, pVisual->vis.sphere.P);
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

		// Add all children, doesn't perform any tests
		CKinematics* pV = (CKinematics*)pVisual;
		if (adjusted_distane < switch_distance)
			pV->CalculateBones(TRUE);
		I = pV->children.begin();
		E = pV->children.end();
		for (; I != E; I++)
			add_leafs_Static(*I);
	}
		return;
	case MT_LOD: {
		//////OPTICK_EVENT("R_dsgraph_structure::add_leafs_Static - Lod");
		FLOD* pV = (FLOD*)pVisual;
		float D;
		float ssa = CalcSSA(D, pV->vis.sphere.P, pV);
		ssa *= pV->lod_factor;
		if (ssa < r_ssaLOD_A)
		{
			if (ssa < r_ssaDISCARD)
				return;
			mapLOD_Node* N = mapLOD.insertInAnyWay(D);
			N->val.ssa = ssa;
			N->val.pVisual = pVisual;
		}
		if (ssa > r_ssaLOD_B)
		{
			// Add all children, doesn't perform any tests
			I = pV->children.begin();
			E = pV->children.end();
			for (; I != E; I++)
				add_leafs_Static(*I);
		}
	}
		return;
	case MT_TREE_PM:
	case MT_TREE_ST: {
		//////OPTICK_EVENT("R_dsgraph_structure::add_leafs_Static - Tree");
		// General type of visual
		r_dsgraph_insert_static(pVisual);
	}
		return;
	default: {
		// General type of visual
		//////OPTICK_EVENT("R_dsgraph_structure::add_leafs_Static - static");
		r_dsgraph_insert_static(pVisual);
	}
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CRender::add_Dynamic(IRender_Visual* pVisual, u32 planes)
{
	PROFILE_FUNCTION();

	// Check frustum visibility and calculate distance to visual's center
	Fvector Tpos; // transformed position
	EFC_Visible VIS;

	val_pTransform->transform_tiny(Tpos, pVisual->vis.sphere.P);
	VIS = View->testSphere(Tpos, pVisual->vis.sphere.R, planes);
	if (fcvNone == VIS)
		return FALSE;

	if (!IsValuableToRender(pVisual, false, RenderImplementation.active_phase() == RenderImplementation.PHASE_SHADOW_DEPTH, *val_pTransform, false))
		return FALSE;

	// If we get here visual is visible or partially visible
	xr_vector<IRender_Visual*>::iterator I, E; // it may be usefull for 'hierrarhy' visuals

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP: {
		//OPTICK_EVENT("MT_PARTICLE_GROUP");
		// Add all children, doesn't perform any tests
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
					 pit != PE_It._children_free.end();
					 pit++)
					add_Dynamic(*pit, planes);
			}
			else
			{
				if (PE_It._effect)
					add_leafs_Dynamic(PE_It._effect);
				for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_related.begin();
					 pit != PE_It._children_related.end(); pit++)
					add_leafs_Dynamic(*pit);
				for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_free.begin();
					 pit != PE_It._children_free.end();
					 pit++)
					add_leafs_Dynamic(*pit);
			}
		}
	}
	break;
	case MT_HIERRARHY: {
		//OPTICK_EVENT("MT_HIERRARHY");
		// Add all children
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
				add_leafs_Dynamic(*I);
		}
	}
	break;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID: {
		//OPTICK_EVENT("MT_SKELETON_ANIM - MT_SKELETON_RIGID");
		// Add all children, doesn't perform any tests
		CKinematics* pV = (CKinematics*)pVisual;
		BOOL _use_lod = FALSE;
		if (pV->m_lod)
		{
			Fvector fTpos;
			float D;
			val_pTransform->transform_tiny(fTpos, pV->vis.sphere.P);
			float ssa = CalcSSA(D, fTpos, pV->vis.sphere.R / 2.f); // assume dynamics never consume full sphere
			if (ssa < r_ssaLOD_A)
				_use_lod = TRUE;
		}
		if (_use_lod)
		{
			add_leafs_Dynamic(pV->m_lod);
		}
		else
		{
#pragma todo(NSDeathman to NSDeathman - разобратьс€)
			Fvector pos;
			val_pTransform->transform_tiny(pos, pVisual->vis.sphere.P);
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
				pV->CalculateWallmarks(); //. bug?
			}

			I = pV->children.begin();
			E = pV->children.end();
			for (; I != E; I++)
				add_leafs_Dynamic(*I);
		}
		/*
		I = pV->children.begin		();
		E = pV->children.end		();
		if (fcvPartial==VIS) {
			for (; I!=E; I++)	add_Dynamic			(*I,planes);
		} else {
			for (; I!=E; I++)	add_leafs_Dynamic	(*I);
		}
		*/
	}
	break;
	default: {
		//OPTICK_EVENT("default");
		// General type of visual
		r_dsgraph_insert_dynamic(pVisual, Tpos);
	}
	break;
	}
	return TRUE;
}

void CRender::add_Static(IRender_Visual* pVisual, u32 planes)
{
	PROFILE_FUNCTION();

	// Check frustum visibility and calculate distance to visual's center
	EFC_Visible VIS;
	vis_data& vis = pVisual->vis;
	VIS = View->testSAABB(vis.sphere.P, vis.sphere.R, vis.box.data(), planes);
	if (fcvNone == VIS)
		return;
	if (!HOM.visible(vis))
		return;

	if (!IsValuableToRender(pVisual, true, RenderImplementation.active_phase() == RenderImplementation.PHASE_SHADOW_DEPTH, *val_pTransform, false))
		return;

	// If we get here visual is visible or partially visible
	xr_vector<IRender_Visual*>::iterator I, E; // it may be usefull for 'hierrarhy' visuals

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP: {
		//OPTICK_EVENT("MT_PARTICLE_GROUP");
		// Add all children, doesn't perform any tests
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
					 pit != PE_It._children_free.end();
					 pit++)
					add_Dynamic(*pit, planes);
			}
			else
			{
				if (PE_It._effect)
					add_leafs_Dynamic(PE_It._effect);
				for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_related.begin();
					 pit != PE_It._children_related.end(); pit++)
					add_leafs_Dynamic(*pit);
				for (xr_vector<IRender_Visual*>::iterator pit = PE_It._children_free.begin();
					 pit != PE_It._children_free.end();
					 pit++)
					add_leafs_Dynamic(*pit);
			}
		}
	}
	break;
	case MT_HIERRARHY: {
		//OPTICK_EVENT("MT_HIERRARHY");
		// Add all children
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
				add_leafs_Static(*I);
		}
	}
	break;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID: {
		//OPTICK_EVENT("SKELETON");
#pragma todo(NSDeathman to NSDeathman - разобратьс€)
		Fvector pos;
		val_pTransform->transform_tiny(pos, pVisual->vis.sphere.P);
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

		// Add all children, doesn't perform any tests
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
				add_leafs_Static(*I);
		}
	}
	break;
	case MT_LOD: {
		//OPTICK_EVENT("MT_LOD");
		FLOD* pV = (FLOD*)pVisual;
		float D;
		float ssa = CalcSSA(D, pV->vis.sphere.P, pV);
		ssa *= pV->lod_factor;
		if (ssa < r_ssaLOD_A)
		{
			if (ssa < r_ssaDISCARD)
				return;
			mapLOD_Node* N = mapLOD.insertInAnyWay(D);
			N->val.ssa = ssa;
			N->val.pVisual = pVisual;
		}
		if (ssa > r_ssaLOD_B)
		{
			// Add all children, perform tests
			I = pV->children.begin();
			E = pV->children.end();
			for (; I != E; I++)
				add_leafs_Static(*I);
		}
	}
	break;
	case MT_TREE_ST:
	case MT_TREE_PM: {
		//OPTICK_EVENT("TREE");
		// General type of visual
		r_dsgraph_insert_static(pVisual);
	}
		return;
	default: {
		//OPTICK_EVENT("default");
		// General type of visual
		r_dsgraph_insert_static(pVisual);
	}
	break;
	}
}
