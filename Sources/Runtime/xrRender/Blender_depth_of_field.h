///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
enum
{
	SE_PASS_DOF_CALC_COC,	   // 0
	SE_PASS_DOF_TILE_DILATION, // 1
	SE_PASS_DOF_SEPARATE,	   // 2
	SE_PASS_DOF_BLUR_FAR,	   // 3
	SE_PASS_DOF_BLUR_NEAR,	   // 4
	SE_PASS_DOF_COMPOSITE	   // 5
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_depth_of_field : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: FidelityFX DoF Implementation";
	}

	virtual void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);
		LPCSTR sh_name = "postprocess_stage_depth_of_field";

		switch (C.iElement)
		{
		// 1. Calc CoC (Pixel Exact)
		case SE_PASS_DOF_CALC_COC:
			C.begin_Pass("screen_quad", sh_name, "main", "PassCalcCoC");
			gbuffer(C);
			C.end_Pass();
			break;

		// 2. Tile Dilation (Low Res)
		// Важно: читаем CoC, пишем в rt_dof_dilation
		case SE_PASS_DOF_TILE_DILATION:
			C.begin_Pass("screen_quad", sh_name, "main", "PassTileDilation");
			C.set_Sampler_point("s_dof_coc", r_RT_dof_coc);
			C.end_Pass();
			break;

		// 3. Separate (Half Res)
		case SE_PASS_DOF_SEPARATE:
			C.begin_Pass("screen_quad", sh_name, "main", "PassSeparate");
			C.set_Sampler_linear("s_image", r_RT_generic0);
			C.set_Sampler_point("s_dof_coc", r_RT_dof_coc);
			C.end_Pass();
			break;

		// 4. Blur Far (Half Res)
		case SE_PASS_DOF_BLUR_FAR:
			C.begin_Pass("screen_quad", sh_name, "main", "PassBlurFar");
			C.set_Sampler_linear("s_image", r_RT_dof_far);
			C.set_Sampler_linear("s_dof_coc", r_RT_dof_coc);
			C.end_Pass();
			break;

		// 5. Blur Near (Half Res)
		case SE_PASS_DOF_BLUR_NEAR:
			C.begin_Pass("screen_quad", sh_name, "main", "PassBlurNear");
			C.set_Sampler_linear("s_image", r_RT_dof_near);
			C.set_Sampler_linear("s_dof_coc", r_RT_dof_coc);
			C.set_Sampler_linear("s_dof_dilation", r_RT_dof_dilation);
			C.end_Pass();
			break;

		// 6. Composite (Full Res)
		case SE_PASS_DOF_COMPOSITE:
			C.begin_Pass("screen_quad", sh_name, "main", "PassComposite");
			C.set_Sampler_point("s_image", r_RT_generic0);	   // Sharp
			C.set_Sampler_linear("s_dof_far", r_RT_dof_far);   // Blur
			C.set_Sampler_linear("s_dof_near", r_RT_dof_near); // Blur
			C.set_Sampler_point("s_dof_coc", r_RT_dof_coc);
			C.end_Pass();
			break;
		}
	};

	CBlender_depth_of_field() = default;
	virtual ~CBlender_depth_of_field() = default;
};
///////////////////////////////////////////////////////////////////////////////////
