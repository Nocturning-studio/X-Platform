///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
#include "blender_effectors.h"
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::u_calc_tc_duality_ss(Fvector2& r0, Fvector2& r1, Fvector2& l0, Fvector2& l1)
{
	// Calculate ordinaty TCs from blur and SS
	float tw = float(dwWidth);
	float th = float(dwHeight);
	if (dwHeight != Device.dwHeight)
		RenderImplementation.EffectorsManager->set_blur(1.f);
	Fvector2 shift, p0, p1;
	shift.set(.5f / tw, .5f / th);
	shift.mul(RenderImplementation.EffectorsManager->get_blur());
	p0.set(.5f / tw, .5f / th).add(shift);
	p1.set((tw + .5f) / tw, (th + .5f) / th).add(shift);

	// Calculate Duality TC
	float shift_u = RenderImplementation.EffectorsManager->get_duality_h() * .5f;
	float shift_v = RenderImplementation.EffectorsManager->get_duality_v() * .5f;

	r0.set(p0.x, p0.y);
	r1.set(p1.x - shift_u, p1.y - shift_v);
	l0.set(p0.x + shift_u, p0.y + shift_v);
	l1.set(p1.x, p1.y);
}
///////////////////////////////////////////////////////////////////////////////////
struct TL_2c3uv
{
	Fvector4 p;
	u32 color0;
	u32 color1;
	Fvector2 uv[3];
	IC void set(float x, float y, u32 c0, u32 c1, float u0, float v0, float u1, float v1, float u2, float v2)
	{
		p.set(x, y, EPS_S, 1.f);
		color0 = c0;
		color1 = c1;
		uv[0].set(u0, v0);
		uv[1].set(u1, v1);
		uv[2].set(u2, v2);
	}
};
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_effectors_pass_generate_radiation_noise()
{
	////OPTICK_EVENT("CRenderTarget::render_effectors_pass_generate_radiation_noise");

	RenderBackend.set_CullMode(CULL_DISABLE);
	RenderBackend.set_Stencil(FALSE);

	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	RenderBackend.set_Element(RenderTarget->s_effectors->E[SE_PASS_RADIATION]);
	RenderBackend.set_Constant("noise_intesity", RenderImplementation.EffectorsManager->get_radiation_intensity(), 1);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Radiation_Noise[0]);

	RenderBackend.set_Element(RenderTarget->s_effectors->E[SE_PASS_RADIATION]);
	RenderBackend.set_Constant("noise_intesity", RenderImplementation.EffectorsManager->get_radiation_intensity(), 0.66f);
	RenderBackend.RenderViewportSurface(w * 0.5f, h * 0.5f, RenderTarget->rt_Radiation_Noise[1]);

	RenderBackend.set_Element(RenderTarget->s_effectors->E[SE_PASS_RADIATION]);
	RenderBackend.set_Constant("noise_intesity", RenderImplementation.EffectorsManager->get_radiation_intensity(), 0.33f);
	RenderBackend.RenderViewportSurface(w * 0.25f, h * 0.25f, RenderTarget->rt_Radiation_Noise[2]);
}

void CRender::render_effectors_pass_color_blind_filter()
{
	////OPTICK_EVENT("CRenderTarget::render_effectors_pass_color_blind_filter");

	RenderBackend.set_CullMode(CULL_DISABLE);
	RenderBackend.set_Stencil(FALSE);

	Fvector3 RedMatrix;
	Fvector3 GreenMatrix;
	Fvector3 BlueMatrix;

	switch (ps_r_color_blind_mode)
	{
	case 1: // achromatomaly
		RedMatrix.set(0.618, 0.32, 0.062);
		GreenMatrix.set(0.163, 0.775, 0.062);
		BlueMatrix.set(0.163, 0.320, 0.516);
		break;
	case 2: // achromatopsia
		RedMatrix.set(0.299, 0.587, 0.114);
		GreenMatrix.set(0.299, 0.587, 0.114);
		BlueMatrix.set(0.299, 0.587, 0.114);
		break;
	case 3: // deuteranomaly
		RedMatrix.set(0.8, 0.2, 0.0);
		GreenMatrix.set(0.25833, 0.74167, 0.0);
		BlueMatrix.set(0.0, 0.14167, 0.85833);
		break;
	case 4: // deuteranopia
		RedMatrix.set(0.625, 0.375, 0.0);
		GreenMatrix.set(0.7, 0.3, 0.0);
		BlueMatrix.set(0.0, 0.3, 0.7);
		break;
	case 5: // protanomaly
		RedMatrix.set(0.81667, 0.18333, 0.0);
		GreenMatrix.set(0.33333, 0.66667, 0.0);
		BlueMatrix.set(0.0, 0.125, 0.875);
		break;
	case 6: // protanopia
		RedMatrix.set(0.56667, 0.43333, 0.0);
		GreenMatrix.set(0.55833, 0.44167, 0.0);
		BlueMatrix.set(0.0, 0.24167, 0.75833);
		break;
	case 7: // tritanomaly
		RedMatrix.set(0.96667, 0.03333, 0.0);
		GreenMatrix.set(0.0, 0.73333, 0.26667);
		BlueMatrix.set(0.0, 0.18333, 0.81667);
		break;
	case 8: // tritanopia
		RedMatrix.set(0.95, 0.05, 0);
		GreenMatrix.set(0.0, 0.43333, 0.56667);
		BlueMatrix.set(0.0, 0.475, 0.525);
		break;
	default:
		RedMatrix.set(1.0f, 0.0f, 0.0f);
		GreenMatrix.set(0.0f, 1.0f, 0.0f);
		BlueMatrix.set(0.0f, 0.0f, 1.0f);
		break;
	}

	RenderBackend.set_Element(RenderTarget->s_effectors->E[SE_PASS_COLOR_BLIND_FILTER], 0);

	RenderBackend.set_Constant("red_matrix", RedMatrix.x, RedMatrix.y, RedMatrix.z);
	RenderBackend.set_Constant("green_matrix", GreenMatrix.x, GreenMatrix.y, GreenMatrix.z);
	RenderBackend.set_Constant("blue_matrix", BlueMatrix.x, BlueMatrix.y, BlueMatrix.z);

	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic[0]);

	RenderBackend.set_Element(RenderTarget->s_effectors->E[SE_PASS_COLOR_BLIND_FILTER], 1);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic[1]);
}

