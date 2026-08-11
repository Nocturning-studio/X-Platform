#include "stdafx.h"

extern fvec3 du_cone_vertices[DU_CONE_NUMVERTEX];

void CRender::accumulate_spot_lights(light* L)
{
	set_light_accumulator();
	RenderImplementation.stats.l_visible++;

	// *** assume accumulator setted up ***
	// *****************************	Mask by stencil		*************************************
	ref_shader shader;
	if (IRender_Light::OMNIPART == L->LightFlags.type)
	{
		shader = L->get_shader_point();
		if (!shader)
			shader = RenderTarget->s_accum_point;
	}
	else
	{
		shader = L->get_shader_spot();
		if (!shader)
			shader = RenderTarget->s_accum_spot;
	}

	BOOL bIntersect = FALSE; // enable_scissor(L);
	{
		// setup transform
		L->transform_calc();
		RenderBackend.set_transform_world(L->get_transform());
		RenderBackend.set_transform_view(Engine.RenderView.View);
		RenderBackend.set_transform_project(Engine.RenderView.Project);
		bIntersect = enable_scissor(L);
		enable_dbt_bounds(L);

		// *** similar to "Carmack's reverse", but assumes convex, non intersecting objects,
		// *** thus can cope without stencil clear with 127 lights
		// *** in practice, 'cause we "clear" it back to 0x1 it usually allows us to > 200 lights :)
		RenderBackend.set_ColorWriteEnable(FALSE);
		RenderBackend.set_Element(RenderTarget->s_accum_mask->E[SE_MASK_SPOT]); // masker

		// backfaces: if (stencil>=1 && zfail)			stencil = light_id
		RenderBackend.set_CullMode(CULL_FRONTFACE);
		RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE);
		draw_volume(L);

		// frontfaces: if (stencil>=light_id && zfail)	stencil = 0x1
		RenderBackend.set_CullMode(CULL_BACKFACE);
		RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE);
		draw_volume(L);
	}

	// *****************************	Minimize overdraw	*************************************
	// Select shader (front or back-faces), *** back, if intersect near plane
	RenderBackend.set_ColorWriteEnable();
	RenderBackend.set_CullMode(CULL_FRONTFACE); // back

	// 2D texgens
	fmat4x4 m_Texgen;
	RenderBackend.u_compute_texgen_screen(m_Texgen);

	// Shadow transform (+texture adjustment matrix)
	fmat4x4 m_Shadow, m_Lmap;
	{
		float smapsize = float(RenderImplementation.o.smapsize);
		float fTexelOffs = (.5f / smapsize);
		float view_dim = float(L->TransformContext.ShadowContext.size - 2) / smapsize;
		float view_sx = float(L->TransformContext.ShadowContext.posX + 1) / smapsize;
		float view_sy = float(L->TransformContext.ShadowContext.posY + 1) / smapsize;
		float fRange = float(1.f) * ps_r_ls_depth_scale;
		float fBias = ps_r_ls_depth_bias;
		fmat4x4 m_TexelAdjust = {view_dim / 2.f,
								 0.0f,
								 0.0f,
								 0.0f,
								 0.0f,
								 -view_dim / 2.f,
								 0.0f,
								 0.0f,
								 0.0f,
								 0.0f,
								 fRange,
								 0.0f,
								 view_dim / 2.f + view_sx + fTexelOffs,
								 view_dim / 2.f + view_sy + fTexelOffs,
								 fBias,
								 1.0f};

		// compute transforms
		fmat4x4 xf_world;
		xf_world.invert(Engine.RenderView.View);
		fmat4x4 xf_view = L->TransformContext.ShadowContext.view;
		fmat4x4 xf_project;
		xf_project.mul(m_TexelAdjust, L->TransformContext.ShadowContext.project);
		m_Shadow.mul(xf_view, xf_world);
		m_Shadow.mulA_44(xf_project);

		// lmap
		view_dim = 1.f;
		view_sx = 0.f;
		view_sy = 0.f;
		fmat4x4 m_TexelAdjust2 = {view_dim / 2.f,
								  0.0f,
								  0.0f,
								  0.0f,
								  0.0f,
								  -view_dim / 2.f,
								  0.0f,
								  0.0f,
								  0.0f,
								  0.0f,
								  fRange,
								  0.0f,
								  view_dim / 2.f + view_sx + fTexelOffs,
								  view_dim / 2.f + view_sy + fTexelOffs,
								  fBias,
								  1.0f};

		// compute transforms
		xf_project.mul(m_TexelAdjust2, L->TransformContext.ShadowContext.project);
		m_Lmap.mul(xf_view, xf_world);
		m_Lmap.mulA_44(xf_project);
	}

	// Common constants
	fvec3 L_dir, L_clr, L_pos;
	L_clr.set(L->get_color().r, L->get_color().g, L->get_color().b);
	L_clr.mul(L->get_LOD());
	Engine.RenderView.View.transform_tiny(L_pos, L->get_position());
	Engine.RenderView.View.transform_dir(L_dir, L->get_direction());
	L_dir.normalize();

	// Draw volume with projective texgen
	{
		// Select shader
		u32 _id = 0;
		if (L->LightFlags.bShadow)
		{
			bool bFullSize = (L->TransformContext.ShadowContext.size == RenderImplementation.o.smapsize);
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
			m_Shadow = m_Lmap;
		}
		RenderBackend.set_Element(shader->E[_id]);

		// Constants
		float att_R = L->get_range() * .95f;
		float att_factor = 1.f / (att_R * att_R); 
		
		// ѕолучаем параметры spot света
		float spot_cutoff = L->get_cone(); // внешний угол (в радианах)

		// ¬ычисл€ем внутренний и внешний углы
		// ќбычно внутренний угол составл€ет 80-90% от внешнего
		float spot_inner_angle = spot_cutoff * 0.8f; // внутренний угол = 80% от внешнего
		float spot_outer_angle = spot_cutoff;		 // внешний угол

		//  онвертируем углы в косинусы дл€ шейдера
		float cos_inner = cosf(spot_inner_angle);
		float cos_outer = cosf(spot_outer_angle);

		float LightSourceRangeSqr = L->get_range() * L->get_range();

		RenderBackend.set_Constant("Ldynamic_pos", L_pos.x, L_pos.y, L_pos.z, att_factor);
		RenderBackend.set_Constant("Ldynamic_spot_att", cos_inner, cos_outer, LightSourceRangeSqr, 0);
		RenderBackend.set_Constant("Ldynamic_color", sRgbToLinear(L_clr.x), sRgbToLinear(L_clr.y), sRgbToLinear(L_clr.z));
		RenderBackend.set_Constant("m_texgen", m_Texgen);
		RenderBackend.set_Constant("m_shadow", m_Shadow);
		RenderBackend.set_Array_Constant("m_lmap", 0, m_Lmap._11, m_Lmap._21, m_Lmap._31, m_Lmap._41);
		RenderBackend.set_Array_Constant("m_lmap", 1, m_Lmap._12, m_Lmap._22, m_Lmap._32, m_Lmap._42);

		RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);
		draw_volume(L);
	}

	dwLightMarkerID += 2; // keep lowest bit always setted up
	RenderBackend.SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	u_DBT_disable();
}
