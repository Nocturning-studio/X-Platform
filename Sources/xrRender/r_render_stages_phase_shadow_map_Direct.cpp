#include "stdafx.h"

void CRender::render_shadow_map_sun(light* L, u32 sub_phase)
{
	// Targets
	RenderBackendLegacy.set_Render_Target_Surface(RenderTarget->rt_smap_surf);
	RenderBackendLegacy.set_Depth_Buffer(RenderTarget->rt_smap_depth->pRT);

	// optimized clear
	D3DRECT R;
	R.x1 = L->TransformContext.Sun.minX;
	R.x2 = L->TransformContext.Sun.maxX;
	R.y1 = L->TransformContext.Sun.minY;
	R.y2 = L->TransformContext.Sun.maxY;
	CHK_DX(HW.GetDevice()->Clear(1L, &R, D3DCLEAR_ZBUFFER, 0xFFFFFFFF, 1.0f, 0L));

	// Stencil - disable
	RenderBackendLegacy.set_Stencil(FALSE);

	// Misc	- draw only front/back-faces
	//if (SE_SUN_NEAR == sub_phase || SE_SUN_MIDDLE == sub_phase)
		RenderBackendLegacy.set_CullMode(CULL_BACKFACE);
	//else
	//	RenderBackendLegacy.set_CullMode(CULL_FRONTFACE);

	RenderBackendLegacy.set_ColorWriteEnable(FALSE);
}

void CRender::render_shadow_map_sun_transluent(light* L, u32 sub_phase)
{
	//VERIFY(RenderImplementation.o.Tshadows);
	u32 _clr = 0xffffffff; // color_rgba(127,127,12,12);
	RenderBackendLegacy.set_ColorWriteEnable();
	CHK_DX(HW.GetDevice()->Clear(0L, NULL, D3DCLEAR_TARGET, _clr, 1.0f, 0L));
}
