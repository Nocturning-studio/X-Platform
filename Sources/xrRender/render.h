#pragma once

#include "SceneGraph.h"
#include "r_occlusion.h"

#include "PSLibrary.h"

#include "r_types.h"
#include "r_rendertarget.h"
#include "r_render_stages.h"

#include "hom.h"
#include "detailmanager.h"
#include "modelpool.h"
#include "wallmarksengine.h"

#include "smap_allocator.h"
#include "light_db.h"
#include "light_render_direct.h"
#include "LightTrack.h"
#include "r_sun_cascades.h"

#include "../xrEngine\irenderable.h"
#include "../xrEngine\fmesh.h"
#include "xrRender_console.h"

#include "SunOccluder.h" 

#include "EffectorsManager.h"

// definition
class CRender : public IRender_interface, public pureFrame
{
  public:
	enum
	{
		PHASE_NORMAL = 0,		// E[0]
		PHASE_SHADOW_DEPTH = 1, // E[1]
		PHASE_DEPTH_PREPASS = 2,
		PHASE_HUD = 3,
		PHASE_SUN_LIGHTING = 4,
		PHASE_SPOT_LIGHTING = 5,
		PHASE_POINT_LIGHTING = 6
	};

	enum
	{
		MODE_NEAR = 0,
		MODE_NORMAL = 1,
		MODE_FAR = 2
	};

  public:
	struct _options
	{
		u32 smapsize : 16;
		u32 nvdbt : 1;
		u32 distortion : 1;
		u32 forceskinw : 1;
		u32 noshadows : 1;

		float sun_depth_near_scale;
		float sun_depth_near_bias;
		float sun_depth_far_scale;
		float sun_depth_far_bias;

		bool use_ssao;
		bool use_atest_aa;
	} o;

	void CheckHWRenderSupporting();
	void update_options();

	struct _stats
	{
		u32 l_total, l_visible;
		u32 l_shadowed, l_unshadowed;
		s32 s_used, s_merged, s_finalclip;
		u32 o_queries, o_culled;
		u32 ic_total, ic_culled;
	} stats;

  public:
	// Sector detection and visibility
	CSector* pLastSector;
	Fvector vLastCameraPos;
	u32 uLastLTRACK;
	xr_vector<IRender_Portal*> Portals;
	xr_vector<IRender_Sector*> Sectors;
	xrXRC Sectors_xrc;
	CDB::MODEL* rmPortals;
	CHOM HOM;
	R_occlusion HWOCC;

	CSceneGraph SceneGraph;

	CSunOccluder* m_SunOccluder;

	// Global vertex-buffer container
	xr_vector<FSlideWindowItem> SWIs;
	xr_vector<ref_shader> Shaders;
	typedef svector<D3DVERTEXELEMENT9, MAXD3DDECLLENGTH + 1> VertexDeclarator;
	xr_vector<VertexDeclarator> nDC, xDC;
	xr_vector<IDirect3DVertexBuffer9*> nVB, xVB;
	xr_vector<IDirect3DIndexBuffer9*> nIB, xIB;
	xr_vector<IRender_Visual*> Visuals;
	CPSLibrary PSLibrary;

	CDetailManager* Details;
	CModelPool* Models;
	CWallmarksEngine* Wallmarks;

	CRenderTarget* RenderTarget;

	CEffectorsManager* EffectorsManager;

	CLight_DB Lights;
	CLight_Compute_XFORM_and_VIS LR;
	xr_vector<light*> Lights_LastFrame;
	SMAP_Allocator LP_smap_pool;
	light_Package LP_normal;
	light_Package LP_pending;

	u32 dwAccumulatorClearMark;
	u32 dwLightMarkerID;

	xr_vector<Fbox3, render_alloc<Fbox3>> main_coarse_structure;

	shared_str c_sbase;
	shared_str c_lmaterial;
	float o_hemi;
	float o_sun;
	IDirect3DQuery9* q_sync_point[2];
	u32 q_sync_count;

	bool m_bFirstFrameAfterReset; // Determines weather the frame is the first after resetting device.

	bool m_b_collect_visuals;

	bool m_need_render_sun;
	xr_vector<sun::cascade> m_sun_cascades;

	//Motion blur
	Fmatrix m_saved_viewproj;
	Fmatrix m_saved_invview;

  private:
	xrCriticalSection resource_lock;

	// Loading / Unloading
	void LoadBuffers(CStreamReader* fs, BOOL _alternative);
	void LoadVisuals(IReader* fs);
	void LoadLights(IReader* fs);
	void LoadSectors(IReader* fs);
	void LoadSWIs(CStreamReader* fs);

  public:
	void RenderScene();
	void RenderDebug();
	void RenderMenu();

