#include "stdafx.h"
#include "render.h"
#include "..\xrEngine\fbasicvisual.h"
#include "..\xrEngine\xr_object.h"
#include "..\xrEngine\CustomHUD.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
#include "..\xrEngine\SkeletonCustom.h"
#include "LightTrack.h"
#include <boost/crc.hpp>
#include "../xrEngine\r_constants.h"
//////////////////////////////////////////////////////////////////////////
CRender RenderImplementation;
//////////////////////////////////////////////////////////////////////////
#pragma todo(NSDeathman to NSDeathman: Добавить поддержку Glow)
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
	virtual void set_position(const Fvector& P)
	{
	}
	virtual void set_direction(const Fvector& D)
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

float r_dtex_range = 50.f;
//////////////////////////////////////////////////////////////////////////
ShaderElement* CRender::rimp_select_sh_static(IRender_Visual* pVisual, float cdist_sq)
{
	if (!pVisual)
	{
		return 0;
	}

	if (!pVisual->shader._get())
	{
		return 0;
	}

	int id = SE_R1_NORMAL_HQ;

	switch (active_phase())
	{
	case CRender::PHASE_HUD:	// HUD Forward Base Pass
	case CRender::PHASE_NORMAL: // Forward Base Pass
		id = ((_sqrt(cdist_sq) - pVisual->vis.sphere.R) < r_dtex_range) ? SE_R1_NORMAL_HQ : SE_R1_NORMAL_LQ;
		break;
	case CRender::PHASE_POINT_LIGHTING: // Additive Point Light Pass
		id = SE_R1_LPOINT;
		break;
	case CRender::PHASE_SPOT_LIGHTING: // Additive Spot Light Pass
		id = SE_R1_LSPOT;
		break;
	case CRender::PHASE_SUN_LIGHTING: // Additive Sun Light Pass
		id = SE_R1_LSUN;
		break;
	case CRender::PHASE_SHADOW_DEPTH:
		id = SE_SHADOW_DEPTH;
		break;
	case CRender::PHASE_DEPTH_PREPASS:
		id = SE_DEPTH_PREPASS;
		break;
	}

	return pVisual->shader->E[id]._get();
}

