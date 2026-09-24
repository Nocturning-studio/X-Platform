////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "xrEngine\irender.h"
#include "xrEngine\irenderable.h"
#include "xrEngine\fmesh.h"

#include "xrRender_console.h"

#include "GPUOcclusion.h"
#include "CPUOcclusion.h"

#include "PSLibrary.h"

#include "r_types.h"
#include "r_rendertarget.h"

#include "modelpool.h"
#include "wallmarksengine.h"

#include "smap_allocator.h"
#include "light_db.h"
#include "light_render_direct.h"
#include "LightTrack.h"
#include "r_sun_cascades.h"

#include "EffectorsManager.h"
#include "SceneGraph.h"

#include "RenderScene.h"
////////////////////////////////////////////////////////////////////////////////
class CGlow : public IRender_Glow
{
public:
	bool bActive;

public:
	CGlow() : bActive(false)
	{
	}
	virtual void set_active(bool b)
	{
		bActive = b;
	}
	virtual bool get_active()
	{
		return bActive;
	}
	virtual void set_position(const fvec3& P)
	{
	}
	virtual void set_direction(const fvec3& D)
	{
	}
	virtual void set_radius(float R)
	{
	}
	virtual void set_texture(LPCSTR name)
	{
	}
	virtual void set_color(const Fcolor& C)
	{
	}
	virtual void set_color(float r, float g, float b)
	{
	}
};
////////////////////////////////////////////////////////////////////////////////
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
		u32 distortion : 1;
		u32 forceskinw : 1;
		u32 noshadows : 1;
	} o;

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
	CRenderScene Scene;

	CSector* pLastSector;
	fvec3 vLastCameraPos;
	u32 uLastLTRACK;
	xr_vector<IRender_Portal*> Portals;
	xr_vector<IRender_Sector*> Sectors;
	CDB::MODEL* rmPortals;

	GPUOcclusion HWOCC;
	CPUOcclusion CPUOCC;

	// Global vertex-buffer container
	xr_vector<FSlideWindowItem> SWIs;
	xr_vector<ref_shader> Shaders;
	typedef svector<D3DVERTEXELEMENT9, MAXD3DDECLLENGTH + 1> VertexDeclarator;
	xr_vector<VertexDeclarator> nDC, xDC;
	xr_vector<IDirect3DVertexBuffer9*> nVB, xVB;
	xr_vector<IDirect3DIndexBuffer9*> nIB, xIB;
	xr_vector<IRender_Visual*> Visuals;
	CPSLibrary PSLibrary;

	CModelPool* Models;

	CRenderTarget* RenderTarget;

	CEffectorsManager* EffectorsManager;

	CLight_Compute_Transform_and_VIS LR;

	u32 dwAccumulatorClearMark;
	u32 dwLightMarkerID;

	xr_vector<Fbox3, render_alloc<Fbox3>> main_coarse_structure;

	float o_hemi;
	float o_sun;

	bool m_bFirstFrameAfterReset;

	bool m_b_collect_visuals;

	bool m_need_render_sun;
	xr_vector<Sun::Cascade> m_sun_cascades;
	SunCascadeBuffer m_sun_cascades_buffer[2];
	u32 m_sun_write_ix;
	u32 m_sun_read_ix;
	std::atomic<bool> m_sun_gather_done{true};
	std::condition_variable m_sun_gather_cv;
	std::mutex m_sun_gather_mutex;
	IC SunCascadeBuffer& GetSunWriteBuffer()
	{
		return m_sun_cascades_buffer[m_sun_write_ix];
	}
	IC SunCascadeBuffer& GetSunReadBuffer()
	{
		return m_sun_cascades_buffer[m_sun_read_ix];
	}

	SSceneVisibilityResult m_scene_visibility_data;
	SSceneVisibilityResult m_spot_shadow_vis;

	// Motion blur
	fmat4x4 m_saved_viewproj;
	fmat4x4 m_saved_invview;

  private:
	// Loading / Unloading
	void LoadBuffers(CStreamReader* fs, BOOL _alternative);
	void LoadVisuals(IReader* fs);
	void LoadLights(IReader* fs);
	void LoadSectors(IReader* fs);
	void LoadSWIs(CStreamReader* fs);

  public:
	void RenderScene();
	void RenderMenu();

  public:
	D3DVERTEXELEMENT9* getVB_Format(int id, BOOL _alt = FALSE);
	IDirect3DVertexBuffer9* getVB(int id, BOOL _alt = FALSE);
	IDirect3DIndexBuffer9* getIB(int id, BOOL _alt = FALSE);
	FSlideWindowItem* getSWI(int id);
	IRender_Portal* getPortal(int id);
	IRender_Sector* getSectorActive();
	IRender_Visual* model_CreatePE(LPCSTR name);
	IRender_Sector* detectSector(const fvec3& P, fvec3& D);
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
	IC u32 occq_get(u32& ID, bool wait = true)
	{
		return HWOCC.occq_get(ID, wait);
	}

	CROS_impl::AOCube compute_object_ao_cube(IRenderable* O)
	{
		CROS_impl::AOCube cube{};
		if(nullptr == O)
			return cube;

		IRender_ObjectSpecific* ros = O->renderable_ROS();
		if(nullptr == ros)
			return cube;

		CROS_impl& LT = *static_cast<CROS_impl*>(ros);
		LT.update_smooth(O);
		CopyMemory(cube.data(), LT.get_ao_cube(), CROS_impl::NUM_FACES * sizeof(float));
		return cube;
	}

	IC void apply_ao_lighting(const CROS_impl::AOCube& cube)
	{
		RenderBackend.SetConstant("ao_cube_pos_faces",
								   cube[CROS_impl::CUBE_FACE_POS_X],
								   cube[CROS_impl::CUBE_FACE_POS_Y],
								   cube[CROS_impl::CUBE_FACE_POS_Z]);
		RenderBackend.SetConstant("ao_cube_neg_faces",
								   cube[CROS_impl::CUBE_FACE_NEG_X],
								   cube[CROS_impl::CUBE_FACE_NEG_Y],
								   cube[CROS_impl::CUBE_FACE_NEG_Z]);
	}

  public:
	// Loading / Unloading
	virtual void Initialize() override;
	virtual void Create() override;
	virtual void Destroy() override;
	virtual void ResetBegin() override;
	virtual void ResetEnd() override;

	virtual void LevelLoad(IReader*) override;
	virtual void LevelUnload() override;

	void WaitForPendingTasks();

	virtual IDirect3DBaseTexture9* TextureLoad(LPCSTR fname, u32& msize) override;

	/**/
