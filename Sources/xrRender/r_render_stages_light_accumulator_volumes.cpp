#include "stdafx.h"
#include "../xrEngine/du_cone.h"
#include "../xrEngine/du_sphere.h"
#include "../xrEngine/du_sphere_part.h"

void CRenderTarget::accum_spot_geom_create()
{
	u32 dwUsage = D3DUSAGE_WRITEONLY;

	// vertices
	{
		u32 vCount = DU_CONE_NUMVERTEX;
		u32 vSize = 3 * 4;
		R_CHK(HW.GetDevice()->CreateVertexBuffer(vCount * vSize, dwUsage, 0, D3DPOOL_DEFAULT, &g_accum_spot_vb, 0));
		BYTE* pData = 0;
		R_CHK(g_accum_spot_vb->Lock(0, 0, (void**)&pData, 0));
		CopyMemory(pData, du_cone_vertices, vCount * vSize);
		g_accum_spot_vb->Unlock();
	}

	// Indices
	{
		u32 iCount = DU_CONE_NUMFACES * 3;

		BYTE* pData = 0;
		R_CHK(HW.GetDevice()->CreateIndexBuffer(iCount * 2, dwUsage, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &g_accum_spot_ib, 0));
		R_CHK(g_accum_spot_ib->Lock(0, 0, (void**)&pData, 0));
		CopyMemory(pData, du_cone_faces, iCount * 2);
		g_accum_spot_ib->Unlock();
	}
}

void CRenderTarget::accum_spot_geom_destroy()
{
#ifdef DEBUG
	_SHOW_REF("g_accum_spot_ib", g_accum_spot_ib);
#endif // DEBUG
	_RELEASE(g_accum_spot_ib);
#ifdef DEBUG
	_SHOW_REF("g_accum_spot_vb", g_accum_spot_vb);
#endif // DEBUG
	_RELEASE(g_accum_spot_vb);
}

void CRenderTarget::accum_point_geom_create()
{
	u32 dwUsage = D3DUSAGE_WRITEONLY;

	// vertices
	{
		u32 vCount = DU_SPHERE_NUMVERTEX;
		u32 vSize = 3 * 4;
		R_CHK(HW.GetDevice()->CreateVertexBuffer(vCount * vSize, dwUsage, 0, D3DPOOL_DEFAULT, &g_accum_point_vb, 0));
		BYTE* pData = 0;
		R_CHK(g_accum_point_vb->Lock(0, 0, (void**)&pData, 0));
		CopyMemory(pData, du_sphere_vertices, vCount * vSize);
		g_accum_point_vb->Unlock();
	}

	// Indices
	{
		u32 iCount = DU_SPHERE_NUMFACES * 3;

		BYTE* pData = 0;
		R_CHK(
			HW.GetDevice()->CreateIndexBuffer(iCount * 2, dwUsage, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &g_accum_point_ib, 0));
		R_CHK(g_accum_point_ib->Lock(0, 0, (void**)&pData, 0));
		CopyMemory(pData, du_sphere_faces, iCount * 2);
		g_accum_point_ib->Unlock();
	}
}

void CRenderTarget::accum_point_geom_destroy()
{
#ifdef DEBUG
	_SHOW_REF("g_accum_point_ib", g_accum_point_ib);
#endif // DEBUG
	_RELEASE(g_accum_point_ib);
#ifdef DEBUG
	_SHOW_REF("g_accum_point_vb", g_accum_point_vb);
#endif // DEBUG
	_RELEASE(g_accum_point_vb);
}

void CRenderTarget::accum_omnip_geom_create()
{
	u32 dwUsage = D3DUSAGE_WRITEONLY;

	// vertices
	{
		u32 vCount = DU_SPHERE_PART_NUMVERTEX;
		u32 vSize = 3 * 4;
		R_CHK(HW.GetDevice()->CreateVertexBuffer(vCount * vSize, dwUsage, 0, D3DPOOL_DEFAULT, &g_accum_omnip_vb, 0));
		BYTE* pData = 0;
		R_CHK(g_accum_omnip_vb->Lock(0, 0, (void**)&pData, 0));
		CopyMemory(pData, du_sphere_part_vertices, vCount * vSize);
		g_accum_omnip_vb->Unlock();
	}

	// Indices
	{
		u32 iCount = DU_SPHERE_PART_NUMFACES * 3;

		BYTE* pData = 0;
		R_CHK(
			HW.GetDevice()->CreateIndexBuffer(iCount * 2, dwUsage, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &g_accum_omnip_ib, 0));
		R_CHK(g_accum_omnip_ib->Lock(0, 0, (void**)&pData, 0));
		CopyMemory(pData, du_sphere_part_faces, iCount * 2);
		g_accum_omnip_ib->Unlock();
	}
}

void CRenderTarget::accum_omnip_geom_destroy()
{
#ifdef DEBUG
	_SHOW_REF("g_accum_omnip_ib", g_accum_omnip_ib);
#endif // DEBUG
	_RELEASE(g_accum_omnip_ib);
#ifdef DEBUG
	_SHOW_REF("g_accum_omnip_vb", g_accum_omnip_vb);
#endif // DEBUG
	_RELEASE(g_accum_omnip_vb);
}

void CRender::draw_volume(light* L)
{
	switch (L->LightFlags.type)
	{
	case IRender_Light::REFLECTED:
	case IRender_Light::POINT:
		RenderBackend.set_Geometry(RenderTarget->g_accum_point);
		RenderBackend.Render(D3DPT_TRIANGLELIST, 0, 0, DU_SPHERE_NUMVERTEX, 0, DU_SPHERE_NUMFACES);
		break;
	case IRender_Light::SPOT:
		RenderBackend.set_Geometry(RenderTarget->g_accum_spot);
		RenderBackend.Render(D3DPT_TRIANGLELIST, 0, 0, DU_CONE_NUMVERTEX, 0, DU_CONE_NUMFACES);
		break;
	case IRender_Light::OMNIPART:
		RenderBackend.set_Geometry(RenderTarget->g_accum_omnipart);
		RenderBackend.Render(D3DPT_TRIANGLELIST, 0, 0, DU_SPHERE_PART_NUMVERTEX, 0, DU_SPHERE_PART_NUMFACES);
		break;
	default:
		break;
	}
}
