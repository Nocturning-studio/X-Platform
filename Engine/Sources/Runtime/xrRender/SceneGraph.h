#pragma once

#include "..\xrEngine\IRender.h"
#include "..\xrEngine\ispatial.h"
#include "SceneGraphTypes.h"
#include "SceneGraphArenaAllocator.h"
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
	static constexpr u32 k_static_entries_hint = 1500;
	static constexpr u32 k_static_batches_hint = 256;
	static constexpr u32 k_dynamic_entries_hint = 1500;
	static constexpr u32 k_dynamic_batches_hint = 256;

	SceneGraphArena arena;

	// Dynamic scene graph containers
	SceneGraphTypes::RenderQueue<SceneGraphTypes::StaticRenderNode>  queue_static[2]; // [0] = deffered lighting, [1] = forward lighting
	SceneGraphTypes::RenderQueue<SceneGraphTypes::DynamicRenderNode> queue_dynamic[2];
	SceneGraphTypes::QueueTransparent queue_transparent;
	SceneGraphTypes::QueueHUD queue_hud;
	SceneGraphTypes::QueueLOD mapLOD;
	SceneGraphTypes::QueueTransparent queue_distortion;
	SceneGraphTypes::QueueTransparent queue_wallmarks;
	SceneGraphTypes::QueueTransparent mapEmissive;

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

	void InitResources(size_t arena_bytes = 2 * 1024 * 1024)
	{
		arena.Init(arena_bytes);
		for (int i = 0; i < 2; ++i)
		{
			queue_static[i].Init(&arena);
			queue_dynamic[i].Init(&arena);
		}

		queue_transparent.Init(&arena);
		queue_hud.Init(&arena);
		mapLOD.Init(&arena);
		queue_distortion.Init(&arena);
		queue_wallmarks.Init(&arena);
		mapEmissive.Init(&arena);

		portal_traverser.CreateResources();
	}

	void FreeResources()
	{
		portal_traverser.DestroyResources();
		arena.Destroy();
	}

	void Clear()
	{
		ReleaseVisualRefs();
		m_culled_lights.clear();
		m_culled_dynamics.clear();
		portal_traverser.Reset();
		visible_sectors_map.clear();

		arena.Reset();

		for (int i = 0; i < 2; ++i)
		{
			queue_static[i].ReserveAfterReset(k_static_entries_hint, k_static_batches_hint);
			queue_dynamic[i].ReserveAfterReset(k_dynamic_entries_hint, k_dynamic_batches_hint);
		}

		queue_transparent.ReserveAfterReset(512);
		queue_distortion.ReserveAfterReset(64);
		queue_wallmarks.ReserveAfterReset(64);
		mapEmissive.ReserveAfterReset(64);
		queue_hud.ReserveAfterReset(128);
		mapLOD.ReserveAfterReset(4096);

		lstLODs.clear();
		lstLODgroups.clear();
		m_spatial_query_results.clear();
		m_visuals_static_visible.clear();
		m_visuals_dynamic_visible.clear();
	}

	void Destroy()
	{
		ReleaseVisualRefs();
		queue_static[0].Clear();
		queue_static[1].Clear();
		queue_dynamic[0].Clear();
		queue_dynamic[1].Clear();
		queue_transparent.Clear();
		queue_hud.Clear();
		mapLOD.Clear();
		queue_distortion.Clear();
		queue_wallmarks.Clear();
		mapEmissive.Clear();
		FreeResources();
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
		  fetch_flags()
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

	SceneGraphPacket& GetLoosePacket() { return m_packet; }

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
