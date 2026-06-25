////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Modified: 13.10.2023
// Modifier: NSDeathman, Mihan-323
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRender::clear_gbuffer()
{
	////OPTICK_EVENT("CRenderTarget::clear_gbuffer");

	if (ps_r_shading_mode == SHADING_MODE_LEASHED)
		RenderBackendLegacy.set_Render_Target_Surface(RenderTarget->rt_GBuffer[0], 
												RenderTarget->rt_GBuffer[1]);
	else
		RenderBackendLegacy.set_Render_Target_Surface(RenderTarget->rt_GBuffer[0], 
												RenderTarget->rt_GBuffer[1], 
												RenderTarget->rt_GBuffer[2], 
												RenderTarget->rt_GBuffer[3]);

	RenderBackendLegacy.set_Depth_Buffer(HW.GetBaseZB());

	CHK_DX(HW.GetDevice()->Clear(0L, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0x0, 1.0f, 0L));
}

void CRender::set_gbuffer()
{
	////OPTICK_EVENT("CRenderTarget::set_gbuffer");

	if (ps_r_shading_mode == SHADING_MODE_LEASHED)
		RenderBackendLegacy.set_Render_Target_Surface(RenderTarget->rt_GBuffer[0], 
												RenderTarget->rt_GBuffer[1]);
	else
		RenderBackendLegacy.set_Render_Target_Surface(RenderTarget->rt_GBuffer[0], 
												RenderTarget->rt_GBuffer[1], 
												RenderTarget->rt_GBuffer[2], 
												RenderTarget->rt_GBuffer[3]);

	RenderBackendLegacy.set_Depth_Buffer(HW.GetBaseZB());

	// Stencil - write 0x1 at pixel pos
	RenderBackendLegacy.set_Stencil(TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);

	// Misc	- draw only front-faces
	RenderBackendLegacy.SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, FALSE);

	//Set backface culling
	RenderBackendLegacy.set_CullMode(CULL_BACKFACE);

	RenderBackendLegacy.set_ColorWriteEnable();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
