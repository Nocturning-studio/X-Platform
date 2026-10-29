///////////////////////////////////////////////////////////////////////////////////
// Created: 15.11.2023
// Author: NSDeathman
// Nocturning studio for X-Platform
///////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "Blender_antialiasing.h"

///////////////////////////////////////////////////////////////////////////////////
void CRender::render_antialiasing()
{
	////OPTICK_EVENT("CRender::render_antialiasing");

	RenderBackendLegacy.SetCullMode(CULL_DISABLE);
	RenderBackendLegacy.SetStencil(FALSE);

	RenderBackendLegacy.SetShaderElement(RenderTarget->s_antialiasing->E[SE_PASS_FXAA], 0);
	RenderBackendLegacy.SetConstant("fxaa_params", ps_r_fxaa_subpix, ps_r_fxaa_edge_treshold, ps_r_fxaa_edge_treshold_min);
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Generic[1]);

	RenderBackendLegacy.CopyViewportSurface(RenderTarget->rt_Generic[1], RenderTarget->rt_Generic[0]);
}
///////////////////////////////////////////////////////////////////////////////////
