#pragma once

#include "..\xrEngine\render.h"
#include "..\xrEngine\ispatial.h"
#include "r_dsgraph_types.h"
#include "r_sector.h"

// Forward declaration
class CRender;

//////////////////////////////////////////////////////////////////////////
// feedback	for receiving visuals										//
//////////////////////////////////////////////////////////////////////////
class R_feedback
{
  public:
	virtual void rfeedback_static(IRender_Visual* V) = 0;
};

//////////////////////////////////////////////////////////////////////////
// Структура для хранения и сортировки рендер-элементов (Scene Graph)
//////////////////////////////////////////////////////////////////////////
class CSceneGraph
{
  public:
	IRenderable* val_pObject;
	Fmatrix* val_pTransform;
	BOOL val_bHUD;
	BOOL val_bInvisible;
	BOOL val_bRecordMP;									 // record nearest for multi-pass
	R_feedback* val_feedback;							 // feedback for geometry being rendered
	u32 val_feedback_breakp;							 // breakpoint
	xr_vector<Fbox3, render_alloc<Fbox3>>* val_recorder; // coarse structure recorder
	u32 render_phase;
	u32 marker;
	bool pmask[2];
	bool pmask_wmark;

  public:
	// Dynamic scene graph
	R_dsgraph::mapNormal_T mapNormal[2]; // 2==(priority/2)
	R_dsgraph::mapMatrix_T mapMatrix[2];
	R_dsgraph::mapSorted_T mapSorted;
	R_dsgraph::mapHUD_T mapHUD;
	R_dsgraph::mapLOD_T mapLOD;
	R_dsgraph::mapSorted_T mapDistort;
	R_dsgraph::mapSorted_T mapWmark; // sorted
	R_dsgraph::mapSorted_T mapEmissive;

	// Runtime structures
	xr_vector<R_dsgraph::mapNormalVS::TNode*, render_alloc<R_dsgraph::mapNormalVS::TNode*>> nrmVS;
	xr_vector<R_dsgraph::mapNormalPS::TNode*, render_alloc<R_dsgraph::mapNormalPS::TNode*>> nrmPS;
	xr_vector<R_dsgraph::mapNormalCS::TNode*, render_alloc<R_dsgraph::mapNormalCS::TNode*>> nrmCS;
	xr_vector<R_dsgraph::mapNormalStates::TNode*, render_alloc<R_dsgraph::mapNormalStates::TNode*>> nrmStates;
	xr_vector<R_dsgraph::mapNormalTextures::TNode*, render_alloc<R_dsgraph::mapNormalTextures::TNode*>> nrmTextures;
	xr_vector<R_dsgraph::mapNormalTextures::TNode*, render_alloc<R_dsgraph::mapNormalTextures::TNode*>> nrmTexturesTemp;

	xr_vector<R_dsgraph::mapMatrixVS::TNode*, render_alloc<R_dsgraph::mapMatrixVS::TNode*>> matVS;
	xr_vector<R_dsgraph::mapMatrixPS::TNode*, render_alloc<R_dsgraph::mapMatrixPS::TNode*>> matPS;
	xr_vector<R_dsgraph::mapMatrixCS::TNode*, render_alloc<R_dsgraph::mapMatrixCS::TNode*>> matCS;
	xr_vector<R_dsgraph::mapMatrixStates::TNode*, render_alloc<R_dsgraph::mapMatrixStates::TNode*>> matStates;
	xr_vector<R_dsgraph::mapMatrixTextures::TNode*, render_alloc<R_dsgraph::mapMatrixTextures::TNode*>> matTextures;
	xr_vector<R_dsgraph::mapMatrixTextures::TNode*, render_alloc<R_dsgraph::mapMatrixTextures::TNode*>> matTexturesTemp;

