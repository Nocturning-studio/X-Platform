#include "stdafx.h"

void CRender::clear_shadow_map_spot()
{
	/*
	if (RenderImplementation.b_HW_smap)		set_Render_Target_Surface	(rt_smap_surf, NULL, NULL, NULL, rt_smap_d_depth->pRT);
	else								set_Render_Target_Surface	(rt_smap_surf, NULL, NULL, NULL, rt_smap_d_ZB);
	CHK_DX								(HW.GetDevice()->Clear( 0L, NULL, D3DCLEAR_ZBUFFER,	0xffffffff,	1.0f, 0L));
	*/
}

void CRender::render_shadow_map_spot(light* L)
{
	// Targets + viewport
	RenderBackendLegacy.set_Render_Target_Surface(RenderTarget->rt_smap_surf);
	RenderBackendLegacy.set_Depth_Buffer(RenderTarget->rt_smap_depth->pRT);

	D3DVIEWPORT9 VP = {L->TransformContext.ShadowContext.posX, L->TransformContext.ShadowContext.posY, L->TransformContext.ShadowContext.size, L->TransformContext.ShadowContext.size, 0, 1};
	CHK_DX(HW.GetDevice()->SetViewport(&VP));

	// Misc	- draw only front-faces
	RenderBackendLegacy.set_CullMode(CULL_BACKFACE);
	RenderBackendLegacy.set_Stencil(FALSE);
	// no transparency
#pragma todo("can optimize for multi-lights covering more than say 50%...")

	RenderBackendLegacy.set_ColorWriteEnable(FALSE);
	CHK_DX(HW.GetDevice()->Clear(0L, NULL, D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0L));
}

void CRender::render_shadow_map_spot_transluent(light* L)
{
	//VERIFY(RenderImplementation.o.Tshadows);
	RenderBackendLegacy.set_ColorWriteEnable();
	if (IRender_Light::OMNIPART == L->LightFlags.type)
	{
		// omni-part
		CHK_DX(HW.GetDevice()->Clear(0L, NULL, D3DCLEAR_TARGET, 0xffffffff, 1.0f, 0L));
	}
	else
	{
		// real-spot
		// Select color-mask
		ref_shader shader = L->get_shader_spot();
		if (!shader)
			shader = RenderTarget->s_accum_spot;
		RenderBackendLegacy.set_Element(shader->E[SE_L_FILL]);

		// Fill vertex buffer
		float2 p0, p1;
		u32 Offset;
		u32 C = color_rgba(255, 255, 255, 255);
		float _w = float(L->TransformContext.ShadowContext.size);
		float _h = float(L->TransformContext.ShadowContext.size);
		float d_Z = EPS_S;
		float d_W = 1.f;
		p0.set(.5f / _w, .5f / _h);
		p1.set((_w + .5f) / _w, (_h + .5f) / _h);

		FVF::TL* pv = (FVF::TL*)RenderBackendLegacy.Vertex.Lock(4, RenderBackendLegacy.g_viewport->vb_stride, Offset);
		pv->set(EPS, float(_h + EPS), d_Z, d_W, C, p0.x, p1.y);
		pv++;
		pv->set(EPS, EPS, d_Z, d_W, C, p0.x, p0.y);
		pv++;
		pv->set(float(_w + EPS), float(_h + EPS), d_Z, d_W, C, p1.x, p1.y);
		pv++;
		pv->set(float(_w + EPS), EPS, d_Z, d_W, C, p1.x, p0.y);
		pv++;
		RenderBackendLegacy.Vertex.Unlock(4, RenderBackendLegacy.g_viewport->vb_stride);
		RenderBackendLegacy.set_Geometry(RenderBackendLegacy.g_viewport);

		// draw
		RenderBackendLegacy.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}
