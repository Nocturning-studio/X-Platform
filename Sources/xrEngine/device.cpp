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

ENGINE_API CRenderDevice Device;
ENGINE_API BOOL g_bRendering = FALSE;

BOOL g_bLoaded = FALSE;
ref_light precache_light = 0;
/////////////////////////////////////
void ProcessLoading(RP_FUNC* f)
{
	Device.seqFrame.Process(f);
	g_bLoaded = TRUE;
}

BOOL CRenderDevice::Begin()
{
#ifndef DEDICATED_SERVER
	OPTICK_EVENT("CRenderDevice::Begin");

	HW.Validate();
	HRESULT _hr = HW.pDevice->TestCooperativeLevel();
	if (FAILED(_hr))
	{
		// Check if the device is ready to be reset
		if (D3DERR_DEVICENOTRESET == _hr)
		{
			Reset();
		}
	}

	DebugUI->OnFrameBegin();

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

	DebugUI->OnFrameEnd();

	BOOL needsPresent = TRUE;

	// Не делаем Present если:
	// 1. Мы в режиме precache
	// 2. Нет активных изменений
	// 3. Окно минимизировано
	if (dwPrecacheFrame || !b_is_Active || IsIconic(m_hWnd))
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

	// Startup timers and calculate timer delta
	dwTimeGlobal = 0;
	Timer_MM_Delta = 0;
	{
		u32 time_mm = timeGetTime();
		while (timeGetTime() == time_mm)
			; // wait for next tick
		u32 time_system = timeGetTime();
		u32 time_local = TimerAsync();
		Timer_MM_Delta = time_system - time_local;
	}

	mt_bMustExit = FALSE;

	seqAppStart.Process(rp_AppStart);
}

void CRenderDevice::Update()
{
	OPTICK_EVENT("CRenderDevice::Update");

	// 1. Считаем кадры
	dwFrame++;

	// 2. Считаем Дельту Времени (Time Delta)
	dwTimeContinual = TimerMM.GetElapsed_ms();

	if (psDeviceFlags.test(rsConstantFPS))
	{
		// Фиксированный шаг (для отладки)
		fTimeDelta = 0.020f;
		fTimeGlobal += 0.020f;
		dwTimeDelta = 20;
		dwTimeGlobal += 20;
	}
	else
	{
		// Реальный таймер
		float fPreviousFrameTime = Timer.GetElapsed_sec();
		Timer.Start();

		// Сглаживание дельты (чтобы физику не трясло)
		fTimeDelta = 0.1f * fTimeDelta + 0.9f * fPreviousFrameTime;

		if (fTimeDelta > .1f)
			fTimeDelta = .1f; // Ограничение мин. FPS (чтобы не проваливаться сквозь пол)
		if (Paused())
			fTimeDelta = 0.0f;

		// Глобальное время
		fTimeGlobal = TimerGlobal.GetElapsed_sec();
		u32 _old_global = dwTimeGlobal;
		dwTimeGlobal = TimerGlobal.GetElapsed_ms();
		dwTimeDelta = dwTimeGlobal - _old_global;
	}

	// 3. Выполняем игровую логику (seqFrame)
	Statistic->EngineTOTAL.Begin();

	if (!g_bLoaded)
		ProcessLoading(rp_Frame);
	else
		seqFrame.Process(rp_Frame); // <-- Тут обновляются Акторы, Физика, Скрипты

	Statistic->EngineTOTAL.End();
}

void CRenderDevice::Render()
{
	OPTICK_EVENT("CRenderDevice::Render");

	// Проверка готовности должна быть снаружи или тут (safety check)
	if (!b_is_Ready)
		return;

	// 1. Очистка экрана
	CHK_DX(HW.pDevice->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1, 0));

	// 2. Сбор статистики
	g_bEnableStatGather = psDeviceFlags.test(rsStatistic) ? TRUE : FALSE;

	// 3. Логика камеры (Precache)
	// (Она тут, потому что влияет на View Matrix перед рендером)
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

	// 4. Расчет Матриц
	mFullTransform.mul(mProject, mView);
	RenderBackend.set_xform_view(mView);
	RenderBackend.set_xform_project(mProject);
	D3DXMatrixInverse((D3DXMATRIX*)&mInvFullTransform, 0, (D3DXMATRIX*)&mFullTransform);

	// 5. Синхронизация потоков (разрешаем вторичному потоку работать)
	syncProcessFrame.Set();

#ifndef DEDICATED_SERVER
	Statistic->RenderTOTAL_Real.FrameStart();
	Statistic->RenderTOTAL_Real.Begin();

	// 6. Собственно Отрисовка
	if (b_is_Active && Begin()) // BeginScene
	{
		DebugUI->DrawUI(); // ImGui / Debug UI

		seqRender.Process(rp_Render); // <-- Главный рендер игры (Уровни, HUD)

		if (psDeviceFlags.test(rsCameraPos) || psDeviceFlags.test(rsStatistic) || Statistic->errors.size())
			Statistic->Show();

		End(); // EndScene + Present (если нужно)
	}

	Statistic->RenderTOTAL_Real.End();
	Statistic->RenderTOTAL_Real.FrameEnd();
	Statistic->RenderTOTAL.accum = Statistic->RenderTOTAL_Real.accum;
#endif

	// 7. Сохранение позиции камеры для следующего кадра
	vCameraPosition_saved = vCameraPosition;
	mFullTransform_saved = mFullTransform;

	// 8. Ждем завершения вторичного потока перед следующим кадром
	syncFrameDone.Wait();
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

void CRenderDevice::time_factor(const float& time_factor)
{
	Timer.time_factor(time_factor);
	TimerGlobal.time_factor(time_factor);
	psTimeFactor = time_factor;
}

IC const float& CRenderDevice::time_factor() const
{
	VERIFY(Timer.time_factor() == TimerGlobal.time_factor());
	return (Timer.time_factor());
}

void CRenderDevice::stop_time()
{
	Timer.time_factor(0.0001f);
	TimerGlobal.time_factor(0.0001f);
	psTimeFactor = 0.0001f;
}