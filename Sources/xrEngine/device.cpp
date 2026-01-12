#include "stdafx.h"
#include "frustum.h"

#pragma warning(disable : 4995)
// mmsystem.h
#define MMNOSOUND
#define MMNOMIDI
#define MMNOAUX
#define MMNOMIXER
#define MMNOJOY
#include <mmsystem.h>
#include <d3dx9.h>
#pragma warning(default : 4995)

#include "Engine.h"
#include "render.h"
#pragma warning(push)
#pragma warning(disable : 4995)
#include <ppl.h>
#pragma warning(pop)
#include "resourcemanager.h"
#include "optick_include.h"
#include "IGame_Persistent.h"

#include "debug_ui.h"

#include "xr_ioc_cmd.h"

#include "resource.h"

ENGINE_API CRenderDevice Device;
ENGINE_API BOOL g_bRendering = FALSE;

BOOL g_bLoaded = FALSE;
ref_light precache_light = 0;
/////////////////////////////////////
BOOL CRenderDevice::Begin()
{
#ifndef DEDICATED_SERVER
	OPTICK_EVENT("CRenderDevice::Begin");

	HW.Validate();
	HRESULT _hr = HW.pDevice->TestCooperativeLevel();
	if (FAILED(_hr) && D3DERR_DEVICENOTRESET == _hr)
		Reset();

	Engine.DebugUI.OnFrameBegin();

	CHK_DX(HW.pDevice->BeginScene());

	RenderBackend.OnFrameBegin();
	RenderBackend.set_CullMode(CULL_BACKFACE);
	if (HW.Caps.SceneMode)
		overdrawBegin();
	FPU::m24r();
	g_bRendering = TRUE;
#endif
	return TRUE;
}

void CRenderDevice::Clear()
{
	CHK_DX(HW.pDevice->Clear(0, 0, D3DCLEAR_ZBUFFER | (psDeviceFlags.test(rsClearBB) ? D3DCLEAR_TARGET : 0) |
								 (HW.Caps.bStencil ? D3DCLEAR_STENCIL : 0),
							 D3DCOLOR_XRGB(0, 0, 0), 1, 0));
}

void Present()
{
	OPTICK_EVENT("PRESENT");

	Device.Statistic->RenderPresentation.Begin();

	HRESULT _hr = HW.pDevice->PresentEx(NULL, NULL, NULL, NULL, NULL);

	Device.Statistic->RenderPresentation.End();
}

void CRenderDevice::End(void)
{
#ifndef DEDICATED_SERVER
	OPTICK_EVENT("CRenderDevice::End");

	VERIFY(HW.pDevice);

	if (HW.Caps.SceneMode)
		overdrawEnd();

	if (dwPrecacheFrame)
	{
		::Sound->set_master_volume(psSoundVFactor);
		dwPrecacheFrame--;
		Engine.LoadingScreen.ForceRender();
		if (0 == dwPrecacheFrame)
		{
			Gamma.Update();

			if (precache_light)
				precache_light->set_active(false);
			if (precache_light)
				precache_light.destroy();
			::Sound->set_master_volume(psSoundVFactor);
			Resources->DestroyNecessaryTextures();
			Memory.mem_compact();
			Msg("* MEMORY USAGE: %d K", Memory.mem_usage() / 1024);
		}
	}

	g_bRendering = FALSE;
	// end scene
	RenderBackend.OnFrameEnd();
	Memory.dbg_check();
	CHK_DX(HW.pDevice->EndScene());

	Engine.DebugUI.OnFrameEnd();

	BOOL needsPresent = TRUE;

	// Не делаем Present если:
	// 1. Мы в режиме precache
	// 2. Нет активных изменений
	// 3. Окно минимизировано
	if (dwPrecacheFrame || !b_is_Active || IsIconic(Engine.WindowManager.GetHandle()))
	{
		needsPresent = FALSE;
	}

	if (needsPresent)
	{
		Statistic->RenderTOTAL_Real.End();
		Statistic->RenderTOTAL_Real.FrameEnd();
		Present();
	}
#endif
}

void CRenderDevice::SecondaryThreadProc(void* context)
{
	OPTICK_THREAD("X-Ray Secondary Thread");
	OPTICK_FRAME("X-Ray Secondary Thread");

	auto& device = *static_cast<CRenderDevice*>(context);
	while (true)
	{
		device.syncProcessFrame.Wait();
		if (device.mt_bMustExit)
		{
			device.mt_bMustExit = FALSE;
			device.syncThreadExit.Set();
			return;
		}

		for (u32 pit = 0; pit < device.seqParallel.size(); pit++)
			device.seqParallel[pit]();

		device.seqParallel.clear_not_free();
		device.seqFrameMT.Process(rp_Frame);
		device.syncFrameDone.Set();
	}
}

