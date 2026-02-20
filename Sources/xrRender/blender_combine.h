///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
enum
{
	SE_PRECOMBINE_SCENE,
	SE_COMBINE_SCENE,
	SE_COMBINE_POSTPROCESS,
	SE_COMBINE_VOLUMETRIC
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_combine : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: combiner";
	}

	virtual void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		CBlender_Compile::PassDesc PassDescription;
		PassDescription.VertexShader = "screen_quad";
		PassDescription.PixelShader = "scene_combine_stage";

		switch (C.iElement)
		{
		case SE_PRECOMBINE_SCENE:
			C.set_Define("USE_FOR_PRECOMBINE", 1);
			C.set_Define("DISABLE_REFLECTIONS", 1);
		case SE_COMBINE_SCENE:
			PassDescription.EnableAlphaBlend = true;
			PassDescription.BlendSRC = D3DBLEND_INVSRCALPHA;
			PassDescription.BlendDST = D3DBLEND_SRCALPHA;
			C.begin_Pass(PassDescription);
			C.set_Sampler_point("s_light_accumulator", r_RT_Light_Accumulator);
			C.set_Sampler_point("s_ao", r_RT_ao);
			C.set_Sampler("env_s0", r_T_irradiance0, false, D3DTADDRESS_CLAMP, D3DTEXF_GAUSSIANQUAD, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
			C.set_Sampler("env_s1", r_T_irradiance1, false, D3DTADDRESS_CLAMP, D3DTEXF_GAUSSIANQUAD, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
			C.set_Sampler("sky_s0", r_T_sky0, false, D3DTADDRESS_CLAMP, D3DTEXF_GAUSSIANQUAD, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
			C.set_Sampler("sky_s1", r_T_sky1, false, D3DTADDRESS_CLAMP, D3DTEXF_GAUSSIANQUAD, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
			C.set_Sampler_linear("s_brdf_lut", "vfx\\vfx_brdf_lut");
			C.set_Sampler("s_image", r_RT_backbuffer_mip, false, D3DTADDRESS_CLAMP, D3DTEXF_GAUSSIANQUAD, D3DTEXF_GAUSSIANQUAD, D3DTEXF_GAUSSIANQUAD, false);
			C.set_Sampler_linear("s_reflections", r_RT_reflections);
			C.set_Sampler_point("s_bent_normals", r_RT_Bent_Normals);
			C.set_Sampler("s_luminance_previous", r_RT_autoexposure_luminance_previous);
			C.set_Sampler("s_luminance", r_RT_autoexposure_luminance);
			C.set_Sampler("s_luminance_mip_chain", r_RT_autoexposure_mip_chain);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		case SE_COMBINE_POSTPROCESS:
			PassDescription.PixelShader += "_pass_postprocess";
			PassDescription.EnableAlphaBlend = false;
			C.begin_Pass(PassDescription);
			C.set_Sampler_point("s_image", r_RT_generic1);
			C.set_Sampler_linear("s_bloom", r_RT_bloom1);
			C.set_Sampler_linear("s_bloom_blades", r_RT_bloom_blades1);
			C.set_Sampler_point("s_light_accumulator", r_RT_Light_Accumulator);
			jitter(C);
			gbuffer(C);
			C.end_Pass();
			break;
		case SE_COMBINE_VOLUMETRIC:
			PassDescription.PixelShader += "_apply_volumetric";
			PassDescription.EnableAlphaBlend = false;
			C.begin_Pass(PassDescription);
			C.set_Sampler_point("s_image", r_RT_generic1);
			C.set_Sampler_linear("s_volumetric_accumulator", r_RT_Volumetric_Sun);
			jitter(C);
			gbuffer(C);
			C.end_Pass();
			break;
		}
	}

	CBlender_combine() = default;
	virtual ~CBlender_combine() = default;
};
///////////////////////////////////////////////////////////////////////////////////
