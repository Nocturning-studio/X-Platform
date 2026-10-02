#include "pch.h"

void CRender::render_shadow_map_sun(light* L, u32 sub_phase)
{
	RenderBackendLegacy.SetRenderTarget(RenderTarget->rt_smap_surf);
	RenderBackendLegacy.SetDepthBuffer(RenderTarget->rt_smap_depth->pRT);

	D3DRECT R;
	R.x1 = L->TransformContext.Sun.minX;
	R.x2 = L->TransformContext.Sun.maxX;
	R.y1 = L->TransformContext.Sun.minY;
	R.y2 = L->TransformContext.Sun.maxY;
	RenderBackendLegacy.Clear(1L, &R, D3DCLEAR_ZBUFFER, 0xFFFFFFFF, 1.0f, 0L);

	RenderBackendLegacy.SetStencil(FALSE);
	RenderBackendLegacy.SetCullMode(CULL_BACKFACE);
	RenderBackendLegacy.SetColorWriteEnable(FALSE);
}
