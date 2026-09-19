////////////////////////////////////////////////////////////////////////////////
// Created: 18.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "SceneGraph.h"
#include "DetailManager.h"
#include "HOM.h"
#include "SunOccluder.h"
#include "Light_DB.h"
#include "Light_Package.h"
#include "SMAP_Allocator.h"
////////////////////////////////////////////////////////////////////////////////
#undef GetObject
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//  SSceneVisibilityRequest
//  Полностью описывает, что и как считать. Не содержит владения.
//  Может использоваться из параллельной задачи для солнца, источника света
//  или основной камеры — разница только в матрицах и frustum_override.
////////////////////////////////////////////////////////////////////////////////
struct SSceneVisibilityRequest
{
	fmat4x4 view = Fidentity;
	fmat4x4 projection = Fidentity;
	fmat4x4 view_projection = Fidentity;
	fvec3 camera_position = {0, 0, 0};
	fvec3 traversal_position = { 0, 0, 0 };
	bool  use_traversal_position = false;

	CSector* start_sector = nullptr;

	bool use_hom = true;
	bool use_feedback = false;

	enum EGatherOptions
	{
		STATIC_GEOM		= (1 << 0),
		DYNAMIC_GEOM	= (1 << 1),
		LOD_GEOM		= (1 << 2),
		LIGHTS			= (1 << 3),
		HUD				= (1 << 4),
		WALLMARKS		= (1 << 5),
	};
	u32 gather_options = STATIC_GEOM | DYNAMIC_GEOM | LOD_GEOM | LIGHTS | HUD | WALLMARKS;

	const CFrustum* frustum_override = nullptr;

	u32 render_phase = 0;

	xr_vector<Fbox3, render_alloc<Fbox3>>* culling_bounds = nullptr;
};

////////////////////////////////////////////////////////////////////////////////
//  SSceneVisibilityResult
//  Результат — то, что сцена передаёт в Render(). Может жить в double-buffer
//  у вызывающего (сцена не хранит его у себя).
////////////////////////////////////////////////////////////////////////////////
struct SSceneVisibilityResult
{
	SceneGraphPacket packet;
	SceneTraversalContext context;

	fmat4x4 view = Fidentity;
	fmat4x4 projection = Fidentity;
	fmat4x4 view_projection = Fidentity;
	fvec3 camera_position = {0, 0, 0};
	u32 render_phase = 0;

	// Управляется вызывающим: выставить false перед постановкой в очередь
	// параллельной задачи, true — по завершении.
	std::atomic<bool> ready{true};

	void InitResources() { packet.InitResources(); }
	void FreeResources() { packet.FreeResources(); }
	void Clear() { packet.Clear(); }

	bool IsReady() const { return ready.load(std::memory_order_acquire); }

	void MarkPending() { ready.store(false, std::memory_order_release); }
	void MarkReady() { ready.store(true, std::memory_order_release); }
};

////////////////////////////////////////////////////////////////////////////////
//  CRenderScene
//  Единственный владелец сцены: графа, деталей, HOM, sun-occluder,
//  smap-пула, глобального light-пула.
//
//  Не занимается спецификой каскадов солнца, фаз и приоритетов — вызывающий
//  сам решает, какие матрицы/frustum передать и когда применить результат.
//
//  Потокобезопасность:
//   - ComputeVisibility(req, result) можно вызывать из параллельных задач,
//     если result'ы разные и m_graph.m_traversal_marker атомарен.
//   - Render(result, ...) должен вызываться на потоке рендера.
//   - CreateResources/DestroyResources/Load/Unload — только main thread.
////////////////////////////////////////////////////////////////////////////////
class CRenderScene
{
  public:
	CRenderScene();
	~CRenderScene();

	void Initialize();
	void Destroy();

	void CreateResources();
	void DestroyResources();

	void OnResetBegin();
	void OnResetEnd();

	void OnFrame();

	void SetLoaded();
	void Unload();

	// =====================================================================
	//  Загрузка компонентов сцены
	// =====================================================================
	void LoadDetails();
	void LoadHOM();
	void LoadSunOccluder();
	void LoadLights(IReader* fs);

	// =====================================================================
	//  Геттеры
	// =====================================================================
	CFrustum& GetFrustumBase() { return m_frustum_base; }
	const CFrustum& GetFrustumBase() const { return m_frustum_base; }

	const CFrustum* GetFrustum() const { return GetActiveContext().frustum; }

