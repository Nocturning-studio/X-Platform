#include "stdafx.h"
#include "../xrEngine/du_sphere_part.h"

void CRender::draw_volume(light* L)
{
	switch (L->LightFlags.type)
	{
	case IRender_Light::REFLECTED:
	case IRender_Light::POINT:
		RenderBackendLegacy.set_Geometry(RenderTarget->g_accum_point);
		RenderBackendLegacy.Render(D3DPT_TRIANGLELIST, 0, 0, DU_SPHERE_NUMVERTEX, 0, DU_SPHERE_NUMFACES);
		break;
	case IRender_Light::SPOT:
		RenderBackendLegacy.set_Geometry(RenderTarget->g_accum_spot);
		RenderBackendLegacy.Render(D3DPT_TRIANGLELIST, 0, 0, DU_CONE_NUMVERTEX, 0, DU_CONE_NUMFACES);
		break;
	case IRender_Light::OMNIPART:
		RenderBackendLegacy.set_Geometry(RenderTarget->g_accum_omnipart);
		RenderBackendLegacy.Render(D3DPT_TRIANGLELIST, 0, 0, DU_SPHERE_PART_NUMVERTEX, 0, DU_SPHERE_PART_NUMFACES);
		break;
	default:
		break;
	}
}