ShaderElement* CRender::rimp_select_sh_dynamic(IRender_Visual* pVisual, float cdist_sq)
{
	if (!pVisual)
	{
		return 0;
	}

	if (!pVisual->shader._get())
	{
		return 0;
	}

	int id = SE_R1_NORMAL_HQ;

	switch (active_phase())
	{
	case CRender::PHASE_HUD:	// HUD Forward Base Pass
	case CRender::PHASE_NORMAL: // Forward Base Pass
		id = ((_sqrt(cdist_sq) - pVisual->vis.sphere.R) < r_dtex_range) ? SE_R1_NORMAL_HQ : SE_R1_NORMAL_LQ;
		break;
	case CRender::PHASE_POINT_LIGHTING: // Additive Point Light Pass
		id = SE_R1_LPOINT;
		break;
	case CRender::PHASE_SPOT_LIGHTING: // Additive Spot Light Pass
		id = SE_R1_LSPOT;
		break;
	case CRender::PHASE_SUN_LIGHTING: // Additive Sun Light Pass
		id = SE_R1_LSUN;
		break;
	case CRender::PHASE_SHADOW_DEPTH:
		id = SE_SHADOW_DEPTH;
		break;
	case CRender::PHASE_DEPTH_PREPASS:
		id = SE_DEPTH_PREPASS;
		break;
	}

	return pVisual->shader->E[id]._get();
}
//////////////////////////////////////////////////////////////////////////
static class cl_parallax : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		float h = ps_r_df_parallax_h;
		RenderBackend.set_Constant(C, h, -(h / 2.0f), 1.f / r_dtex_range, 1.f / r_dtex_range);
	}
} binder_parallax;
//////////////////////////////////////////////////////////////////////////
static class cl_sun_far : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		float fValue = ps_r_sun_far;
		RenderBackend.set_Constant(C, fValue, fValue, fValue, 0);
	}
} binder_sun_far;
//////////////////////////////////////////////////////////////////////////
static class cl_sun_dir : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		light* sun = (light*)RenderImplementation.Lights.sun_adapted._get();

		Fvector L_dir;
		Device.mView.transform_dir(L_dir, sun->get_direction());
		L_dir.normalize();

		RenderBackend.set_Constant(C, L_dir.x, L_dir.y, L_dir.z, 0);
	}
} binder_sun_dir;
//////////////////////////////////////////////////////////////////////////
static class cl_sun_normal_bias : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RenderBackend.set_Constant(C, ps_r_sun_depth_normal_bias, 0, 0, 0);
	}
} binder_sun_normal_bias;
//////////////////////////////////////////////////////////////////////////
static class cl_sun_directional_bias : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RenderBackend.set_Constant(C, ps_r_sun_depth_directional_bias, 0, 0, 0);
	}
} binder_sun_directional_bias;
//////////////////////////////////////////////////////////////////////////
static class cl_sun_color : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		light* sun = (light*)RenderImplementation.Lights.sun_adapted._get();
		RenderBackend.set_Constant(C, sRgbToLinear(sun->get_color().r), sRgbToLinear(sun->get_color().g), sRgbToLinear(sun->get_color().b), 0);
	}
} binder_sun_color;
//////////////////////////////////////////////////////////////////////////
static class cl_debug_reserved : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RenderBackend.set_Constant("debug_reserved", ps_r_debug_reserved_0, ps_r_debug_reserved_1, ps_r_debug_reserved_2, ps_r_debug_reserved_3);
	}
} binder_debug_reserved;
//////////////////////////////////////////////////////////////////////////
static class cl_ao_brightness : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RenderBackend.set_Constant("ao_brightness", ps_r_ao_brightness);
	}
} binder_ao_brightness;
//////////////////////////////////////////////////////////////////////////
static class cl_is_hud_render_phase : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		int is_hud_render_phase = 0;

		if (RenderImplementation.active_phase() == CRender::PHASE_HUD)
			is_hud_render_phase = 1;

		RenderBackend.set_Constant("is_hud_render_phase", (float)is_hud_render_phase, 0, 0, 0);
	}
} binder_is_hud_render_phase;
//////////////////////////////////////////////////////////////////////////
void CRender::CheckHWRenderSupporting()
{
	R_ASSERT2(CAP_VERSION(HW.Caps.raster_major, HW.Caps.raster_minor) >= CAP_VERSION(3, 0),
			  make_string("Your graphics accelerator don`t meet minimal mod system requirements (DX9.0c supporting)"));

	R_ASSERT2(HW.Caps.raster.dwInstructions >= 512, 
		make_string("Your graphics accelerator don`t meet minimal mod system requirements (Instructions count less than 512)"));

	R_ASSERT2(HW.Caps.raster.dwMRT_count >= 3,
		make_string("Your graphics accelerator don`t meet minimal mod system requirements (Multiple render targets)"));

	R_ASSERT2(HW.Caps.raster.b_MRT_mixdepth, 
		make_string("Your graphics accelerator don`t meet minimal mod system requirements (Multiple render targets independent depths)"));

	R_ASSERT2(HW.support(D3DFMT_D24X8, D3DRTYPE_TEXTURE, D3DUSAGE_DEPTHSTENCIL), 
		make_string("Your graphics accelerator don`t meet minimal mod system requirements (D24X8 rendertarget format)"));

	R_ASSERT2(HW.support(D3DFMT_A16B16G16R16F, D3DRTYPE_TEXTURE, D3DUSAGE_QUERY_FILTER),
			  make_string("Your graphics accelerator don`t meet minimal mod system requirements (Floating point 16-bits rendertarget format)"));

	R_ASSERT2(HW.support(D3DFMT_A16B16G16R16F, D3DRTYPE_TEXTURE, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING),
			  make_string("Your graphics accelerator don`t meet minimal mod system requirements (Post-Pixel Shader blending)"));
}
//////////////////////////////////////////////////////////////////////////
// update with vid_restart
void CRender::update_options()
{
	o.smapsize = 1024;

	o.nvdbt = HW.support((D3DFORMAT)MAKEFOURCC('N', 'V', 'D', 'B'), D3DRTYPE_SURFACE, 0);
	if (o.nvdbt)
		Msg("- Nvidia Depth Bounds supported");
}

