///////////////////////////////////////////////////////////////////////////////////
// Created: 15.11.2023
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::create_distortion_mask()
{
	RenderBackend.ClearTexture(RenderTarget->rt_Distortion_Mask, color_rgba(127, 127, 0, 127));
	RenderBackend.SetDepthBuffer(RenderBackend.GetBaseZB());

	RenderBackend.SetCullMode(CULL_BACKFACE);
	RenderBackend.SetStencil(FALSE);
	RenderBackend.SetColorWriteEnable();

	RenderImplementation.Scene.Render(RenderImplementation.m_scene_visibility_data, SceneRenderFlags::Distortion);
}

void CRender::render_distortion()
{
	RenderBackend.SetCullMode(CULL_DISABLE);
	RenderBackend.SetStencil(FALSE);

	RenderBackend.SetShaderElement(RenderTarget->s_distortion->E[0]);

	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic[1]);
}
