////////////////////////////////////////////////////////////////////////////////
// Created: 16.03.2025
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_render_pipeline.h"
#include <xrEngine/ShaderPass.h>
////////////////////////////////////////////////////////////////////////////////
void Test()
{
	CShaderPass pass;
	pass.SetVertexShader("test.hlsl", "vs_main");
	pass.SetPixelShader("test.hlsl", "ps_main");

	if (!pass.IsValid())
	{
		if (!pass.Compile(RenderBackend.GetDevice()))
		{
			Msg("! pass failed to compile");
			return;
		}
	}

	if (!pass.SetVector("test_vec", fvec4{ 0.1f, 0.2f, 0.3f, 1.0f })) Msg("! test_vec not found in constant table");

	RenderBackend.SetShaderPass(pass);

	RenderBackend.RenderViewportSurface(Device.dwWidth, Device.dwHeight, RenderBackend.GetBaseRT(), RenderBackend.GetBaseZB());
}

void CRender::RenderMenu()
{
	PROFILE_FUNCTION();

	// Globals
	RenderBackend.set_CullMode(CULL_BACKFACE);
	RenderBackend.set_Stencil(FALSE);
	RenderBackend.set_ColorWriteEnable();

	Test();
	return;

	// Main Render
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic[0], RenderBackend.GetBaseZB());
	g_pGamePersistent->OnRenderPPUI_main(); // PP-UI

	// Prepare distortion mask
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Distortion_Mask, RenderBackend.GetBaseZB());
	RenderBackend.Clear(0, 0, CLEAR_RENDERTARGET, color_rgba(127, 127, 0, 127), 1.0f, 0);
	g_pGamePersistent->OnRenderPPUI_PP(); // PP-UI

	// Apply distortion
	RenderBackend.set_Shader(RenderTarget->s_menu_distortion);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic[1], RenderBackend.GetBaseZB());

	// Resolve gamma and actual display
	RenderBackend.set_Shader(RenderTarget->s_menu_gamma);
	RenderBackend.RenderViewportSurface(Device.dwWidth, Device.dwHeight, RenderBackend.GetBaseRT(), RenderBackend.GetBaseZB());
}
////////////////////////////////////////////////////////////////////////////////