void CRender::render_effectors_pass_combine()
{
	////OPTICK_EVENT("CRenderTarget::render_effectors_pass_combine");

	// combination/postprocess
	RenderBackend.set_Render_Target_Surface(RenderTarget->rt_Generic[1]);
	RenderBackend.set_Depth_Buffer(NULL);

	RenderBackend.set_Element(RenderTarget->s_effectors->E[SE_PASS_COMBINE]);

	int gblend = clampr(iFloor((1 - RenderImplementation.EffectorsManager->get_gray()) * 255.f), 0, 255);
	int nblend = clampr(iFloor((1 - RenderImplementation.EffectorsManager->get_noise()) * 255.f), 0, 255);
	u32 p_color = subst_alpha(RenderImplementation.EffectorsManager->get_color_base(), nblend);
	u32 p_gray = subst_alpha(RenderImplementation.EffectorsManager->get_color_gray(), gblend);
	u32 p_brightness = RenderImplementation.EffectorsManager->get_color_add();

	// Draw full-screen quad textured with our scene image
	u32 Offset;
	float _w = float(Device.dwWidth);
	float _h = float(Device.dwHeight);

	Fvector2 r0, r1, l0, l1;
	RenderTarget->u_calc_tc_duality_ss(r0, r1, l0, l1);

	float NightVisionEnabled = 0.0f;

	if (g_pGamePersistent && g_pGamePersistent->GetNightVisionState())
		NightVisionEnabled = 1.0f;

	// Fill vertex buffer
	float du = ps_pps_u, dv = ps_pps_v;
	TL_2c3uv* pv = (TL_2c3uv*)RenderBackend.Vertex.Lock(4, RenderTarget->g_effectors.stride(), Offset);
	pv->set(du + 0, dv + float(_h), p_color, p_gray, r0.x, r1.y, l0.x, l1.y, 0, 0);
	pv++;
	pv->set(du + 0, dv + 0, p_color, p_gray, r0.x, r0.y, l0.x, l0.y, 0, 0);
	pv++;
	pv->set(du + float(_w), dv + float(_h), p_color, p_gray, r1.x, r1.y, l1.x, l1.y, 0, 0);
	pv++;
	pv->set(du + float(_w), dv + 0, p_color, p_gray, r1.x, r0.y, l1.x, l0.y, 0, 0);
	pv++;
	RenderBackend.Vertex.Unlock(4, RenderTarget->g_effectors.stride());

	// Actual rendering
	RenderBackend.set_Constant(	"c_colormap", 
								RenderImplementation.EffectorsManager->get_cm_imfluence(),
							    RenderImplementation.EffectorsManager->get_cm_interpolate());

	RenderBackend.set_Constant(	"c_brightness", 
								color_get_R(p_brightness) / 255.f, 
								color_get_G(p_brightness) / 255.f, 
								color_get_B(p_brightness) / 255.f, 
								RenderImplementation.EffectorsManager->get_noise());

	RenderBackend.set_Constant("night_vision_enabled", NightVisionEnabled);

	RenderBackend.set_Constant("actor_health", get_actor_health());

	RenderBackend.set_Geometry(RenderTarget->g_effectors);

	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRender::render_effectors_pass_resolve_gamma()
{
	////OPTICK_EVENT("CRenderTarget::render_effectors_pass_resolve_gamma");

	RenderBackend.set_CullMode(CULL_DISABLE);
	RenderBackend.set_Stencil(FALSE);

	RenderBackend.set_Element(RenderTarget->s_effectors->E[SE_PASS_RESOLVE_GAMMA]);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic[0]);
}

void CRender::render_effectors_pass_lut()
{
	////OPTICK_EVENT("CRenderTarget::render_effectors_pass_lut");

	RenderBackend.set_CullMode(CULL_DISABLE);
	RenderBackend.set_Stencil(FALSE);

	CEnvDescriptorMixer* envdesc = g_pGamePersistent->Environment().CurrentEnv;
	IDirect3DBaseTexture9* e0 = envdesc->lut_r_textures[0].second->surface_get();
	RenderTarget->t_LUT_0->surface_set(e0);
	_RELEASE(e0);

	IDirect3DBaseTexture9* e1 = envdesc->lut_r_textures[1].second->surface_get();
	RenderTarget->t_LUT_1->surface_set(e1);
	_RELEASE(e1);

	RenderBackend.set_Element(RenderTarget->s_effectors->E[SE_PASS_LUT], 0);
	RenderBackend.set_Constant("c_lut_params", envdesc->weight, 0, 0, 0);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic[1]);
}
///////////////////////////////////////////////////////////////////////////////////
