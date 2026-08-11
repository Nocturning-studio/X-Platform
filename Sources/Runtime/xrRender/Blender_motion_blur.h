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
	SE_PASS_PREPARE_DILATION_MAP,
	SE_PASS_BLUR_DILATION_MAP,
	SE_PASS_BLUR_FRAME,
	SE_PASS_SAVE_DEPTH_BUFFER,
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_motion_blur : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: Motion blur";
	}

	virtual void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		LPCSTR sh_name = "postprocess_stage_motion_blur";

		switch (C.iElement)
		{
		case SE_PASS_SAVE_DEPTH_BUFFER:
			C.begin_Pass("screen_quad", sh_name, "main", "SaveDepth");
			gbuffer(C);
			C.end_Pass();
			break;

		case SE_PASS_PREPARE_DILATION_MAP:
			C.begin_Pass("screen_quad", sh_name, "main", "CreateDilationMap");
			C.set_Sampler_point("s_image", r_RT_generic0);
			C.set_Sampler_point("s_previous_depth", r_RT_mblur_previous_frame_depth);
			gbuffer(C);
			C.end_Pass();
			break;

		case SE_PASS_BLUR_DILATION_MAP:
			// Фильтрация самой карты скоростей (можно оставить 2 прохода для дешевизны)
			C.set_Define("USE_VERTICAL_FILTER", "1");
			C.begin_Pass("screen_quad", sh_name, "main", "BlurDilationMap");
			C.set_Sampler_gaussian("s_dilation_map", r_RT_mblur_dilation_map_0);
			C.end_Pass();

			C.set_Define("USE_HORIZONTAL_FILTER", "1");
			C.begin_Pass("screen_quad", sh_name, "main", "BlurDilationMap");
			C.set_Sampler_gaussian("s_dilation_map", r_RT_mblur_dilation_map_1);
			C.end_Pass();
			break;

		case SE_PASS_BLUR_FRAME:
			// [ИЗМЕНЕНО] Финальное размытие теперь в ОДИН проход
			// Мы не делим его на H/V, так как размытие диагональное (вдоль вектора)
			C.begin_Pass("screen_quad", sh_name, "main", "Blur");

			C.set_Sampler_point("s_image", r_RT_generic0);						 // Исходная картинка
			C.set_Sampler_gaussian("s_dilation_map", r_RT_mblur_dilation_map_0); // Векторы
			gbuffer(C);
			C.end_Pass();
			break;
		}
	}

	CBlender_motion_blur() = default;
	virtual ~CBlender_motion_blur() = default;
};
///////////////////////////////////////////////////////////////////////////////////
