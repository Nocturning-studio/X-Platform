///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_light_occq : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: occlusion testing";
	}

	~CBlender_light_occq() = default;

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case 0: // occlusion testing
			C.begin_Pass("accumulating_light_stage_occlusion_culling", "accumulating_light_stage_occlusion_culling",
						 "main", "main", false,
					 TRUE, FALSE, FALSE);
			C.end_Pass();
			break;
		}
	}
};
///////////////////////////////////////////////////////////////////////////////////
