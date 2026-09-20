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
#include "xrEngine\r_constants.h"
//////////////////////////////////////////////////////////////////////////
CRender RenderImplementation;
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
		light* sun = (light*)RenderImplementation.Scene.GetLights().sun_adapted._get();

		fvec3 L_dir;
		Engine.RenderView.View.transform_dir(L_dir, sun->get_direction());
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
		light* sun = (light*)RenderImplementation.Scene.GetLights().sun_adapted._get();
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

		if(RenderImplementation.active_phase() == CRender::PHASE_HUD)
			is_hud_render_phase = 1;

		RenderBackend.set_Constant("is_hud_render_phase", (float)is_hud_render_phase, 0.0f, 0.0f, 0.0f);
	}
} binder_is_hud_render_phase;
//////////////////////////////////////////////////////////////////////////
// update with vid_restart
void CRender::update_options()
{
	m_skinning = -1;

	o.smapsize = 1024;

	o.noshadows = (strstr(Core.Params, "-noshadows")) ? TRUE : FALSE;
	o.forceskinw = (strstr(Core.Params, "-skinw")) ? TRUE : FALSE;
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

	return macros;
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CRender::CRender() : Scene(), m_bFirstFrameAfterReset(false)
{
	LPCSTR CompilerName = "D3DCompiler_43.dll";
	Msg("Loading d3d compiler DLL: %s", CompilerName);
	hCompiler = LoadLibrary(CompilerName);

	if(!hCompiler)
		make_string("Can't find 'D3DCompiler_43.dll'\nPlease install latest version of DirectX before running this program");

	m_actor_health = 1.0f;

	m_sun_write_ix = 0;
	m_sun_read_ix = 0;
}

CRender::~CRender()
{
	if(hCompiler)
	{
		FreeLibrary(hCompiler);
		hCompiler = 0;
	}
}

void CRender::Initialize()
{
	Engine.Events.Frame.Add(this, REG_PRIORITY_HIGH + 0x12345678);

	update_options();

	Scene.Initialize();
	Scene.CreateResources();

	RenderTarget = xr_new<CRenderTarget>();

	EffectorsManager = xr_new<CEffectorsManager>();

	Models = xr_new<CModelPool>();
	PSLibrary.OnCreate();
	HWOCC.occq_create(occq_size);

	xrRender_apply_tf();

	init_cacades();

	m_scene_visibility_data.InitResources();
	m_spot_shadow_vis.InitResources();
}

void CRender::Create()
{
	Engine.ResourceManager->RegisterConstantSetup("sun_far", &binder_sun_far);
	Engine.ResourceManager->RegisterConstantSetup("sun_dir", &binder_sun_dir);
	Engine.ResourceManager->RegisterConstantSetup("sun_normal_bias", &binder_sun_normal_bias);
	Engine.ResourceManager->RegisterConstantSetup("sun_directional_bias", &binder_sun_directional_bias);
	Engine.ResourceManager->RegisterConstantSetup("sun_color", &binder_sun_color);
	Engine.ResourceManager->RegisterConstantSetup("debug_reserved", &binder_debug_reserved);
	Engine.ResourceManager->RegisterConstantSetup("ao_brightness", &binder_ao_brightness);
	Engine.ResourceManager->RegisterConstantSetup("is_hud_render_phase", &binder_is_hud_render_phase);

	RenderTarget->CompileShaders();
}

void CRender::Destroy()
{
	WaitForPendingTasks();

	m_scene_visibility_data.FreeResources();
	m_spot_shadow_vis.FreeResources();

	m_sun_cascades_buffer[0].Destroy();
	m_sun_cascades_buffer[1].Destroy();

	Scene.Destroy();

	HWOCC.occq_destroy();

	xr_delete(Models);
	xr_delete(RenderTarget);
	PSLibrary.OnDestroy();
	Engine.Events.Frame.Remove(this);
	xr_delete(EffectorsManager);
}

void CRender::ResetBegin()
{
	WaitForPendingTasks();

	m_scene_visibility_data.FreeResources();
	m_spot_shadow_vis.FreeResources();
	m_sun_cascades_buffer[0].Destroy();
	m_sun_cascades_buffer[1].Destroy();
	Scene.OnResetBegin();
	xr_delete(RenderTarget);
	HWOCC.occq_destroy();
}

void CRender::ResetEnd()
{
	HWOCC.occq_create(occq_size);

	update_options();
	RenderTarget = xr_new<CRenderTarget>();
	RenderTarget->CompileShaders();
	dwAccumulatorClearMark = 0;

	xrRender_apply_tf();

	// Set this flag true to skip the first render frame,
	// that some data is not ready in the first frame (for example device camera position)
	m_bFirstFrameAfterReset = true;

	Scene.OnResetEnd();

	m_scene_visibility_data.InitResources();
	m_spot_shadow_vis.InitResources();
	m_sun_cascades_buffer[0].Init();
	m_sun_cascades_buffer[1].Init();
}

void CRender::WaitForPendingTasks()
{
	wait_for_sun_task();
}

void CRender::OnFrame()
{
	PROFILE_FUNCTION();

	WaitForPendingTasks();
	Models->DeleteQueue();
	CPUOCC.Update();
	Scene.OnFrame();

	if(need_render_sun())
	{
		wait_for_sun_task();
		swap_sun_buffers();
		m_sun_gather_done.store(false);
		Engine.ThreadManager.AddParallelTask([this]() { schedule_cascades(); });
	}
}

// Implementation
bool CRender::is_dynamic_sun_enabled()
{
	return ps_r_lighting_flags.test(RFLAG_SUN);
}

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
	if(F)
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
	if(SE)
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
	if(_alt)
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
	if(_alt)
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
	if(_alt)
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

void CRender::set_render_mode(int mode)
{
	float ZMin = 0.0f;
	float ZMax = 0.0f;

	switch(mode)
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
	CHK_DX(RenderBackend.GetDevice()->SetViewport(&VP));
}

#include "..\xrEngine\GameFont.h"
// #include "xrRender/xrRender_console.cpp"
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
	F.OutNext(" **** Occ-Q(%03.1f) **** ", 100.f * float(stats.o_culled) / float(stats.o_queries ? stats.o_queries : 1));
	F.OutNext(" total  : %2d", stats.o_queries);
	stats.o_queries = 0;
	F.OutNext(" culled : %2d", stats.o_culled);
	stats.o_culled = 0;
	F.OutSkip();
	u32 ict = stats.ic_total + stats.ic_culled;
	F.OutNext(" **** iCULL(%03.1f) **** ", 100.f * float(stats.ic_culled) / float(ict ? ict : 1));
	F.OutNext(" visible: %2d", stats.ic_total);
	stats.ic_total = 0;
	F.OutNext(" culled : %2d", stats.ic_culled);
	stats.ic_culled = 0;
}

float CRender::hclip(float v, float dim)
{
	return 2.f * v / dim - 1.f;
}
