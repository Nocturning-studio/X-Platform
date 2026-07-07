///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
///////////////////////////////////////////////////////////////////////////////////
#include "r_rendertarget.h"
#include "render.h"
#include "..\xrEngine\resourcemanager.h"
#include "blender_ambient_occlusion.h"
#include "blender_bloom.h"
#include "blender_antialiasing.h"
#include "blender_combine.h"
#include "blender_distortion.h"
#include "blender_depth_of_field.h"
#include "blender_reflections.h"
#include "blender_motion_blur.h"
#include "blender_frame_overlay.h"
#include "blender_light_direct_cascade.h"
#include "blender_light_mask.h"
#include "blender_light_occq.h"
#include "blender_light_point.h"
#include "blender_light_spot.h"
#include "blender_autoexposure.h"
#include "blender_effectors.h"
#include "blender_output_to_screen.h"
#include "blender_bent_normals.h"
#include "blender_hi_z.h"
///////////////////////////////////////////////////////////////////////////////////
using namespace xrRHI;
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::create_textures()
{
	Msg("Creating render target textures");

	// SCREENSHOT
	R_CHK(RenderBackend.GetDevice()->CreateOffscreenPlainSurface(dwWidth, dwHeight, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &surf_screenshot_normal, NULL));
	R_CHK(RenderBackend.GetDevice()->CreateTexture(128, 128, 1, NULL, D3DFMT_DXT5, D3DPOOL_SYSTEMMEM, &tex_screenshot_gamesave, NULL));
	R_CHK(tex_screenshot_gamesave->GetSurfaceLevel(0, &surf_screenshot_gamesave));

	// G-Buffer
	if (ps_r_shading_mode == SHADING_MODE_LEASHED)
	{
		rt_GBuffer[0].create(r_RT_GBuffer_1, dwWidth, dwHeight, RHI_Format::RGBA8_UNORM);
		rt_GBuffer[1].create(r_RT_GBuffer_2, dwWidth, dwHeight, RHI_Format::RGBA16_FLOAT);
	}
	else
	{
		rt_GBuffer[0].create(r_RT_GBuffer_1, dwWidth, dwHeight, RHI_Format::RGBA8_UNORM);
		rt_GBuffer[1].create(r_RT_GBuffer_2, dwWidth, dwHeight, RHI_Format::RGBA8_UNORM);
		rt_GBuffer[2].create(r_RT_GBuffer_3, dwWidth, dwHeight, RHI_Format::RGBA16_FLOAT);
	}

	rt_Hi_z.create(r_RT_Hi_z, dwWidth, dwHeight, RHI_Format::R16_FLOAT, 9);

	//rt_Bent_Normals.create(r_RT_Bent_Normals, dwWidth, dwHeight, RHI_Format::RGBA16_FLOAT);
	
	// DOF Resources
	// G16R16F: R = Real CoC, G = Dilated (Max) CoC
	rt_dof_coc.create(r_RT_dof_coc, dwWidth, dwHeight, RHI_Format::RG16_FLOAT);
	// R16F, разрешение / 8 (тайлы 8x8)
	// Содержит Max Near CoC для тайла.
	u32 tileW = dwWidth / 8;
	u32 tileH = dwHeight / 8;
	rt_dof_dilation.create(r_RT_dof_dilation, tileW, tileH, RHI_Format::R16_FLOAT);
	// Буферы для слоев
	rt_dof_near.create(r_RT_dof_near, dwWidth, dwHeight, RHI_Format::RGBA16_FLOAT);
	rt_dof_far.create(r_RT_dof_far, dwWidth, dwHeight, RHI_Format::RGBA16_FLOAT);

	rt_Volumetric_Sun.create(r_RT_Volumetric_Sun, dwWidth, dwHeight, RHI_Format::A8_UNORM);

	rt_Light_Accumulator.create(r_RT_Light_Accumulator, dwWidth, dwHeight, RHI_Format::RGBA16_FLOAT);

	rt_Distortion_Mask.create(r_RT_distortion_mask, dwWidth, dwHeight, RHI_Format::RG16_FLOAT);

	rt_Generic[0].create(r_RT_generic0, dwWidth, dwHeight, RHI_Format::RGBA16_FLOAT);
	rt_Generic[1].create(r_RT_generic1, dwWidth, dwHeight, RHI_Format::RGBA16_FLOAT);

	RenderBackend.ClearTexture(rt_Generic[0], color_rgba_f(0.5f, 0.5f, 0.5f, 1.0f));
	RenderBackend.ClearTexture(rt_Generic[1], color_rgba_f(0.5f, 0.5f, 0.5f, 1.0f));

	rt_Motion_Blur_Previous_Frame_Depth.create(r_RT_mblur_previous_frame_depth, dwWidth, dwHeight, RHI_Format::R16_FLOAT);
	rt_Motion_Blur_Dilation_Map_0.create(r_RT_mblur_dilation_map_0, u32(dwWidth * 0.5f), u32(dwHeight * 0.5f), RHI_Format::RG16_FLOAT);
	rt_Motion_Blur_Dilation_Map_1.create(r_RT_mblur_dilation_map_1, u32(dwWidth * 0.5f), u32(dwHeight * 0.5f), RHI_Format::RG16_FLOAT);

	rt_BackbufferMip.create(r_RT_backbuffer_mip, dwWidth, dwHeight, RHI_Format::RGBA16_FLOAT, 3);
	rt_Reflections.create(r_RT_reflections, dwWidth, dwHeight, RHI_Format::RGBA16_FLOAT);

	rt_Radiation_Noise[0].create(r_RT_radiation_noise0, dwWidth, dwHeight, RHI_Format::R8_UNORM);
	rt_Radiation_Noise[1].create(r_RT_radiation_noise1, u32(dwWidth * 0.5f), u32(dwHeight * 0.5f), RHI_Format::R8_UNORM);
	rt_Radiation_Noise[2].create(r_RT_radiation_noise2, u32(dwWidth * 0.25f), u32(dwHeight * 0.25f), RHI_Format::R8_UNORM);

	rt_ao.create(r_RT_ao, dwWidth, dwHeight, RHI_Format::R8_UNORM);

	// autoexposure
	rt_LUM_Mip_Chain.create(r_RT_autoexposure_mip_chain, dwWidth, dwWidth, RHI_Format::RGBA16_FLOAT, 9);
	rt_SceneLuminance.create(r_RT_autoexposure_luminance, 1, 1, RHI_Format::RGBA16_FLOAT);
	rt_SceneLuminancePrevious.create(r_RT_autoexposure_luminance_previous, 1, 1, RHI_Format::RGBA16_FLOAT);

	RenderBackend.ClearTexture(rt_LUM_Mip_Chain, color_rgba_f(0.18f, 0.18f, 0.18f, 1.0f));
	RenderBackend.ClearTexture(rt_SceneLuminance, color_rgba_f(0.18f, 0.18f, 0.18f, 1.0f));
	RenderBackend.ClearTexture(rt_SceneLuminancePrevious, color_rgba_f(0.18f, 0.18f, 0.18f, 1.0f));

	t_irradiance_map_0.create(r_T_irradiance0);
	t_irradiance_map_1.create(r_T_irradiance1);

	t_LUT_0.create(r_T_LUTs0);
	t_LUT_1.create(r_T_LUTs1);

	// BLOOM
	float BloomResolutionMultiplier = 0.5f;
	u32 w = u32(dwWidth * BloomResolutionMultiplier), h = u32(dwHeight * BloomResolutionMultiplier);
	rt_Bloom[0].create(r_RT_bloom1, w, h, RHI_Format::RGBA16_FLOAT);
	rt_Bloom[1].create(r_RT_bloom2, w, h, RHI_Format::RGBA16_FLOAT);
	rt_Bloom_Blades[0].create(r_RT_bloom_blades1, w, h, RHI_Format::RGBA16_FLOAT);
	rt_Bloom_Blades[1].create(r_RT_bloom_blades2, w, h, RHI_Format::RGBA16_FLOAT);
}

