#include "stdafx.h"

void CRender::accumulate_point_lights(light* L)
{
	set_light_accumulator();
	RenderImplementation.stats.l_visible++;

	ref_shader shader = L->get_shader_point();
	if (!shader)
		shader = RenderTarget->s_accum_point;

	// Common
	Fvector L_pos;
	float L_R = L->get_range();
	Fvector L_clr;
	L_clr.set(L->get_color().r, L->get_color().g, L->get_color().b);
	Device.mView.transform_tiny(L_pos, L->get_position());

	// Xforms
	L->xform_calc();
	RenderBackend.set_xform_world(L->get_xform());
	RenderBackend.set_xform_view(Device.mView);
	RenderBackend.set_xform_project(Device.mProject);
	enable_scissor(L);
	enable_dbt_bounds(L);

	// *****************************	Mask by stencil		*************************************
	// *** similar to "Carmack's reverse", but assumes convex, non intersecting objects,
	// *** thus can cope without stencil clear with 127 lights
	// *** in practice, 'cause we "clear" it back to 0x1 it usually allows us to > 200 lights :)
	RenderBackend.set_Element(RenderTarget->s_accum_mask->E[SE_MASK_POINT]); // masker
	RenderBackend.set_ColorWriteEnable(FALSE);

	// backfaces: if (stencil>=1 && zfail)	stencil = light_id
	RenderBackend.set_CullMode(CULL_FRONTFACE);
	RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP,
					   D3DSTENCILOP_REPLACE);
	draw_volume(L);

	// frontfaces: if (stencil>=light_id && zfail)	stencil = 0x1
	RenderBackend.set_CullMode(CULL_BACKFACE);
	RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP,
					   D3DSTENCILOP_REPLACE);
	draw_volume(L);

	// *****************************	Minimize overdraw	*************************************
	// Select shader (front or back-faces), *** back, if intersect near plane
	RenderBackend.set_ColorWriteEnable();
	RenderBackend.set_CullMode(CULL_FRONTFACE); // back
	/*
	if (bIntersect)	RenderBackend.set_CullMode		(CULL_FRONTFACE);		// back
	else			RenderBackend.set_CullMode		(CULL_BACKFACE);		// front
	*/

	// 2D texgens
	Fmatrix m_Texgen;
	RenderBackend.u_compute_texgen_screen(m_Texgen);

	// Draw volume with projective texgen
	{
		// Select shader
		u32 _id = 0;
		if (L->flags.bShadow)
		{
			bool bFullSize = (L->X.S.size == u32(RenderImplementation.o.smapsize));
			if (L->X.S.transluent)
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
		RenderBackend.set_Element(shader->E[_id]);

		// Constants
		RenderBackend.set_Constant("Ldynamic_pos", L_pos.x, L_pos.y, L_pos.z, 1 / (L_R * L_R));
		RenderBackend.set_Constant("Ldynamic_color", sRgbToLinear(L_clr.x), sRgbToLinear(L_clr.y), sRgbToLinear(L_clr.z));
		RenderBackend.set_Constant("m_texgen", m_Texgen);

		// Render if (stencil >= light_id && z-pass)
		RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP,
						   D3DSTENCILOP_KEEP);
		draw_volume(L);
	}

	dwLightMarkerID += 2; // keep lowest bit always setted up
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE));

	u_DBT_disable();
}
