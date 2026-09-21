#pragma once

#include <unordered_map>

#include "..\xrEngine\IRender.h"
#include "..\xrEngine\ispatial.h"
#include "SceneGraphTypes.h"
#include "r_sector.h"
#include "r_portal.h"
#include "r_portal_traverser.h"
#include <xrEngine/FBasicVisual.h>
#include "RenderSceneFlags.h"

class CRender;
class IRender_Visual;
class light;

class R_feedback
{
  public:
	virtual void rfeedback_static(IRender_Visual* V) = 0;
};

////////////////////////////////////////////////////////////////////////////////
//  Render Packet (Thread-Safe Data Buffer)
//  Хранит только очереди отрисовки. Может быть локальным для потока.
////////////////////////////////////////////////////////////////////////////////
struct SceneGraphPacket
{
	// Dynamic scene graph containers
	SceneGraphTypes::mapNormal_T queue_static[2]; // [0] = deffered lighting, [1] = forward lighting
	SceneGraphTypes::mapMatrix_T queue_dynamic[2];
	SceneGraphTypes::mapSorted_T queue_transparent;
	SceneGraphTypes::mapHUD_T queue_hud;
	SceneGraphTypes::mapLOD_T mapLOD;
	SceneGraphTypes::mapSorted_T queue_distortion;
	SceneGraphTypes::mapSorted_T queue_wallmarks;
	SceneGraphTypes::mapSorted_T mapEmissive;

	// Добавляем персональный обходчик порталов для этого пакета
	CPortalTraverser portal_traverser;

	// Списки для LOD (данные наполнения)
	xr_vector<SceneGraphTypes::LodRenderNode, render_alloc<SceneGraphTypes::LodRenderNode>> lstLODs;
	xr_vector<int, render_alloc<int>> lstLODgroups;

	// Result lists
	xr_vector<ISpatial*> m_spatial_query_results;
	xr_vector<IRender_Visual*, render_alloc<IRender_Visual*>> m_visuals_static_visible;

	std::unordered_map<IRender_Sector*, const CPortalTraverser::SectorVisibility*> visible_sectors_map;

	struct DReuseItem
	{
		IRender_Visual* visual;
		fmat4x4 matrix;
	};
	xr_vector<DReuseItem> m_visuals_dynamic_visible;

	xr_vector<IRenderable*> m_culled_dynamics;
	xr_vector<light*> m_culled_lights;

	xr_vector<IRender_Visual*> m_visual_refs;

	IC void AddVisualRef(IRender_Visual* V)
	{
		if(!V)
			return;
		V->AddRef();
		m_visual_refs.push_back(V);
	}

	void ReleaseVisualRefs()
	{
		for(IRender_Visual* V : m_visual_refs)
			V->ReleaseRef();
		m_visual_refs.clear();
	}

	xrCriticalSection cs;

	SceneGraphPacket()
	{
		portal_traverser.Reset();
	}

	void InitResources()
	{
		portal_traverser.CreateResources();
	}

	void FreeResources()
	{
		portal_traverser.DestroyResources();
	}

	void Clear()
	{
		ReleaseVisualRefs();
		m_culled_lights.clear();
		m_culled_dynamics.clear();
		portal_traverser.Reset();
		visible_sectors_map.clear();
		queue_static[0].clear();
		queue_static[1].clear();
		queue_dynamic[0].clear();
		queue_dynamic[1].clear();
		queue_transparent.clear();
		queue_hud.clear();
		mapLOD.clear();
		queue_distortion.clear();
		queue_wallmarks.clear();
		mapEmissive.clear();

		lstLODs.clear();
		lstLODgroups.clear();
		m_spatial_query_results.clear();
		m_visuals_static_visible.clear();
		m_visuals_dynamic_visible.clear();
	}

	void Destroy()
	{
		ReleaseVisualRefs();
		queue_static[0].destroy();
		queue_static[1].destroy();
		queue_dynamic[0].destroy();
		queue_dynamic[1].destroy();
		queue_transparent.destroy();
		queue_hud.destroy();
		mapLOD.destroy();
		queue_distortion.destroy();
		queue_wallmarks.destroy();
		mapEmissive.destroy();
	}
};

