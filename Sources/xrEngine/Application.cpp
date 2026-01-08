////////////////////////////////////////////////////////////////////////////////
// Created: 14.01.2025
// Author: NSDeathman
// Refactored code: Application class realization
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Application.h"
#include "Optick_Capture.h"
#include "igame_level.h"
#include "igame_persistent.h"
#include "xr_input.h"
#include "xr_ioconsole.h"
#include "x_ray.h"
#include "std_classes.h"
#include "GameFont.h"
#include "resource.h"
#include "LightAnimLibrary.h"
#include "ispatial.h"
#include "Text_Console.h"
#include <process.h>
#include "xr_ioconsole.h"
#include "../xrDiscordAPI/DiscordAPI.h"

struct _SoundProcessor : public pureFrame
{
	virtual void OnFrame()
	{
		Device.Statistic->Sound.Begin();
		::Sound->update(Device.vCameraPosition, Device.vCameraDirection, Device.vCameraTop);
		Device.Statistic->Sound.End();
	}
} SoundProcessor;

CApplication::CApplication()
{
	OPTICK_EVENT("CApplication::CApplication");

	eQuit = Engine.Event.Handler_Attach("KERNEL:quit", this);
	eStart = Engine.Event.Handler_Attach("KERNEL:start", this);
	eStartLoad = Engine.Event.Handler_Attach("KERNEL:load", this);
	eDisconnect = Engine.Event.Handler_Attach("KERNEL:disconnect", this);

	Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 1000);

	if (psDeviceFlags.test(mtSound))
		Device.seqFrameMT.Add(&SoundProcessor);
	else
		Device.seqFrame.Add(&SoundProcessor);

	DiscordAPI.Init();

#ifdef ENABLE_PROFILING
	OptickCapture.Initialize();
#endif
}

CApplication::~CApplication()
{
	Console->Hide();

#ifdef ENABLE_PROFILING
	OptickCapture.Destroy();
#endif

	Device.seqFrameMT.Remove(&SoundProcessor);
	Device.seqFrame.Remove(&SoundProcessor);
	Device.seqFrame.Remove(this);

	Engine.Event.Handler_Detach(eDisconnect, this);
	Engine.Event.Handler_Detach(eStartLoad, this);
	Engine.Event.Handler_Detach(eStart, this);
	Engine.Event.Handler_Detach(eQuit, this);
}

void CApplication::OnEvent(EVENT E, u64 P1, u64 P2)
{
	OPTICK_EVENT("CApplication::OnEvent");

	if (E == eQuit)
	{
		PostQuitMessage(0);
	}
	else if (E == eStart)
	{
		LPSTR op_server = LPSTR(P1);
		LPSTR op_client = LPSTR(P2);

		// ... (ëîãèêà main_menu) ...
		{
			Console->Execute("main_menu off");
			Console->Hide();

			g_pGamePersistent->PreStart(op_server);
			g_pGameLevel = (IGame_Level*)NEW_INSTANCE(CLSID_GAME_LEVEL);

			// --- ÄÅËÅÃÈÐÓÅÌ ÇÀÃÐÓÇÊÓ ---
			Engine.LoadingScreen.Show();
			// ----------------------------

			Msg("\nStart level loading...");
			g_pGamePersistent->Start(op_server);
			g_pGameLevel->net_Start(op_server, op_client);

			// --- ÄÅËÅÃÈÐÓÅÌ ÇÀÂÅÐØÅÍÈÅ ---
			Engine.LoadingScreen.Hide();
			// -----------------------------
		}
		xr_free(op_server);
		xr_free(op_client);
	}
	else if (E == eDisconnect)
	{
		// ... (êîä äèñêîííåêòà áåç èçìåíåíèé) ...
		if (g_pGameLevel)
		{
			g_pGameLevel->net_Stop();
			DEL_INSTANCE(g_pGameLevel);
			if ((FALSE == Engine.Event.Peek("KERNEL:quit")) && (FALSE == Engine.Event.Peek("KERNEL:start")))
			{
				Console->Execute("main_menu off");
				Console->Execute("main_menu on");
			}
		}
		g_pGamePersistent->Disconnect();
	}
}

void CApplication::OnFrame()
{
	OPTICK_EVENT("CApplication::OnFrame");
	Engine.Event.OnFrame();
	g_SpatialSpace->update();
	g_SpatialSpacePhysic->update();
	if (g_pGameLevel)
		g_pGameLevel->SoundEvent_Dispatch();
	if (!g_dedicated_server)
		DiscordAPI.Update();
	if (g_dedicated_server)
		Console->OnFrame();
#ifdef ENABLE_PROFILING
	OptickCapture.OnFrame();
#endif
}
