///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "xrRender_console.h"
///////////////////////////////////////////////////////////////////////////////////
/*-------------------------------------------------------------------------------*/
// Render common tokens
/*-------------------------------------------------------------------------------*/
u32 ps_Preset = 2;
xr_token qpreset_token[] = 
{
	{"Minimum", 0}, 
	{"Default", 1}, 
	{"Maximum", 2}, 
	{"Ultra", 3}, 
	{0, 0}
};

u32 ps_r_shading_mode = SHADING_MODE_PBR;
xr_token shading_mode_token[] = 
{
	{"st_opt_leashed_shading", 0}, 
	{"st_opt_physically_based_shading", 1}, 
	{0, 0}
};

u32 ps_EffPreset = 2;
xr_token qeffpreset_token[] = 
{
	{"st_opt_eff_disabled", 0}, 
	{"st_opt_eff_default", 1}, 
	{"st_opt_eff_cinematic", 2}, 
	{0, 0}
};

u32 ps_r_cubemap_size = 2048;
xr_token cubemap_size_token[] = 
{
	{"1024", 1024}, 
	{"2048", 2048}, 
	{"3072", 3072}, 
	{"4096", 4096}, 
	{"6144", 6144}, 
	{"8192", 8192}, 
	{0, 0}
};

xr_token detail_quality_token[] = {{"st_opt_low", 1},	 // 1 = Low
								   {"st_opt_medium", 2}, // 2 = Medium
								   {"st_opt_high", 3},	 // 3 = High
								   {"st_opt_ultra", 4},	 // 4 = Ultra
								   {0, 0}};

/*
u32 ps_r1_aa = 0;
xr_token r1_aa_token[] = {
	{"st_opt_disabled", 0},  
	{"st_opt_msaa_2x", MSAA_2X}, 
	{"st_opt_msaa_4x", MSAA_4X}, 
	{"st_opt_msaa_8x", MSAA_8X},
	{"st_opt_csaa_4x", CSAA_4X},
	{"st_opt_csaa_8x", CSAA_8X},
	{"st_opt_ssaa_2x", SSAA_2X}, 
	{"st_opt_ssaa_4x", SSAA_4X},
	{0, 0},
};

u32 ps_r1_aa_transluency = 0;
D3DFORMAT trans_ssaa_fmt = (D3DFORMAT)MAKEFOURCC('S', 'S', 'A', 'A');
D3DFORMAT trans_atoc_fmt = (D3DFORMAT)MAKEFOURCC('A', 'T', 'O', 'C');
xr_token r1_aa_transluency_token[] = {
	{"st_opt_disabled", 0}, 
	{"st_opt_ssaa", trans_ssaa_fmt},
	{"st_opt_atoc", trans_atoc_fmt},
	{0, 0},
};
*/

u32 ps_vignette_mode = 2;
xr_token vignette_mode_token[] = 
{
	{"st_opt_disabled", 0}, 
	{"st_opt_static", 1}, 
	{"st_opt_dynamic", 2}, 
	{0, 0}
};

u32 ps_geometry_quality_mode = 3;
xr_token geometry_quality_mode_token[] = 
{
	{"st_opt_low", 1}, 
	{"st_opt_medium", 2}, 
	{"st_opt_high", 3}, 
	{0, 0}
};

Flags32 ps_r_ls_flags = {0};

/*-------------------------------------------------------------------------------*/
// R2a/R2/R2.5 specific tokens
/*-------------------------------------------------------------------------------*/
u32 ps_r_ao_quality = 2;
xr_token ao_quality_token[] = 
{
	{"st_opt_ssao", 1}, 
	{"st_opt_hbao_plus", 2}, 
	{"st_opt_gtao", 3}, 
	{"st_opt_ssptao", 4}, 
	{0, 0}
};

u32 ps_r_bloom_quality = 2;
xr_token bloom_quality_token[] = 
{
	{"st_opt_low", 1}, 
	{"st_opt_medium", 2}, 
	{"st_opt_high", 3}, 
	{0, 0}
};

u32 ps_r_shadow_filtering = 2;
xr_token shadow_filter_token[] = 
{
	{"st_opt_low", 1}, 
	{"st_opt_medium", 2}, 
	{"st_opt_high", 3}, 
	{0, 0}
};

u32 ps_r_sun_shafts_quality = 2;
xr_token qsun_shafts_token[] = 
{
	{"st_opt_low", 1}, 
	{"st_opt_medium", 2}, 
	{"st_opt_high", 3}, 
	{0, 0}
};

u32 ps_r_material_quality = 1;
xr_token material_quality_token[] = 
{
	{"st_opt_low", 1}, 
	{"st_opt_medium", 2}, 
	{"st_opt_high", 3}, 
	{0, 0}
};

