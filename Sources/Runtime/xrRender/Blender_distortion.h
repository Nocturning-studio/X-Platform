///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_distortion : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: Distortion";
	}

	CBlender_distortion()
	{
		description.CLS = 0;
	}

	~CBlender_distortion()
	{
	}

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case 0:
			C.begin_Pass("screen_quad", "postprocess_stage_distortion");
			gbuffer(C);
			C.set_Sampler_point("s_image", r_RT_generic1);
			C.set_Sampler_linear("s_distort", r_RT_distortion_mask);
			jitter(C);
			C.end_Pass();
			break;
		}
	}
};
///////////////////////////////////////////////////////////////////////////////////