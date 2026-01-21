///////////////////////////////////////////////////////////////////////////////////
// Created: 15.11.2023
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::create_distortion_mask()
{
	////OPTICK_EVENT("CRender::create_distortion_mask");

	RenderBackend.ClearTexture(RenderTarget->rt_Distortion_Mask, color_rgba(127, 127, 0, 127));
	RenderBackend.setDepthBuffer(HW.pBaseZB);

	RenderBackend.set_CullMode(CULL_BACKFACE);
	RenderBackend.set_Stencil(FALSE);
	RenderBackend.set_ColorWriteEnable();

	RenderImplementation.SceneGraph.Render(RenderImplementation.SceneGraph.m_packet, SceneGraphRenderType::Distortion);
}

void CRender::render_distortion()
{
	////OPTICK_EVENT("CRender::render_distortion");

	RenderBackend.set_CullMode(CULL_DISABLE);
	RenderBackend.set_Stencil(FALSE);

	RenderBackend.set_Element(RenderTarget->s_distortion->E[0]);

	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic[1]);
}