#pragma todo(Deathman to Deathman : Rewrite)
	float m_actor_health;
	virtual void set_actor_health(float health) { m_actor_health = health; }
	virtual float get_actor_health() { return m_actor_health; }
	/**/

	// Information
	virtual void Statistics(CGameFont* F) override;
	virtual LPCSTR getShaderPath() { return ""; }
	virtual ref_shader getShader(int id) override;
	virtual IRender_Sector* getSector(int id) override;
	virtual IRender_Visual* getVisual(int id) override;
	virtual IRender_Sector* detectSector(const fvec3& P) override;
	virtual IRender_Target* getTarget() override;

	virtual IEffectorsManager* getEffectorsManager() override;

	// Main
#pragma fixme(Occluders) 
	virtual void add_Occluder(Fbox2& bb_screenspace) override {};						// mask screen region as oclluded
	virtual void add_Visual(IRender_Visual* V) override { Scene.AddVisual(V); };		// add visual leaf	(no culling performed at all)
	virtual void add_Geometry(IRender_Visual* V) override { Scene.AddGeometry(V); };	// add visual(s)	(all culling performed)

	virtual void set_Transform(fmat4x4* M) override { Scene.SetTransform(M); };
	virtual void set_HUD(BOOL V) override { Scene.SetHUD(V); };
	virtual BOOL get_HUD() override { return Scene.GetHUD(); };
	virtual void set_Invisible(BOOL V) override { Scene.SetInvisible(V); };
	virtual void set_Frustum(CFrustum* O) override { Scene.SetFrustum(O); };
	virtual const CFrustum* get_Frustum() override { return Scene.GetFrustum(); };
	virtual void set_Object(IRenderable* O) override { Scene.SetObject(O); };

	// wallmarks
	virtual void add_StaticWallmark(ref_shader& S, const fvec3& P, float s, CDB::TRI* T, fvec3* V) override { Scene.AddStaticWallmark(S, P, s, T, V); };
	virtual void clear_static_wallmarks() override { Scene.ClearStaticWallmarks(); };
	virtual void add_SkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm) override { Scene.AddSkeletonWallmark(wm); };;
	virtual void add_SkeletonWallmark(const fmat4x4* xf, CKinematics* obj, ref_shader& sh, const fvec3& start, const fvec3& dir, float size) override { Scene.AddSkeletonWallmark(xf, obj, sh, start, dir, size); };

	virtual IBlender* blender_create(CLASS_ID cls) override;
	virtual void blender_destroy(IBlender*&) override;

	virtual IRender_ObjectSpecific* ros_create(IRenderable* parent) override;
	virtual void ros_destroy(IRender_ObjectSpecific*&) override;

	// Lighting
	virtual IRender_Light* light_create() override { return Scene.CreateLight(); };
	virtual IRender_Glow* glow_create() override { return xr_new<CGlow>(); };

	// Models
	virtual IRender_Visual* model_CreateParticles(LPCSTR name) override;
	virtual IRender_DetailModel* model_CreateDM(IReader* F) override;
	virtual IRender_Visual* model_Create(LPCSTR name, IReader* data = 0) override;
	virtual IRender_Visual* model_CreateChild(LPCSTR name, IReader* data) override;
	virtual IRender_Visual* model_Duplicate(IRender_Visual* V) override;
	virtual void model_Delete(IRender_Visual*& V, BOOL bDiscard) override;
	virtual void model_Delete(IRender_DetailModel*& F) override;
	virtual void model_Logging(BOOL bEnable) { Models->Logging(bEnable); }
	virtual void models_Prefetch() override;
	virtual void models_Clear(BOOL b_complete) override;

	// Occlusion culling
	virtual BOOL occ_visible(vis_data& V) override { return Scene.IsVisible(V); };
	virtual BOOL occ_visible(Fbox& B) override { return Scene.IsVisible(B); };
	virtual BOOL occ_visible(sPoly& P) override { return Scene.IsVisible(P); };

	// Main
	void clear_gbuffer();
	void set_gbuffer();
	void render_wallmarks();
	void render_shadow_map_sun(light* L, u32 sub_phase);
	void clear_shadow_map_spot();
	void render_shadow_map_spot(light* L);
	void set_light_accumulator();
	BOOL enable_scissor(light* L); // true if intersects near plane
	float hclip(float v, float dim);
	void draw_volume(light* L);
	void accumulate_sun(u32 sub_phase, fmat4x4& transform, fmat4x4& transform_prev); // , float fBias); //, float fSize);
	void accumulate_volumetric_sun(u32 sub_phase, fmat4x4 m_shadow, fvec3 L_dir);
	void accumulate_point_lights(light* L);
	void accumulate_spot_lights(light* L);
	void clear_bloom();
	void calculate_bloom();
	void apply_bloom();
	void render_bloom();
	void downsample_scene_luminance();
	void prepare_scene_luminance();
	void swap_luminance();
	void apply_exposure();
	void dummy_exposure();
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
	void update_light_tracking(SceneGraphPacket& packet);
	void MergeCulledLights(SceneGraphPacket& packet);
	void calculate_scene_culling();
	void render_lights(light_Package& LP);
	void ProcessRemainingLights(light_Package& LP);
	void init_cacades();
	void __stdcall schedule_cascades();
	void wait_for_sun_task();
	void swap_sun_buffers();
	bool need_render_sun();
	void prepare_sun_cascade(u32 cascade_ind, ShadowCascadeWorkItem& item);
	void gather_scene_for_cascade(u32 cascade_ind, ShadowCascadeWorkItem& item);
	void draw_sun_cascade(u32 cascade_ind, ShadowCascadeWorkItem& item);
	void render_sun_cascades();
	void render_ambient_occlusion();
	void render_gbuffer();
	void render_stage_lights_culling();
	void update_shadow_map_visibility();
	void render_stage_forward();
	void render_scene_to_gbuffer();
	void render_sun();
	void render_lights();
	void create_hi_z_mip_chain();
	void render_postprocess();
	void render_bent_normals();

	virtual void Calculate() override;
	void prepare_to_render();
	virtual void Render() override;
	virtual void Screenshot(ScreenshotMode mode = ScreenshotMode::SM_NORMAL, LPCSTR name = 0) override;
	virtual void OnFrame() override;

	virtual u32 memory_usage()
	{
#ifdef USE_DOUG_LEA_ALLOCATOR_FOR_RENDER
		return ((u32)dlmallinfo().uordblks);
#else
		return (0);
#endif
	}

	// Render mode
	virtual void set_render_mode(int mode) override;

	virtual bool is_dynamic_sun_enabled() override;

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
};
////////////////////////////////////////////////////////////////////////////////
extern CRender RenderImplementation;
////////////////////////////////////////////////////////////////////////////////