  public:
	ShaderElement* rimp_select_sh_static(IRender_Visual* pVisual, float cdist_sq);
	ShaderElement* rimp_select_sh_dynamic(IRender_Visual* pVisual, float cdist_sq);
	D3DVERTEXELEMENT9* getVB_Format(int id, BOOL _alt = FALSE);
	IDirect3DVertexBuffer9* getVB(int id, BOOL _alt = FALSE);
	IDirect3DIndexBuffer9* getIB(int id, BOOL _alt = FALSE);
	FSlideWindowItem* getSWI(int id);
	IRender_Portal* getPortal(int id);
	IRender_Sector* getSectorActive();
	IRender_Visual* model_CreatePE(LPCSTR name);
	IRender_Sector* detectSector(const Fvector& P, Fvector& D);
	int translateSector(IRender_Sector* pSector);

	// HW-occlusion culling
	IC u32 occq_begin(u32& ID)
	{
		return HWOCC.occq_begin(ID);
	}
	IC void occq_end(u32& ID)
	{
		HWOCC.occq_end(ID);
	}
	IC u32 occq_get(u32& ID)
	{
		return HWOCC.occq_get(ID);
	}

	ICF void apply_object(IRenderable* O)
	{
		if (0 == O)
			return;
		if (0 == O->renderable_ROS())
			return;
		CROS_impl& LT = *((CROS_impl*)O->renderable_ROS());
		LT.update_smooth(O);
		CopyMemory(o_hemi_cube, LT.get_hemi_cube(), CROS_impl::NUM_FACES * sizeof(float));
	}

	float o_hemi_cube[CROS_impl::NUM_FACES];
	IC void apply_lmaterial()
	{
		R_constant* C = &*RenderBackend.get_Constant(c_sbase); // get sampler
		if (0 == C)
			return;
		VERIFY(RC_dest_sampler == C->destination);
		VERIFY(RC_sampler == C->type);
		RenderBackend.set_Constant("hemi_cube_pos_faces", o_hemi_cube[CROS_impl::CUBE_FACE_POS_X], o_hemi_cube[CROS_impl::CUBE_FACE_POS_Y], o_hemi_cube[CROS_impl::CUBE_FACE_POS_Z]);
		RenderBackend.set_Constant("hemi_cube_neg_faces", o_hemi_cube[CROS_impl::CUBE_FACE_NEG_X], o_hemi_cube[CROS_impl::CUBE_FACE_NEG_Y], o_hemi_cube[CROS_impl::CUBE_FACE_NEG_Z]);
	}

  public:
	// Loading / Unloading
	virtual void create();
	virtual void destroy();
	virtual void reset_begin();
	virtual void reset_end();

	virtual void level_Load(IReader*);
	virtual void level_Unload();

	virtual IDirect3DBaseTexture9* texture_load(LPCSTR fname, u32& msize);

	/**/
	#pragma todo(Deathman to Deathman: Переписать передачу здоровья в рендер)
	float m_actor_health;
	virtual void set_actor_health(float health)
	{
		m_actor_health = health;
	}
	virtual float get_actor_health()
	{
		return m_actor_health;
	}
	/**/

	// Information
	virtual void Statistics(CGameFont* F);
	virtual LPCSTR getShaderPath()
	{
		return "";
	}
	virtual ref_shader getShader(int id);
	virtual IRender_Sector* getSector(int id);
	virtual IRender_Visual* getVisual(int id);
	virtual IRender_Sector* detectSector(const Fvector& P);
	virtual IRender_Target* getTarget();

	virtual IEffectorsManager* getEffectorsManager();

	// Main
	virtual void flush();
	virtual void set_Object(IRenderable* O);
	virtual void add_Occluder(Fbox2& bb_screenspace); // mask screen region as oclluded
	virtual void add_Visual(IRender_Visual* V);		  // add visual leaf	(no culling performed at all)
	virtual void add_Geometry(IRender_Visual* V);	  // add visual(s)	(all culling performed)