//////////////////////////////////////////////////////////////////////
CShaderMacros CRender::FetchShaderMacros()
{
	CShaderMacros macros;

	macros.add(m_skinning < 0, "SKIN_NONE", "1");
	macros.add(0 == m_skinning, "SKIN_0", "1");
	macros.add(1 == m_skinning, "SKIN_1", "1");
	macros.add(2 == m_skinning, "SKIN_2", "1");
	macros.add(3 == m_skinning, "SKIN_3", "1");
	macros.add(4 == m_skinning, "SKIN_4", "1");

	macros.add(o.forceskinw, "SKIN_COLOR", "1");

	macros.add(ps_r_shading_mode == 1, "ENABLE_PBR", "1");

	return macros;
}
//////////////////////////////////////////////////////////////////////////
void CRender::create()
{
	Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 0x12345678);

	CheckHWRenderSupporting();
	xrRender_console_apply_conditions();

	m_skinning = -1;

	// options
	o.noshadows = (strstr(Core.Params, "-noshadows")) ? TRUE : FALSE;
	o.forceskinw = (strstr(Core.Params, "-skinw")) ? TRUE : FALSE;

	// constants
	::Device.Resources->RegisterConstantSetup("parallax_heigt", &binder_parallax);
	::Device.Resources->RegisterConstantSetup("sun_far", &binder_sun_far);
	::Device.Resources->RegisterConstantSetup("sun_dir", &binder_sun_dir);
	::Device.Resources->RegisterConstantSetup("sun_normal_bias", &binder_sun_normal_bias);
	::Device.Resources->RegisterConstantSetup("sun_directional_bias", &binder_sun_directional_bias);
	::Device.Resources->RegisterConstantSetup("sun_color", &binder_sun_color);
	::Device.Resources->RegisterConstantSetup("debug_reserved", &binder_debug_reserved);
	::Device.Resources->RegisterConstantSetup("ao_brightness", &binder_ao_brightness);
	::Device.Resources->RegisterConstantSetup("is_hud_render_phase", &binder_is_hud_render_phase);

	c_lmaterial = "L_material";
	c_sbase = "s_base";

	update_options();
	RenderTarget = xr_new<CRenderTarget>();

	EffectorsManager = xr_new<CEffectorsManager>();

	Models = xr_new<CModelPool>();
	PSLibrary.OnCreate();
	HWOCC.occq_create(occq_size);

	SceneGraph.marker = 0;
	R_CHK(HW.pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &q_sync_point[0]));
	R_CHK(HW.pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &q_sync_point[1]));

	xrRender_apply_tf();
	::PortalTraverser.initialize();
}

void CRender::destroy()
{
	::PortalTraverser.destroy();
	_RELEASE(q_sync_point[1]);
	_RELEASE(q_sync_point[0]);
	HWOCC.occq_destroy();
	xr_delete(Models);
	xr_delete(RenderTarget);
	PSLibrary.OnDestroy();
	Device.seqFrame.Remove(this);
	SceneGraph.destroy();
	xr_delete(EffectorsManager);
}

void CRender::reset_begin()
{
	// Update incremental shadowmap-visibility solver
	// BUG-ID: 10646
	{
		u32 it = 0;
		for (it = 0; it < Lights_LastFrame.size(); it++)
		{
			if (0 == Lights_LastFrame[it])
				continue;
			try
			{
				Lights_LastFrame[it]->get_smapvis().resetoccq();
			}
			catch (...)
			{
				Msg("! Failed to flush-OCCq on light [%d] %X", it, *(u32*)(&Lights_LastFrame[it]));
			}
		}
		Lights_LastFrame.clear();
	}

	xr_delete(RenderTarget);
	HWOCC.occq_destroy();
	_RELEASE(q_sync_point[1]);
	_RELEASE(q_sync_point[0]);
}

