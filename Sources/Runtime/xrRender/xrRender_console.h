///////////////////////////////////////////////////////////////////////////////////
#ifndef xrRender_consoleH
#define xrRender_consoleH
///////////////////////////////////////////////////////////////////////////////////
/*-------------------------------------------------------------------------------*/
// Render common values
/*-------------------------------------------------------------------------------*/
extern u32 ps_r_cubemap_size;

/*
enum enum_r1_msaa
{
	MSAA_2X = 12,
	MSAA_4X = 14,
	MSAA_8X = 18,
	CSAA_4X = 22,
	CSAA_8X = 24,
	SSAA_2X = 32,
	SSAA_4X = 34,
	MSAA = 1,
	SSAA = 3,
};
extern u32 ps_r1_aa;
extern u32 ps_r1_aa_transluency;
*/

extern int ps_r_LightSleepFrames;

extern float ps_r_ao_radius;
extern float ps_r_ao_bias;

extern float ps_r_Detail_l_ambient;
extern float ps_r_Detail_l_aniso;
extern float ps_r_Detail_density;

extern float ps_r_Detail_radius; // Радиус отрисовки
extern float ps_r_Detail_scale;  // Глобальный масштаб
extern float ps_r_Detail_height; // Смещение по высоте
extern u32 ps_r_Detail_quality;  // Сам пресет (Low/Med/High)

extern float ps_r_Tree_w_rot;
extern float ps_r_Tree_w_speed;
extern float ps_r_Tree_w_amp;
extern float ps_r_Tree_SBC; // scale bias correct
extern fvec3 ps_r_Tree_Wave;

extern float ps_r_WallmarkTTL;
extern float ps_r_WallmarkSHIFT;
extern float ps_r_WallmarkSHIFT_V;

extern float ps_r_GLOD_ssa_start;
extern float ps_r_GLOD_ssa_end;
extern float ps_r_LOD;
extern float ps_r_ssaDISCARD;
extern float ps_r_ssaDONTSORT;
extern float ps_r_ssaHZBvsTEX;

extern int ps_r_thread_wait_sleep;

extern float ps_r_debug_reserved_0;
extern float ps_r_debug_reserved_1;
extern float ps_r_debug_reserved_2;
extern float ps_r_debug_reserved_3;

// Postprocess effects
extern u32 ps_vignette_mode;

extern u32 ps_geometry_quality_mode;

extern float ps_pps_u;
extern float ps_pps_v;

// Render common flags
extern Flags32 ps_render_flags;
enum
{
	RFLAG_LENS_FLARES = (1 << 0),
	RFLAG_EXP_MT_CALC = (1 << 1),
	RFLAG_EXP_HW_OCC = (1 << 2),
	RFLAG_LUT = (1 << 3),
	RFLAG_CHROMATIC_ABBERATION = (1 << 4),
	RFLAG_DISABLE_POSTPROCESS = (1 << 5)
};

/*-------------------------------------------------------------------------------*/
// R2/R2a/R2.5-specific values
/*-------------------------------------------------------------------------------*/

extern float ps_r_ssaLOD_A;
extern float ps_r_ssaLOD_B;
extern float ps_r_detalization_distance;

extern float ps_r_tf_Mipbias;

extern float ps_r_gmaterial;

extern float ps_r_autoexposure_middlegray;
extern float ps_r_autoexposure_adaptation;
extern float ps_r_autoexposure_low_lum;
extern float ps_r_autoexposure_amount;

extern u32 ps_r_bloom_quality;
extern float ps_r_bloom_threshold;
extern float ps_r_bloom_brightness;
extern float ps_r_bloom_blades_threshold;
extern float ps_r_bloom_blades_brightness;

extern float ps_cas_contrast;
extern float ps_cas_sharpening;

extern u32 ps_r_aa;
extern u32 ps_r_aa_quality;

extern float ps_r_fxaa_subpix;
extern float ps_r_fxaa_edge_treshold;
extern float ps_r_fxaa_edge_treshold_min;

extern float ps_r_mblur;

extern fvec3 ps_r_dof;
extern u32 ps_r_dof_quality;

