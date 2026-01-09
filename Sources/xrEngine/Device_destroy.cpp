#include "stdafx.h"

#include "ResourceManager.h"
#include "render.h"
#include "IGame_Persistent.h"
#include "debug_ui.h"

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
	u32 tm_start = TimerAsync();
	// if (g_pGamePersistent)
	//{
	//.		g_pGamePersistent->Environment().OnDeviceDestroy();
	//}

	RenderBackend.reset_begin();

	Resources->reset_begin();
	Memory.mem_compact();
	HW.Reset(m_hWnd);
	dwWidth = HW.DevPP.BackBufferWidth;
	dwHeight = HW.DevPP.BackBufferHeight;
	fWidth_2 = float(dwWidth / 2);
	fHeight_2 = float(dwHeight / 2);
	Resources->reset_end();

	if (g_pGamePersistent)
	{
		g_pGamePersistent->Environment().bNeed_re_create_env = TRUE;
	}
	_SetupStates();
	//if (precache)
	//	PreCache(20);
	u32 tm_end = TimerAsync();
	Msg("*** RESET [%d ms]", tm_end - tm_start);

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
