#include "stdafx.h"
#include "GameStateManager.h"
#include "../xrDiscordAPI/DiscordAPI.h"
#include "x_ray.h"
#include "igame_level.h"
#include "igame_persistent.h"
#include "xr_ioconsole.h"
#include <ISpatial.h>

// -------------------------------

void CGameStateManager::Initialize()
{
	// Подписываемся на события
	eQuit = Engine.Event.Handler_Attach("KERNEL:quit", this);
	eStart = Engine.Event.Handler_Attach("KERNEL:start", this);
	eStartLoad = Engine.Event.Handler_Attach("KERNEL:load", this);
	eDisconnect = Engine.Event.Handler_Attach("KERNEL:disconnect", this);

	// Подписываемся на обновление кадра
	// Приоритет ставим пониже, чтобы это происходило после рендера UI, но перед физикой или наоборот
	Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 1000);
}

void CGameStateManager::Destroy()
{
	// Отписываемся
	Device.seqFrame.Remove(this);

	Engine.Event.Handler_Detach(eDisconnect, this);
	Engine.Event.Handler_Detach(eStartLoad, this);
	Engine.Event.Handler_Detach(eStart, this);
	Engine.Event.Handler_Detach(eQuit, this);
}

void CGameStateManager::OnEvent(EVENT E, u64 P1, u64 P2)
{
	if (E == eQuit)
	{
		PostQuitMessage(0);
	}
	else if (E == eStart)
	{
		LPSTR op_server = LPSTR(P1);
		LPSTR op_client = LPSTR(P2);

		// Вся сложная логика ушла в LevelManager
		Engine.LevelManager.StartGame(op_server, op_client);

		xr_free(op_server);
		xr_free(op_client);
	}
	else if (E == eDisconnect)
	{
		// Останавливаем игру через менеджер
		Engine.LevelManager.StopGame();

		// Логика "показать меню" остается здесь, так как это управление потоком событий.
		// LevelManager не должен знать про очередь событий движка (Peek).
		if ((FALSE == Engine.Event.Peek("KERNEL:quit")) && (FALSE == Engine.Event.Peek("KERNEL:start")))
		{
			Console->Execute("main_menu off");
			Console->Execute("main_menu on");
		}
	}
}

void CGameStateManager::OnFrame()
{
	// Обработка очереди событий движка
	Engine.Event.OnFrame();

	// Обновление пространственных баз данных (Spatial DB)
	if (g_SpatialSpace)
		g_SpatialSpace->update();
	if (g_SpatialSpacePhysic)
		g_SpatialSpacePhysic->update();

	// Звуковые события уровня
	if (g_pGameLevel)
		g_pGameLevel->SoundEvent_Dispatch();

	// Discord Rich Presence
	if (!g_dedicated_server)
		DiscordAPI.Update();

	// Консоль
	if (g_dedicated_server)
		Console->OnFrame();
}
