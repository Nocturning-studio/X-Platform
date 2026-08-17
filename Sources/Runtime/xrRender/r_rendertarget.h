#pragma once

class CRenderTarget : public IRender_Target
{
  private:
	u32 dwWidth;
	u32 dwHeight;

  public:
	IBlender* b_occq;
	IBlender* b_accum_mask;
	IBlender* b_accum_direct_cascade;
	IBlender* b_accum_point;
	IBlender* b_accum_spot;
	IBlender* b_bloom;
	IBlender* b_distortion;
	IBlender* b_ambient_occlusion;
	IBlender* b_autoexposure;
	IBlender* b_combine;
	IBlender* b_antialiasing;
	IBlender* b_dof;
	IBlender* b_motion_blur;
	IBlender* b_frame_overlay;
	IBlender* b_reflections;
	IBlender* b_effectors;
	IBlender* b_output_to_screen;
	IBlender* b_bent_normals;
	ref_shader s_bent_normals;

	IBlender* b_hi_z;
	ref_shader s_hi_z;

#ifdef DEBUG
	struct dbg_line_t
	{
		fvec3 P0, P1;
		u32 color;
	};
	xr_vector<std::pair<Fsphere, Fcolor>> dbg_spheres;
	xr_vector<dbg_line_t> dbg_lines;
	xr_vector<Fplane> dbg_planes;
#endif

	// depth-stencil buffer
	ref_rt rt_ZB;

	// Geometry Buffer
	ref_rt rt_GBuffer[4];

	ref_rt rt_Hi_z;

	ref_rt rt_Bent_Normals;
	ref_rt rt_Volumetric_Sun;

	ref_rt rt_dof_coc;
	ref_rt rt_dof_near;
	ref_rt rt_dof_far;
	ref_rt rt_dof_dilation; // Low-Res Tile Map (R16F)

	// Accumulation Buffer
	ref_rt rt_Light_Accumulator;

	ref_rt rt_Generic[2];

	ref_rt rt_Distortion_Mask;

	ref_rt rt_Bloom[2];
	ref_rt rt_Bloom_Blades[2];
	ref_rt rt_LUM_Mip_Chain;
	ref_rt rt_SceneLuminance;
	ref_rt rt_SceneLuminancePrevious;

	ref_rt rt_BackbufferMip;
	ref_rt rt_Reflections;

	ref_rt rt_Radiation_Noise[3];

	ref_rt rt_Generic_Prev;

	// ao
	ref_rt rt_ao_raw;
	ref_rt rt_ao;

	// env
	ref_texture t_irradiance_map_0; // env-0
	ref_texture t_irradiance_map_1; // env-1

	ref_texture t_LUT_0; // lut-0
	ref_texture t_LUT_1; // lut-1

	ref_rt rt_Motion_Blur_Previous_Frame_Depth;
	ref_rt rt_Motion_Blur_Dilation_Map_0;
	ref_rt rt_Motion_Blur_Dilation_Map_1;

	// smap
	ref_rt rt_smap_surf;		   // 32bit,		color
	ref_rt rt_smap_depth;		   // 24(32) bit,	depth
	IDirect3DSurface9* rt_smap_ZB; //

	IDirect3DTexture9* t_noise_surf[TEX_jitter_count];
	ref_texture t_noise[TEX_jitter_count];

	IDirect3DSurface9* surf_screenshot_normal; // HW.fTarget, SM_NORMAL
	IDirect3DTexture9* tex_screenshot_gamesave; // Container of "surf_screenshot_gamesave"
	IDirect3DSurface9* surf_screenshot_gamesave; // DXT1, SM_FOR_GAMESAVE
	
  public:
	// OCCq
	ref_shader s_occq;

	// Accum
	ref_shader s_accum_mask;
	ref_shader s_accum_direct_cascade;
	ref_shader s_accum_point;
	ref_shader s_accum_spot;

	ref_geom g_accum_point;
	ref_geom g_accum_spot;
	ref_geom g_accum_omnipart;

	IDirect3DVertexBuffer9* g_accum_point_vb;
	IDirect3DIndexBuffer9* g_accum_point_ib;

	IDirect3DVertexBuffer9* g_accum_omnip_vb;
	IDirect3DIndexBuffer9* g_accum_omnip_ib;

	IDirect3DVertexBuffer9* g_accum_spot_vb;
	IDirect3DIndexBuffer9* g_accum_spot_ib;

	// Bloom
	ref_shader s_bloom;

	// AO
	ref_shader s_ambient_occlusion;

	// AA
	ref_shader s_aa;

	// Luminance
	ref_shader s_autoexposure;
	float f_autoexposure_adapt;

	ref_geom g_cuboid;

	ref_shader s_combine;
	ref_shader s_contrast_adaptive_sharpening;
	ref_shader s_antialiasing;
	ref_shader s_barrel_blur;
	ref_shader s_reflections;
	ref_shader s_dof;
	ref_shader s_distortion;
	ref_shader s_motion_blur;
	ref_shader s_frame_overlay;
	ref_shader s_tonemapping;
	ref_shader s_fog_scattering;
	ref_shader s_output_to_screen;

	ref_shader s_effectors;
	ref_geom g_effectors;
	ref_shader s_menu_distortion;
	ref_shader s_menu_gamma;

  public:
	CRenderTarget();
	~CRenderTarget();

	void create_textures();
	void create_blenders();
	void delete_blenders();
	void delete_textures();
 
	void CompileShaders();

	void accum_point_geom_create();
	void accum_point_geom_destroy();
	void accum_omnip_geom_create();
	void accum_omnip_geom_destroy();
	void accum_spot_geom_create();
	void accum_spot_geom_destroy();

	void u_calc_tc_duality_ss(fvec2& r0, fvec2& r1, fvec2& l0, fvec2& l1);

	virtual u32 get_width()
	{
		return dwWidth;
	}
	virtual u32 get_height()
	{
		return dwHeight;
	}
};
