#include "pch.h"
#include "frustum.h"

#pragma warning(disable : 4995)
#define MMNOSOUND
#define MMNOMIDI
#define MMNOAUX
#define MMNOMIXER
#define MMNOJOY
#include <mmsystem.h>
#pragma warning(default : 4995)

#include "Engine.h"
#include "IRender.h"
#include "resourcemanager.h"
#include "optick_include.h"
#include "IGame_Persistent.h"
#include "CustomHUD.h"
#include "debug_ui.h"
#include "xr_ioc_cmd.h"
#include "resource.h"
#include "LevelLoadingScreen.h"
#include "igame_level.h"

ENGINE_API CRenderDevice Device;
ENGINE_API BOOL g_bRendering = FALSE;

ref_light precache_light = 0;

namespace
{
	static void SelectResolution(u32& dwWidth, u32& dwHeight, BOOL /*bWindowed*/)
	{
#ifdef DEDICATED_SERVER
		dwWidth = 32;
		dwHeight = 32;
#else
		dwWidth = psCurrentVidMode[0];
		dwHeight = psCurrentVidMode[1];
#endif
	}

	static u32 SelectPresentInterval()
	{
#ifdef DEDICATED_SERVER
		return 0;
#else
		return psDeviceFlags.test(rsVSync) ? 1 : 0;
#endif
	}
}

void CRenderDevice::Begin()
{
#ifndef DEDICATED_SERVER
	if (Engine.RHI.NeedReset())
		Reset();

	Engine.RHI.BeginFrame();

	Engine.DebugUI.OnFrameBegin();

	g_bRendering = TRUE;
#endif
}

void CRenderDevice::End(void)
{
#ifndef DEDICATED_SERVER
	PROFILE_FUNCTION();

	g_bRendering = FALSE;
	Engine.RHI.EndFrame();
	Engine.DebugUI.OnFrameEnd();
	Memory.dbg_check();

	if (IsIconic(Engine.WindowManager.GetHandle()))
		return;

	Engine.Statistic->RenderPresentation.Begin();
	Engine.RHI.Present();
	Engine.Statistic->RenderPresentation.End();
#endif
}

int g_frametime = 166;

void CRenderDevice::RenderFrame()
{
	PROFILE_FUNCTION();

	if(!b_is_Active)
		return;

	Engine.Statistic->RenderTOTAL_Real.FrameStart();
	Engine.Statistic->RenderTOTAL_Real.Begin();

	Begin();

	Engine.Events.Render.Process(rp_Render);

	if(psDeviceFlags.test(rsCameraPos) || psDeviceFlags.test(rsStatistic) || Engine.Statistic->errors.size())
		Engine.Statistic->Show();

	Engine.DebugUI.DrawUI();

	End();

	Engine.Statistic->RenderTOTAL_Real.End();
	Engine.Statistic->RenderTOTAL_Real.FrameEnd();
}

ENGINE_API BOOL bShowPauseString = TRUE;

void CRenderDevice::Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason)
{
	static int snd_emitters_ = -1;

#ifdef DEBUG
	Msg("pause [%s] timer=[%s] sound=[%s] reason=%s", bOn ? "ON" : "OFF", bTimer ? "ON" : "OFF",
		bSound ? "ON" : "OFF", reason);
#endif

#ifndef DEDICATED_SERVER
	if(bOn)
	{
		if(!Paused())
			bShowPauseString = TRUE;

		if(bTimer && g_pGamePersistent->CanBePaused())
			g_pauseMngr.Pause(TRUE);

		if(bSound)
		{
			snd_emitters_ = ::Sound->pause_emitters(true);
#ifdef DEBUG
			Log("snd_emitters_[true]", snd_emitters_);
#endif
		}
	}
	else
	{
		if(bTimer && g_pauseMngr.Paused())
			g_pauseMngr.Pause(FALSE);

		if(bSound)
		{
			if(snd_emitters_ > 0)
			{
				snd_emitters_ = ::Sound->pause_emitters(false);
#ifdef DEBUG
				Log("snd_emitters_[false]", snd_emitters_);
#endif
			}
			else
			{
#ifdef DEBUG
				Log("Sound->pause_emitters underflow");
#endif
			}
		}
	}
#endif
}