	// wallmarks
	virtual void add_StaticWallmark(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V);
	virtual void clear_static_wallmarks();
	virtual void add_SkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm);
	virtual void add_SkeletonWallmark(const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start,
									  const Fvector& dir, float size);

	//
	virtual IBlender* blender_create(CLASS_ID cls);
	virtual void blender_destroy(IBlender*&);

	//
	virtual IRender_ObjectSpecific* ros_create(IRenderable* parent);
	virtual void ros_destroy(IRender_ObjectSpecific*&);

	// Lighting
	virtual IRender_Light* light_create();
	virtual IRender_Glow* glow_create();

	// Models
	virtual IRender_Visual* model_CreateParticles(LPCSTR name);
	virtual IRender_DetailModel* model_CreateDM(IReader* F);
	virtual IRender_Visual* model_Create(LPCSTR name, IReader* data = 0);
	virtual IRender_Visual* model_CreateChild(LPCSTR name, IReader* data);
	virtual IRender_Visual* model_Duplicate(IRender_Visual* V);
	virtual void model_Delete(IRender_Visual*& V, BOOL bDiscard);
	virtual void model_Delete(IRender_DetailModel*& F);
	virtual void model_Logging(BOOL bEnable)
	{
		Models->Logging(bEnable);
	}
	virtual void models_Prefetch();
	virtual void models_Clear(BOOL b_complete);

	// Occlusion culling
	virtual BOOL occ_visible(vis_data& V);
	virtual BOOL occ_visible(Fbox& B);
	virtual BOOL occ_visible(sPoly& P);

	// Main
	void clear_gbuffer();
	void set_gbuffer();
	void phase_occq();
	void render_wallmarks();
	void render_shadow_map_sun(light* L, u32 sub_phase);
	void render_shadow_map_sun_transluent(light* L, u32 sub_phase);
	void clear_shadow_map_spot();
	void render_shadow_map_spot(light* L);
	void render_shadow_map_spot_transluent(light* L);
	void set_light_accumulator();
	BOOL enable_scissor(light* L); // true if intersects near plane
	void enable_dbt_bounds(light* L);
	BOOL u_DBT_enable(float zMin, float zMax);
	void u_DBT_disable();
	float hclip(float v, float dim);
	void draw_volume(light* L);
	void accumulate_sun(u32 sub_phase, Fmatrix& xform, Fmatrix& xform_prev, float fBias); //, float fSize);
	void accumulate_volumetric_sun(u32 sub_phase, Fmatrix m_shadow, Fvector L_dir);
	void accumulate_point_lights(light* L);
	void accumulate_spot_lights(light* L);
	void clear_bloom();
	void calculate_bloom();
	void apply_bloom();
	void render_bloom();
	void downsample_scene_luminance();
	void prepare_scene_luminance();
	void save_scene_luminance();
	void apply_exposure();
	void render_autoexposure();
	void combine_additional_postprocess();
	void combine_sun_shafts();
	void render_skybox();
	void precombine_scene();
	void combine_scene_lighting();
	void clear_reflections();
	void create_backbuffer_mip_chain();
	void render_reflections();
	void render_screen_space_reflections();
	void render_screen_overlays();
	void render_antialiasing();
	void create_distortion_mask();
	void render_distortion();
	void render_depth_of_field();
	void motion_blur_pass_prepare_dilation_map();
	void motion_blur_pass_blur();
	void motion_blur_pass_save_depth();
	void render_motion_blur();
	void render_effectors_pass_generate_radiation_noise();
	void render_effectors_pass_color_blind_filter();
	void render_effectors_pass_lut();
	void render_effectors_pass_combine();
	void render_effectors_pass_resolve_gamma();
	void output_frame_to_screen();
	bool need_render_sun();
	void check_distort();
	void render_main(Fmatrix& mCombined, bool _fportals);
	void query_wait();
	void render_lights(light_Package& LP);
	void ProcessRemainingLightsOptimized(light_Package& LP);
	void render_sun_cascade(u32 cascade_ind);
	void init_cacades();
	void render_sun_cascades();
	void render_hom();
	void render_ambient_occlusion();
	void combine_scene();
	void render_depth_prepass();
	void render_gbuffer_primary();
	void render_gbuffer_secondary();
	void render_forward_lights(xr_vector<light*>& lights, int phase);
	void render_stage_occlusion_culling();
	void render_stage_forward();
	void update_shadow_map_visibility();
	void render_sun();
	void render_lights();
	void create_hi_z_mip_chain();
	void render_postprocess();
	void render_stage_main_geometry();
	void render_bent_normals();

	virtual void Calculate();
	void PrepareToRender();
	virtual void Render();
	virtual void Screenshot(ScreenshotMode mode = SM_NORMAL, LPCSTR name = 0);
	virtual void OnFrame();

	virtual void set_Transform(Fmatrix* M)
	{
		SceneGraph.set_Transform(M);
	}

	virtual void set_HUD(BOOL V)
	{
		SceneGraph.set_HUD(V);
	}
	virtual BOOL get_HUD()
	{
		return SceneGraph.get_HUD();
	}
	virtual void set_Invisible(BOOL V)
	{
		SceneGraph.set_Invisible(V);
	}

	virtual u32 memory_usage()
	{
#ifdef USE_DOUG_LEA_ALLOCATOR_FOR_RENDER
		return ((u32)dlmallinfo().uordblks);
#else
		return (0);
#endif
	}

	// Render mode
	virtual void set_render_mode(int mode);

	virtual bool is_dynamic_sun_enabled();

	u32 render_phase;

	// KD: need to know, what R2 phase is active now
	virtual u32 active_phase()
	{
		return render_phase;
	};

	virtual void set_active_phase(int active_phase)
	{
		render_phase = active_phase;
	};

	// Constructor/destructor/loader
	CRender();
	virtual ~CRender();

	CShaderMacros FetchShaderMacros();

	HMODULE hCompiler;
  private:
	FS_FileSet m_file_set;
};

extern CRender RenderImplementation;
