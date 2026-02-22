///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Blender_ambient_occlusion.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_ambient_occlusion()
{
	////OPTICK_EVENT("CRender::render_ambient_occlusion");

	Engine.Statistic->RenderCALC_AO.Begin();

	RenderBackendLegacy.set_ColorWriteEnable();
	RenderBackendLegacy.set_CullMode(CULL_DISABLE);
	RenderBackendLegacy.set_Stencil(FALSE);

	float w = float(RenderTarget->rt_ao->dwWidth);
	float h = float(RenderTarget->rt_ao->dwHeight);

	// HBAO+ stuff
	float negInvR2 = -(1.0f / (pow(ps_r_ao_radius, 2.0f)));
	float RadiusPrecalc = (10.0f * h * 0.5f);

	int AOType = SE_AO_SSAO;

	switch (ps_r_ao_quality)
	{
	case 1:
		AOType = SE_AO_SSAO;
		break;
	case 2:
		AOType = SE_AO_HBAO_PLUS;
		break;
	case 3:
		AOType = SE_AO_GTAO;
		break;
	case 4:
		AOType = SE_AO_SSAO_PATH_TRACE;
		break;
	}

	RenderBackendLegacy.set_Element(RenderTarget->s_ambient_occlusion->E[AOType], SE_AO_PASS_CALC);
	RenderBackendLegacy.set_Constant("image_resolution", w, h, 1 / w, 1 / h);
	RenderBackendLegacy.set_Constant("ao_params", ps_r_ao_bias, ps_r_ao_radius, negInvR2, RadiusPrecalc);
	RenderBackendLegacy.RenderViewportSurface(w, h, RenderTarget->rt_ao);

	RenderBackendLegacy.set_Element(RenderTarget->s_ambient_occlusion->E[AOType], SE_AO_PASS_DENOISE);
	RenderBackendLegacy.set_Constant("image_resolution", w, h, 1 / w, 1 / h);
	RenderBackendLegacy.RenderViewportSurface(w, h, RenderTarget->rt_ao);

	Engine.Statistic->RenderCALC_AO.End();
}
///////////////////////////////////////////////////////////////////////////////////