void CRender::reset_end()
{
	R_CHK(HW.pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &q_sync_point[0]));
	R_CHK(HW.pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &q_sync_point[1]));
	HWOCC.occq_create(occq_size);

	update_options();
	RenderTarget = xr_new<CRenderTarget>();
	RenderTarget->CompileShaders();
	dwAccumulatorClearMark = 0;

	xrRender_apply_tf();

	// Set this flag true to skip the first render frame,
	// that some data is not ready in the first frame (for example device camera position)
	m_bFirstFrameAfterReset = true;
}

void CRender::OnFrame()
{
	PROFILE_FUNCTION();

	Models->DeleteQueue();

	if (Details && Details->dtFS)
	{
		// 1. Подготовка: Своп буферов (новые данные -> в рендер, старые -> на перезапись)
		// и захват позиции камеры.
		Details->PrepareToCalc();

		// 2. Запуск задачи в параллель.
		// Details->MT_CALC() теперь работает с "теневым" буфером и не мешает рендеру.
		Engine.ThreadManager.AddParallelTask(fastdelegate::FastDelegate0<>(Details, &CDetailManager::MT_CALC));
	}

	// MT-HOM (@front)
	Engine.ThreadManager.AddParallelTask(fastdelegate::FastDelegate0<>(&HOM, &CHOM::MT_RENDER));
}

// Implementation
IRender_ObjectSpecific* CRender::ros_create(IRenderable* parent)
{
	return xr_new<CROS_impl>();
}

void CRender::ros_destroy(IRender_ObjectSpecific*& p)
{
	xr_delete(p);
}

IRender_Visual* CRender::model_Create(LPCSTR name, IReader* data)
{
	return Models->Create(name, data);
}

IRender_Visual* CRender::model_CreateChild(LPCSTR name, IReader* data)
{
	return Models->CreateChild(name, data);
}

IRender_Visual* CRender::model_Duplicate(IRender_Visual* V)
{
	return Models->Instance_Duplicate(V);
}

void CRender::model_Delete(IRender_Visual*& V, BOOL bDiscard)
{
	Models->Delete(V, bDiscard);
}

IRender_DetailModel* CRender::model_CreateDM(IReader* F)
{
	CDetail* D = xr_new<CDetail>();
	D->Load(F);
	return D;
}

void CRender::model_Delete(IRender_DetailModel*& F)
{
	if (F)
	{
		CDetail* D = (CDetail*)F;
		D->Unload();
		xr_delete(D);
		F = NULL;
	}
}

IRender_Visual* CRender::model_CreatePE(LPCSTR name)
{
	PS::CPEDef* SE = PSLibrary.FindPED(name);
	R_ASSERT3(SE, "Particle effect doesn't exist", name);
	return Models->CreatePE(SE);
}

IRender_Visual* CRender::model_CreateParticles(LPCSTR name)
{
	PS::CPEDef* SE = PSLibrary.FindPED(name);
	if (SE)
		return Models->CreatePE(SE);
	else
	{
		PS::CPGDef* SG = PSLibrary.FindPGD(name);
		R_ASSERT3(SG, "Particle effect or group doesn't exist", name);
		return Models->CreatePG(SG);
	}
}

void CRender::models_Prefetch()
{
	Models->Prefetch();
}

void CRender::models_Clear(BOOL b_complete)
{
	Models->ClearPool(b_complete);
}

ref_shader CRender::getShader(int id)
{
	VERIFY(id < int(Shaders.size()));
	return Shaders[id];
}

IRender_Portal* CRender::getPortal(int id)
{
	VERIFY(id < int(Portals.size()));
	return Portals[id];
}
IRender_Sector* CRender::getSector(int id)
{
	VERIFY(id < int(Sectors.size()));
	return Sectors[id];
}

IRender_Sector* CRender::getSectorActive()
{
	return pLastSector;
}

