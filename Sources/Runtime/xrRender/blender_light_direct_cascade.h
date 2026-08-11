///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_accum_direct_cascade : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: accumulate direct light";
	}

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		CBlender_Compile::PassDesc PassDescription;
		PassDescription.VertexShader = "accumulating_light_stage_volume";
		PassDescription.PixelShader = "accumulating_light_stage_direct";
		PassDescription.EnableZTest = true;

		switch (C.iElement)
		{
		case SE_SUN_NEAR: // near pass - enable Z-test to perform depth-clipping
		case SE_SUN_MIDDLE:
			C.set_Define("SMAP_SIZE", (int)RenderImplementation.o.smapsize, CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("SHADOW_FILTER_QUALITY", (int)ps_r_shadow_filtering, CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("MIDDLE_CASCADE", CBlender_Compile::ShaderScope::Pixel);
			C.begin_Pass(PassDescription);
			C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer
			C.set_Sampler_point("s_smap", r_RT_smap_depth);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		case SE_SUN_FAR: // far pass, only stencil clipping performed
			C.set_Define("SMAP_SIZE", (int)RenderImplementation.o.smapsize, CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("SHADOW_FILTER_QUALITY", (int)ps_r_shadow_filtering, CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("USE_SMOOTH_FADING", CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("FAR_CASCADE", CBlender_Compile::ShaderScope::Pixel);
			C.begin_Pass(PassDescription);
			C.set_Sampler_point("s_smap", r_RT_smap_depth);
			gbuffer(C);
			jitter(C);
			{
				u32 s = C.i_Sampler("s_smap");
				C.i_Address(s, D3DTADDRESS_BORDER);
				C.i_BorderColor(s, D3DCOLOR_ARGB(255, 255, 255, 255));
			}
			C.end_Pass();
			break;
		case SE_SUN_VOL_NEAR: // near pass - enable Z-test to perform depth-clipping
		case SE_SUN_VOL_MIDDLE:
			C.set_Define("SUN_SHAFTS_QUALITY", (int)ps_r_sun_shafts_quality, CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("SMAP_SIZE", (int)RenderImplementation.o.smapsize, CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("SHADOW_FILTER_QUALITY", (int)ps_r_shadow_filtering, CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("MIDDLE_CASCADE", CBlender_Compile::ShaderScope::Pixel);
			PassDescription.PixelShader += "_volumetric";
			C.begin_Pass(PassDescription);
			C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer
			C.set_Sampler_point("s_smap", r_RT_smap_depth);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		case SE_SUN_VOL_FAR: // far pass, only stencil clipping performed
			PassDescription.PixelShader += "_volumetric";
			C.set_Define("SUN_SHAFTS_QUALITY", (int)ps_r_sun_shafts_quality, CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("SMAP_SIZE", (int)RenderImplementation.o.smapsize, CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("SHADOW_FILTER_QUALITY", (int)ps_r_shadow_filtering, CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("USE_SMOOTH_FADING", CBlender_Compile::ShaderScope::Pixel);
			C.set_Define("FAR_CASCADE", CBlender_Compile::ShaderScope::Pixel);
			C.begin_Pass(PassDescription);
			C.set_Sampler_point("s_smap", r_RT_smap_depth);
			gbuffer(C);
			jitter(C);
			{
				u32 s = C.i_Sampler("s_smap");
				C.i_Address(s, D3DTADDRESS_BORDER);
				C.i_BorderColor(s, D3DCOLOR_ARGB(255, 255, 255, 255));
			}
			C.end_Pass();
			break;
		}
	}

	CBlender_accum_direct_cascade() = default;
	virtual ~CBlender_accum_direct_cascade() = default;
};
///////////////////////////////////////////////////////////////////////////////////