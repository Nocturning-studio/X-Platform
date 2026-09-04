#include "stdafx.h"
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
#include "Render.h"
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

void CRenderDevice::Begin()
{
#ifndef DEDICATED_SERVER
    if (RenderBackend.NeedReset())
        Reset();

    RenderBackend.OnFrameBegin();

    Engine.DebugUI.OnFrameBegin();

    g_bRendering = TRUE;
#endif
}

void CRenderDevice::End(void)
{
#ifndef DEDICATED_SERVER
    PROFILE_FUNCTION();

    VERIFY(RenderBackend.GetDevice());

    g_bRendering = FALSE;
    RenderBackend.OnFrameEnd();
    Engine.DebugUI.OnFrameEnd();
    Memory.dbg_check();

    if (IsIconic(Engine.WindowManager.GetHandle()))
        return;

    Engine.Statistic->RenderPresentation.Begin();
    RenderBackend.Present();
    Engine.Statistic->RenderPresentation.End();
#endif
}

int g_frametime = 166;

void CRenderDevice::RenderFrame()
{
    PROFILE_FUNCTION();

    if (!b_is_Active)
        return;

    Engine.Statistic->RenderTOTAL_Real.FrameStart();
    Engine.Statistic->RenderTOTAL_Real.Begin();

    Begin();

    Engine.Events.Render.Process(rp_Render);

    if (psDeviceFlags.test(rsCameraPos) || psDeviceFlags.test(rsStatistic) || Engine.Statistic->errors.size())
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
    if (bOn)
    {
        if (!Paused())
            bShowPauseString = TRUE;

        if (bTimer && g_pGamePersistent->CanBePaused())
            g_pauseMngr.Pause(TRUE);

        if (bSound)
        {
            snd_emitters_ = ::Sound->pause_emitters(true);
#ifdef DEBUG
            Log("snd_emitters_[true]", snd_emitters_);
#endif
        }
    }
    else
    {
        if (bTimer && g_pauseMngr.Paused())
            g_pauseMngr.Pause(FALSE);

        if (bSound)
        {
            if (snd_emitters_ > 0)
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

    RenderBackend.Create(Engine.WindowManager.GetHandle());

    dwWidth = RenderBackend.m_DevPP.BackBufferWidth;
    dwHeight = RenderBackend.m_DevPP.BackBufferHeight;
    Engine.WindowManager.UpdateSize(dwWidth, dwHeight);
    fWidth_2 = float(dwWidth / 2);
    fHeight_2 = float(dwHeight / 2);

    Memory.mem_compact();

    b_is_Ready = TRUE;

    RenderBackend.OnDeviceCreate();

    string_path fname;
    FS.update_path(fname, "$game_data$", "shaders.xr");
    Engine.ResourceManager->OnDeviceCreate(fname);
    Engine.Statistic->OnDeviceCreate();

#ifndef DEDICATED_SERVER
    m_WireShader.create("hud\\crosshair");
    m_SelectionShader.create("hud\\crosshair");
    DU.OnDeviceCreate();
#endif
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
    RenderBackend.DeleteResources();
    Engine.ResourceManager->OnDeviceDestroy(FALSE);
    RenderBackend.OnDeviceDestroy();

    Memory.mem_compact();

    RenderBackend.Destroy();
}

void CRenderDevice::Reset()
{
    Engine.DebugUI.OnResetBegin();

#ifdef DEBUG
    _SHOW_REF("*ref -CRenderDevice::ResetTotal: DeviceREF:", RenderBackend.GetDevice());
#endif
    bool b_16_before = (float)dwWidth / (float)dwHeight > (1024.0f / 768.0f + 0.01f);

    ShowCursor(TRUE);

    RenderBackend.reset_begin();
    Engine.ResourceManager->reset_begin();
    Memory.mem_compact();
    RenderBackend.Reset();
    dwWidth = Engine.WindowManager.GetWidth();
    dwHeight = Engine.WindowManager.GetHeight();
    fWidth_2 = float(dwWidth / 2);
    fHeight_2 = float(dwHeight / 2);
    Engine.ResourceManager->reset_end();

    if (g_pGamePersistent)
        g_pGamePersistent->Environment().bNeed_re_create_env = TRUE;

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
    _SHOW_REF("*ref +CRenderDevice::ResetTotal: DeviceREF:", RenderBackend.GetDevice());
#endif
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