void CRenderDevice::RenderThreadProc(void* context)
{
	OPTICK_THREAD("X-Ray Render Thread");
	OPTICK_FRAME("CRenderDevice::SecondaryThreadProc()");

	auto& device = *static_cast<CRenderDevice*>(context);
	while (true)
	{
		device.renderProcessFrame.Wait();
		if (device.mt_bMustExit)
		{
			device.renderThreadExit.Set();
			return;
		}
		device.seqRender.Process(rp_Render);
		device.renderFrameDone.Set();
	}
}

#include "igame_level.h"
#include <ThreadUtil.h>
void CRenderDevice::PreCache(u32 amount)
{
	OPTICK_EVENT("CRenderDevice::PreCache");

	if (HW.Caps.bForceGPU_REF)
		amount = 0;
#ifdef DEDICATED_SERVER
	amount = 0;
#endif
	//Msg("* LIGHT PRECACHE: start for %d...", amount);
	dwPrecacheFrame = dwPrecacheTotal = amount;
	if (amount && !precache_light && g_pGameLevel)
	{
		precache_light = ::Render->light_create();
		precache_light->set_shadow(false);
		precache_light->set_position(vCameraPosition);
		precache_light->set_color(255, 255, 255);
		precache_light->set_range(5.0f);
		precache_light->set_active(true);
	}
}

int g_svDedicateServerUpdateReate = 100;

ENGINE_API xr_list<LOADING_EVENT> g_loading_events;

void CRenderDevice::PrepareEventLoop()
{
	OPTICK_EVENT("CRenderDevice::PrepareEventLoop");

	g_bLoaded = FALSE;

	Msg("Preparing event loop...");

	LPCSTR MainThreadName = "X-RAY Primary thread";
	Msg("Setting main thread name: %s", MainThreadName);
	OPTICK_THREAD(MainThreadName);

	Threading::SpawnThread(SecondaryThreadProc, "X-RAY Secondary thread", 0, this);

	mt_bMustExit = FALSE;

	// [Moved from StartEventLoop start]
	Log("\nStarting event loop...");
	seqAppStart.Process(rp_AppStart);
	CHK_DX(HW.pDevice->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1, 0));
}

void CRenderDevice::DoFrame()
{
	// [Moved from StartEventLoop inside the 'else' block of PeekMessage]

	if (b_is_Ready)
	{
#ifdef DEDICATED_SERVER
		u32 FrameStartTime = TimerGlobal.GetElapsed_ms();
#endif
		if (psDeviceFlags.test(rsStatistic))
			g_bEnableStatGather = TRUE;
		else
			g_bEnableStatGather = FALSE;

		if (g_loading_events.size())
		{
			if (g_loading_events.front()())
				g_loading_events.pop_front();

			Engine.LoadingScreen.ForceRender();
			return; // continue в цикле заменяется на return
		}
		else
		{
			FrameMove();
		}

		// Precache
		if (dwPrecacheFrame)
		{
			float factor = float(dwPrecacheFrame) / float(dwPrecacheTotal);
			float angle = PI_MUL_2 * factor;
			vCameraDirection.set(_sin(angle), 0, _cos(angle));
			vCameraDirection.normalize();
			vCameraTop.set(0, 1, 0);
			vCameraRight.crossproduct(vCameraTop, vCameraDirection);

			mView.build_camera_dir(vCameraPosition, vCameraDirection, vCameraTop);
		}

		// Matrices
		mFullTransform.mul(mProject, mView);
		RenderBackend.set_xform_view(mView);
		RenderBackend.set_xform_project(mProject);
		D3DXMatrixInverse((D3DXMATRIX*)&mInvFullTransform, 0, (D3DXMATRIX*)&mFullTransform);

		syncProcessFrame.Set(); // allow secondary thread to do its job

#ifndef DEDICATED_SERVER
		Statistic->RenderTOTAL_Real.FrameStart();
		Statistic->RenderTOTAL_Real.Begin();
		if (b_is_Active)
		{
			if (Begin())
			{
				Engine.DebugUI.DrawUI();
				seqRender.Process(rp_Render);

				if (psDeviceFlags.test(rsCameraPos) || psDeviceFlags.test(rsStatistic) || Statistic->errors.size())
					Statistic->Show();

				End();
			}
		}
		Statistic->RenderTOTAL_Real.End();
		Statistic->RenderTOTAL_Real.FrameEnd();
		Statistic->RenderTOTAL.accum = Statistic->RenderTOTAL_Real.accum;
#endif

		vCameraPosition_saved = vCameraPosition;
		mFullTransform_saved = mFullTransform;

		syncFrameDone.Wait();

#ifdef DEDICATED_SERVER
		u32 FrameEndTime = TimerGlobal.GetElapsed_ms();
		u32 FrameTime = (FrameEndTime - FrameStartTime);
		u32 DSUpdateDelta = 1000 / g_svDedicateServerUpdateReate;
		if (FrameTime < DSUpdateDelta)
		{
			Sleep(DSUpdateDelta - FrameTime);
		}
#endif
	}
	else
	{
		Sleep(100);
	}

	if (!b_is_Active)
		Sleep(1);
}