BOOL CRenderDevice::Paused()
{
	return g_pauseMngr.Paused();
}

void CRenderDevice::SetActivate(bool bActive)
{
	if(bActive != Device.b_is_Active)
	{
		Device.b_is_Active = bActive;

		if(Device.b_is_Active)
		{
			Engine.Events.AppActivate.Process(rp_AppActivate);
#ifndef DEDICATED_SERVER
			ShowCursor(FALSE);
#endif
		}
		else
		{
			Engine.Events.AppDeactivate.Process(rp_AppDeactivate);
			ShowCursor(TRUE);
		}
	}
}

void CRenderDevice::Initialize()
{
	if (b_is_Ready)
		return;

	Msg("Initializing Render Device...");

	m_dwWindowStyle = GetWindowLong(Engine.WindowManager.GetHandle(), GWL_STYLE);
	GetWindowRect(Engine.WindowManager.GetHandle(), &m_rcWindowBounds);
	GetClientRect(Engine.WindowManager.GetHandle(), &m_rcWindowClient);

#ifdef _EDITOR
	psCurrentVidMode[0] = dwWidth;
	psCurrentVidMode[1] = dwHeight;
#endif

	if (!Engine.RHI.Initialize(RHI_BackendType::DirectX9Ex))
	{
		FATAL("! [Device] Engine.RHI.Initialize failed");
		return;
	}

#ifndef DEDICATED_SERVER
	BOOL bWindowed = !psDeviceFlags.is(rsFullscreen);
#else
	BOOL bWindowed = TRUE;
#endif

	u32 width, height;
	SelectResolution(width, height, bWindowed);
	u32 presentInterval = SelectPresentInterval();

	Engine.WindowManager.SetWindowed(bWindowed);
	Engine.WindowManager.SetResolution(width, height);
	Engine.WindowManager.SetRefreshRate(60);
	Engine.WindowManager.Apply();

	RECT rcClient;
	GetClientRect(Engine.WindowManager.GetHandle(), &rcClient);

	RHI_PresentationParams params;
	params.BackBufferWidth = rcClient.right - rcClient.left;
	params.BackBufferHeight = rcClient.bottom - rcClient.top;
	params.Windowed = bWindowed;
	params.BackBufferFormat = RHI_Format::RGBA8_UNORM;
	params.DepthStencilFormat = RHI_Format::D24_UNORM_S8_UINT;
	params.BackBufferCount = 2;
	params.SyncInterval = (presentInterval == 0) ? 0 : 1;
	params.FullscreenRefreshHz = 60;
	params.SwapEffect = RHI_SwapEffect::Discard;
	params.EnableAutoDepthStencil = true;

	if (!Engine.RHI.CreateDevice(Engine.WindowManager.GetHandle(), params))
	{
		FATAL("! [Device] Engine.RHI.CreateDevice failed");
		return;
	}

	RenderBackendLegacy.OnDeviceCreate(Engine.WindowManager.GetHandle(), params);

	dwWidth = Engine.RHI->GetBackBufferWidth();
	dwHeight = Engine.RHI->GetBackBufferHeight();
	Engine.WindowManager.UpdateSize(dwWidth, dwHeight);
	fWidth_2 = float(dwWidth / 2);
	fHeight_2 = float(dwHeight / 2);

	Memory.mem_compact();

	b_is_Ready = TRUE;

	string_path fname;
	FS.update_path(fname, "$game_data$", "shaders.xr");
	Engine.ResourceManager->OnDeviceCreate(fname);
	Engine.Statistic->OnDeviceCreate();

#ifndef DEDICATED_SERVER
	m_WireShader.create("hud\\crosshair");
	m_SelectionShader.create("hud\\crosshair");
	DU.OnDeviceCreate();
#endif

	R_InitVidModeList();
}

void CRenderDevice::Destroy(void)
{
	if (!b_is_Ready)
		return;

	Log("\nDestroying Direct3D...");
	ShowCursor(TRUE);

	DU.OnDeviceDestroy();
	m_WireShader.destroy();
	m_SelectionShader.destroy();

	b_is_Ready = FALSE;

	Engine.Statistic->OnDeviceDestroy();
	RenderBackendLegacy.DeleteResources();
	Engine.ResourceManager->OnDeviceDestroy(FALSE);

	RenderBackendLegacy.OnDeviceDestroy();

	Memory.mem_compact();

	Engine.RHI.Destroy();

	R_FreeVidModeList();
}


