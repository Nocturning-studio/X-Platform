///////////////////////////////////////////////////////////////////////////////////
// Created: 19.11.2023
// Author: NSDeathman
// Nocturning studio for X-Platform
///////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_wallmarks()
{
	////OPTICK_EVENT("CRender::render_wallmarks");

	// Targets
	RenderBackendLegacy.SetRenderTarget(RenderTarget->rt_GBuffer[0]);
	RenderBackendLegacy.SetDepthBuffer(RenderBackendLegacy.GetBaseZB());

	// Stencil	- draw only where stencil >= 0x1
	RenderBackendLegacy.SetStencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
	RenderBackendLegacy.SetCullMode(CULL_BACKFACE);
	RenderBackendLegacy.SetColorWriteEnable(D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
}
///////////////////////////////////////////////////////////////////////////////////