void CRenderDevice::EndEventLoop()
{
	OPTICK_EVENT("CRenderDevice::EndEventLoop");

	Msg("Ending event loop...");

	seqAppEnd.Process(rp_AppEnd);

	// Stop Balance-Thread
	mt_bMustExit = TRUE;
	syncProcessFrame.Set();
	syncThreadExit.Wait();
	while (mt_bMustExit)
		Sleep(0);
}

void ProcessLoading(RP_FUNC* f);
void CRenderDevice::FrameMove()
{
	OPTICK_EVENT("CRenderDevice::FrameMove");

	// Вся логика расчета времени перенесена в Engine.TimeManager.Update(),
	// который вызывается в Engine.cpp перед DoFrame().

	// Frame move logic
	Statistic->EngineTOTAL.Begin();

	// Используем Engine.TimeManager для проверки загрузки, если нужно,
	// или просто выполняем логику кадров.
	if (!g_bLoaded)
		ProcessLoading(rp_Frame);
	else
		seqFrame.Process(rp_Frame);

	Statistic->EngineTOTAL.End();
}

void ProcessLoading(RP_FUNC* f)
{
	Device.seqFrame.Process(rp_Frame);
	g_bLoaded = TRUE;
}

ENGINE_API BOOL bShowPauseString = TRUE;

void CRenderDevice::Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason)
{
	OPTICK_EVENT("CRenderDevice::Pause");

	static int snd_emitters_ = -1;

#ifdef DEBUG
	Msg("pause [%s] timer=[%s] sound=[%s] reason=%s", bOn ? "ON" : "OFF", bTimer ? "ON" : "OFF", bSound ? "ON" : "OFF",
		reason);
#endif // DEBUG

#ifndef DEDICATED_SERVER

	if (bOn)
	{
#pragma todo("NSDeathman to NSDeathman: Добавить сюда проверку на разрешение отрисовку HUD")
		if (!Paused())
			bShowPauseString = TRUE;

		if (bTimer && g_pGamePersistent->CanBePaused())
			g_pauseMngr.Pause(TRUE);

		if (bSound)
		{
			snd_emitters_ = ::Sound->pause_emitters(true);
#ifdef DEBUG
			Log("snd_emitters_[true]", snd_emitters_);
#endif // DEBUG
		}
	}
	else
	{
		if (bTimer && /*g_pGamePersistent->CanBePaused() &&*/ g_pauseMngr.Paused())
			g_pauseMngr.Pause(FALSE);

		if (bSound)
		{
			if (snd_emitters_ > 0) // avoid crash
			{
				snd_emitters_ = ::Sound->pause_emitters(false);
#ifdef DEBUG
				Log("snd_emitters_[false]", snd_emitters_);
#endif // DEBUG
			}
			else
			{
#ifdef DEBUG
				Log("Sound->pause_emitters underflow");
#endif // DEBUG
			}
		}
	}

#endif
}

BOOL CRenderDevice::Paused()
{
	return g_pauseMngr.Paused();
};

void CRenderDevice::OnWM_Activate(WPARAM wParam, LPARAM lParam)
{
	OPTICK_EVENT("CRenderDevice::OnWM_Activate");

	u16 fActive = LOWORD(wParam);
	BOOL fMinimized = (BOOL)HIWORD(wParam);
	BOOL bActive = ((fActive != WA_INACTIVE) && (!fMinimized)) ? TRUE : FALSE;

	if (bActive != Device.b_is_Active)
	{
		Device.b_is_Active = bActive;

		if (Device.b_is_Active)
		{
			Device.seqAppActivate.Process(rp_AppActivate);
#ifndef DEDICATED_SERVER
			ShowCursor(FALSE);
#endif
		}
		else
		{
			Device.seqAppDeactivate.Process(rp_AppDeactivate);
			ShowCursor(TRUE);
		}
	}
}

