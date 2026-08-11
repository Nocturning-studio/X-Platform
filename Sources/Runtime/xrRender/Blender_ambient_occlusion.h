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
	SE_AO_SSAO,
	SE_AO_HBAO_PLUS,
	SE_AO_GTAO,
	SE_AO_SSAO_PATH_TRACE
};

enum
{
	SE_AO_PASS_CALC = 0,
	SE_AO_PASS_DENOISE = 1
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_ambient_occlusion : public IBlender
{
  public:
	virtual LPCSTR getComment() { return "INTERNAL: Ambient occlusion"; }

	virtual void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case SE_AO_SSAO:
			C.begin_Pass("screen_quad", "ambient_occlusion_stage_pass_ssao", "main", "ao_pass");
			C.set_Sampler_point("s_bent_normals", r_RT_Bent_Normals);
			gbuffer(C);
			jitter(C);
			C.end_Pass();

			C.begin_Pass("screen_quad", "ambient_occlusion_stage_pass_ssao", "main", "denoise_pass");
			C.set_Sampler("s_ao", r_RT_ao);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		case SE_AO_HBAO_PLUS:
			C.begin_Pass("screen_quad", "ambient_occlusion_stage_pass_hbao_plus", "main", "ao_pass");
			C.set_Sampler_point("s_bent_normals", r_RT_Bent_Normals);
			gbuffer(C);
			jitter(C);
			C.end_Pass();

			C.begin_Pass("screen_quad", "ambient_occlusion_stage_pass_hbao_plus", "main", "denoise_pass");
			C.set_Sampler("s_ao", r_RT_ao);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		case SE_AO_GTAO:
			C.begin_Pass("screen_quad", "ambient_occlusion_stage_pass_gtao", "main", "ao_pass");
			C.set_Sampler_point("s_bent_normals", r_RT_Bent_Normals);
			gbuffer(C);
			jitter(C);
			C.end_Pass();

			C.begin_Pass("screen_quad", "ambient_occlusion_stage_pass_gtao", "main", "denoise_pass");
			C.set_Sampler("s_ao", r_RT_ao);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		case SE_AO_SSAO_PATH_TRACE:
			C.begin_Pass("screen_quad", "ambient_occlusion_stage_pass_ssao_pt", "main", "ao_pass");
			C.set_Sampler_point("s_bent_normals", r_RT_Bent_Normals);
			gbuffer(C);
			jitter(C);
			C.end_Pass();

			C.begin_Pass("screen_quad", "ambient_occlusion_stage_pass_ssao_pt", "main", "denoise_pass");
			C.set_Sampler("s_ao", r_RT_ao);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		}
	}

	CBlender_ambient_occlusion() = default;
	virtual ~CBlender_ambient_occlusion() = default;
};
///////////////////////////////////////////////////////////////////////////////////