u32 ps_r_dof_quality = 2;
xr_token dof_quality_token[] = 
{
	{"st_opt_low", 1}, 
	{"st_opt_medium", 2}, 
	{"st_opt_high", 3}, 
	{0, 0}
};

u32 ps_r_debug_render = 0;
xr_token debug_render_token[] = 
{
	{"disabled", 0},
	{"gbuffer_albedo", 1},
	{"gbuffer_position", 2},
	{"gbuffer_normal", 3},
	{"gbuffer_roughness", 4},
	{"gbuffer_matallness", 5},
	{"gbuffer_subsurface", 6},
	{"gbuffer_emissive", 7},
	{"gbuffer_lightmap_ao", 8},
	{"gbuffer_baked_ao", 9},
	{"direct_light", 10},
	{"indirect_light", 11},
	{"real_time_ao", 12},
	{0, 0}
};

u32 ps_r_color_blind_mode = 0;
xr_token color_blind_token[] = 
{
	{"st_opt_disable", 0},
	{"st_opt_achromatomaly", 1},
	{"st_opt_achromatopsia", 2},
	{"st_opt_deuteranomaly", 3},
	{"st_opt_protanomaly", 4},
	{"st_opt_protanopia", 5},
	{"st_opt_tritanomaly", 6},
	{"st_opt_tritanopia", 7},
	{0, 0}
};
/*-------------------------------------------------------------------------------*/
// Render common values
/*-------------------------------------------------------------------------------*/
int ps_r_LightSleepFrames = 10;

float ps_r_ao_radius = 2.0f;
float ps_r_ao_bias = 0.01f;

float ps_r_debug_reserved_0 = 1.0f;
float ps_r_debug_reserved_1 = 1.0f;
float ps_r_debug_reserved_2 = 1.0f;
float ps_r_debug_reserved_3 = 1.0f;

float ps_r_Detail_l_ambient = 0.9f;
float ps_r_Detail_l_aniso = 0.25f;
float ps_r_Detail_density = 0.3f;

float ps_r_Detail_radius = 50.0f;
float ps_r_Detail_scale = 1.0f;
float ps_r_Detail_height = 0.0f;
u32 ps_r_Detail_quality = 2;

float ps_r_Tree_w_rot = 10.0f;
float ps_r_Tree_w_speed = 1.00f;
float ps_r_Tree_w_amp = 0.005f;
fvec3 ps_r_Tree_Wave = {.1f, .01f, .11f};
float ps_r_Tree_SBC = 1.5f; // scale bias correct

float ps_r_WallmarkTTL = 300.f;
float ps_r_WallmarkSHIFT = 0.0001f;
float ps_r_WallmarkSHIFT_V = 0.0001f;

float ps_r_GLOD_ssa_start = 256.f;
float ps_r_GLOD_ssa_end = 64.f;
float ps_r_LOD = 1.f;
float ps_r_ssaDISCARD = 3.5f;
float ps_r_ssaDONTSORT = 32.f;
float ps_r_ssaHZBvsTEX = 96.f;

float ps_pps_u = 0.0f;
float ps_pps_v = 0.0f;

int ps_r_thread_wait_sleep = 0;

// Render common flags
Flags32 ps_render_flags = {RFLAG_LENS_FLARES | RFLAG_EXP_MT_CALC | RFLAG_EXP_HW_OCC};

ECORE_API Flags32 ps_r_debug_flags = {NULL};

/*-------------------------------------------------------------------------------*/
// R2/R2a/R2.5-specific values
/*-------------------------------------------------------------------------------*/
float ps_r_ssaLOD_A = 48.f;
float ps_r_ssaLOD_B = 32.f;
float ps_r_detalization_distance = 75.f;

float ps_r_tf_Mipbias = -0.5f;

float ps_r_df_parallax_h = 0.02f;

float ps_r_ao_brightness = 1.0f;

float ps_r_autoexposure_middlegray = 0.5f;
float ps_r_autoexposure_adaptation = 0.3f;
float ps_r_autoexposure_low_lum = 0.5f;
float ps_r_autoexposure_amount = 1.0f;

float ps_r_bloom_threshold = 0.25f;
float ps_r_bloom_brightness = 2.0f;
float ps_r_bloom_blades_threshold = 0.9f;
float ps_r_bloom_blades_brightness = 0.2f;

float ps_r_fxaa_subpix = 0.6f;
float ps_r_fxaa_edge_treshold = 0.063f;
float ps_r_fxaa_edge_treshold_min = 0.0f;

float ps_r_mblur = 0.5f;

