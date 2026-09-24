///////////////////////////////////////////////////////////////////////////////////
// Created: 19.11.2023
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_wallmarks()
{
	////OPTICK_EVENT("CRender::render_wallmarks");

	// Targets
	RenderBackend.SetRenderTarget(RenderTarget->rt_GBuffer[0]);
	RenderBackend.SetDepthBuffer(RenderBackend.GetBaseZB());

	// Stencil	- draw only where stencil >= 0x1
	RenderBackend.SetStencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
	RenderBackend.SetCullMode(CULL_BACKFACE);
	RenderBackend.SetColorWriteEnable(D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
}
///////////////////////////////////////////////////////////////////////////////////
