////////////////////////////////////////////////////////////////////////////////
// Created: 16.03.2025
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "r_render_pipeline.h"
////////////////////////////////////////////////////////////////////////////////
void CRender::RenderMenu()
{
	PROFILE_FUNCTION();

	// Globals
	RenderBackendLegacy.SetCullMode(CULL_BACKFACE);
	RenderBackendLegacy.SetStencil(FALSE);
	RenderBackendLegacy.SetColorWriteEnable();

	// Main Render
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Generic[0], RenderBackendLegacy.GetBaseZB());
	g_pGamePersistent->OnRenderPPUI_main(); // PP-UI

	// Prepare distortion mask
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Distortion_Mask, RenderBackendLegacy.GetBaseZB());
	RenderBackendLegacy.Clear(0, 0, CLEAR_RENDERTARGET, color_rgba(127, 127, 0, 127), 1.0f, 0);
	g_pGamePersistent->OnRenderPPUI_PP(); // PP-UI

	// Apply distortion
	RenderBackendLegacy.SetShader(RenderTarget->s_menu_distortion);
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Generic[1], RenderBackendLegacy.GetBaseZB());

	// Resolve gamma and actual display
	RenderBackendLegacy.SetShader(RenderTarget->s_menu_gamma);
	RenderBackendLegacy.RenderViewportSurface(Device.dwWidth, Device.dwHeight, RenderBackendLegacy.GetBaseRT(), RenderBackendLegacy.GetBaseZB());
}
////////////////////////////////////////////////////////////////////////////////