fvec3 ps_r_dof = fvec3().set(100.0f, 100.0f, 100.0f);

float ps_r_ls_depth_scale = 1.00001f;
float ps_r_ls_depth_bias = -0.001f;
float ps_r_ls_squality = 1.0f;
float ps_r_tsm_bias = -0.05f;
float ps_r_ls_far = 200.0f;

float ps_r_sun_tsm_bias = -0.05f;
float ps_r_sun_near = 12.f;
float ps_r_sun_far = 200.f;

// Эмпирически подобранные данные - настраиваются командами
float ps_r_sun_depth_far_normal_bias = 0.0f;
float ps_r_sun_depth_far_directional_bias = 0.0f;
float ps_r_sun_depth_far_bias = -0.0025f;
float ps_r_sun_depth_middle_normal_bias = 0.12f;
float ps_r_sun_depth_middle_directional_bias = 0.0f;
float ps_r_sun_depth_middle_bias = 0.0f;
float ps_r_sun_depth_near_normal_bias = 0.0f;
float ps_r_sun_depth_near_directional_bias = 0.0f;
float ps_r_sun_depth_near_bias = 0.0f;
float ps_r_sun_depth_normal_bias = 0.05f;
float ps_r_sun_depth_directional_bias = 0.02f;
float ps_r_sun_lumscale = 1.0f;
float ps_r_sun_lumscale_hemi = 1.0f;
float ps_r_sun_lumscale_amb = 1.0f;

float ps_r_dhemi_sky_scale = 0.08f;
float ps_r_dhemi_light_scale = 0.2f;
float ps_r_dhemi_light_flow = 0.1f;
float ps_r_dhemi_scale = 1.f;
int ps_r_dhemi_count = 5;

float ps_r_lt_smooth = 1.f;

int ps_r_light_fragments_cull = 10.f;
float ps_r_light_distance_cull = 100.f;

float ps_r_slight_fade = 1.f;

float ps_cas_contrast = 0.1f;
float ps_cas_sharpening = 0.1f;

// R2-specific flags
Flags32 ps_r_lighting_flags = {RFLAG_SUN | RFLAG_EXP_DONT_TEST_UNSHADOWED};

Flags32 ps_r_postprocess_flags = {RFLAG_DOF | RFLAG_MBLUR | RFLAG_AUTOEXPOSURE};

Flags32 ps_r_overlay_flags = {NULL};

/*-------------------------------------------------------------------------------*/
// Methods
/*-------------------------------------------------------------------------------*/
#ifndef _EDITOR
///////////////////////////////////////////////////////////////////////////////////
#include "..\xrEngine\xr_ioconsole.h"
#include "..\xrEngine\xr_ioc_cmd.h"
///////////////////////////////////////////////////////////////////////////////////
#define MAX_CONDITIONS_TOKEN_SIZE 32
class CCC_ConditionsToken : public CCC_Token
{
  protected:
	xr_token tokens_mem[MAX_CONDITIONS_TOKEN_SIZE];
	bool conditions[MAX_CONDITIONS_TOKEN_SIZE];

  public:
	CCC_ConditionsToken(LPCSTR N, u32* V, xr_token* T) : CCC_Token(N, V, T)
	{
		memset(tokens_mem, 0, sizeof(tokens_mem));
		memset(conditions, 1, sizeof(conditions));
	};

	void SetCondition(int key, bool condition)
	{
		for (int i = 0; tokens[i].name && i < MAX_CONDITIONS_TOKEN_SIZE; i++)
		{
			if (key == tokens[i].id)
			{
				conditions[i] = condition;
				break;
			}
		}
	}

	void ApplyConditions()
	{
		memset(tokens_mem, 0, sizeof(tokens_mem));

		for (int i = 0, j = 0; tokens[i].name && j < MAX_CONDITIONS_TOKEN_SIZE; i++)
		{
			if (conditions[i] == true)
			{
				tokens_mem[j] = tokens[i];
				j++;
			}
		}

		tokens = tokens_mem;
	};
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_tf_MipBias : public CCC_Float
{
  public:
	void apply()
	{
		if (0 == RenderBackend.GetDevice())
			return;
		for (u32 i = 0; i < RHI()->GetDeviceCaps().MaxSimultaneousTextures; i++)
			CHK_DX(RenderBackend.GetDevice()->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, *((LPDWORD)value)));
	}

