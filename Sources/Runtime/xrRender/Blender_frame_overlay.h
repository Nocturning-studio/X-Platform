///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
enum
{
	SE_OVERLAYS_MAIN,
	SE_OVERLAYS_WATERMARK
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_frame_overlay : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: Frame overlay";
	}

	~CBlender_frame_overlay() = default;

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		CBlender_Compile::PassDesc PassDescription;

		switch (C.iElement)
		{
		case SE_OVERLAYS_MAIN:
			PassDescription.VertexShader = "screen_quad";
			PassDescription.PixelShader = "overlay_stage_apply";
			C.begin_Pass(PassDescription);
			C.set_Sampler_point("s_image", r_RT_generic1);
			C.end_Pass();
			break;		
		case SE_OVERLAYS_WATERMARK:
			PassDescription.VertexShader = "screen_quad";
			PassDescription.PixelShader = "simple_image";
			PassDescription.EnableAlphaBlend = true;
			PassDescription.BlendSRC = D3DBLEND_SRCALPHA;
			PassDescription.BlendDST = D3DBLEND_INVSRCALPHA;
			C.begin_Pass(PassDescription);
			C.set_Sampler_linear_wrap("s_image", "vfx\\vfx_watermark");
			C.end_Pass();
			break;
		}
	}
};
///////////////////////////////////////////////////////////////////////////////////
