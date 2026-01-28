#include "stdafx.h"
#include "../xrEngine/du_sphere_part.h"

void CRender::draw_volume(light* L)
{
	switch (L->m_LightFlags.type)
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