	CCC_tf_MipBias(LPCSTR N, float* v) : CCC_Float(N, v, -0.5f, +0.5f){};
	virtual void Execute(LPCSTR args)
	{
		CCC_Float::Execute(args);
		apply();
	}
	virtual void Status(TStatus& S)
	{
		CCC_Float::Status(S);
		apply();
	}
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_Screenshot : public IConsole_Command
{
  public:
	CCC_Screenshot(LPCSTR N) : IConsole_Command(N){};
	virtual void Execute(LPCSTR args)
	{
		string_path name;
		name[0] = 0;
		sscanf(args, "%s", name);
		LPCSTR image = xr_strlen(name) ? name : 0;
		::Render->Screenshot(IRender_interface::SM_NORMAL, image);
	}
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_ModelPoolStat : public IConsole_Command
{
  public:
	CCC_ModelPoolStat(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled = TRUE;
	};
	virtual void Execute(LPCSTR args)
	{
		RenderImplementation.Models->dump();
	}
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_SaveOcclusionDepthBuffer : public IConsole_Command
{
public:
	CCC_SaveOcclusionDepthBuffer(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled = TRUE;
	};
	virtual void Execute(LPCSTR args)
	{
		RenderImplementation.CPUOCC.SaveDepthBuffer();
	}
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_Preset : public CCC_Token
{
  public:
	CCC_Preset(LPCSTR N, u32* V, xr_token* T) : CCC_Token(N, V, T){};

	virtual void Execute(LPCSTR args)
	{
		CCC_Token::Execute(args);
		string_path _cfg;
		string_path cmd;

		switch (*value)
		{
		case 0:
			strcpy(_cfg, "rspec_minimum.ltx");
			break;
		case 1:
			strcpy(_cfg, "rspec_default.ltx");
			break;
		case 2:
			strcpy(_cfg, "rspec_maximum.ltx");
			break;
		case 3:
			strcpy(_cfg, "rspec_ultra.ltx");
			break;
		}
		FS.update_path(_cfg, "$game_config$", _cfg);
		strconcat(sizeof(cmd), cmd, "cfg_load", " ", _cfg);
		Console->Execute(cmd);
	}
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_EffPreset : public CCC_Token
{
  public:
	CCC_EffPreset(LPCSTR N, u32* V, xr_token* T) : CCC_Token(N, V, T){};

	virtual void Execute(LPCSTR args)
	{
		CCC_Token::Execute(args);
		string_path _cfg;
		string_path cmd;

		switch (*value)
		{
		case 0:
			strcpy(_cfg, "eff_disabled.ltx");
			break;
		case 1:
			strcpy(_cfg, "eff_default.ltx");
			break;
		case 2:
			strcpy(_cfg, "eff_cinematic.ltx");
			break;
		}
		FS.update_path(_cfg, "$game_config$", _cfg);
		strconcat(sizeof(cmd), cmd, "cfg_load", " ", _cfg);
		Console->Execute(cmd);
	}
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_DofFStop : public CCC_Float
{
  public:
	CCC_DofFStop(LPCSTR N, float* V, float _min = 2.0f, float _max = 100.0f) : CCC_Float(N, V, _min, _max)
	{
	}

	virtual void Execute(LPCSTR args)
	{
		float v = float(atof(args));

		CCC_Float::Execute(args);
		if (g_pGamePersistent)
			g_pGamePersistent->SetBaseDof(ps_r_dof);
	}

	//	CCC_Dof should save all data as well as load from config
	virtual void Save(IWriter* F)
	{
		;
	}
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_DofFocalDepth : public CCC_Float
{
  public:
	CCC_DofFocalDepth(LPCSTR N, float* V, float _min = 0.5f, float _max = 100.0f) : CCC_Float(N, V, _min, _max)
	{
	}

	virtual void Execute(LPCSTR args)
	{
		CCC_Float::Execute(args);
		if (g_pGamePersistent)
			g_pGamePersistent->SetBaseDof(ps_r_dof);
	}

	//	CCC_Dof should save all data as well as load from config
	virtual void Save(IWriter* F)
	{
		;
	}
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_Dof : public CCC_Vector3
{
  public:
	CCC_Dof(LPCSTR N, fvec3* V, const fvec3 _min, const fvec3 _max) : CCC_Vector3(N, V, _min, _max)
	{
		;
	}

	virtual void Execute(LPCSTR args)
	{
		//fvec3 v;
		//if (3 != sscanf(args, "%f,%f,%f", &v.x, &v.y, &v.z))
		//	InvalidSyntax();
		//else if ((v.x > v.y - 0.1f) || (v.z < v.y + 0.1f))
		//{
		//	InvalidSyntax();
		//	Msg("x <= y - 0.1");
		//	Msg("y <= z - 0.1");
		//}
		//else
		//{
			CCC_Vector3::Execute(args);
			if (g_pGamePersistent)
				g_pGamePersistent->SetBaseDof(ps_r_dof);
		//}
	}
	virtual void Status(TStatus& S)
	{
		sprintf(S, "%f,%f,%f", value->x, value->y, value->z);
	}
	virtual void Info(TInfo& I)
	{
		sprintf(I, "vector3 in range [%f,%f,%f]-[%f,%f,%f]", min.x, min.y, min.z, max.x, max.y, max.z);
	}
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_DetailQuality : public CCC_Token
{
  public:
	CCC_DetailQuality(LPCSTR N, u32* V, xr_token* T) : CCC_Token(N, V, T){};

	virtual void Execute(LPCSTR args)
	{
		CCC_Token::Execute(args);
		switch (*value)
		{
		case 1: // Low
			ps_r_Detail_density = 0.50f;
			ps_r_Detail_radius = 30.0f;
			ps_r_Detail_scale = 1.00f;
			ps_r_Detail_height = 0.00f;
			break;
		case 2: // Medium
			ps_r_Detail_density = 0.50f;
			ps_r_Detail_radius = 50.0f;
			ps_r_Detail_scale = 1.00f;
			ps_r_Detail_height = 0.00f;
			break;
		case 3: // High
			ps_r_Detail_density = 0.45f;
			ps_r_Detail_radius = 60.0f;
			ps_r_Detail_scale = 1.00f;
			ps_r_Detail_height = 0.00f;
			break;
		case 4: // Ultra
			ps_r_Detail_density = 0.40f;
			ps_r_Detail_radius = 75.0f;
			ps_r_Detail_scale = 1.00f;
			ps_r_Detail_height = 0.00f;
			break;
		}

		// Обновляем кеш
		if (RenderImplementation.Details)
			RenderImplementation.Details->InvalidateCache();
	}
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_tf_Aniso : public CCC_Integer
{
public:
	void apply()
	{
		if (0 == RenderBackend.GetDevice())
			return;
		int val = *value;
		clamp(val, 2, 16);

		RenderBackend.set_anisotropy_filtering(val);
	}
	CCC_tf_Aniso(LPCSTR N, int* v) : CCC_Integer(N, v, 2, 16) {};
	virtual void Execute(LPCSTR args)
	{
		CCC_Integer::Execute(args);
		apply();
	}
	virtual void Status(TStatus& S)
	{
		CCC_Integer::Status(S);
		apply();
	}
};
///////////////////////////////////////////////////////////////////////////////////
void xrRender_initconsole()
{
	fvec3 tw_min, tw_max;

	// Render common commands
	CMD3(CCC_Preset, "_preset", &ps_Preset, qpreset_token);
	CMD3(CCC_EffPreset, "eff_preset", &ps_EffPreset, qeffpreset_token);

	CMD1(CCC_Screenshot, "screenshot");
	
	CMD4(CCC_Float, "r_ao_radius", &ps_r_ao_radius, 1, 10);
	CMD4(CCC_Float, "r_ao_bias", &ps_r_ao_bias, -1.0f, 1.0f);

	//CMD4(CCC_Integer, "r_lsleep_frames", &ps_r_LightSleepFrames, 4, 30);
	CMD4(CCC_Float, "r_ssa_glod_start", &ps_r_GLOD_ssa_start, 128, 512);
	CMD4(CCC_Float, "r_ssa_glod_end", &ps_r_GLOD_ssa_end, 16, 96);
	//CMD4(CCC_Float, "r_wallmark_shift_pp", &ps_r_WallmarkSHIFT, 0.0f, 1.f);
	//CMD4(CCC_Float, "r_wallmark_shift_v", &ps_r_WallmarkSHIFT_V, 0.0f, 1.f);
	//CMD4(CCC_Float, "r_wallmark_ttl", &ps_r_WallmarkTTL, 1.0f, 5.f * 60.f);
	CMD1(CCC_ModelPoolStat, "stat_models");
	
	CMD1(CCC_SaveOcclusionDepthBuffer, "save_occlusion_culling_depth_buffer");

	CMD3(CCC_Token, "r_cubemap_size", &ps_r_cubemap_size, cubemap_size_token);
	
	CMD3(CCC_Token, "r_shading_mode", &ps_r_shading_mode, shading_mode_token);

	CMD4(CCC_Float, "r_geometry_lod", &ps_r_LOD, 0.1f, 1.2f);

	CMD4(CCC_Float, "r_detail_density", &ps_r_Detail_density, .01f, 0.6f);

	//CMD4(CCC_Float, "r_detail_l_ambient", &ps_r_Detail_l_ambient, .5f, .95f);
	//CMD4(CCC_Float, "r_detail_l_aniso", &ps_r_Detail_l_aniso, .1f, .5f);

	CMD3(CCC_DetailQuality, "r_detail_quality", &ps_r_Detail_quality, detail_quality_token);

	tw_min.set(EPS, EPS, EPS);
	tw_max.set(2, 2, 2);

	CMD3(CCC_Mask, "r_lens_flares", &ps_render_flags, RFLAG_LENS_FLARES);

	//CMD3(CCC_Mask, "r_lut", &ps_render_flags, RFLAG_LUT);
	CMD3(CCC_Token, "r_vignette_mode", &ps_vignette_mode, vignette_mode_token);
	CMD3(CCC_Mask, "r_chromatic_abberation", &ps_render_flags, RFLAG_CHROMATIC_ABBERATION);

	//CMD3(CCC_Mask, "r_mt", &ps_render_flags, RFLAG_EXP_MT_CALC);

	CMD4(CCC_Integer, "r_wait_sleep", &ps_r_thread_wait_sleep, 0, 10);

	CMD3(CCC_Mask, "r_hardware_occlusion_culling", &ps_render_flags, RFLAG_EXP_HW_OCC);

	//CMD4(CCC_Float, "r_pps_u", &ps_pps_u, -1.f, +1.f);
	//CMD4(CCC_Float, "r_pps_v", &ps_pps_v, -1.f, +1.f);

	CMD3(CCC_Mask, "r_anti_aliasing", &ps_r_postprocess_flags, RFLAG_ANTI_ALIASING);
	CMD4(CCC_Float, "r_fxaa_subpix", &ps_r_fxaa_subpix, 0.0f, 1.0f);
	CMD4(CCC_Float, "r_fxaa_treshold", &ps_r_fxaa_edge_treshold, 0.0f, 1.0f);
	CMD4(CCC_Float, "r_fxaa_treshold_min", &ps_r_fxaa_edge_treshold_min, 0.0f, 1.0f);

	CMD3(CCC_Token, "r_ao_quality", &ps_r_ao_quality, ao_quality_token);
	CMD4(CCC_Float, "r_ao_brightness", &ps_r_ao_brightness, 0.0f, 1.0f);

	CMD3(CCC_Mask, "r_autoexposure", &ps_r_postprocess_flags, RFLAG_AUTOEXPOSURE);
	CMD4(CCC_Float, "r_autoexposure_middlegray", &ps_r_autoexposure_middlegray, 0.0f, 2.0f);
	CMD4(CCC_Float, "r_autoexposure_adaptation", &ps_r_autoexposure_adaptation, 0.0001f, 10.0f);
	CMD4(CCC_Float, "r_autoexposure_lowlum", &ps_r_autoexposure_low_lum, 0.0f, 1.0f);
	CMD4(CCC_Float, "r_autoexposure_amount", &ps_r_autoexposure_amount, 0.0f, 1.0f);

	CMD3(CCC_Mask, "r_bloom", &ps_r_postprocess_flags, RFLAG_BLOOM);
	CMD3(CCC_Token, "r_bloom_quality", &ps_r_bloom_quality, bloom_quality_token);
	CMD4(CCC_Float, "r_bloom_threshold", &ps_r_bloom_threshold, 0.0f, 1.0f);
	CMD4(CCC_Float, "r_bloom_brightness", &ps_r_bloom_brightness, 0.0f, 10.0f);
	CMD4(CCC_Float, "r_bloom_blades_threshold", &ps_r_bloom_blades_threshold, 0.0f, 1.0f);
	CMD4(CCC_Float, "r_bloom_blades_brightness", &ps_r_bloom_blades_brightness, 0.0f, 10.0f);

	CMD3(CCC_Mask, "r_mblur_enabled", &ps_r_postprocess_flags, RFLAG_MBLUR);
	CMD4(CCC_Float, "r_mblur_power", &ps_r_mblur, 0.0f, 1.0f);

	CMD3(CCC_Mask, "r_reflections", &ps_r_postprocess_flags, RFLAG_REFLECTIONS);

	tw_min.set(-10000, -10000, 0);
	tw_max.set(10000, 10000, 10000);
	CMD4(CCC_Dof, "r_dof", &ps_r_dof, tw_min, tw_max);
	CMD4(CCC_DofFocalDepth, "r_dof_focal_depth", &ps_r_dof.x, 0.5f, 100.0f);
	CMD4(CCC_DofFStop, "r_dof_fstop", &ps_r_dof.z, 0.1f, 100.0f);
	CMD3(CCC_Mask, "r_dof_enabled", &ps_r_postprocess_flags, RFLAG_DOF);
	CMD3(CCC_Token, "r_dof_quality", &ps_r_dof_quality, dof_quality_token);

	CMD3(CCC_Mask, "r_cas_enabled", &ps_r_postprocess_flags, RFLAG_CONTRAST_ADAPTIVE_SHARPENING);
	CMD4(CCC_Float, "r_cas_contrast", &ps_cas_contrast, 0.0f, 1.0f);
	CMD4(CCC_Float, "r_cas_sharpening", &ps_cas_sharpening, 0.0f, 1.0f);

	CMD3(CCC_Mask, "r_photo_grid", &ps_r_overlay_flags, RFLAG_PHOTO_GRID);
	CMD3(CCC_Mask, "r_cinema_borders", &ps_r_overlay_flags, RFLAG_CINEMA_BORDERS);
	CMD3(CCC_Mask, "r_watermark", &ps_r_overlay_flags, RFLAG_WATERMARK);

	CMD3(CCC_Mask, "r_sun_shafts_enabled", &ps_r_lighting_flags, RFLAG_SUN_SHAFTS);
	CMD3(CCC_Token, "r_sun_shafts_quality", &ps_r_sun_shafts_quality, qsun_shafts_token);
	CMD3(CCC_Token, "r_shadow_filtering", &ps_r_shadow_filtering, shadow_filter_token);
	CMD3(CCC_Mask, "r_sun", &ps_r_lighting_flags, RFLAG_SUN);
	CMD3(CCC_Mask, "r_sun_details", &ps_r_lighting_flags, RFLAG_SUN_DETAILS);
	//CMD3(CCC_Mask, "r_exp_donttest_uns", &ps_r_lighting_flags, RFLAG_EXP_DONT_TEST_UNSHADOWED);
	CMD4(CCC_Float, "r_sun_tsm_bias", &ps_r_sun_tsm_bias, -0.5, +0.5);
	//CMD4(CCC_Float, "r_sun_near", &ps_r_sun_near, 1.f, 50.f);
	CMD4(CCC_Float, "r_sun_far", &ps_r_sun_far, 100.f, 360.f);

	CMD4(CCC_Float, "r_sun_depth_far_normal_bias", &ps_r_sun_depth_far_normal_bias, -0.5, 0.5);
	CMD4(CCC_Float, "r_sun_depth_far_directional_bias", &ps_r_sun_depth_far_directional_bias, -0.5, 0.5);
	CMD4(CCC_Float, "r_sun_depth_far_bias", &ps_r_sun_depth_far_bias, -0.5, +0.5);
	CMD4(CCC_Float, "r_sun_depth_middle_normal_bias", &ps_r_sun_depth_middle_normal_bias, -0.5, 0.5);
	CMD4(CCC_Float, "r_sun_depth_middle_directional_bias", &ps_r_sun_depth_middle_directional_bias, -0.5, 0.5);
	CMD4(CCC_Float, "r_sun_depth_middle_bias", &ps_r_sun_depth_middle_bias, -0.5, +0.5);
	CMD4(CCC_Float, "r_sun_depth_near_normal_bias", &ps_r_sun_depth_near_normal_bias, -0.5, 0.5);
	CMD4(CCC_Float, "r_sun_depth_near_directional_bias", &ps_r_sun_depth_near_directional_bias, -0.5, 0.5);
	CMD4(CCC_Float, "r_sun_depth_near_bias", &ps_r_sun_depth_near_bias, -0.5, +0.5);

	//CMD4(CCC_Float, "r_sun_lumscale", &ps_r_sun_lumscale, -1.0, +3.0);
	//CMD4(CCC_Float, "r_sun_lumscale_hemi", &ps_r_sun_lumscale_hemi, 0.0, +3.0);
	//CMD4(CCC_Float, "r_sun_lumscale_amb", &ps_r_sun_lumscale_amb, 0.0, +3.0);

	//CMD3(CCC_Mask, "r_shadow_cascede_zcul", &ps_r_lighting_flags, RFLAGEXT_SUN_ZCULLING);

	CMD3(CCC_Mask, "r_allow_r1_lights", &ps_r_lighting_flags, RFLAG_R1LIGHTS);

	//CMD4(CCC_Float, "r_slight_fade", &ps_r_slight_fade, .02f, 2.f);

	CMD4(CCC_Integer, "r_dhemi_count", &ps_r_dhemi_count, 4, 25);
	CMD4(CCC_Float, "r_dhemi_scale", &ps_r_dhemi_scale, .5f, 3.f);
	CMD4(CCC_Float, "r_dhemi_smooth", &ps_r_lt_smooth, 0.f, 10.f);
	

	CMD4(CCC_Float, "r_lights_distance_culling", &ps_r_light_distance_cull, 10.f, 300.f);	
	CMD4(CCC_Integer, "r_lights_fragments_culling", &ps_r_light_fragments_cull, 0u, 1000u);

	CMD3(CCC_Token, "r_material_quality", &ps_r_material_quality, material_quality_token);

	CMD3(CCC_Token, "r_debug_render", &ps_r_debug_render, debug_render_token);	
	
	CMD3(CCC_Token, "r_color_blind_mode", &ps_r_color_blind_mode, color_blind_token);

	CMD4(CCC_Float, "r_ssa_lod_a", &ps_r_ssaLOD_A, 16, 96);
	CMD4(CCC_Float, "r_ssa_lod_b", &ps_r_ssaLOD_B, 32, 64);
	CMD4(CCC_Float, "r_detalization_distance", &ps_r_detalization_distance, 10.0f, 100.0f);

	CMD2(CCC_tf_MipBias, "r_tf_mipbias", &ps_r_tf_Mipbias);
	CMD2(CCC_tf_Aniso, "rs_anisothropy", &psAnisotropic); //	{1..16}

	//CMD3(CCC_Mask, "r_use_nvdbt", &ps_r_ls_flags, RFLAG_USE_NVDBT);

	CMD4(CCC_Float, "r_ls_depth_scale", &ps_r_ls_depth_scale, 0.5, 1.5);
	CMD4(CCC_Float, "r_ls_depth_bias", &ps_r_ls_depth_bias, -0.5, +0.5);
	//CMD4(CCC_Float, "r_ls_squality", &ps_r_ls_squality, .5f, 1.f);
	CMD4(CCC_Float, "r_tsm_bias", &ps_r_sun_tsm_bias, -0.5, +0.5);
	CMD4(CCC_Float, "r_ls_far", &ps_r_ls_far, 50.0f, 300.f);

	CMD3(CCC_Mask, "r_disable_postprocess", &ps_render_flags, RFLAG_DISABLE_POSTPROCESS);

	CMD4(CCC_Float, "r_debug_reserved_0", &ps_r_debug_reserved_0, -1000, 1000);
	CMD4(CCC_Float, "r_debug_reserved_1", &ps_r_debug_reserved_1, -1000, 1000);
	CMD4(CCC_Float, "r_debug_reserved_2", &ps_r_debug_reserved_2, -1000, 1000);
	CMD4(CCC_Float, "r_debug_reserved_3", &ps_r_debug_reserved_3, -1000, 1000);

	CMD3(CCC_Mask, "r_draw_sun_occluders", &ps_r_debug_flags, RFLAG_DRAW_SUN_OCCLUDERS);
	CMD3(CCC_Mask, "r_draw_hom_occluders", &ps_r_debug_flags, RFLAG_DRAW_HOM_OCCLUDERS);

	CMD3(CCC_Token, "r_geometry_quality_mode", &ps_geometry_quality_mode, geometry_quality_mode_token);

	// !!! СТРОГО ВНИЗУ ПОСЛЕ ВСЕХ ОПЦИЙ !!!
//#pragma todo("Брух")
	//CMD3(CCC_ConditionsToken, "r1_aa_type", &ps_r1_aa, r1_aa_token);
	//CMD3(CCC_ConditionsToken, "r1_aa_transluency", &ps_r1_aa_transluency, r1_aa_transluency_token);
}

void xrRender_console_apply_conditions()
{
#if 0
	CCC_ConditionsToken* r1_aa_type = dynamic_cast<CCC_ConditionsToken*>(Console->GetCommand("r1_aa_type"));
	r1_aa_type->SetCondition(CSAA_4X, HW.GetCaps().max_coverage >= 2);
	r1_aa_type->SetCondition(CSAA_8X, HW.GetCaps().max_coverage >= 4);
	r1_aa_type->ApplyConditions();

	CCC_ConditionsToken* r1_aa_transluency = dynamic_cast<CCC_ConditionsToken*>(Console->GetCommand("r1_aa_transluency"));
	r1_aa_transluency->SetCondition(trans_ssaa_fmt, HW.support(trans_ssaa_fmt, D3DRTYPE_SURFACE, NULL));
	r1_aa_transluency->SetCondition(trans_atoc_fmt, HW.support(trans_atoc_fmt, D3DRTYPE_SURFACE, NULL));
	r1_aa_transluency->ApplyConditions();
#endif
}
///////////////////////////////////////////////////////////////////////////////////
void xrRender_apply_tf()
{
	Console->Execute("rs_anisothropy");
	Console->Execute("r_tf_mipbias");
}
///////////////////////////////////////////////////////////////////////////////////
#endif // #ifndef _EDITOR
///////////////////////////////////////////////////////////////////////////////////
