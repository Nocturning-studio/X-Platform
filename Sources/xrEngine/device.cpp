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

#include "LevelLoadingScreen.h"

#include "igame_level.h"

ENGINE_API CRenderDevice Device;
ENGINE_API BOOL g_bRendering = FALSE;

ref_light precache_light = 0;
/////////////////////////////////////
BOOL CRenderDevice::Begin()
{
#ifndef DEDICATED_SERVER
	HRESULT _hr = HW.GetDevice()->TestCooperativeLevel();
	if (FAILED(_hr) && D3DERR_DEVICENOTRESET == _hr)
		Reset();

	Engine.DebugUI.OnFrameBegin();

	CHK_DX(HW.GetDevice()->BeginScene());

	RenderBackend.OnFrameBegin();
	RenderBackend.set_CullMode(CULL_BACKFACE);

	g_bRendering = TRUE;
#endif
	return TRUE;
}

void CRenderDevice::Clear()
{
	CHK_DX(HW.GetDevice()->Clear(0, 0, D3DCLEAR_ZBUFFER | (psDeviceFlags.test(rsClearBB) ? D3DCLEAR_TARGET : 0) | D3DCLEAR_STENCIL, D3DCOLOR_XRGB(0, 0, 0), 1, 0));
}

void Present()
{
	PROFILE_FUNCTION();

	Engine.Statistic->RenderPresentation.Begin();

	HRESULT _hr = HW.GetDevice()->PresentEx(NULL, NULL, NULL, NULL, NULL);

	Engine.Statistic->RenderPresentation.End();
}

void CRenderDevice::End(void)
{
#ifndef DEDICATED_SERVER
	PROFILE_FUNCTION();

	VERIFY(HW.GetDevice());

	if (dwPrecacheFrame)
	{
		::Sound->set_master_volume(psSoundVFactor);
		dwPrecacheFrame--;
		Engine.LoadingScreen->ForceRender();
		if (0 == dwPrecacheFrame)
		{
			Gamma.Update();

			if (precache_light)
				precache_light->set_active(false);
			if (precache_light)
				precache_light.destroy();
			::Sound->set_master_volume(psSoundVFactor);
			Engine.ResourceManager->DestroyNecessaryTextures();
			Memory.mem_compact();
			Msg("* MEMORY USAGE: %d K", Memory.mem_usage() / 1024);
		}
	}

	g_bRendering = FALSE;
	// end scene
	RenderBackend.OnFrameEnd();
	Memory.dbg_check();
	CHK_DX(HW.GetDevice()->EndScene());

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
		Engine.Statistic->RenderTOTAL_Real.End();
		Engine.Statistic->RenderTOTAL_Real.FrameEnd();
		Present();
	}
#endif
}

void CRenderDevice::PreCache(u32 amount)
{
#ifdef DEDICATED_SERVER
	amount = 0;
#endif
	//Msg("* LIGHT PRECACHE: start for %d...", amount);
	dwPrecacheFrame = dwPrecacheTotal = amount;
	if (amount && !precache_light && g_pGameLevel)
	{
		precache_light = ::Render->light_create();
		precache_light->set_shadow(false);
		precache_light->set_position(Engine.RenderView.Position);
		precache_light->set_color(255, 255, 255);
		precache_light->set_range(5.0f);
		precache_light->set_active(true);
	}
}

void CRenderDevice::PreCache()
{
	// Расчет угла вращения
	float factor = float(dwPrecacheFrame) / float(dwPrecacheTotal);
	float angle = PI_MUL_2 * factor;

	// Расчет векторов
	fvec3 dir, top, right;
	dir.set(_sin(angle), 0, _cos(angle));
	dir.normalize();
	top.set(0, 1, 0);
	right.crossproduct(top, dir);

	// Установка в RenderView (вместо mView.build_camera_dir)
	// Позицию берем текущую, какая есть в RenderView
	Engine.RenderView.SetupView(Engine.RenderView.Position, dir, top);
}