void CRenderDevice::Reset()
{
	Engine.DebugUI.OnResetBegin();

#ifdef DEBUG
	_SHOW_REF("*ref -CRenderDevice::ResetTotal: DeviceREF:",
		Engine.RHI.GetRawRHI() ? Engine.RHI.GetRawRHI()->GetDeviceHandle() : nullptr);
#endif

	bool b_16_before = (float)dwWidth / (float)dwHeight > (1024.0f / 768.0f + 0.01f);

	ShowCursor(TRUE);

	// --- 1. Legacy: освобождаем DEFAULT-pool ресурсы ---
	RenderBackendLegacy.ResetBegin();
	Engine.ResourceManager->ResetBegin();

	Memory.mem_compact();

	// --- 2. RHI Reset ---
#ifndef DEDICATED_SERVER
	BOOL bWindowed = strstr(Core.Params, "-windowed") ? TRUE : !psDeviceFlags.is(rsFullscreen);
#else
	BOOL bWindowed = TRUE;
#endif

	u32 width, height;
	SelectResolution(width, height, bWindowed);
	u32 presentInterval = SelectPresentInterval();

	Engine.WindowManager.SetWindowed(bWindowed);
	Engine.WindowManager.SetResolution(width, height);
	Engine.WindowManager.Apply();

	RECT rcClient;
	GetClientRect(Engine.WindowManager.GetHandle(), &rcClient);

	RHI_PresentationParams params;
	params.BackBufferWidth = rcClient.right - rcClient.left;
	params.BackBufferHeight = rcClient.bottom - rcClient.top;
	params.Windowed = bWindowed;
	params.BackBufferFormat = RHI_Format::RGBA8_UNORM;
	params.DepthStencilFormat = RHI_Format::D24_UNORM_S8_UINT;
	params.BackBufferCount = 1;
	params.SyncInterval = (presentInterval == 0) ? 0 : 1;
	params.FullscreenRefreshHz = 60;
	params.SwapEffect = RHI_SwapEffect::Discard;
	params.EnableAutoDepthStencil = true;

	if (!Engine.RHI.ResetDevice(params))
		R_ERROR("! [Device] RHI Reset failed");

	// --- 3. Реальные размеры ---
	dwWidth = Engine.RHI->GetBackBufferWidth();
	dwHeight = Engine.RHI->GetBackBufferHeight();
	Engine.WindowManager.UpdateSize(dwWidth, dwHeight);
	fWidth_2 = float(dwWidth / 2);
	fHeight_2 = float(dwHeight / 2);

	// --- 4. Legacy: пересоздаём ресурсы ---
	RenderBackendLegacy.ResetEnd();
	Engine.ResourceManager->ResetEnd();

	if (g_pGamePersistent)
		g_pGamePersistent->Environment().bNeed_re_create_env = TRUE;

#ifndef DEDICATED_SERVER
	ShowCursor(FALSE);
#endif

	Engine.Events.DeviceReset.Process(rp_DeviceReset);

	bool b_16_after = (float)dwWidth / (float)dwHeight > (1024.0f / 768.0f + 0.01f);
	if (b_16_after != b_16_before && g_pGameLevel && g_pGameLevel->pHUD)
		g_pGameLevel->pHUD->OnScreenRatioChanged();

	Engine.DebugUI.OnResetEnd();

#ifdef DEBUG
	_SHOW_REF("*ref +CRenderDevice::ResetTotal: DeviceREF:", Engine.RHI.GetRawRHI() ? Engine.RHI.GetRawRHI()->GetDeviceHandle() : nullptr);
#endif
}

bool CRenderDevice::NeedReset() const
{
	return Engine.RHI.NeedReset();
}

void CRenderDevice::SetNearer(BOOL enabled)
{
	if(enabled && !m_bNearer)
	{
		m_bNearer = TRUE;
		Engine.RenderView.Project._43 -= EPS_L;
	}
	else if(!enabled && m_bNearer)
	{
		m_bNearer = FALSE;
		Engine.RenderView.Project._43 += EPS_L;
	}
	RenderBackendLegacy.SetTransformProject(Engine.RenderView.Project);
}
