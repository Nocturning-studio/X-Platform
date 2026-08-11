///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_output_to_screen : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: output to screen";
	}

	virtual BOOL canBeDetailed()
	{
		return FALSE;
	}

	virtual BOOL canBeLMAPped()
	{
		return FALSE;
	}

	CBlender_output_to_screen()
	{
		description.CLS = 0;
	}

	~CBlender_output_to_screen() = default;

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case 0:
			C.begin_Pass("screen_quad", "output_to_screen_stage");
			C.set_Sampler_point("s_image", r_RT_generic0);
			C.end_Pass();
			break;
		}
	}
};
///////////////////////////////////////////////////////////////////////////////////
