#pragma once

// Geometry Buffer
#define r_RT_GBuffer_1 "$user$gbuffer_1"
#define r_RT_GBuffer_2 "$user$gbuffer_2"
#define r_RT_GBuffer_3 "$user$gbuffer_3"
#define r_RT_GBuffer_4 "$user$gbuffer_4"

#define r_RT_Hi_z "$user$hi_z"

// Light Accumulating
#define r_RT_Light_Accumulator "$user$accumulator"

#define r_RT_Volumetric_Sun "$user$Volumetric_Sun"

#define r_RT_Bent_Normals "$user$bent_normals"

// Environment
#define r_T_irradiance0 "$user$env_s0"
#define r_T_irradiance1 "$user$env_s1"
#define r_T_sky0 "$user$sky0"
#define r_T_sky1 "$user$sky1"

#define r_T_LUTs0 "$user$lut_s0"
#define r_T_LUTs1 "$user$lut_s1"

// Output textures
#define r_RT_generic0 "$user$generic0"
#define r_RT_generic1 "$user$generic1"

#define r_RT_backbuffer_mip "$user$r_RT_backbuffer_mip"

// DOF Textures
#define r_RT_dof_coc "$user$dof_coc"   // R16F
#define r_RT_dof_near "$user$dof_near" // RGBA (Color + CoC/Alpha)
#define r_RT_dof_far "$user$dof_far"   // RGBA (Color + CoC)
#define r_RT_dof_dilation "$user$dof_dilation"

#define r_RT_distortion_mask "$user$distortion"

// Motion blur previous frame
#define r_RT_mblur_previous_frame_depth "$user$mblur_previous_frame_depth"
#define r_RT_mblur_dilation_map_0 "$user$mblur_dilation_map_0"
#define r_RT_mblur_dilation_map_1 "$user$mblur_dilation_map_1"

#define r_RT_reflections_raw "$user$reflections_raw"
#define r_RT_reflections "$user$reflections"

#define r_RT_bloom1 "$user$bloom1"
#define r_RT_bloom2 "$user$bloom2"
#define r_RT_bloom_blades1 "$user$bloom_blades1"
#define r_RT_bloom_blades2 "$user$bloom_blades2"

#define r_RT_radiation_noise0 "$user$radiation_noise0"
#define r_RT_radiation_noise1 "$user$radiation_noise1"
#define r_RT_radiation_noise2 "$user$radiation_noise2"

// Ambient occlusion
#define r_RT_ao "$user$ao"
#define r_RT_ao_raw "$user$ao_raw"

// Autoexposure
#define r_RT_autoexposure_mip_chain "$user$r_RT_autoexposure_mip_chain"
#define r_RT_autoexposure_luminance "$user$autoexposure_luminance"
#define r_RT_autoexposure_luminance_previous "$user$autoexposure_luminance_previous"

// Shadow map
#define r_RT_smap_surf "$user$smap_surf"
#define r_RT_smap_depth "$user$smap_depth"

#define r_jitter "$user$jitter_"

#define r_sunmask "sunmask"

#define r_colormap0 "$user$cmap0"
#define r_colormap1 "$user$cmap1"

#define JITTER(a) r_jitter #a

const float SMAP_near_plane = .1f;

const u32 SMAP_adapt_min = 32;
const u32 SMAP_adapt_optimal = 768;
const u32 SMAP_adapt_max = 1536;

const u32 TEX_jitter = 64;
const u32 TEX_jitter_count = 4;

const u32 BLOOM_size_X = 256;
const u32 BLOOM_size_Y = 256;
const u32 LUMINANCE_size = 16;

// deffer
#define SE_NORMAL_HQ 0 // high quality/detail
#define SE_NORMAL_LQ 1 // low quality
#define SE_SHADOW_DEPTH 2	  // shadow generation
#define SE_DEPTH_PREPASS 3

#define SE_DETAIL_NORMAL_ANIMATED 0
#define SE_DETAIL_NORMAL_STATIC 1

#define SE_DETAIL_SHADOW_DEPTH_ANIMATED 2
#define SE_DETAIL_SHADOW_DEPTH_STATIC 3

#define SE_DETAIL_DEPTH_PREPASS_ANIMATED 4
#define SE_DETAIL_DEPTH_PREPASS_STATIC 5

// spot
#define SE_L_FILL 0
#define SE_L_UNSHADOWED 1
#define SE_L_NORMAL 2	  // typical, scaled
#define SE_L_FULLSIZE 3	  // full texture coverage
#define SE_L_TRANSLUENT 4 // with opacity/color mask

// mask
#define SE_MASK_SPOT 0
#define SE_MASK_POINT 1
#define SE_MASK_DIRECT 2

// sun
#define SE_SUN_NEAR 0
#define SE_SUN_MIDDLE 1
#define SE_SUN_FAR 2
#define SE_SUN_VOL_NEAR 3
#define SE_SUN_VOL_MIDDLE 4
#define SE_SUN_VOL_FAR 5
