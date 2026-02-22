#include "stdafx.h"

void CRender::accumulate_point_lights(light* L)
{
	set_light_accumulator();
	RenderImplementation.stats.l_visible++;

	ref_shader shader = L->get_shader_point();
	if (!shader)
		shader = RenderTarget->s_accum_point;

	// Common
	float3 L_pos;
	float L_R = L->get_range();
	float3 L_clr;
	L_clr.set(L->get_color().r, L->get_color().g, L->get_color().b);
	Engine.RenderView.View.transform_tiny(L_pos, L->get_position());

	// Transforms
	L->transform_calc();
	RenderBackendLegacy.set_transform_world(L->get_transform());
	RenderBackendLegacy.set_transform_view(Engine.RenderView.View);
	RenderBackendLegacy.set_transform_project(Engine.RenderView.Project);
	enable_scissor(L);
	enable_dbt_bounds(L);

	// *****************************	Mask by stencil		*************************************
	// *** similar to "Carmack's reverse", but assumes convex, non intersecting objects,
	// *** thus can cope without stencil clear with 127 lights
	// *** in practice, 'cause we "clear" it back to 0x1 it usually allows us to > 200 lights :)
	RenderBackendLegacy.set_Element(RenderTarget->s_accum_mask->E[SE_MASK_POINT]); // masker
	RenderBackendLegacy.set_ColorWriteEnable(FALSE);

	// backfaces: if (stencil>=1 && zfail)	stencil = light_id
	RenderBackendLegacy.set_CullMode(CULL_FRONTFACE);
	RenderBackendLegacy.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP,
					   D3DSTENCILOP_REPLACE);
	draw_volume(L);

	// frontfaces: if (stencil>=light_id && zfail)	stencil = 0x1
	RenderBackendLegacy.set_CullMode(CULL_BACKFACE);
	RenderBackendLegacy.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP,
					   D3DSTENCILOP_REPLACE);
	draw_volume(L);

	// *****************************	Minimize overdraw	*************************************
	// Select shader (front or back-faces), *** back, if intersect near plane
	RenderBackendLegacy.set_ColorWriteEnable();
	RenderBackendLegacy.set_CullMode(CULL_FRONTFACE); // back
	/*
	if (bIntersect)	RenderBackendLegacy.set_CullMode		(CULL_FRONTFACE);		// back
	else			RenderBackendLegacy.set_CullMode		(CULL_BACKFACE);		// front
	*/

	// 2D texgens
	float4x4 m_Texgen;
	RenderBackendLegacy.u_compute_texgen_screen(m_Texgen);

	// Draw volume with projective texgen
	{
		// Select shader
		u32 _id = 0;
		if (L->LightFlags.bShadow)
		{
			bool bFullSize = (L->TransformContext.ShadowContext.size == u32(RenderImplementation.o.smapsize));
			if (L->TransformContext.ShadowContext.transluent)
				_id = SE_L_TRANSLUENT;
			else if (bFullSize)
				_id = SE_L_FULLSIZE;
			else
				_id = SE_L_NORMAL;
		}
		else
		{
			_id = SE_L_UNSHADOWED;
			// m_Shadow				= m_Lmap;
		}
		RenderBackendLegacy.set_Element(shader->E[_id]);

		// Constants
		RenderBackendLegacy.set_Constant("Ldynamic_pos", L_pos.x, L_pos.y, L_pos.z, 1 / (L_R * L_R));
		RenderBackendLegacy.set_Constant("Ldynamic_color", sRgbToLinear(L_clr.x), sRgbToLinear(L_clr.y), sRgbToLinear(L_clr.z));
		RenderBackendLegacy.set_Constant("m_texgen", m_Texgen);

		// Render if (stencil >= light_id && z-pass)
		RenderBackendLegacy.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP,
						   D3DSTENCILOP_KEEP);
		draw_volume(L);
	}

	dwLightMarkerID += 2; // keep lowest bit always setted up
	RenderBackendLegacy.SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	u_DBT_disable();
}