int g_frametime = 166;

void ProcessLoading(RP_FUNC* f)
{
	Engine.Events.Frame.Process(rp_Frame);
	Engine.SetLoaded();
}

void CRenderDevice::PrepareEventLoop()
{
	Engine.SetUnloaded();
	CHK_DX(HW.GetDevice()->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1, 0));
}

void CRenderDevice::RenderFrame()
{
	PROFILE_FUNCTION();

	if (!b_is_Active)
		return;

	Engine.Statistic->RenderTOTAL_Real.FrameStart();
	Engine.Statistic->RenderTOTAL_Real.Begin();

	// Begin() настраивает DX9 контекст, очищает Z-буфер
	if (Begin())
	{
		Engine.DebugUI.DrawUI();

		// Вызываем подписчиков на рендер (Level, HUD, etc)
		Engine.Events.Render.Process(rp_Render);

		// Статистика
		if (psDeviceFlags.test(rsCameraPos) || psDeviceFlags.test(rsStatistic) || Engine.Statistic->errors.size())
			Engine.Statistic->Show();

		// End() делает Present()
		End();
	}

	Engine.Statistic->RenderTOTAL_Real.End();
	Engine.Statistic->RenderTOTAL_Real.FrameEnd();
}

void CRenderDevice::EndEventLoop()
{
	Msg("Ending event loop...");

	Engine.Events.AppEnd.Process(rp_AppEnd);
}

ENGINE_API BOOL bShowPauseString = TRUE;

void CRenderDevice::Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason)
{
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
	u16 fActive = LOWORD(wParam);
	BOOL fMinimized = (BOOL)HIWORD(wParam);
	BOOL bActive = ((fActive != WA_INACTIVE) && (!fMinimized)) ? TRUE : FALSE;

	if (bActive != Device.b_is_Active)
	{
		Device.b_is_Active = bActive;

		if (Device.b_is_Active)
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

void CRenderDevice::_SetupStates()
{
	for (u32 i = 0; i < RHI()->GetDeviceCaps().MaxSimultaneousTextures; i++)
	{
		float fBias = 1.0f;
		CHK_DX(HW.GetDevice()->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, 4));
		CHK_DX(HW.GetDevice()->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, *((LPDWORD)(&fBias))));
		CHK_DX(HW.GetDevice()->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
		CHK_DX(HW.GetDevice()->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
		CHK_DX(HW.GetDevice()->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR));
	}
	RenderBackend.SetRenderState(D3DRS_DITHERENABLE, TRUE);
	RenderBackend.SetRenderState(D3DRS_COLORVERTEX, TRUE);
	RenderBackend.SetRenderState(D3DRS_ZENABLE, TRUE);
	RenderBackend.SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	RenderBackend.SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	RenderBackend.SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	RenderBackend.SetRenderState(D3DRS_LOCALVIEWER, TRUE);

	RenderBackend.SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
	RenderBackend.SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);

	RenderBackend.SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
}

void CRenderDevice::_Create(LPCSTR shName)
{
	Memory.mem_compact();

	// after creation
	b_is_Ready = TRUE;
	_SetupStates();

	// Signal everyone - device created
	RenderBackend.OnDeviceCreate();
	Gamma.Update();
	Engine.ResourceManager->OnDeviceCreate(shName);
	::Render->create();
	Engine.Statistic->OnDeviceCreate();

#ifndef DEDICATED_SERVER
	m_WireShader.create("hud\\crosshair");
	m_SelectionShader.create("hud\\crosshair");

	DU.OnDeviceCreate();
#endif
}

