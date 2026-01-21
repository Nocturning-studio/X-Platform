#pragma once

// Включаем необходимые хидеры
#include "..\xrEngine\render.h"
#include "..\xrEngine\ispatial.h"
#include "SceneGraphTypes.h"
#include "r_sector.h"

// Forward declaration
class CRender;
class IRender_Visual; // На всякий случай

// Enum для типов рендеринга графа
enum class SceneGraphRenderType
{
	Opaque,		 // Обычная геометрия (бывший render_graph)
	Transparent, // Alpha (бывший render_sorted)
	HUD,		 // Оружие и руки
	LOD,		 // LODы деревьев
	Emissive,	 // Светящиеся объекты
	Wallmarks,	 // Следы от пуль и т.д.
	Distortion	 // Искажения
};

struct SceneGraphFetchConfig
{
	bool fetch_priority_0 : 1; // Priority 0 (Base Opaque)
	bool fetch_priority_1 : 1; // Priority 1 (Secondary/AlphaTest)
	bool fetch_wallmarks : 1;  // Wallmarks

	// Конструктор по умолчанию (все включено, как в старом коде по дефолту)
	SceneGraphFetchConfig() : fetch_priority_0(true), fetch_priority_1(true), fetch_wallmarks(false)
	{
	}

	SceneGraphFetchConfig(bool p0, bool p1, bool wm) : fetch_priority_0(p0), fetch_priority_1(p1), fetch_wallmarks(wm)
	{
	}
};

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
	// == PUBLIC DATA MEMBERS (TODO: Encapsulate later) ==
	IRenderable* m_current_owner;
	Fmatrix* m_current_xform;
	BOOL m_is_hud_pass;
	BOOL m_is_invisible_mode;
	BOOL m_record_multipass;									 // record nearest for multi-pass
	R_feedback* m_feedback_interface;							 // feedback for geometry being rendered
	u32 val_feedback_breakp;							 // breakpoint
	xr_vector<Fbox3, render_alloc<Fbox3>>* m_culling_bounds_recorder; // coarse structure recorder
	u32 render_phase;
	u32 m_traversal_marker;
	SceneGraphFetchConfig m_fetch_config;

  public:
	// Dynamic scene graph containers
	SceneGraphTypes::mapNormal_T m_queue_static[2]; // 2==(priority/2)
	SceneGraphTypes::mapMatrix_T m_queue_dynamic[2];
	SceneGraphTypes::mapSorted_T m_queue_transparent;
	SceneGraphTypes::mapHUD_T m_queue_hud;
	SceneGraphTypes::mapLOD_T mapLOD;
	SceneGraphTypes::mapSorted_T m_queue_distortion;
	SceneGraphTypes::mapSorted_T m_queue_wallmarks; // sorted
	SceneGraphTypes::mapSorted_T mapEmissive;

	// Runtime structures (Lists for sorting)
	xr_vector<SceneGraphTypes::mapNormalVS::TNode*, render_alloc<SceneGraphTypes::mapNormalVS::TNode*>> nrmVS;
	xr_vector<SceneGraphTypes::mapNormalPS::TNode*, render_alloc<SceneGraphTypes::mapNormalPS::TNode*>> nrmPS;
	xr_vector<SceneGraphTypes::mapNormalCS::TNode*, render_alloc<SceneGraphTypes::mapNormalCS::TNode*>> nrmCS;
	xr_vector<SceneGraphTypes::mapNormalStates::TNode*, render_alloc<SceneGraphTypes::mapNormalStates::TNode*>> nrmStates;
	xr_vector<SceneGraphTypes::mapNormalTextures::TNode*, render_alloc<SceneGraphTypes::mapNormalTextures::TNode*>> nrmTextures;
	xr_vector<SceneGraphTypes::mapNormalTextures::TNode*, render_alloc<SceneGraphTypes::mapNormalTextures::TNode*>> nrmTexturesTemp;

	xr_vector<SceneGraphTypes::mapMatrixVS::TNode*, render_alloc<SceneGraphTypes::mapMatrixVS::TNode*>> matVS;
	xr_vector<SceneGraphTypes::mapMatrixPS::TNode*, render_alloc<SceneGraphTypes::mapMatrixPS::TNode*>> matPS;
	xr_vector<SceneGraphTypes::mapMatrixCS::TNode*, render_alloc<SceneGraphTypes::mapMatrixCS::TNode*>> matCS;
	xr_vector<SceneGraphTypes::mapMatrixStates::TNode*, render_alloc<SceneGraphTypes::mapMatrixStates::TNode*>> matStates;
	xr_vector<SceneGraphTypes::mapMatrixTextures::TNode*, render_alloc<SceneGraphTypes::mapMatrixTextures::TNode*>> matTextures;
	xr_vector<SceneGraphTypes::mapMatrixTextures::TNode*, render_alloc<SceneGraphTypes::mapMatrixTextures::TNode*>> matTexturesTemp;

	xr_vector<SceneGraphTypes::LodRenderNode, render_alloc<SceneGraphTypes::LodRenderNode>> lstLODs;
	xr_vector<int, render_alloc<int>> lstLODgroups;

	// Lists populated during traversal
	xr_vector<ISpatial* /**,render_alloc<ISpatial*>/**/> lstRenderables;
	xr_vector<ISpatial* /**,render_alloc<ISpatial*>/**/> lstSpatial;
	xr_vector<IRender_Visual*, render_alloc<IRender_Visual*>> lstVisuals;
	xr_vector<IRender_Visual*, render_alloc<IRender_Visual*>> lstRecorded;

	u32 counter_S;
	u32 counter_D;
	BOOL b_loaded;

	// Списки видимых объектов за текущий кадр (для Re-use)
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
		m_current_xform = M;
	}
	void set_HUD(BOOL V)
	{
		m_is_hud_pass = V;
	}
	BOOL get_HUD()
	{
		return m_is_hud_pass;
	}
	void set_Invisible(BOOL V)
	{
		m_is_invisible_mode = V;
	}
	void set_Feedback(R_feedback* V, u32 id)
	{
		val_feedback_breakp = id;
		m_feedback_interface = V;
	}
	void SetCullingBoundsCollector(xr_vector<Fbox3, render_alloc<Fbox3>>* dest)
	{
		m_culling_bounds_recorder = dest;
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
	CSceneGraph(); // Конструктор

	void destroy(); // Деструктор/Очистка ресурсов

	void SetFetchConfig(const SceneGraphFetchConfig& config);

	// Низкоуровневая вставка в граф (используется внутри add_leafs)
	void EnqueueDynamic(IRender_Visual* pVisual, Fvector& Center);
	void EnqueueStatic(IRender_Visual* pVisual);

	// Методы обхода пространства (Subspace traversal)
	// Первая версия принимает Frustum явно
	void render_subspace(IRender_Sector* _sector, CFrustum* _frustum, Fmatrix& mCombined, Fvector& _cop, BOOL _dynamic,
						 BOOL _precise_portals = FALSE);
	// Вторая версия создает Frustum из матрицы
	void render_subspace(IRender_Sector* _sector, Fmatrix& mCombined, Fvector& _cop, BOOL _dynamic,
						 BOOL _precise_portals = FALSE);

	// Вспомогательная функция для переиспользования списков отрисовки
	void render_reuse();

	// Проверка на значимость для рендера (LOD, distance cull)
	bool ShouldRenderVisual(IRender_Visual* pVisual, bool isStatic, bool ignore_optimize);

	// Методы добавления геометрии (Building Phase)
	BOOL add_Dynamic(IRender_Visual* pVisual, u32 planes); // normal processing (с проверкой фрустума)
	void add_Static(IRender_Visual* pVisual, u32 planes);
	void ProcessDynamicVisual(IRender_Visual* pVisual); // если нода полностью видима
	void ProcessStaticVisual(IRender_Visual* pVisual);	 // если нода полностью видима

	// =========================================================================
	//  Unified Render Method
	// =========================================================================
	// Единая точка входа для отрисовки разных типов геометрии
	void Render(SceneGraphRenderType type, u32 priority = 0, bool clear = true, bool setup_zb = true);

  private:
	// Внутренние реализации методов рендеринга
	void _RenderOpaque(u32 priority, bool clear);
	void _RenderHUD();
	void _RenderTranslucent(); // ex sorted
	void _RenderLODs(bool setup_zb, bool clear);
	void _RenderEmissive();
	void _RenderWmarks();
	void _RenderDistortion();
};
