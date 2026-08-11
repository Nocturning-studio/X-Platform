#include "stdafx.h"
#include "..\xrEngine\customhud.h"

float g_fSCREEN;

extern float r_dtex_range;
extern float r_ssaDISCARD;
extern float r_ssaDONTSORT;
extern float r_ssaLOD_A;
extern float r_ssaLOD_B;
extern float r_ssaHZBvsTEX;
extern float r_ssaGLOD_start, r_ssaGLOD_end;

void CRender::Calculate()
{
	PROFILE_FUNCTION();

	// Transfer to global space to avoid deep pointer access
	IRender_Target* T = getTarget();
	float fov_factor = _sqr(90.f / Engine.RenderView.Fov);
	g_fSCREEN = float(T->get_width() * T->get_height()) * fov_factor * (EPS_S + ps_r_LOD);
	r_ssaDISCARD = _sqr(ps_r_ssaDISCARD) / g_fSCREEN;
	r_ssaDONTSORT = _sqr(ps_r_ssaDONTSORT / 3) / g_fSCREEN;
	r_ssaLOD_A = _sqr(ps_r_ssaLOD_A / 3) / g_fSCREEN;
	r_ssaLOD_B = _sqr(ps_r_ssaLOD_B / 3) / g_fSCREEN;
	r_ssaGLOD_start = _sqr(ps_r_GLOD_ssa_start / 3) / g_fSCREEN;
	r_ssaGLOD_end = _sqr(ps_r_GLOD_ssa_end / 3) / g_fSCREEN;
	r_ssaHZBvsTEX = _sqr(ps_r_ssaHZBvsTEX / 3) / g_fSCREEN;
	r_dtex_range = ps_r_detalization_distance * g_fSCREEN / (Device.dwWidth * Device.dwHeight);

	// Detect camera-sector
	if (!vLastCameraPos.similar(Engine.RenderView.Position, EPS_S))
	{
		CSector* pSector = (CSector*)detectSector(Engine.RenderView.Position);
		if (0 == pSector)
			pSector = pLastSector;
		pLastSector = pSector;
		vLastCameraPos.set(Engine.RenderView.Position);
	}

	// Check if camera is too near to some portal - if so force DualRender
	/*
	if (rmPortals)
	{
		float eps = VIEWPORT_NEAR + EPS_L;
		fvec3 box_radius;
		box_radius.set(eps, eps, eps);
		Sectors_xrc.box_options(CDB::OPT_FULL_TEST);
		Sectors_xrc.box_query(rmPortals, Engine.RenderView.Position, box_radius);
		for (int K = 0; K < Sectors_xrc.r_count(); K++)
		{
			CPortal* pPortal = (CPortal*)Portals[rmPortals->get_tris()[Sectors_xrc.r_begin()[K].id].dummy];
			pPortal->bDualRender = TRUE;
		}
	}
	*/

	Lights.Update();

	// Check if we touch some light even trough portal
	// Используем m_packet.m_spatial_query_results
	SceneGraph.m_packet.m_spatial_query_results.clear();
	g_SpatialSpace->q_sphere(SceneGraph.m_packet.m_spatial_query_results, 0, STYPE_LIGHTSOURCE, Engine.RenderView.Position,
							 EPS_L);

	// Итерируемся по m_packet.m_spatial_query_results
	for (u32 _it = 0; _it < SceneGraph.m_packet.m_spatial_query_results.size(); _it++)
	{
		ISpatial* spatial = SceneGraph.m_packet.m_spatial_query_results[_it];
		spatial->spatial_updatesector();
		CSector* sector = (CSector*)spatial->spatial.sector;
		if (0 == sector)
			continue; // disassociated from S/P structure

		VERIFY(spatial->spatial.type & STYPE_LIGHTSOURCE);
		// lightsource
		light* L = (light*)(spatial->dcast_Light());
		VERIFY(L);
		Lights.add_light(L);
	}
}