	CSceneGraph& GetGraph() { return m_graph; }
	const CSceneGraph& GetGraph() const { return m_graph; }

	CDetailManager* GetDetails() { return m_details; }
	const CDetailManager* GetDetails() const { return m_details; }

	CHOM& GetHOM() { return m_hom; }
	CSunOccluder* GetSunOccluder() { return m_sun_occluder; }

	CLight_DB& GetLights() { return m_lights; }
	SMAP_Allocator& GetSmapPool() { return m_smap_pool; }

	xr_vector<light*>& GetLightsLastFrame() { return m_lights_last_frame; }
	xr_vector<light*>& GetCpuOccPendingLights() { return m_cpu_occ_pending_lights; }

	light_Package& GetNormalLights() { return m_lp_normal; }
	light_Package& GetPendingLights() { return m_lp_pending; }

	SceneTraversalContext& GetDefaultContext() { return m_default_context; }
	const SceneTraversalContext& GetDefaultContext() const { return m_default_context; }

	SceneTraversalContext& GetActiveContext();
	const SceneTraversalContext& GetActiveContext() const;

	// =====================================================================
	//  Проверка видимости
	// =====================================================================
	BOOL IsVisible(vis_data& P) { return m_hom.visible(P); }
	BOOL IsVisible(sPoly& P) { return m_hom.visible(P); }
	BOOL IsVisible(Fbox& P) { return m_hom.visible(P); }

	// =====================================================================
	//  Если нужно что-то создать или добавить
	// =====================================================================
	IRender_Light* CreateLight() { return m_lights.Create(); }

	void AddVisual(IRender_Visual* V);
	void AddGeometry(IRender_Visual* V);

	void SetTransform(fmat4x4* M) { GetActiveContext().transform = M; }
	fmat4x4* GetTransform() const { return GetActiveContext().transform; }

	void SetHUD(BOOL V) { GetActiveContext().is_hud_pass = V; }
	BOOL GetHUD() const { return GetActiveContext().is_hud_pass; }

	void SetInvisible(BOOL V) { GetActiveContext().is_invisible_mode = V; }
	BOOL GetInvisible() const { return GetActiveContext().is_invisible_mode; }

	void SetFrustum(CFrustum* O) { GetActiveContext().frustum = O; }
	void ResetFrustum() { GetActiveContext().frustum = nullptr; }

	void SetObject(IRenderable* O) { GetActiveContext().owner = O; }
	IRenderable* GetObject() const { return GetActiveContext().owner; }

	// =====================================================================
	//  Куллинг
	// =====================================================================
	void ComputeVisibility(const SSceneVisibilityRequest& request, SSceneVisibilityResult& out_result);
	void ComputeVisibility(const SSceneVisibilityRequest& request, SceneGraphPacket& out_packet);

	// =====================================================================
	//  Рендер
	// =====================================================================
	void Render(SSceneVisibilityResult& result, SceneGraphRenderType type, u32 priority = 0, bool clear = true, bool setup_zb = true);
	void RenderDetails(DetailsRenderMode mode, fmat4x4* cull_matrix = nullptr, const CFrustum* external_cull = nullptr);
	void RenderRaw(SceneGraphPacket& packet, SceneTraversalContext& ctx, SceneGraphRenderType type, u32 priority = 0, bool clear = true, bool setup_zb = true);

  private:
	void ComputeVisibilityInternal(const SSceneVisibilityRequest& req, SceneGraphPacket& packet, SceneTraversalContext& ctx);

	void CollectStaticGeometry(SceneGraphPacket& packet, const SceneTraversalContext& ctx);
	void CollectDynamicGeometry(SceneGraphPacket& packet, const SceneTraversalContext& ctx);
	void CollectLights(SceneGraphPacket& packet, const SceneTraversalContext& ctx);

  private:
	CFrustum m_frustum_base;

	CSceneGraph m_graph;
	CDetailManager* m_details = nullptr;
	CHOM m_hom;
	CSunOccluder* m_sun_occluder = nullptr;

	SceneTraversalContext m_default_context;

	CLight_DB m_lights;
	SMAP_Allocator m_smap_pool;

	light_Package m_lp_normal;
	light_Package m_lp_pending;

	xr_vector<light*> m_lights_last_frame;
	xr_vector<light*> m_cpu_occ_pending_lights;

	u32 m_last_ltrack = 0;

	bool m_resources_created = false;
	bool m_loaded = false;
};
////////////////////////////////////////////////////////////////////////////////
