///////////////////////////////////////////////////////////////////////////////////
// Created: 15.11.2023
// Author: NSDeathman
// Nocturning studio for X-Platform
///////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::create_distortion_mask()
{
	RenderBackendLegacy.ClearTexture(RenderTarget->rt_Distortion_Mask, color_rgba(127, 127, 0, 127));
	RenderBackendLegacy.SetDepthBuffer(RenderBackendLegacy.GetBaseZB());

	RenderBackendLegacy.SetCullMode(CULL_BACKFACE);
	RenderBackendLegacy.SetStencil(FALSE);
	RenderBackendLegacy.SetColorWriteEnable();

	RenderImplementation.Scene.Render(RenderImplementation.m_scene_visibility_data, SceneRenderFlags::Distortion);
}

void CRender::render_distortion()
{
	RenderBackendLegacy.SetCullMode(CULL_DISABLE);
	RenderBackendLegacy.SetStencil(FALSE);

	RenderBackendLegacy.SetShaderElement(RenderTarget->s_distortion->E[0]);

	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Generic[1]);
}
