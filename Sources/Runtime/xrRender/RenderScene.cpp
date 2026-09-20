////////////////////////////////////////////////////////////////////////////////
// Created: 18.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "RenderScene.h"
////////////////////////////////////////////////////////////////////////////////
CRenderScene::CRenderScene()
{
}

CRenderScene::~CRenderScene()
{
	Destroy();
}

void CRenderScene::Initialize()
{
	m_graph.m_traversal_marker = 0;
	m_last_ltrack = 0;

	if (g_dedicated_server)
		return;

	if(!m_details)
		m_details = xr_new<CDetailManager>();

	if(!m_sun_occluder)
		m_sun_occluder = xr_new<CSunOccluder>();

	if (!m_wallmarks)
		m_wallmarks = xr_new<CWallmarksEngine>();
}

void CRenderScene::Destroy()
{
	if (m_loaded)
		Unload();

	DestroyResources();

	xr_delete(m_details);

	xr_delete(m_sun_occluder);

	xr_delete(m_wallmarks);
}

void CRenderScene::CreateResources()
{
	if (m_resources_created) 
		return;
	m_resources_created = true;

	m_graph.m_packet.InitResources();
}

void CRenderScene::DestroyResources()
{
	if (!m_resources_created) 
		return;
	m_resources_created = false;

	m_graph.m_packet.FreeResources();
}

void CRenderScene::OnResetBegin()
{
	for (u32 it = 0; it < m_lights_last_frame.size(); it++)
	{
		if (0 == m_lights_last_frame[it])
			continue;
		try
		{
			m_lights_last_frame[it]->get_smapvis().resetoccq();
		}
		catch (...)
		{
			Msg("! Failed to flush-OCCq on light [%d] %X", it, *(u32*)(&m_lights_last_frame[it]));
		}
	}
	m_lights_last_frame.clear();
}

void CRenderScene::OnResetEnd()
{

}

void CRenderScene::OnFrame()
{
	PROFILE_FUNCTION();

	if (m_details && m_details->dtFS)
	{
		m_details->PrepareToCalc();
		Engine.ThreadManager.AddParallelTask([this]() { m_details->MT_CALC(); });
	}
}

void CRenderScene::SetLoaded()
{
	m_loaded = true;
}

void CRenderScene::Unload()
{
	m_cpu_occ_pending_lights.clear();
	m_lights_last_frame.clear();

	m_lights.Unload();

	if (m_details)
		m_details->Unload();

	if (m_sun_occluder)
		m_sun_occluder->Unload();

	if (m_wallmarks)
		xr_delete(m_wallmarks);

	m_hom.Unload();

	m_loaded = false;
}

void CRenderScene::LoadDetails()
{
	if (m_details)
		m_details->Load();
}

void CRenderScene::LoadSunOccluder()
{
	if (m_sun_occluder)
		m_sun_occluder->Load();
}

void CRenderScene::LoadLights(IReader* fs)
{
	m_lights.Load(fs);
}

void CRenderScene::LoadHOM()
{
	m_hom.Load();
}

void CRenderScene::LoadWallmarks()
{
	if (!g_dedicated_server && !m_wallmarks)
		m_wallmarks = xr_new<CWallmarksEngine>();
}

SceneTraversalContext& CRenderScene::GetActiveContext()
{
	return CurrentRenderContext::context ? *CurrentRenderContext::context : m_default_context;
}

const SceneTraversalContext& CRenderScene::GetActiveContext() const
{
	return CurrentRenderContext::context ? *CurrentRenderContext::context : m_default_context;
}

void CRenderScene::AddVisual(IRender_Visual* V)
{
	if (!V)
		return;

	if (CurrentRenderContext::packet && CurrentRenderContext::context)
	{
		m_graph.ProcessDynamicVisual(V, *CurrentRenderContext::context, *CurrentRenderContext::packet);
		return;
	}

	m_graph.ProcessDynamicVisual(V, m_default_context, m_graph.m_packet);
}

void CRenderScene::AddGeometry(IRender_Visual* V)
{
	if (!V) return;

	const SceneTraversalContext& ctx = GetActiveContext();
	const u32 frustumMask = ctx.frustum ? ctx.frustum->getMask() : 0;

	if (CurrentRenderContext::packet && CurrentRenderContext::context)
		m_graph.add_Static(V, frustumMask, *CurrentRenderContext::context, *CurrentRenderContext::packet);
	else
		m_graph.add_Static(V, frustumMask, m_default_context, m_graph.m_packet);
}

