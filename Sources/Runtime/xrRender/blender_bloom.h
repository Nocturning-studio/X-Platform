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
	SE_PASS_PREPARE,
	SE_PASS_PROCESS_BLOOM,
	SE_PASS_PROCESS_BLADES,
	SE_PASS_APPLY_BLOOM
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_bloom : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: combine to bloom target";
	}
	virtual BOOL canBeDetailed()
	{
		return FALSE;
	}
	virtual BOOL canBeLMAPped()
	{
		return FALSE;
	}

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		// Единый файл шейдера
		LPCSTR sh_name = "postprocess_stage_bloom";

		switch (C.iElement)
		{
		case SE_PASS_PREPARE:
			// Pass 1: Prepare

			C.begin_Pass("screen_quad", sh_name, "main", "PrepareColorBuffer");
			C.set_Sampler_gaussian("s_image", r_RT_generic1);
			gbuffer(C);
			C.end_Pass();
			break;

		case SE_PASS_PROCESS_BLOOM:
			// Pass 2: Bloom Filter

			// Horizontal pass
			C.set_Define("USE_HORIZONTAL_FILTER", "1");
			C.begin_Pass("screen_quad", sh_name, "main", "FilterBloom");
			C.set_Sampler_gaussian("s_bloom", r_RT_bloom1);
			C.end_Pass();

			// Vertical pass (Macros clear automatically in end_Pass or manually if reusing C object logic)
			// Примечание: Макросы очищаются в end_Pass, так что для второго прохода USE_HORIZONTAL_FILTER не будет
			// определен, что активирует ветку #else (Vertical) в шейдере.
			C.begin_Pass("screen_quad", sh_name, "main", "FilterBloom");
			C.set_Sampler_gaussian("s_bloom", r_RT_bloom2);
			C.end_Pass();
			break;

		case SE_PASS_PROCESS_BLADES:
			// Pass 3: Blades Filter

			// Step 1
			C.begin_Pass("screen_quad", sh_name, "main", "FilterBlades");
			C.set_Sampler_gaussian("s_bloom_blades", r_RT_bloom_blades1);
			C.end_Pass();

			// Step 2
			C.begin_Pass("screen_quad", sh_name, "main", "FilterBlades");
			C.set_Sampler_gaussian("s_bloom_blades", r_RT_bloom_blades2);
			C.end_Pass();
			break;
		case SE_PASS_APPLY_BLOOM:
			// Pass 4: Apply

			// Настраиваем аддитивный блендинг (Сложение цветов)
			CBlender_Compile::PassDesc PassDescription;
			PassDescription.VertexShader = "screen_quad";
			PassDescription.VertexShaderEntry = "main";
			PassDescription.PixelShader = sh_name;
			PassDescription.PixelShaderEntry = "ApplyBloom";

			PassDescription.EnableAlphaBlend = true;
			PassDescription.BlendSRC = D3DBLEND_ONE; // Src * 1
			PassDescription.BlendDST = D3DBLEND_ONE; // Dst * 1

			C.begin_Pass(PassDescription);

			// Используем Linear семплеры, чтобы растянутый блум был мягким, а не пиксельным
			C.set_Sampler_linear("s_bloom", r_RT_bloom1);
			C.set_Sampler_linear("s_bloom_blades", r_RT_bloom_blades1);

			C.end_Pass();
			break;
		}
	}

	CBlender_bloom() = default;
	virtual ~CBlender_bloom() = default;
};
///////////////////////////////////////////////////////////////////////////////////