	xr_vector<R_dsgraph::_LodItem, render_alloc<R_dsgraph::_LodItem>> lstLODs;
	xr_vector<int, render_alloc<int>> lstLODgroups;
	xr_vector<ISpatial* /**,render_alloc<ISpatial*>/**/> lstRenderables;
	xr_vector<ISpatial* /**,render_alloc<ISpatial*>/**/> lstSpatial;
	xr_vector<IRender_Visual*, render_alloc<IRender_Visual*>> lstVisuals;

	xr_vector<IRender_Visual*, render_alloc<IRender_Visual*>> lstRecorded;

	u32 counter_S;
	u32 counter_D;
	BOOL b_loaded;

	// Списки видимых объектов за текущий кадр
	xr_vector<IRender_Visual*> m_visuals_static_visible;

	struct DReuseItem
	{
		IRender_Visual* visual;
		Fmatrix matrix;
	};
	xr_vector<DReuseItem> m_visuals_dynamic_visible;

  public:
	// Методы управления состоянием (ранее были виртуальными из IRender_interface)
	void set_Transform(Fmatrix* M)
	{
		VERIFY(M);
		val_pTransform = M;
	}
	void set_HUD(BOOL V)
	{
		val_bHUD = V;
	}
	BOOL get_HUD()
	{
		return val_bHUD;
	}
	void set_Invisible(BOOL V)
	{
		val_bInvisible = V;
	}
	void set_Feedback(R_feedback* V, u32 id)
	{
		val_feedback_breakp = id;
		val_feedback = V;
	}
	void set_Recorder(xr_vector<Fbox3, render_alloc<Fbox3>>* dest)
	{
		val_recorder = dest;
		if (dest)
			dest->clear();
	}
	void get_Counters(u32& s, u32& d)
	{
		s = counter_S;
		d = counter_D;
	}
	void clear_Counters()
	{
		counter_S = counter_D = 0;
	}

  public:
	CSceneGraph()
	{
		val_pObject = NULL;
		val_pTransform = NULL;
		val_bHUD = FALSE;
		val_bInvisible = FALSE;
		val_bRecordMP = FALSE;
		val_feedback = 0;
		val_feedback_breakp = 0;
		val_recorder = 0;
		marker = 0;
		r_pmask(true, true);
		b_loaded = FALSE;
	};

	void destroy()
	{
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

		mapNormal[0].destroy();
		mapNormal[1].destroy();
		mapMatrix[0].destroy();
		mapMatrix[1].destroy();
		mapSorted.destroy();
		mapHUD.destroy();
		mapLOD.destroy();
		mapDistort.destroy();
		mapWmark.destroy();
		mapEmissive.destroy();
	}

	void r_pmask(bool _1, bool _2, bool _wm = false)
	{
		pmask[0] = _1;
		pmask[1] = _2;
		pmask_wmark = _wm;
	}

	void insert_dynamic(IRender_Visual* pVisual, Fvector& Center);
	void insert_static(IRender_Visual* pVisual);

	void render_graph(u32 _priority, bool _clear = true);
	void render_hud();
	void render_lods(bool _setup_zb, bool _clear);
	void render_sorted();
	void render_emissive();
	void render_wmarks();
	void render_distort();

	void render_subspace(IRender_Sector* _sector, CFrustum* _frustum, Fmatrix& mCombined, Fvector& _cop,
								   BOOL _dynamic, BOOL _precise_portals = FALSE);
	void render_subspace(IRender_Sector* _sector, Fmatrix& mCombined, Fvector& _cop, BOOL _dynamic,
								   BOOL _precise_portals = FALSE);

	// Вспомогательная функция для переиспользования списков отрисовки
	void render_reuse();

	BOOL add_Dynamic(IRender_Visual* pVisual, u32 planes); // normal processing
	void add_Static(IRender_Visual* pVisual, u32 planes);
	void add_leafs_Dynamic(IRender_Visual* pVisual); // if detected node's full visibility
	void add_leafs_Static(IRender_Visual* pVisual);	 // if detected node's full visibility
};
