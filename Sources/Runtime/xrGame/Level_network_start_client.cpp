#include "stdafx.h"
#include "../xrEngine/resourcemanager.h"
#include "HUDmanager.h"
#include "PHdynamicdata.h"
#include "Physics.h"
#include "level.h"
#include "../xrEngine/Engine.h"
#include "../xrEngine/igame_persistent.h"
#include "PhysicsGamePars.h"
#include "ai_space.h"
#include "../xrEngine/xr_ioconsole.h"
#include "../xrEngine/LevelLoadingScreen.h"

extern pureFrame* g_pNetProcessor;

BOOL CLevel::net_Start_client(LPCSTR options)
{
	return FALSE;
}
#include "string_table.h"
bool CLevel::net_start_client1()
{
	Engine.LoadingScreen->Show();
	// name_of_server
	string64 name_of_server = "";
	//	strcpy						(name_of_server,*m_caClientOptions);
	if (strchr(*m_caClientOptions, '/'))
		strncpy(name_of_server, *m_caClientOptions, strchr(*m_caClientOptions, '/') - *m_caClientOptions);

	if (strchr(name_of_server, '/'))
		*strchr(name_of_server, '/') = 0;

	// Startup client
	string256 temp;
	sprintf_s(temp, "%s %s", CStringTable().translate("st_client_connecting_to").c_str(), name_of_server);

	g_pGamePersistent->LoadTitle(temp);
	return true;
}

#include "xrServer.h"

bool CLevel::net_start_client2()
{
	if (psNET_direct_connect)
	{
		Server->create_direct_client();
	}

	connected_to_server = Connect2Server(*m_caClientOptions);

	return true;
}

bool CLevel::net_start_client3()
{
	if (connected_to_server)
	{
		LPCSTR level_name = NULL;
		if (psNET_direct_connect)
		{
			level_name = ai().get_alife() ? *name() : Server->level_name(Server->GetConnectOptions()).c_str();
		}
		else
			level_name = ai().get_alife() ? *name() : net_SessionName();

		// Determine internal level-ID
		int level_id = Engine.LevelManager.GetLevelID(level_name);
		if (level_id < 0)
		{
			Disconnect();
			Engine.LoadingScreen->Hide();
			connected_to_server = FALSE;
			m_name = level_name;
			m_connect_server_err = xrServer::ErrNoLevel;
			return false;
		}
		Engine.LevelManager.SetLevel(level_id);
		m_name = level_name;
		// Load level
		R_ASSERT2(Load(level_id), "Loading failed.");
	}
	return true;
}

bool CLevel::net_start_client4()
{
	if (connected_to_server)
	{
		// Begin spawn
		g_pGamePersistent->LoadTitle("st_client_spawning");

		// Send physics to single or multithreaded mode
		LoadPhysicsGameParams();
		ph_world = xr_new<CPHWorld>();
		ph_world->Create();

		// Send network to single or multithreaded mode
		// *note: release version always has "mt_*" enabled
		Engine.ThreadManager.LegacyFrameMT.Remove(g_pNetProcessor);
		Engine.Events.Frame.Remove(g_pNetProcessor);
		Engine.ThreadManager.LegacyFrameMT.Add(g_pNetProcessor, REG_PRIORITY_HIGH);

		if (!psNET_direct_connect)
		{
			// Waiting for connection/configuration completition
			CTimer timer_sync;
			timer_sync.Start();
			while (!net_isCompleted_Connect())
				Sleep(5);
			Msg("* connection sync: %d ms", timer_sync.GetElapsed_ms());
			while (!net_isCompleted_Sync())
			{
				ClientReceive();
				Sleep(5);
			}
		}

		while (!game_configured)
		{
			ClientReceive();
			if (Server)
				Server->Update();
			Sleep(5);
		}
	}
	return true;
}

bool CLevel::net_start_client5()
{
	if (connected_to_server)
	{
		// Textures
		if (!g_dedicated_server)
		{
			g_pGamePersistent->LoadTitle("st_loading_textures");
			Engine.ResourceManager->DeferredLoad(FALSE);
			Engine.ResourceManager->DeferredUpload();
			pHUD->Load();
			LL_CheckTextures();
		}
	}
	return true;
}

bool CLevel::net_start_client6()
{
	if (connected_to_server)
	{
		// Sync
		if (g_hud)
			g_hud->OnConnected();

		g_pGamePersistent->LoadTitle("st_client_synchronising");
		net_start_result_total = TRUE;
	}
	else
	{
		net_start_result_total = FALSE;
	}

	Engine.LoadingScreen->Hide();
	return true;
}
