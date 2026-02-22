////////////////////////////////////////////////////////////////////////////////
// Created: 16.03.2025
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_render_pipeline.h"
////////////////////////////////////////////////////////////////////////////////
void CRender::RenderMenu()
{
	PROFILE_FUNCTION();

	// Globals
	RenderBackendLegacy.set_CullMode(CULL_BACKFACE);
	RenderBackendLegacy.set_Stencil(FALSE);
	RenderBackendLegacy.set_ColorWriteEnable();

	// Main Render
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Generic[0], HW.GetBaseZB());
	g_pGamePersistent->OnRenderPPUI_main(); // PP-UI

	// Prepare distortion mask
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Distortion_Mask, HW.GetBaseZB());
	RenderBackendLegacy.Clear(0, 0, CLEAR_RENDERTARGET, color_rgba(127, 127, 0, 127), 1.0f, 0);
	g_pGamePersistent->OnRenderPPUI_PP(); // PP-UI

	// Apply distortion
	RenderBackendLegacy.set_Shader(RenderTarget->s_menu_distortion);
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Generic[1], HW.GetBaseZB());

	// Resolve gamma and actual display
	RenderBackendLegacy.set_Shader(RenderTarget->s_menu_gamma);
	RenderBackendLegacy.RenderViewportSurface(Device.dwWidth, Device.dwHeight, HW.GetBaseRT(), HW.GetBaseZB());
}
////////////////////////////////////////////////////////////////////////////////