void CRenderScene::AddStaticWallmark(ref_shader& S, const fvec3& P, float s, CDB::TRI* T, fvec3* V)
{
	if (g_dedicated_server || !m_wallmarks)
		return;
	m_wallmarks->AddStaticWallmark(T, V, P, &*S, s);
}

void CRenderScene::AddSkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm)
{
	if (!g_dedicated_server && m_wallmarks)
		m_wallmarks->AddSkeletonWallmark(wm);
}

void CRenderScene::AddSkeletonWallmark(const fmat4x4* xf, CKinematics* obj, ref_shader& sh, const fvec3& start, const fvec3& dir, float size)
{
	PROFILE_FUNCTION();
#pragma fixme(Декали на скелетах)
	// if (!g_dedicated_server && m_wallmarks) m_wallmarks->AddSkeletonWallmark(xf, obj, sh, start, dir, size);
}

void CRenderScene::ClearStaticWallmarks()
{
	if (!g_dedicated_server && m_wallmarks)
		m_wallmarks->clear();
}

void CRenderScene::ComputeVisibility(const SSceneVisibilityRequest& req, SSceneVisibilityResult& out)
{
	PROFILE_FUNCTION();

	out.Clear();

	out.view = req.view;
	out.projection = req.projection;
	out.view_projection = req.view_projection;
	out.camera_position = req.camera_position;
	out.render_phase = req.render_phase;

	ComputeVisibilityInternal(req, out.packet, out.context);
}

void CRenderScene::ComputeVisibility(const SSceneVisibilityRequest& req, SceneGraphPacket& out_packet)
{
	PROFILE_FUNCTION();
	SceneTraversalContext tmp_ctx;
	ComputeVisibilityInternal(req, out_packet, tmp_ctx);
}

void CRenderScene::ComputeVisibilityInternal(const SSceneVisibilityRequest& req, SceneGraphPacket& packet, SceneTraversalContext& ctx)
{
	ctx.RenderView.View = req.view;
	ctx.RenderView.Project = req.projection;
	ctx.RenderView.ViewProjection = req.view_projection;
	ctx.RenderView.Position = req.camera_position;

	ctx.use_hom = req.use_hom;
	ctx.use_feedback = req.use_feedback;
	ctx.render_phase = req.render_phase;
	ctx.is_hud_pass = (req.gather_options & SSceneVisibilityRequest::HUD) ? TRUE : FALSE;
	ctx.is_invisible_mode = FALSE;
	ctx.fetch_config.fetch_priority_0 = (req.gather_options & (SSceneVisibilityRequest::STATIC_GEOM | SSceneVisibilityRequest::DYNAMIC_GEOM)) != 0;
	ctx.fetch_config.fetch_priority_1 = (req.gather_options & SSceneVisibilityRequest::LOD_GEOM) != 0;
	ctx.fetch_config.fetch_wallmarks = (req.gather_options & SSceneVisibilityRequest::WALLMARKS) != 0;
	ctx.culling_bounds = req.culling_bounds;
	ctx.frustum = req.frustum_override;
	ctx.transform = &Fidentity;
	ctx.traversal_marker_id = ++m_graph.m_traversal_marker;

	// Локальный фрустум живёт в пределах вызова — это безопасно,
	// т.к. в конце ctx.frustum обнуляется. Render() его не использует.
	CFrustum local_frustum;
	if(!ctx.frustum)
	{
		local_frustum.CreateFromMatrix(req.view_projection, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);
		ctx.frustum = &local_frustum;
	}

	CurrentRenderContext::Scope tls_scope(packet, ctx);

	if (!req.start_sector)
	{
		ctx.frustum = nullptr;
		return;
	}

	g_SpatialSpace->q_frustum(packet.m_spatial_query_results, ISpatial_DB::O_ORDERED, STYPE_RENDERABLE | STYPE_LIGHTSOURCE, *ctx.frustum);

	u32 traverse_flags = CPortalTraverser::VQ_HOM | CPortalTraverser::VQ_SSA | (ctx.is_hud_pass ? FALSE : CPortalTraverser::VQ_FADE);
	const fvec3 traversal_cop = req.use_traversal_position ? req.traversal_position : req.camera_position;

	packet.portal_traverser.Traverse(req.start_sector, *ctx.frustum, traversal_cop, req.view_projection, traverse_flags);

	if(req.gather_options & SSceneVisibilityRequest::STATIC_GEOM)
		CollectStaticGeometry(packet, ctx);

	if(req.gather_options & SSceneVisibilityRequest::DYNAMIC_GEOM)
		CollectDynamicGeometry(packet, ctx);

	if(req.gather_options & SSceneVisibilityRequest::LIGHTS)
		CollectLights(packet, ctx);

	ctx.frustum = nullptr;
}

