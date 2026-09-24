#include "stdafx.h"

void CRender::clear_shadow_map_spot()
{
}

void CRender::render_shadow_map_spot(light* L)
{
	// Targets + viewport
	RenderBackend.SetRenderTarget(RenderTarget->rt_smap_surf);
	RenderBackend.SetDepthBuffer(RenderTarget->rt_smap_depth->pRT);

	D3DVIEWPORT9 VP = {L->TransformContext.ShadowContext.posX, L->TransformContext.ShadowContext.posY, L->TransformContext.ShadowContext.size, L->TransformContext.ShadowContext.size, 0, 1};
	RenderBackend.SetViewport(VP);

	// Misc	- draw only front-faces
	RenderBackend.SetCullMode(CULL_BACKFACE);
	RenderBackend.SetStencil(FALSE);
#pragma todo("can optimize for multi-lights covering more than say 50%...")

	RenderBackend.SetColorWriteEnable(FALSE);
	RenderBackend.Clear(0L, NULL, D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0L);
}