extern float ps_r_ls_depth_scale; // 1.0f
extern float ps_r_ls_depth_bias;	 // -0.0001f
extern float ps_r_ls_squality;	 // 1.0f
extern float ps_r_tsm_bias;
extern float ps_r_ls_far;

extern int ps_r_light_fragments_cull;
extern float ps_r_light_distance_cull;

extern float ps_r_sun_near; // 10.0f
extern float ps_r_sun_far;
extern float ps_r_sun_tsm_bias;		   // 0.0001f
extern float ps_r_sun_depth_far_normal_bias;
extern float ps_r_sun_depth_far_directional_bias;
extern float ps_r_sun_depth_far_bias;
extern float ps_r_sun_depth_middle_normal_bias;
extern float ps_r_sun_depth_middle_directional_bias;
extern float ps_r_sun_depth_middle_bias;
extern float ps_r_sun_depth_near_normal_bias;
extern float ps_r_sun_depth_near_directional_bias;
extern float ps_r_sun_depth_near_bias;
extern float ps_r_sun_depth_normal_bias;
extern float ps_r_sun_depth_directional_bias;
extern float ps_r_sun_lumscale;		   // 0.5f
extern float ps_r_sun_lumscale_hemi;	   // 1.0f
extern float ps_r_sun_lumscale_amb;	   // 1.0f
extern u32 ps_r_sun_shafts_quality;			   //=	0;
extern xr_token qsun_shafts_token[];
extern u32 ps_r_shadow_filtering;

extern float ps_r_zfill; // .1f

extern float ps_r_dhemi_scale; // 1.5f
extern float ps_r_dhemi_sky_scale;
extern int ps_r_dhemi_count;	  // 5

extern float ps_r_slight_fade; // 1.f

extern u32 ps_r_ao_quality;
extern float ps_r_ao_brightness;

extern u32 ps_r_debug_render;

extern u32 ps_r_color_blind_mode;

extern float ps_r_df_parallax_h;
extern u32 ps_r_material_quality;

extern u32 ps_r_aa_transluency;

// R2/R2a/R2.5-specific flags
extern Flags32 ps_r_lighting_flags;
enum
{
	RFLAG_SUN = (1 << 0),
	RFLAG_SUN_DETAILS = (1 << 1),
	RFLAGEXT_SUN_ZCULLING = (1 << 2),
	RFLAG_R1LIGHTS = (1 << 3),
	RFLAG_EXP_DONT_TEST_UNSHADOWED = (1 << 4),
	RFLAG_SUN_SHAFTS = (1 << 5)
};

extern Flags32 ps_r_postprocess_flags;
enum
{
	RFLAG_AUTOEXPOSURE = (1 << 0),
	RFLAG_BLOOM = (1 << 1),
	RFLAG_DOF = (1 << 2),
	RFLAG_MBLUR = (1 << 3),
	RFLAG_CONTRAST_ADAPTIVE_SHARPENING = (1 << 4),
	RFLAG_ANTI_ALIASING = (1 << 5),
	RFLAG_ANTI_ALIASING_ALPHA_TEST = (1 << 6),
	RFLAG_REFLECTIONS = (1 << 7)
};

extern Flags32 ps_r_overlay_flags;
enum
{
	RFLAG_PHOTO_GRID = (1 << 0),
	RFLAG_CINEMA_BORDERS = (1 << 1),
	RFLAG_WATERMARK = (1 << 2)
};

extern Flags32 ps_r_ls_flags;
enum
{
	RFLAG_USE_NVDBT = (1 << 0)
};

extern Flags32 ps_r_debug_flags;
enum
{
	RFLAG_DRAW_SUN_OCCLUDERS = (1 << 0),
	RFLAG_DRAW_HOM_OCCLUDERS = (1 << 1)
};

/*-------------------------------------------------------------------------------*/
// Functions
/*-------------------------------------------------------------------------------*/
extern void xrRender_initconsole();
extern void xrRender_console_apply_conditions();
extern void xrRender_apply_tf();
///////////////////////////////////////////////////////////////////////////////////
#endif
///////////////////////////////////////////////////////////////////////////////////