void CRenderTarget::create_blenders()
{
	Msg("Creating blenders (allocation only)");

	b_occq = xr_new<CBlender_light_occq>();
	b_accum_mask = xr_new<CBlender_accum_direct_mask>();
	b_accum_direct_cascade = xr_new<CBlender_accum_direct_cascade>();
	b_accum_point = xr_new<CBlender_accum_point>();
	b_accum_spot = xr_new<CBlender_accum_spot>();
	b_effectors = xr_new<CBlender_effectors>();
	b_output_to_screen = xr_new<CBlender_output_to_screen>();
	b_ambient_occlusion = xr_new<CBlender_ambient_occlusion>();
	b_bloom = xr_new<CBlender_bloom>();
	b_autoexposure = xr_new<CBlender_autoexposure>();
	b_combine = xr_new<CBlender_combine>();
	b_antialiasing = xr_new<CBlender_antialiasing>();
	b_distortion = xr_new<CBlender_distortion>();
	b_reflections = xr_new<CBlender_reflections>();
	b_dof = xr_new<CBlender_depth_of_field>();
	b_motion_blur = xr_new<CBlender_motion_blur>();
	b_frame_overlay = xr_new<CBlender_frame_overlay>();
	b_bent_normals = xr_new<CBlender_bent_normals>();
	b_hi_z = xr_new<CBlender_hi_z>();
}

