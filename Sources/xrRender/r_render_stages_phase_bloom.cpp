///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "blender_bloom.h"
///////////////////////////////////////////////////////////////////////////////////
bool TexturesIsClean = false;
///////////////////////////////////////////////////////////////////////////////////
void CRender::clear_bloom()
{
	TexturesIsClean = true;
	RenderBackendLegacy.ClearTexture(	RenderTarget->rt_Bloom[0], 
								RenderTarget->rt_Bloom[1], 
								RenderTarget->rt_Bloom_Blades[0],
								RenderTarget->rt_Bloom_Blades[1] );
}

void CRender::calculate_bloom()
{
	RenderBackendLegacy.set_CullMode(CULL_DISABLE);
	RenderBackendLegacy.set_Stencil(FALSE);

	float BloomResolutionMultiplier = 0.5f;

	float w = float(Device.dwWidth) * BloomResolutionMultiplier;
	float h = float(Device.dwHeight) * BloomResolutionMultiplier;

	// Downsample, prepare image and store in rt_Bloom[0] and rt_Bloom_Blades
	{
		RenderBackendLegacy.set_Element(RenderTarget->s_bloom->E[SE_PASS_PREPARE]);
		RenderBackendLegacy.set_Constant("bloom_parameters",  ps_r_bloom_threshold, 
														ps_r_bloom_brightness, 
														ps_r_bloom_blades_threshold, 
														ps_r_bloom_blades_brightness);
		RenderBackendLegacy.set_Constant("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RenderBackendLegacy.RenderViewportSurface(w, h, RenderTarget->rt_Bloom[0], RenderTarget->rt_Bloom_Blades[0]);
	}

	//Main bloom effect
	for (int i = 0; i < 4; i++)
	{
		RenderBackendLegacy.set_Element(RenderTarget->s_bloom->E[SE_PASS_PROCESS_BLOOM], 0);
		RenderBackendLegacy.set_Constant("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RenderBackendLegacy.set_Constant("bloom_blur_params", float(i) / 4, 0, 0, 0);
		RenderBackendLegacy.RenderViewportSurface(w, h, RenderTarget->rt_Bloom[1]);

		RenderBackendLegacy.set_Element(RenderTarget->s_bloom->E[SE_PASS_PROCESS_BLOOM], 1);
		RenderBackendLegacy.set_Constant("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RenderBackendLegacy.set_Constant("bloom_blur_params", float(i) / 4, 0, 0, 0);
		RenderBackendLegacy.RenderViewportSurface(w, h, RenderTarget->rt_Bloom[0]);
	}

	// Blades effect
	if (ps_r_bloom_quality > 1)
	{
		RenderBackendLegacy.set_Element(RenderTarget->s_bloom->E[SE_PASS_PROCESS_BLADES], 0);
		RenderBackendLegacy.set_Constant("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RenderBackendLegacy.set_Constant("bloom_blur_params", 0.5f, 0, 0, 0);
		RenderBackendLegacy.RenderViewportSurface(w, h, RenderTarget->rt_Bloom_Blades[1]);

		RenderBackendLegacy.set_Element(RenderTarget->s_bloom->E[SE_PASS_PROCESS_BLADES], 1);
		RenderBackendLegacy.set_Constant("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RenderBackendLegacy.set_Constant("bloom_blur_params", 0.5f, 0, 0, 0);
		RenderBackendLegacy.RenderViewportSurface(w, h, RenderTarget->rt_Bloom_Blades[0]);
	}
}

void CRender::apply_bloom()
{
	RenderBackendLegacy.set_Element(RenderTarget->s_bloom->E[SE_PASS_APPLY_BLOOM]);
	RenderBackendLegacy.set_Constant("bloom_parameters", ps_r_bloom_threshold, 
												   ps_r_bloom_brightness,
												   ps_r_bloom_blades_threshold, 
												   ps_r_bloom_blades_brightness);
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Generic[1]);
}

void CRender::render_bloom()
{
	////OPTICK_EVENT("CRender::render_bloom");

	if (ps_r_postprocess_flags.test(RFLAG_BLOOM))
	{
		TexturesIsClean = false;
		calculate_bloom();
		apply_bloom();
	}
	else
	{
		if (!TexturesIsClean)
			clear_bloom();
	}
}
///////////////////////////////////////////////////////////////////////////////////