IRender_Visual* CRender::getVisual(int id)
{
	VERIFY(id < int(Visuals.size()));
	return Visuals[id];
}

D3DVERTEXELEMENT9* CRender::getVB_Format(int id, BOOL _alt)
{
	if (_alt)
	{
		VERIFY(id < int(xDC.size()));
		return xDC[id].begin();
	}
	else
	{
		VERIFY(id < int(nDC.size()));
		return nDC[id].begin();
	}
}

IDirect3DVertexBuffer9* CRender::getVB(int id, BOOL _alt)
{
	if (_alt)
	{
		VERIFY(id < int(xVB.size()));
		return xVB[id];
	}
	else
	{
		VERIFY(id < int(nVB.size()));
		return nVB[id];
	}
}

IDirect3DIndexBuffer9* CRender::getIB(int id, BOOL _alt)
{
	if (_alt)
	{
		VERIFY(id < int(xIB.size()));
		return xIB[id];
	}
	else
	{
		VERIFY(id < int(nIB.size()));
		return nIB[id];
	}
}

FSlideWindowItem* CRender::getSWI(int id)
{
	VERIFY(id < int(SWIs.size()));
	return &SWIs[id];
}

IRender_Target* CRender::getTarget()
{
	return RenderTarget;
}

IEffectorsManager* CRender::getEffectorsManager()
{
	return EffectorsManager;
}

IRender_Light* CRender::light_create()
{
	return Lights.Create();
}

IRender_Glow* CRender::glow_create()
{
	return xr_new<CGlow>();
}

BOOL CRender::occ_visible(vis_data& P)
{
	return HOM.visible(P);
}

BOOL CRender::occ_visible(sPoly& P)
{
	return HOM.visible(P);
}

BOOL CRender::occ_visible(Fbox& P)
{
	return HOM.visible(P);
}

void CRender::add_Visual(IRender_Visual* V)
{
	SceneGraph.add_leafs_Dynamic(V);
}

void CRender::add_Geometry(IRender_Visual* V)
{
	SceneGraph.add_Static(V, View->getMask());
}

void CRender::add_StaticWallmark(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* verts)
{
	if (g_dedicated_server)
		return;

	if (T->suppress_wm)
		return;
	VERIFY2(_valid(P) && _valid(s) && T && verts && (s > EPS_L), "Invalid static wallmark params");
	Wallmarks->AddStaticWallmark(T, verts, P, &*S, s);
}

void CRender::clear_static_wallmarks()
{
	Wallmarks->clear();
}

void CRender::add_SkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm)
{
	Wallmarks->AddSkeletonWallmark(wm);
}

void CRender::add_SkeletonWallmark(const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start,
								   const Fvector& dir, float size)
{
	Wallmarks->AddSkeletonWallmark(xf, obj, sh, start, dir, size);
}

void CRender::add_Occluder(Fbox2& bb_screenspace)
{
	HOM.occlude(bb_screenspace);
}

void CRender::set_Object(IRenderable* O)
{
	SceneGraph.val_pObject = O;
}