void CRenderScene::CollectStaticGeometry(SceneGraphPacket& packet, const SceneTraversalContext& ctx)
{
	PROFILE_FUNCTION();

	const auto& visible = packet.portal_traverser.GetVisibleSectors();

	packet.visible_sectors_map.clear();
	for (const auto& sec_vis : visible)
		packet.visible_sectors_map[sec_vis.sector] = &sec_vis;

	for (const auto& sec_vis : visible)
	{
		CSector* sector = sec_vis.sector;
		IRender_Visual* root = sector->GetRootVisual();

		for (const auto& frustum : sec_vis.frustums)
		{
			if (CurrentRenderContext::context)
				CurrentRenderContext::context->frustum = &frustum;

			m_graph.add_Static(root, frustum.getMask(), ctx, packet);
		}
	}
}

void CRenderScene::CollectDynamicGeometry(SceneGraphPacket& packet, const SceneTraversalContext& ctx)
{
	PROFILE_FUNCTION();

	for (ISpatial* spatial : packet.m_spatial_query_results)
	{
		spatial->spatial_updatesector();
		CSector* sector = (CSector*)spatial->spatial.sector;

		if (!(spatial->spatial.type & STYPE_RENDERABLE))
			continue;

		IRenderable* renderable = spatial->dcast_Renderable();
		if (!renderable)
			continue;

		auto it = packet.visible_sectors_map.find(sector);
		if (it == packet.visible_sectors_map.end())
			continue;

		const CPortalTraverser::SectorVisibility* vis = it->second;

		bool in_frustum = false;
		for (const auto& f : vis->frustums)
		{
			if (f.testSphere_dirty(spatial->spatial.sphere.P, spatial->spatial.sphere.R))
			{
				in_frustum = true;
				break;
			}
		}
		if (!in_frustum)
			continue;

		vis_data& vis_orig = renderable->renderable.visual->vis;
		vis_data vis_temp = vis_orig;
		vis_temp.box.transform(renderable->renderable.transform);

		BOOL visible = !ctx.use_hom || m_hom.visible(vis_temp);

		vis_orig.hom_frame = vis_temp.hom_frame;
		vis_orig.hom_tested = vis_temp.hom_tested;

		if (visible)
			packet.m_culled_dynamics.push_back(renderable);
	}

	m_graph.PrepareDynamicInstances(packet, ctx);
}

void CRenderScene::CollectLights(SceneGraphPacket& packet, const SceneTraversalContext& ctx)
{
	PROFILE_FUNCTION();

	for (ISpatial* spatial : packet.m_spatial_query_results)
	{
		spatial->spatial_updatesector();

		if (!(spatial->spatial.type & STYPE_LIGHTSOURCE))
			continue;

		light* pLight = (light*)spatial->dcast_Light();
		if (!pLight || pLight->get_LOD() <= EPS_L)
			continue;

		if (!ctx.use_hom || m_hom.visible(pLight->get_homdata()))
			packet.m_culled_lights.push_back(pLight);
	}
}

void CRenderScene::Render(SSceneVisibilityResult& result, SceneGraphRenderType type, u32 priority, bool clear, bool setup_zb)
{
	PROFILE_FUNCTION();

	RenderRaw(result.packet, result.context, type, priority, clear, setup_zb);
}

void CRenderScene::RenderDetails(DetailsRenderMode mode, fmat4x4* cull_matrix, const CFrustum* external_cull)
{
	PROFILE_FUNCTION();

	if (!m_details)
		return;

	m_details->Render(mode, cull_matrix, external_cull);
}

void CRenderScene::RenderWallmarks()
{
	if (m_wallmarks)
		m_wallmarks->Render();
}

void CRenderScene::RenderRaw(SceneGraphPacket& packet, SceneTraversalContext& ctx, SceneGraphRenderType type, u32 priority, bool clear, bool setup_zb)
{
	CurrentRenderContext::Scope tls_scope(packet, ctx);
	m_graph.Render(packet, type, priority, clear, setup_zb);
}
////////////////////////////////////////////////////////////////////////////////
