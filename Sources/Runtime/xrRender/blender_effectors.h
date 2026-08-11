///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
enum
{
	SE_PASS_COMBINE,
	SE_PASS_RADIATION,
	SE_PASS_RESOLVE_GAMMA,
	SE_PASS_LUT,
	SE_PASS_COLOR_BLIND_FILTER,
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_effectors : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: resolve effectors";
	}

	virtual void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case SE_PASS_COMBINE:
			C.begin_Pass("null", "postprocess_stage_pass_combine_effectors");
			C.set_Sampler_point("s_image", r_RT_generic0);
			C.set_Sampler_linear("s_grad0", r_colormap0);
			C.set_Sampler_linear("s_grad1", r_colormap1);
			C.set_Sampler_linear("s_noise0", r_RT_radiation_noise0);
			C.set_Sampler_linear("s_noise1", r_RT_radiation_noise1);
			C.set_Sampler_linear("s_noise2", r_RT_radiation_noise2);
			C.end_Pass();
			break;
		case SE_PASS_RADIATION:
			C.begin_Pass("screen_quad", "postprocess_stage_pass_radiation");
			C.end_Pass();
			break;
		case SE_PASS_RESOLVE_GAMMA:
			C.begin_Pass("screen_quad", "postprocess_stage_pass_resolve_gamma");
			C.set_Sampler_point("s_image", r_RT_generic1);
			C.end_Pass();
			break;
		case SE_PASS_LUT:
			C.begin_Pass("screen_quad", "postprocess_stage_pass_lut");
			C.set_Sampler_point("s_image", r_RT_generic0);
			C.set_Sampler_linear("s_env_lut0", r_T_LUTs0);
			C.set_Sampler_linear("s_env_lut1", r_T_LUTs1);
			C.end_Pass();

			C.begin_Pass("screen_quad", "simple_image");
			C.set_Sampler_point("s_image", r_RT_generic1);
			C.end_Pass();
			break;
		case SE_PASS_COLOR_BLIND_FILTER:
			C.begin_Pass("screen_quad", "postprocess_stage_pass_color_blind_filter");
			C.set_Sampler_point("s_image", r_RT_generic1);
			jitter(C);
			C.end_Pass();

			C.begin_Pass("null", "simple_image");
			C.set_Sampler_point("s_image", r_RT_generic0);
			C.end_Pass();
			break;
		}
	}

	CBlender_effectors() = default;
	virtual ~CBlender_effectors() = default;
};
///////////////////////////////////////////////////////////////////////////////////
