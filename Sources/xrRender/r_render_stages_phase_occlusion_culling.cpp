#include "stdafx.h"

void CRender::phase_occq()
{
	////OPTICK_EVENT("CRenderTarget::phase_occq");

	RenderBackendLegacy.set_Render_Target_Surface(Device.dwWidth, Device.dwHeight, HW.GetBaseRT());
	RenderBackendLegacy.set_Depth_Buffer(HW.GetBaseZB());
	RenderBackendLegacy.set_CullMode(CULL_BACKFACE);
	RenderBackendLegacy.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
	RenderBackendLegacy.set_ColorWriteEnable(FALSE);
	RenderBackendLegacy.set_Shader(RenderTarget->s_occq);
}