////////////////////////////////////////////////////////////////////////////////
//  Scratch Pad (Working Buffers)
//  Используется только при рендеринге (сортировке/флаттенинге)
//  Чтобы не переаллоцировать вектора каждый кадр.
////////////////////////////////////////////////////////////////////////////////
struct SceneGraphScratchPad
{
	// Static Geometry Sorting Buffers
	xr_vector<SceneGraphTypes::mapNormalVS::TNode*, render_alloc<SceneGraphTypes::mapNormalVS::TNode*>> nrmVS;
	xr_vector<SceneGraphTypes::mapNormalPS::TNode*, render_alloc<SceneGraphTypes::mapNormalPS::TNode*>> nrmPS;
	xr_vector<SceneGraphTypes::mapNormalCS::TNode*, render_alloc<SceneGraphTypes::mapNormalCS::TNode*>> nrmCS;
	xr_vector<SceneGraphTypes::mapNormalStates::TNode*, render_alloc<SceneGraphTypes::mapNormalStates::TNode*>> nrmStates;
	xr_vector<SceneGraphTypes::mapNormalTextures::TNode*, render_alloc<SceneGraphTypes::mapNormalTextures::TNode*>> nrmTextures;
	xr_vector<SceneGraphTypes::mapNormalTextures::TNode*, render_alloc<SceneGraphTypes::mapNormalTextures::TNode*>> nrmTexturesTemp;

	// Dynamic Geometry Sorting Buffers
	xr_vector<SceneGraphTypes::mapMatrixVS::TNode*, render_alloc<SceneGraphTypes::mapMatrixVS::TNode*>> matVS;
	xr_vector<SceneGraphTypes::mapMatrixPS::TNode*, render_alloc<SceneGraphTypes::mapMatrixPS::TNode*>> matPS;
	xr_vector<SceneGraphTypes::mapMatrixCS::TNode*, render_alloc<SceneGraphTypes::mapMatrixCS::TNode*>> matCS;
	xr_vector<SceneGraphTypes::mapMatrixStates::TNode*, render_alloc<SceneGraphTypes::mapMatrixStates::TNode*>> matStates;
	xr_vector<SceneGraphTypes::mapMatrixTextures::TNode*, render_alloc<SceneGraphTypes::mapMatrixTextures::TNode*>> matTextures;
	xr_vector<SceneGraphTypes::mapMatrixTextures::TNode*, render_alloc<SceneGraphTypes::mapMatrixTextures::TNode*>> matTexturesTemp;

	void Clear()
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
	}
};

////////////////////////////////////////////////////////////////////////////////
//  Traversal Context
//  Состояние, которое меняется в процессе обхода.
//  В будущем это должно передаваться аргументом, а не лежать в классе.
////////////////////////////////////////////////////////////////////////////////
struct SceneTraversalContext
{
	IRenderable* owner;
	fmat4x4* transform;
	const CFrustum* frustum;
	BOOL is_hud_pass;
	BOOL is_invisible_mode;
	u32 traversal_marker_id;
	u32 render_phase;
	CRenderView RenderView;
	bool use_hom;
	bool use_feedback;
	SceneRenderFlags fetch_flags = SceneRenderPresets::GatherMainView;
	xr_vector<Fbox3, render_alloc<Fbox3>>* culling_bounds;

	SceneTraversalContext()
		: owner(nullptr),
		  transform(nullptr),
		  frustum(nullptr),
		  is_hud_pass(FALSE),
		  is_invisible_mode(FALSE),
		  traversal_marker_id(0),
		  render_phase(0),
		  RenderView(),
		  use_hom(true),
		  use_feedback(false),
		  fetch_flags(),
		  culling_bounds(nullptr)
	{
	}

	IC bool has(SceneRenderFlags f) const noexcept { return (static_cast<u32>(fetch_flags) & static_cast<u32>(f)) != 0u; }
	IC bool hasnt(SceneRenderFlags f) const noexcept { return (static_cast<u32>(fetch_flags) & static_cast<u32>(f)) == 0u; }
};