void CRenderDevice::_SetupStates()
{
	OPTICK_EVENT("CRenderDevice::_SetupStates");

	// General Render States
	mView.identity();
	mProject.identity();
	mFullTransform.identity();
	vCameraPosition.set(0, 0, 0);
	vCameraDirection.set(0, 0, 1);
	vCameraTop.set(0, 1, 0);
	vCameraRight.set(1, 0, 0);

	HW.Caps.Update();
	for (u32 i = 0; i < HW.Caps.raster.dwStages; i++)
	{
		float fBias = -.5f;
		CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, 4));
		CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, *((LPDWORD)(&fBias))));
		CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
		CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
		CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR));
	}
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_DITHERENABLE, TRUE));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_COLORVERTEX, TRUE));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZENABLE, TRUE));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_LOCALVIEWER, TRUE));

	CHK_DX(HW.pDevice->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR1));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE));

	CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID));

	// ******************** Fog parameters
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_FOGCOLOR, 0));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_RANGEFOGENABLE, FALSE));
	if (HW.Caps.bTableFog)
	{
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_EXP2));
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_NONE));
	}
	else
	{
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_NONE));
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_EXP2));
	}
}

void CRenderDevice::_Create(LPCSTR shName)
{
	OPTICK_EVENT("CRenderDevice::_Create");

	Memory.mem_compact();

	// after creation
	b_is_Ready = TRUE;
	_SetupStates();

	// Signal everyone - device created
	RenderBackend.OnDeviceCreate();
	Gamma.Update();
	Resources->OnDeviceCreate(shName);
	::Render->create();
	Statistic->OnDeviceCreate();

#ifndef DEDICATED_SERVER
	m_WireShader.create("hud\\crosshair");
	m_SelectionShader.create("hud\\crosshair");

	DU.OnDeviceCreate();
#endif

	dwFrame = 0;
}

void CRenderDevice::Create()
{
	OPTICK_EVENT("CRenderDevice::Create");

	if (b_is_Ready)
		return; // prevent double call
	Statistic = xr_new<CStats>();
	Log("\nStarting RENDER device...");

#ifdef _EDITOR
	psCurrentVidMode[0] = dwWidth;
	psCurrentVidMode[1] = dwHeight;
#endif

	HW.CreateDevice(Engine.WindowManager.GetHandle());
	dwWidth = HW.DevPP.BackBufferWidth;
	dwHeight = HW.DevPP.BackBufferHeight;
	Engine.WindowManager.UpdateSize(dwWidth, dwHeight);
	fWidth_2 = float(dwWidth / 2);
	fHeight_2 = float(dwHeight / 2);
	fFOV = 90.f;
	fASPECT = 1.f;

	string_path fname;
	FS.update_path(fname, "$game_data$", "shaders.xr");

	//////////////////////////////////////////////////////////////////////////
	Resources = xr_new<CResourceManager>();
	_Create(fname);

	PreCache(0);
}

void CRenderDevice::_Destroy(BOOL bKeepTextures)
{
	OPTICK_EVENT("CRenderDevice::_Destroy");

	DU.OnDeviceDestroy();
	m_WireShader.destroy();
	m_SelectionShader.destroy();

	// before destroy
	b_is_Ready = FALSE;
	Statistic->OnDeviceDestroy();
	::Render->destroy();
	RenderBackend.DeleteResources();
	Resources->OnDeviceDestroy(bKeepTextures);
	RenderBackend.OnDeviceDestroy();

	Memory.mem_compact();
}

void CRenderDevice::Destroy(void)
{
	OPTICK_EVENT("CRenderDevice::Destroy");

	if (!b_is_Ready)
		return;

	Log("\nDestroying Direct3D...");

	ShowCursor(TRUE);
	HW.Validate();

	_Destroy(FALSE);

	xr_delete(Resources);

	// real destroy
	HW.DestroyDevice();

	seqRender.R.clear();
	seqAppActivate.R.clear();
	seqAppDeactivate.R.clear();
	seqAppStart.R.clear();
	seqAppEnd.R.clear();
	seqFrame.R.clear();
	seqFrameMT.R.clear();
	seqDeviceReset.R.clear();
	seqParallel.clear();

	xr_delete(Statistic);
}