void CRenderTarget::CompileShaders()
{
	Msg("Compiling render target shaders...");

	s_occq.create(b_occq);
	s_accum_mask.create(b_accum_mask);
	s_accum_direct_cascade.create(b_accum_direct_cascade);
	s_accum_point.create(b_accum_point, "r\\accum_point", "lights\\lights_point01");
	s_accum_spot.create(b_accum_spot, "r\\accum_spot", "lights\\lights_spot01");
	s_effectors.create(b_effectors);
	s_output_to_screen.create(b_output_to_screen);

	s_ambient_occlusion.create(b_ambient_occlusion);
	s_bloom.create(b_bloom);
	s_autoexposure.create(b_autoexposure);
	s_combine.create(b_combine);
	s_antialiasing.create(b_antialiasing);
	s_distortion.create(b_distortion);
	s_reflections.create(b_reflections);
	s_dof.create(b_dof);
	s_motion_blur.create(b_motion_blur);
	s_frame_overlay.create(b_frame_overlay);
	s_bent_normals.create(b_bent_normals);
	s_hi_z.create(b_hi_z);
}

void CRenderTarget::delete_textures()
{
	if (g_dedicated_server)
		return;

	Msg("Destroying render target textures");

	// Освобождение обычных COM-поверхностей и текстур
	_RELEASE(surf_screenshot_normal);
	_RELEASE(surf_screenshot_gamesave);
	_RELEASE(tex_screenshot_gamesave);

	// G-Buffer
	for (int i = 0; i < 4; ++i)
		rt_GBuffer[i].destroy();

	rt_Hi_z.destroy();
	//rt_Bent_Normals.destroy();

	// DOF
	rt_dof_coc.destroy();
	rt_dof_dilation.destroy();
	rt_dof_near.destroy();
	rt_dof_far.destroy();

	rt_Volumetric_Sun.destroy();
	rt_Light_Accumulator.destroy();
	rt_Distortion_Mask.destroy();

	for (int i = 0; i < 2; ++i)
		rt_Generic[i].destroy();

	rt_Motion_Blur_Previous_Frame_Depth.destroy();
	rt_Motion_Blur_Dilation_Map_0.destroy();
	rt_Motion_Blur_Dilation_Map_1.destroy();

	rt_BackbufferMip.destroy();
	rt_Reflections.destroy();

	for (int i = 0; i < 3; ++i)
		rt_Radiation_Noise[i].destroy();

	rt_ao.destroy();

	t_irradiance_map_0.destroy();
	t_irradiance_map_1.destroy();

	t_LUT_0.destroy();
	t_LUT_1.destroy();

	for (int i = 0; i < 2; ++i)
		rt_Bloom[i].destroy();

	for (int i = 0; i < 2; ++i)
		rt_Bloom_Blades[i].destroy();

	rt_LUM_Mip_Chain.destroy();
	rt_SceneLuminance.destroy();
	rt_SceneLuminancePrevious.destroy();

	rt_smap_depth.destroy();
	rt_smap_surf.destroy();
}