void CRender::set_render_mode(int mode)
{
	float ZMin = 0.0f;
	float ZMax = 0.0f;

	switch (mode)
	{
	case MODE_NEAR:
		ZMin = 0.0f;
		ZMax = 0.02f;
		break;
	case MODE_NORMAL:
		ZMin = 0.0f;
		ZMax = 1.0f;
		break;
	case MODE_FAR:
		ZMin = 0.99999f;
		ZMax = 1.0f;
		break;
	}

	IRender_Target* T = getTarget();
	D3DVIEWPORT9 VP = {0, 0, T->get_width(), T->get_height(), ZMin, ZMax};
	CHK_DX(HW.pDevice->SetViewport(&VP));
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CRender::CRender() : m_bFirstFrameAfterReset(false)
{
	LPCSTR CompilerName = "D3DCompiler_43.dll";
	Msg("Loading d3d compiler DLL: %s", CompilerName);
	hCompiler = LoadLibrary(CompilerName);

	if (!hCompiler)
		make_string("Can't find 'D3DCompiler_43.dll'\nPlease install latest version of DirectX before running this program");

	init_cacades();

	m_actor_health = 1.0f;
}

CRender::~CRender()
{
	if (hCompiler)
	{
		FreeLibrary(hCompiler);
		hCompiler = 0;
	}
}

#include "..\xrEngine\GameFont.h"
// #include "../xrRender/xrRender_console.cpp"
void CRender::Statistics(CGameFont* _F)
{
	CGameFont& F = *_F;
	F.OutNext(" **** LT:%2d,LV:%2d **** ", stats.l_total, stats.l_visible);
	stats.l_visible = 0;
	F.OutNext("    S(%2d)   | (%2d)NS   ", stats.l_shadowed, stats.l_unshadowed);
	F.OutNext("smap use[%2d], merge[%2d], finalclip[%2d]", stats.s_used, stats.s_merged - stats.s_used,
			  stats.s_finalclip);
	stats.s_used = 0;
	stats.s_merged = 0;
	stats.s_finalclip = 0;
	F.OutSkip();
	F.OutNext(" **** Occ-Q(%03.1f) **** ", 100.f * f32(stats.o_culled) / f32(stats.o_queries ? stats.o_queries : 1));
	F.OutNext(" total  : %2d", stats.o_queries);
	stats.o_queries = 0;
	F.OutNext(" culled : %2d", stats.o_culled);
	stats.o_culled = 0;
	F.OutSkip();
	u32 ict = stats.ic_total + stats.ic_culled;
	F.OutNext(" **** iCULL(%03.1f) **** ", 100.f * f32(stats.ic_culled) / f32(ict ? ict : 1));
	F.OutNext(" visible: %2d", stats.ic_total);
	stats.ic_total = 0;
	F.OutNext(" culled : %2d", stats.ic_culled);
	stats.ic_culled = 0;
#ifdef DEBUG
	HOM.stats();
#endif
}

void CRender::enable_dbt_bounds(light* L)
{
	if (!RenderImplementation.o.nvdbt)
		return;
	if (!ps_r_ls_flags.test(RFLAG_USE_NVDBT))
		return;

	u32 mask = 0xffffffff;
	EFC_Visible vis = RenderImplementation.ViewBase.testSphere(L->spatial.sphere.P, L->spatial.sphere.R * 1.01f, mask);
	if (vis != fcvFully)
		return;

	// xform BB
	Fbox BB;
	Fvector rr;
	rr.set(L->spatial.sphere.R, L->spatial.sphere.R, L->spatial.sphere.R);
	BB.setb(L->spatial.sphere.P, rr);

	Fbox bbp;
	bbp.invalidate();
	for (u32 i = 0; i < 8; i++)
	{
		Fvector pt;
		BB.getpoint(i, pt);
		Device.mFullTransform.transform(pt);
		bbp.modify(pt);
	}
	u_DBT_enable(bbp.min.z, bbp.max.z);
}

// nv-DBT
BOOL CRender::u_DBT_enable(float zMin, float zMax)
{
	if (!RenderImplementation.o.nvdbt)
		return FALSE;
	if (!ps_r_ls_flags.test(RFLAG_USE_NVDBT))
		return FALSE;

	// enable cheat
	RenderBackend.SetRenderState(D3DRS_ADAPTIVETESS_X, MAKEFOURCC('N', 'V', 'D', 'B'));
	RenderBackend.SetRenderState(D3DRS_ADAPTIVETESS_Z, *(DWORD*)&zMin);
	RenderBackend.SetRenderState(D3DRS_ADAPTIVETESS_W, *(DWORD*)&zMax);

	return TRUE;
}

void CRender::u_DBT_disable()
{
	if (RenderImplementation.o.nvdbt && ps_r_ls_flags.test(RFLAG_USE_NVDBT))
		RenderBackend.SetRenderState(D3DRS_ADAPTIVETESS_X, 0);
}

float CRender::hclip(float v, float dim)
{
	return 2.f * v / dim - 1.f;
}