class CurrentRenderContext
{
  public:
	// Эти переменные уникальны для каждого потока
	static thread_local SceneGraphPacket* packet;
	static thread_local SceneTraversalContext* context;

	// Helper RAII для безопасной установки контекста в скоупе
	struct Scope
	{
		SceneGraphPacket* prev_packet;
		SceneTraversalContext* prev_context;

		Scope(SceneGraphPacket& p, SceneTraversalContext& c)
		{
			prev_packet = CurrentRenderContext::packet;
			prev_context = CurrentRenderContext::context;

			CurrentRenderContext::packet = &p;
			CurrentRenderContext::context = &c;
		}

		~Scope()
		{
			CurrentRenderContext::packet = prev_packet;
			CurrentRenderContext::context = prev_context;
		}
	};
};

////////////////////////////////////////////////////////////////////////////////
//  CSceneGraph Class
////////////////////////////////////////////////////////////////////////////////
class CSceneGraph
{
  public:
	SceneGraphPacket m_packet;

	SceneGraphScratchPad m_scratch;

	R_feedback* m_feedback_interface;
	u32 val_feedback_breakp;
	std::atomic<u32> m_traversal_marker;

	std::atomic<u32> counter_S;
	std::atomic<u32> counter_D;
	BOOL b_loaded;

	friend class CRender;

  public:
	CSceneGraph();
	void destroy();

	// === State Management ===
	void set_Feedback(R_feedback* V, u32 id)
	{
		val_feedback_breakp = id;
		m_feedback_interface = V;
	}

	void PrepareDynamicInstances(SceneGraphPacket& packet, const SceneTraversalContext& gather_ctx);

	void DebugCheckDuplicateVisuals(SceneGraphPacket& packet);

	void get_Counters(u32& s, u32& d)
	{
		s = counter_S;
		d = counter_D;
	}
	void clear_Counters()
	{
		counter_S = counter_D = 0;
	}

	// === Insertion API (Updated Signatures) ===
	BOOL add_Dynamic(IRender_Visual* pVisual, u32 planes, const SceneTraversalContext& ctx, SceneGraphPacket& dest);
	void add_Static(IRender_Visual* pVisual, u32 planes, const SceneTraversalContext& ctx, SceneGraphPacket& dest);

	void ProcessDynamicVisual(IRender_Visual* pVisual, const SceneTraversalContext& ctx, SceneGraphPacket& dest);
	void ProcessStaticVisual(IRender_Visual* pVisual, const SceneTraversalContext& ctx, SceneGraphPacket& dest);

	// Low-level insertion
	void EnqueueDynamic(IRender_Visual* pVisual, fvec3& Center, const SceneTraversalContext& ctx, SceneGraphPacket& dest);
	void EnqueueStatic(IRender_Visual* pVisual, const SceneTraversalContext& ctx, SceneGraphPacket& dest);

	// Helper
	bool ShouldRenderVisual(IRender_Visual* pVisual, bool isStatic, bool ignore_optimize, const SceneTraversalContext& ctx);

	// === Rendering API ===
	void Render(SceneGraphPacket& packet, SceneRenderFlags flags, bool clear = true, bool setup_zb = true);
	void RenderFromCache(const SceneTraversalContext& initial_ctx, SceneGraphPacket& packet);

  private:
	// Render implementations
	void _RenderStatic(SceneGraphPacket& packet, u32 prior, bool clear);
	void _RenderDynamic(SceneGraphPacket& packet, u32 prior, bool clear);
	void _RenderHUD(SceneGraphPacket& packet);
	void _RenderTranslucent(SceneGraphPacket& packet);
	void _RenderLODs(SceneGraphPacket& packet, bool setup_zb, bool clear);
	void _RenderEmissive(SceneGraphPacket& packet);
	void _RenderWmarks(SceneGraphPacket& packet);
	void _RenderDistortion(SceneGraphPacket& packet);
};
