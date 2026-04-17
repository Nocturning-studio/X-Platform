///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
enum
{
	SE_PASS_AUTOEXPOSURE_GENERATE_MIP_CHAIN,
	SE_PASS_AUTOEXPOSURE_PREPARE_LUMINANCE,
	SE_PASS_AUTOEXPOSURE_APPLY_EXPOSURE
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_autoexposure : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: autoexposure estimate";
	}

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		// Имя единого шейдерного файла (без расширения)
		LPCSTR sh_name = "postprocess_stage_autoexposure";

		switch (C.iElement)
		{
		case SE_PASS_AUTOEXPOSURE_GENERATE_MIP_CHAIN:
			// Pass 1: Initial
			C.begin_Pass("screen_quad", sh_name, "main", "InitialPass");
			C.set_Sampler("s_image", r_RT_generic1);
			C.end_Pass();

			// Pass 2: Generate Mip Chain
			C.begin_Pass("screen_quad", sh_name, "main", "GenerateMipChainPass");
			C.set_Sampler("s_mip_chain", r_RT_autoexposure_mip_chain, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR, false);
			C.end_Pass();
			break;

		case SE_PASS_AUTOEXPOSURE_PREPARE_LUMINANCE:
			// Pass 3: Prepare/Adapt
			C.begin_Pass("screen_quad", sh_name, "main", "PrepareLuminancePass");
			C.set_Sampler("s_luminance_previous", r_RT_autoexposure_luminance_previous);
			C.set_Sampler("s_luminance", r_RT_autoexposure_mip_chain, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR, false);
			C.end_Pass();
			break;

		case SE_PASS_AUTOEXPOSURE_APPLY_EXPOSURE:
			// Pass 4: Apply
			CBlender_Compile::PassDesc PassDescription;
			PassDescription.VertexShader = "screen_quad";
			PassDescription.VertexShaderEntry = "main";
			PassDescription.PixelShader = sh_name;
			PassDescription.PixelShaderEntry = "ApplyPass";
			PassDescription.EnableAlphaBlend = true;
			PassDescription.BlendSRC = D3DBLEND_ZERO;
			PassDescription.BlendDST = D3DBLEND_SRCCOLOR;

			C.begin_Pass(PassDescription);
			C.set_Sampler("s_luminance_previous", r_RT_autoexposure_luminance_previous);
			C.set_Sampler("s_luminance", r_RT_autoexposure_luminance);
			C.end_Pass();

			PassDescription.VertexShader = "screen_quad";
			PassDescription.VertexShaderEntry = "main";
			PassDescription.PixelShader = sh_name;
			PassDescription.PixelShaderEntry = "ApplyPassDummy";
			PassDescription.EnableAlphaBlend = true;
			PassDescription.BlendSRC = D3DBLEND_ZERO;
			PassDescription.BlendDST = D3DBLEND_SRCCOLOR;

			C.begin_Pass(PassDescription);
			C.set_Sampler("s_luminance_previous", r_RT_autoexposure_luminance_previous);
			C.set_Sampler("s_luminance", r_RT_autoexposure_luminance);
			C.end_Pass();
			break;
		}
	}

	CBlender_autoexposure() = default;
	~CBlender_autoexposure() = default;
};
///////////////////////////////////////////////////////////////////////////////////
