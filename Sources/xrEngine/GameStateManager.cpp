#include "stdafx.h"
#include "GameStateManager.h"
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
#include "../xrDiscordAPI/DiscordAPI.h"

void CGameStateManager::Initialize()
{
	eQuit = Engine.Event.Handler_Attach("KERNEL:quit", this);
	eStart = Engine.Event.Handler_Attach("KERNEL:start", this);
	eStartLoad = Engine.Event.Handler_Attach("KERNEL:load", this);
	eDisconnect = Engine.Event.Handler_Attach("KERNEL:disconnect", this);

	Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 1000);

	DiscordAPI.Init();

#ifdef ENABLE_PROFILING
	OptickCapture.Initialize();
#endif
}

void CGameStateManager::Destroy()
{
	Console->Hide();

#ifdef ENABLE_PROFILING
	OptickCapture.Destroy();
#endif

	Device.seqFrame.Remove(this);

	Engine.Event.Handler_Detach(eDisconnect, this);
	Engine.Event.Handler_Detach(eStartLoad, this);
	Engine.Event.Handler_Detach(eStart, this);
	Engine.Event.Handler_Detach(eQuit, this);
}

void CGameStateManager::OnEvent(EVENT E, u64 P1, u64 P2)
{
	OPTICK_EVENT("CGameStateManager::OnEvent");

	if (E == eQuit)
	{
		PostQuitMessage(0);
	}
	else if (E == eStart)
	{
		LPSTR op_server = LPSTR(P1);
		LPSTR op_client = LPSTR(P2);

		// ... (логика main_menu) ...
		{
			Console->Execute("main_menu off");
			Console->Hide();

			g_pGamePersistent->PreStart(op_server);
			g_pGameLevel = (IGame_Level*)NEW_INSTANCE(CLSID_GAME_LEVEL);

			// --- ДЕЛЕГИРУЕМ ЗАГРУЗКУ ---
			Engine.LoadingScreen.Show();
			// ----------------------------

			Msg("\nStart level loading...");
			g_pGamePersistent->Start(op_server);
			g_pGameLevel->net_Start(op_server, op_client);

			// --- ДЕЛЕГИРУЕМ ЗАВЕРШЕНИЕ ---
			Engine.LoadingScreen.Hide();
			// -----------------------------
		}
		xr_free(op_server);
		xr_free(op_client);
	}
	else if (E == eDisconnect)
	{
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

void CGameStateManager::OnFrame()
{
	OPTICK_EVENT("CGameStateManager::OnFrame");

	// Обработка событий
	Engine.Event.OnFrame();

	// Обновление пространственных баз
	g_SpatialSpace->update();
	g_SpatialSpacePhysic->update();

	// Звуковые события уровня
	if (g_pGameLevel)
		g_pGameLevel->SoundEvent_Dispatch();

	// Discord API update
	if (!g_dedicated_server)
		DiscordAPI.Update();

	// Для выделенного сервера обновление консоли здесь
	if (g_dedicated_server)
		Console->OnFrame();

#ifdef ENABLE_PROFILING
	OptickCapture.OnFrame();
#endif
}
