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
	SE_SSR_GENERATE_MIP_CHAIN_PASS,
	SE_SSR_RENDER_PASS,
	SE_SSR_DENOISE_PASS
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_reflections : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: Reflections";
	}

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		CBlender_Compile::PassDesc PassDescription;

		// Имя единого шейдерного файла (без расширения)
		LPCSTR sh_name = "postprocess_stage_reflections";

		switch (C.iElement)
		{
		case SE_SSR_GENERATE_MIP_CHAIN_PASS:
			// Pass 1: Simple Image (Initial copy if needed, preserved from original logic)
			C.begin_Pass("screen_quad", "simple_image");
			C.set_Sampler("s_image", r_RT_generic0);
			gbuffer(C);
			C.end_Pass();

			// Pass 2: Generate Mip / Blur
			// Используем наш единый файл и точку входа GenerateMipChainPass
			C.begin_Pass("screen_quad", sh_name, "main", "GenerateMipChainPass");
			C.set_Sampler("s_mip_chain", r_RT_backbuffer_mip, false, D3DTADDRESS_CLAMP, D3DTEXF_GAUSSIANQUAD,
						  D3DTEXF_GAUSSIANQUAD, D3DTEXF_GAUSSIANQUAD, false);
			gbuffer(C);
			C.end_Pass();
			break;

		case SE_SSR_RENDER_PASS:
			// Pass 3: Render SSR
			// Используем наш единый файл и точку входа ReflectionsRenderPass
			PassDescription.VertexShader = "screen_quad";
			PassDescription.VertexShaderEntry = "main";
			PassDescription.PixelShader = sh_name;
			PassDescription.PixelShaderEntry = "ReflectionsRenderPass";

			// В оригинале использовался begin_Pass("screen_quad", ...).
			// Используем простой вариант, так как в оригинале не было сложного блендинга,
			// но если нужно, можно развернуть PassDescription как в примере с автоэкспозицией.
			C.begin_Pass(PassDescription);

			C.set_Sampler("s_hi_z_mip_chain", r_RT_Hi_z, false, D3DTADDRESS_CLAMP, D3DTEXF_POINT, D3DTEXF_POINT,
						  D3DTEXF_POINT, false);
			C.set_Sampler("s_image", r_RT_backbuffer_mip, false, D3DTADDRESS_CLAMP, D3DTEXF_GAUSSIANQUAD,
						  D3DTEXF_GAUSSIANQUAD, D3DTEXF_GAUSSIANQUAD, false);
			gbuffer(C);
			C.end_Pass();
			break;

		case SE_SSR_DENOISE_PASS:
			PassDescription.VertexShader = "screen_quad";
			PassDescription.VertexShaderEntry = "main";
			PassDescription.PixelShader = sh_name;
			PassDescription.PixelShaderEntry = "DenoisePass";

			C.begin_Pass(PassDescription);

			C.set_Sampler_linear("s_reflections", r_RT_generic1);

			gbuffer(C);
			C.end_Pass();
			break;
		}
	}

	CBlender_reflections() = default;
	~CBlender_reflections() = default;
};
///////////////////////////////////////////////////////////////////////////////////