void CRenderDevice::Create()
{
	if (b_is_Ready)
		return; // prevent double call
	Engine.Statistic = xr_new<CStats>();
	Engine.Statistic->Initialize();
	Log("\nStarting RENDER device...");

#ifdef _EDITOR
	psCurrentVidMode[0] = dwWidth;
	psCurrentVidMode[1] = dwHeight;
#endif

	HW.CreateDevice(Engine.WindowManager.GetHandle());
	dwWidth = HW.GetDevPP().BackBufferWidth;
	dwHeight = HW.GetDevPP().BackBufferHeight;
	Engine.WindowManager.UpdateSize(dwWidth, dwHeight);
	fWidth_2 = float(dwWidth / 2);
	fHeight_2 = float(dwHeight / 2);

	string_path fname;
	FS.update_path(fname, "$game_data$", "shaders.xr");

	//////////////////////////////////////////////////////////////////////////
	_Create(fname);

	PreCache(30);
}

void CRenderDevice::_Destroy(BOOL bKeepTextures)
{
	DU.OnDeviceDestroy();
	m_WireShader.destroy();
	m_SelectionShader.destroy();

	// before destroy
	b_is_Ready = FALSE;
	Engine.Statistic->OnDeviceDestroy();
	::Render->destroy();
	RenderBackend.DeleteResources();
	Engine.ResourceManager->OnDeviceDestroy(bKeepTextures);
	RenderBackend.OnDeviceDestroy();

	Memory.mem_compact();
}

void CRenderDevice::Destroy(void)
{
	if (!b_is_Ready)
		return;

	Log("\nDestroying Direct3D...");

	ShowCursor(TRUE);

	_Destroy(FALSE);

	// real destroy
	HW.DestroyDevice();
}

#include "IGame_Level.h"
#include "CustomHUD.h"
void CRenderDevice::Reset(bool precache)
{
	Engine.DebugUI.OnResetBegin();

#ifdef DEBUG
	_SHOW_REF("*ref -CRenderDevice::ResetTotal: DeviceREF:", HW.GetDevice());
#endif // DEBUG
	bool b_16_before = (float)dwWidth / (float)dwHeight > (1024.0f / 768.0f + 0.01f);

	ShowCursor(TRUE);

	RenderBackend.reset_begin();

	Engine.ResourceManager->reset_begin();
	Memory.mem_compact();
	HW.Reset();
	_SetupStates();
	dwWidth = Engine.WindowManager.GetWidth();
	dwHeight = Engine.WindowManager.GetHeight();
	fWidth_2 = float(dwWidth / 2);
	fHeight_2 = float(dwHeight / 2);
	Engine.ResourceManager->reset_end();

	if (g_pGamePersistent)
	{
		g_pGamePersistent->Environment().bNeed_re_create_env = TRUE;
	}

#ifndef DEDICATED_SERVER
	ShowCursor(FALSE);
#endif

	Engine.Events.DeviceReset.Process(rp_DeviceReset);

	RenderBackend.reset_end();

	bool b_16_after = (float)dwWidth / (float)dwHeight > (1024.0f / 768.0f + 0.01f);
	if (b_16_after != b_16_before && g_pGameLevel && g_pGameLevel->pHUD)
		g_pGameLevel->pHUD->OnScreenRatioChanged();

	Engine.DebugUI.OnResetEnd();

#ifdef DEBUG
	_SHOW_REF("*ref +CRenderDevice::ResetTotal: DeviceREF:", HW.GetDevice());
#endif // DEBUG
}

void CRenderDevice::Initialize()
{
	Msg("Initializing Render Device...");

	// Save window properties
	m_dwWindowStyle = GetWindowLong(Engine.WindowManager.GetHandle(), GWL_STYLE);
	GetWindowRect(Engine.WindowManager.GetHandle(), &m_rcWindowBounds);
	GetClientRect(Engine.WindowManager.GetHandle(), &m_rcWindowClient);
}

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

void CRenderDevice::SetNearer(BOOL enabled)
{
	if (enabled && !m_bNearer)
	{
		m_bNearer = TRUE;
		Engine.RenderView.Project._43 -= EPS_L;
	}
	else if (!enabled && m_bNearer)
	{
		m_bNearer = FALSE;
		Engine.RenderView.Project._43 += EPS_L;
	}
	RenderBackend.set_transform_project(Engine.RenderView.Project);
}
