#include "stdafx.h"

void CRender::phase_occq()
{
	OPTICK_EVENT("CRenderTarget::phase_occq");

	RenderBackend.set_Render_Target_Surface(Device.dwWidth, Device.dwHeight, HW.pBaseRT);
	RenderBackend.set_Depth_Buffer(HW.pBaseZB);
	RenderBackend.set_CullMode(CULL_BACKFACE);
	RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
	RenderBackend.set_ColorWriteEnable(FALSE);
	RenderBackend.set_Shader(RenderTarget->s_occq);
}
