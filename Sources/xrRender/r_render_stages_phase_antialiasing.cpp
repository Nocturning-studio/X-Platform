///////////////////////////////////////////////////////////////////////////////////
// Created: 15.11.2023
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Blender_antialiasing.h"

///////////////////////////////////////////////////////////////////////////////////
void CRender::render_antialiasing()
{
	////OPTICK_EVENT("CRender::render_antialiasing");

	RenderBackendLegacy.set_CullMode(CULL_DISABLE);
	RenderBackendLegacy.set_Stencil(FALSE);

	RenderBackendLegacy.set_Element(RenderTarget->s_antialiasing->E[SE_PASS_FXAA], 0);
	RenderBackendLegacy.set_Constant("fxaa_params", ps_r_fxaa_subpix, ps_r_fxaa_edge_treshold, ps_r_fxaa_edge_treshold_min);
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Generic[1]);

	RenderBackendLegacy.CopyViewportSurface(RenderTarget->rt_Generic[1], RenderTarget->rt_Generic[0]);
}
///////////////////////////////////////////////////////////////////////////////////
