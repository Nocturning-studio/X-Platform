///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_accum_direct_mask : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: mask direct light";
	}

	~CBlender_accum_direct_mask() = default;

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case SE_MASK_SPOT: // spot or omni-part
			C.begin_Pass("accumulating_light_stage_mask", "dumb", "main", "main", false, TRUE, FALSE);
			gbuffer(C);
			C.end_Pass();
			break;
		case SE_MASK_POINT: // point
			C.begin_Pass("accumulating_light_stage_mask", "dumb", "main", "main", false, TRUE, FALSE);
			gbuffer(C);
			C.end_Pass();
			break;
		case SE_MASK_DIRECT: // stencil mask for directional light
			C.begin_Pass("screen_quad", "accumulating_light_stage_sun_mask", "main", "main", false, FALSE, FALSE, TRUE, D3DBLEND_ZERO, D3DBLEND_ONE);
			gbuffer(C);
			C.end_Pass();
			break;
		}
	}
};
///////////////////////////////////////////////////////////////////////////////////
