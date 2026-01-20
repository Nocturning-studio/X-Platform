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
	RenderBackend.set_CullMode(CULL_BACKFACE);
	RenderBackend.set_Stencil(FALSE);
	RenderBackend.set_ColorWriteEnable();

	// Main Render
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic[0], HW.pBaseZB);
	g_pGamePersistent->OnRenderPPUI_main(); // PP-UI

	// Prepare distortion mask
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Distortion_Mask, HW.pBaseZB);
	RenderBackend.Clear(0, 0, CLEAR_RENDERTARGET, color_rgba(127, 127, 0, 127), 1.0f, 0);
	g_pGamePersistent->OnRenderPPUI_PP(); // PP-UI

	// Apply distortion
	RenderBackend.set_Shader(RenderTarget->s_menu_distortion);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic[1], HW.pBaseZB);

	// Resolve gamma and actual display
	RenderBackend.set_Shader(RenderTarget->s_menu_gamma);
	RenderBackend.RenderViewportSurface(Device.dwWidth, Device.dwHeight, HW.pBaseRT, HW.pBaseZB);
}
////////////////////////////////////////////////////////////////////////////////
