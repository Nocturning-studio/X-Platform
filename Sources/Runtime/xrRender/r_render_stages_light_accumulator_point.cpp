#include "stdafx.h"

void CRender::accumulate_point_lights(light* L)
{
	set_light_accumulator();
	RenderImplementation.stats.l_visible++;

	ref_shader shader = L->get_shader_point();
	if(!shader)
		shader = RenderTarget->s_accum_point;

	// Common
	fvec3 L_pos;
	float L_R = L->get_range();
	fvec3 L_clr;
	L_clr.set(L->get_color().r, L->get_color().g, L->get_color().b);
	Engine.RenderView.View.transform_tiny(L_pos, L->get_position());

	// Transforms
	L->transform_calc();
	RenderBackend.SetTransformWorld(L->get_transform());
	RenderBackend.SetTransformView(Engine.RenderView.View);
	RenderBackend.SetTransformProject(Engine.RenderView.Project);
	enable_scissor(L);

	// *****************************	Mask by stencil		*************************************
	// *** similar to "Carmack's reverse", but assumes convex, non intersecting objects,
	// *** thus can cope without stencil clear with 127 lights
	// *** in practice, 'cause we "clear" it back to 0x1 it usually allows us to > 200 lights :)
	RenderBackend.SetShaderElement(RenderTarget->s_accum_mask->E[SE_MASK_POINT]); // masker
	RenderBackend.SetColorWriteEnable(FALSE);

	// backfaces: if (stencil>=1 && zfail)	stencil = light_id
	RenderBackend.SetCullMode(CULL_FRONTFACE);
	RenderBackend.SetStencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE);
	draw_volume(L);

	// frontfaces: if (stencil>=light_id && zfail)	stencil = 0x1
	RenderBackend.SetCullMode(CULL_BACKFACE);
	RenderBackend.SetStencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE);
	draw_volume(L);

	// *****************************	Minimize overdraw	*************************************
	// Select shader (front or back-faces), *** back, if intersect near plane
	RenderBackend.SetColorWriteEnable();
	RenderBackend.SetCullMode(CULL_FRONTFACE); // back
	/*
	if (bIntersect)	RenderBackend.SetCullMode		(CULL_FRONTFACE);		// back
	else			RenderBackend.SetCullMode		(CULL_BACKFACE);		// front
	*/

	// 2D texgens
	fmat4x4 m_Texgen;
	RenderBackend.ComputeTexgenScreen(m_Texgen);

	// Draw volume with projective texgen
	{
		// Select shader
		u32 _id = 0;
		if(L->LightFlags.bShadow)
		{
			bool bFullSize = (L->TransformContext.ShadowContext.size == u32(RenderImplementation.o.smapsize));
			if(L->TransformContext.ShadowContext.transluent)
				_id = SE_L_TRANSLUENT;
			else if(bFullSize)
				_id = SE_L_FULLSIZE;
			else
				_id = SE_L_NORMAL;
		}
		else
		{
			_id = SE_L_UNSHADOWED;
			// m_Shadow				= m_Lmap;
		}
		RenderBackend.SetShaderElement(shader->E[_id]);

		// Constants
		RenderBackend.SetConstant("Ldynamic_pos", L_pos.x, L_pos.y, L_pos.z, 1 / (L_R * L_R));
		RenderBackend.SetConstant("Ldynamic_color", sRgbToLinear(L_clr.x), sRgbToLinear(L_clr.y), sRgbToLinear(L_clr.z));
		RenderBackend.SetConstant("m_texgen", m_Texgen);

		// Render if (stencil >= light_id && z-pass)
		RenderBackend.SetStencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP);
		draw_volume(L);
	}

	dwLightMarkerID += 2; // keep lowest bit always setted up
	RenderBackend.SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
}
