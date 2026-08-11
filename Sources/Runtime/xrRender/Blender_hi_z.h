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
	SE_HI_Z_GENERATE_MIP_CHAIN_PASS,
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_hi_z : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: Reflections";
	}

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case SE_HI_Z_GENERATE_MIP_CHAIN_PASS:
			C.begin_Pass("screen_quad", "hi_z_stage_pass_copy_buffer");
			gbuffer(C);
			C.end_Pass();

			C.begin_Pass("screen_quad", "hi_z_stage_pass_generate");
			C.set_Sampler("s_hi_z_mip_chain", r_RT_Hi_z, false, D3DTADDRESS_CLAMP, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_POINT, false);
			gbuffer(C);
			C.end_Pass();
			break;
		}
	}

	~CBlender_hi_z() = default;
};
///////////////////////////////////////////////////////////////////////////////////