void CRenderTarget::delete_blenders()
{
	Msg("Deleting blenders");

	xr_delete(b_hi_z);
	xr_delete(b_bent_normals);
	xr_delete(b_frame_overlay);
	xr_delete(b_motion_blur);
	xr_delete(b_dof);
	xr_delete(b_reflections);
	xr_delete(b_distortion);
	xr_delete(b_antialiasing);
	xr_delete(b_combine);
	xr_delete(b_autoexposure);
	xr_delete(b_bloom);
	xr_delete(b_ambient_occlusion);
	xr_delete(b_output_to_screen);
	xr_delete(b_effectors);
	xr_delete(b_accum_spot);
	xr_delete(b_accum_point);
	xr_delete(b_accum_direct_cascade);
	xr_delete(b_accum_mask);
	xr_delete(b_occq);
}

CRenderTarget::CRenderTarget()
{
	Msg("Creating render target class...");

	dwWidth = Device.dwWidth;
	dwHeight = Device.dwHeight;

	Engine.ResourceManager->Evict();

	// POINT
	accum_point_geom_create();
	g_accum_point.create(D3DFVF_XYZ, g_accum_point_vb, g_accum_point_ib);
	accum_omnip_geom_create();
	g_accum_omnipart.create(D3DFVF_XYZ, g_accum_omnip_vb, g_accum_omnip_ib);

	// SPOT
	accum_spot_geom_create();
	g_accum_spot.create(D3DFVF_XYZ, g_accum_spot_vb, g_accum_spot_ib);

	// PP
	g_effectors.create(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX3, RenderBackend.Vertex.Buffer(), RenderBackend.QuadIB);

	g_cuboid.create(FVF::F_L, RenderBackend.Vertex.Buffer(), RenderBackend.Index.Buffer());

	if (g_dedicated_server)
		return;

	u32 size = RenderImplementation.o.smapsize;
	rt_smap_depth.create(r_RT_smap_depth, size, size, RHI_Format::D32_FLOAT);
	rt_smap_surf.create(r_RT_smap_surf, size, size, RHI_Format::NULLRT);
	rt_smap_ZB = NULL;

	static CTimer phase_timer;
	phase_timer.Start();
	create_textures();
	Msg("- All textures successfully created");
	phase_timer.Dump();

	phase_timer.Start();
	create_blenders();
	CompileShaders();
	Msg("- All blenders successfully created");
	phase_timer.Dump();

	s_menu_distortion.create("main_menu_distort");
	s_menu_gamma.create("main_menu_gamma");
}

CRenderTarget::~CRenderTarget()
{
	accum_spot_geom_destroy();
	accum_omnip_geom_destroy();
	accum_point_geom_destroy();

	if (g_dedicated_server)
		return;

#ifdef DEBUG
	_SHOW_REF("t_irradiance_map_0 - #small", t_irradiance_map_0->pSurface);
	_SHOW_REF("t_irradiance_map_1 - #small", t_irradiance_map_1->pSurface);

	_SHOW_REF("t_LUT_0", t_LUT_0->pSurface);
	_SHOW_REF("t_LUT_1", t_LUT_1->pSurface);
#endif // DEBUG

	delete_textures();

	delete_blenders();
}
///////////////////////////////////////////////////////////////////////////////////