#include "IGame_Level.h"
#include "CustomHUD.h"
void CRenderDevice::Reset(bool precache)
{
	OPTICK_EVENT("CRenderDevice::Reset");

	Engine.DebugUI.OnResetBegin();

#ifdef DEBUG
	_SHOW_REF("*ref -CRenderDevice::ResetTotal: DeviceREF:", HW.pDevice);
#endif // DEBUG
	bool b_16_before = (float)dwWidth / (float)dwHeight > (1024.0f / 768.0f + 0.01f);

	ShowCursor(TRUE);

	RenderBackend.reset_begin();

	Resources->reset_begin();
	Memory.mem_compact();
	HW.Reset(Engine.WindowManager.GetHandle());
	dwWidth = HW.DevPP.BackBufferWidth;
	dwHeight = HW.DevPP.BackBufferHeight;
	Engine.WindowManager.UpdateSize(dwWidth, dwHeight);
	fWidth_2 = float(dwWidth / 2);
	fHeight_2 = float(dwHeight / 2);
	Resources->reset_end();

	if (g_pGamePersistent)
	{
		g_pGamePersistent->Environment().bNeed_re_create_env = TRUE;
	}
	_SetupStates();

#ifndef DEDICATED_SERVER
	ShowCursor(FALSE);
#endif

	seqDeviceReset.Process(rp_DeviceReset);

	RenderBackend.reset_end();

	bool b_16_after = (float)dwWidth / (float)dwHeight > (1024.0f / 768.0f + 0.01f);
	if (b_16_after != b_16_before && g_pGameLevel && g_pGameLevel->pHUD)
		g_pGameLevel->pHUD->OnScreenRatioChanged();

	Engine.DebugUI.OnResetEnd();

#ifdef DEBUG
	_SHOW_REF("*ref +CRenderDevice::ResetTotal: DeviceREF:", HW.pDevice);
#endif // DEBUG
}

void CRenderDevice::Initialize()
{
	OPTICK_EVENT("CRenderDevice::Initialize");

	Msg("Initializing Render Device...");

	// Save window properties
	m_dwWindowStyle = GetWindowLong(Engine.WindowManager.GetHandle(), GWL_STYLE);
	GetWindowRect(Engine.WindowManager.GetHandle(), &m_rcWindowBounds);
	GetClientRect(Engine.WindowManager.GetHandle(), &m_rcWindowClient);

		HW.Caps.bForceGPU_SW = FALSE;
		HW.Caps.bForceGPU_NonPure = FALSE;
		HW.Caps.bForceGPU_REF = FALSE;
}

// *****************************************************************************************
// Error handling

//----------------------------- FLAGS
static struct _DF
{
	char* name;
	u32 mask;
} DF[] = {{"rsFullscreen", rsFullscreen},
		  {"rsClearBB", rsClearBB},
		  {"rsVSync", rsVSync},
		  {"rsWireframe", rsWireframe},
		  {NULL, 0}};

void CRenderDevice::DumpFlags()
{
	Log("- Dumping device flags");
	_DF* p = DF;
	while (p->name)
	{
		Msg("* %20s %s", p->name, psDeviceFlags.test(p->mask) ? "on" : "off");
		p++;
	}
}

void CRenderDevice::overdrawBegin()
{
	OPTICK_EVENT("CRenderDevice::overdrawBegin");

	// Turn stenciling
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILREF, 0));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILMASK, 0x00000000));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILWRITEMASK, 0xffffffff));

	// Increment the stencil buffer for each pixel drawn
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCRSAT));

	if (1 == HW.Caps.SceneMode)
	{
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP));
	} // Overdraw
	else
	{
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_INCRSAT));
	} // ZB access
}

void CRenderDevice::overdrawEnd()
{
	OPTICK_EVENT("CRenderDevice::overdrawEnd");

	// Set up the stencil states
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL));
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILMASK, 0xff));

	// Set the background to black
	RenderBackend.Clear(0, 0, CLEAR_RENDERTARGET, D3DCOLOR_XRGB(255, 0, 0), 0, 0);

	// Draw a rectangle wherever the count equal I
	RenderBackend.OnFrameEnd();
	CHK_DX(HW.pDevice->SetFVF(FVF::F_TL));

	// Render gradients
	for (int I = 0; I < 12; I++)
	{
		u32 _c = I * 256 / 13;
		u32 c = D3DCOLOR_XRGB(_c, _c, _c);

		FVF::TL pv[4]{};
		pv[0].set(float(0), float(dwHeight), c, 0, 0);
		pv[1].set(float(0), float(0), c, 0, 0);
		pv[2].set(float(dwWidth), float(dwHeight), c, 0, 0);
		pv[3].set(float(dwWidth), float(0), c, 0, 0);

		CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILREF, I));
		CHK_DX(HW.pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pv, sizeof(FVF::TL)));
	}
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE));
}
