////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Modified: 13.10.2023
// Modifier: NSDeathman, Mihan-323
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRender::clear_gbuffer()
{
	RenderBackendLegacy.SetRenderTarget(RenderTarget->rt_GBuffer[0], RenderTarget->rt_GBuffer[1], RenderTarget->rt_GBuffer[2]);
	RenderBackendLegacy.SetDepthBuffer(RenderBackendLegacy.GetBaseZB());
	RenderBackendLegacy.Clear(0L, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0x0, 1.0f, 0L);
}

void CRender::set_gbuffer()
{
	RenderBackendLegacy.SetRenderTarget(RenderTarget->rt_GBuffer[0], RenderTarget->rt_GBuffer[1], RenderTarget->rt_GBuffer[2]);
	RenderBackendLegacy.SetDepthBuffer(RenderBackendLegacy.GetBaseZB());

	// Stencil - write 0x1 at pixel pos
	RenderBackendLegacy.SetStencil(TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);

	// Misc	- draw only front-faces
	RenderBackendLegacy.SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, FALSE);

	// Set backface culling
	RenderBackendLegacy.SetCullMode(CULL_BACKFACE);

	RenderBackendLegacy.SetColorWriteEnable();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
