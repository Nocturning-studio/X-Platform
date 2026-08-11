///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_accum_point : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: accumulate point light";
	}

	~CBlender_accum_point() = default;

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		CBlender_Compile::PassDesc PassDescription;
		PassDescription.VertexShader = "accumulating_light_stage_volume";
		PassDescription.PixelShader = "accumulating_light_stage_point";
		PassDescription.EnableAlphaBlend = true;
		PassDescription.BlendSRC = D3DBLEND_ONE;
		PassDescription.BlendDST = D3DBLEND_ONE;

		switch (C.iElement)
		{
		case SE_L_FILL: // fill projective
			C.begin_Pass("null", "simple_image");
			C.set_Sampler("s_image", C.L_textures[0]);
			C.end_Pass();
			break;
		case SE_L_UNSHADOWED: // unshadowed
			C.begin_Pass(PassDescription);
			C.set_Sampler_linear("s_lmap", *C.L_textures[0]);
			gbuffer(C);
			C.end_Pass();
			break;
		case SE_L_NORMAL: // normal
			C.set_Define("USE_SHADOW_MAPPING", "1", CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("SMAP_SIZE", (int)RenderImplementation.o.smapsize, CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("SHADOW_FILTER_QUALITY", (int)ps_r_shadow_filtering, CBlender_Compile::ShaderScope::Pixel);
			C.begin_Pass(PassDescription);
			C.set_Sampler("s_lmap", C.L_textures[0]);
			C.set_Sampler("s_smap", r_RT_smap_depth);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		case SE_L_FULLSIZE: // normal-fullsize
			C.set_Define("USE_SHADOW_MAPPING", "1", CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("SMAP_SIZE", (int)RenderImplementation.o.smapsize, CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("SHADOW_FILTER_QUALITY", (int)ps_r_shadow_filtering, CBlender_Compile::ShaderScope::Pixel);
			C.begin_Pass(PassDescription);
			C.set_Sampler("s_lmap", C.L_textures[0]);
			C.set_Sampler("s_smap", r_RT_smap_depth);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		case SE_L_TRANSLUENT: // shadowed + transluency
			C.set_Define("SMAP_SIZE", (int)RenderImplementation.o.smapsize, CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("SHADOW_FILTER_QUALITY", (int)ps_r_shadow_filtering, CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("USE_SHADOW_MAPPING", "1", CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("USE_LIGHT_MAPPING", "1", CBlender_Compile::ShaderScope::Pixel);
			C.begin_Pass(PassDescription);
			C.set_Sampler("s_lmap", r_RT_smap_surf);
			C.set_Sampler("s_smap", r_RT_smap_depth);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		}
	}

};
///////////////////////////////////////////////////////////////////////////////////
