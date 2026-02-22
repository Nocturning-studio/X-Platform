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
	RenderBackendLegacy.set_Render_Target_Surface(RenderTarget->rt_GBuffer[0]);
	RenderBackendLegacy.set_Depth_Buffer(HW.GetBaseZB());

	// Stencil	- draw only where stencil >= 0x1
	RenderBackendLegacy.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
	RenderBackendLegacy.set_CullMode(CULL_BACKFACE);
	RenderBackendLegacy.set_ColorWriteEnable(D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
}
///////////////////////////////////////////////////////////////////////////////////
