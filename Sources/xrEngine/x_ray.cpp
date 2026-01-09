////////////////////////////////////////////////////////////////////////////////
// Authors: 
//			Oles (Oles Shishkovtsov)
//			AlexMX (Alexander Maksimchuk)
// 
// Engine class realization
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
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
#include "build_identificator.h"
#include "debug_ui.h"
#include "LogoWindow.h"
//////////////////////////////////////////////////////////////////////////
ENGINE_API CDebugUI* DebugUI = nullptr;
//////////////////////////////////////////////////////////////////////////
#define TRIVIAL_ENCRYPTOR_DECODER
#include "trivial_encryptor.h"
#include <xrCPU_Pipe.h>
//////////////////////////////////////////////////////////////////////////
extern CRenderDevice Device;
//////////////////////////////////////////////////////////////////////////
xrDispatchTable PSGP;
//////////////////////////////////////////////////////////////////////////
ENGINE_API CInifile* pGameIni = NULL;
//////////////////////////////////////////////////////////////////////////
ENGINE_API bool g_dedicated_server = false;
//////////////////////////////////////////////////////////////////////////
void __cdecl dummy(void){};
//////////////////////////////////////////////////////////////////////////
extern void msCreate(LPCSTR name);
extern void __cdecl xrBind_PSGP(xrDispatchTable* T, DWORD dwFeatures);
//////////////////////////////////////////////////////////////////////////
struct _SoundProcessor : public pureFrame
{
	virtual void OnFrame()
	{
		Device.Statistic->Sound.Begin();
		::Sound->update(Device.vCameraPosition, Device.vCameraDirection, Device.vCameraTop);
		Device.Statistic->Sound.End();
	}
} SoundProcessor;
//////////////////////////////////////////////////////////////////////////
CXRay::CXRay()
{
	hGame = 0;
	hRender = 0;
	hTuner = 0;
	hOptick = 0;
	pCreate = 0;
	pDestroy = 0;
	tune_pause = dummy;
	tune_resume = dummy;
}

CXRay::~CXRay()
{
}

void CXRay::LoadLibraries()
{
	Msg("Initializing Engine API...");

	// render
	Msg("Initializing Renderer...");
	LPCSTR render_name = "xrRender.dll";
	Log("Loading DLL:", render_name);
	hRender = LoadLibrary(render_name);
	R_ASSERT2(hRender, "! Can't load renderer");

	// game
	{
		Msg("Initializing Game API...");

		LPCSTR g_name = "xrGame.dll";
		Msg("Loading DLL: %s", g_name);
		hGame = LoadLibrary(g_name);

		if (0 == hGame)
			R_CHK(GetLastError());

		R_ASSERT2(hGame, "Game DLL raised exception during loading or there is no game DLL at all");

		Msg("Initializing xrFactory...");
		pCreate = (Factory_Create*)GetProcAddress(hGame, "xrFactory_Create");
		R_ASSERT2(pCreate, "Error in xrFactory_Create");

		pDestroy = (Factory_Destroy*)GetProcAddress(hGame, "xrFactory_Destroy");
		R_ASSERT2(pDestroy, "Error in xrFactory_Destroy");
	}

	// vTune
	tune_enabled = FALSE;
	if (strstr(Core.Params, "-tune"))
	{
		LPCSTR g_name = "vTuneAPI.dll";
		Log("Loading DLL:", g_name);
		hTuner = LoadLibrary(g_name);
		if (0 == hTuner)
		{
			R_CHK(GetLastError());
			Msg("Intel vTune is not installed");
		}
		else
		{
			tune_enabled = TRUE;
			tune_pause = (VTPause*)GetProcAddress(hTuner, "VTPause");
			tune_resume = (VTResume*)GetProcAddress(hTuner, "VTResume");
		}
	}

#ifdef ENABLE_PROFILING
	LPCSTR g_name = "OptickCore.dll";
	Log("Loading DLL:", g_name);
	hOptick = LoadLibrary(g_name);
	if (0 == hOptick)
	{
		R_CHK(GetLastError());
		Msg("Optick is not installed");
	}
#endif

	LPCSTR DiscordAPI_name = "xrDiscordAPI.dll";
	Log("Loading DLL:", DiscordAPI_name);
	hDiscordAPI = LoadLibrary(DiscordAPI_name);
	R_ASSERT2(DiscordAPI_name, "! Can't load discord api");
}

void CXRay::UnloadLibraries()
{
	if (hGame)
	{
		FreeLibrary(hGame);
		hGame = 0;
	}
	if (hRender)
	{
		FreeLibrary(hRender);
		hRender = 0;
	}
	if (hOptick)
	{
		FreeLibrary(hOptick);
		hOptick = 0;
	}
	if (hDiscordAPI)
	{
		FreeLibrary(hDiscordAPI);
		hDiscordAPI = 0;
	}
	pCreate = 0;
	pDestroy = 0;
	Event._destroy();
	XRC.r_clear_compact();
}

void HandleComandLine()
{
	// ...command line for auto start
	{
		LPCSTR pStartup = strstr(Core.Params, "-start ");
		if (pStartup)
			Console->Execute(pStartup + 1);
	}
	{
		LPCSTR pStartup = strstr(Core.Params, "-load ");
		if (pStartup)
			Console->Execute(pStartup + 1);
	}
	if (strstr(Core.Params, "-load_last_save"))
	{
		Console->Execute("load_last_save");
	}
	if (strstr(Core.Params, "-load_last_quick_save"))
	{
		Console->Execute("load_last_quick_save");
	}

#ifdef BENCHMARK_BUILD
	R_ASSERT2(strstr(Core.Params, "-demo_play "), "For benchmark build demo file required");
#endif
}

bool CXRay::Initialize()
{
	auto Logo = xr_make_unique<LogoWindow>();
	Logo->Show();

#ifndef DEDICATED_SERVER
	Debug._initialize(false);
#else  // DEDICATED_SERVER
	Debug._initialize(true);
	g_dedicated_server = true;
#endif // DEDICATED_SERVER

	DecodeResources();

	ComputeBuildIdentificator();
	PrintBuildIdentificator();

	Core.Initialize("X-Ray Engine", "xray_engine");

	FPU::m24r();

	InitSettings();

	Msg("Initializing Engine...");

	xrBind_PSGP(&PSGP, true);

	Msg("Initializing Engine Sheduler...");
	Sheduler.Initialize();

#ifdef DEBUG
	msCreate("game");
#endif

	Device.Initialize();

	InitInput();

	InitConsole();

	LoadLibraries();

	execUserScript();
	InitSound();

	DebugUI = new CDebugUI();
	DebugUI->Initialize();

	HandleComandLine();

	Device.Create();

	LALib.OnCreate();

	GameStateManager.Initialize();

	Msg("Initializing Font Manager...");
	FontManager.Initialize();

	Msg("Scanning levels...");
	LevelManager.Scan();

	Device.seqFrameMT.Add(&SoundProcessor);

	g_pGamePersistent = (IGame_Persistent*)NEW_INSTANCE(CLSID_GAME_PERSISTANT);

	g_SpatialSpace = xr_new<ISpatial_DB>();
	g_SpatialSpacePhysic = xr_new<ISpatial_DB>();

	Memory.mem_usage();

	Logo->Hide();

	ShowWindow(Device.m_hWnd, SW_SHOWNORMAL);

	return true;
}

void CXRay::Run()
{
	Initialize();

	ProcessEventLoop();

	Destroy();
}

void CXRay::InitSettings()
{
	Msg("Initializing Settings...");

	string_path fname;
	FS.update_path(fname, "$game_config$", "system.ltx");
	pSettings = xr_new<CInifile>(fname, TRUE);
	CHECK_OR_EXIT(!pSettings->sections().empty(),
				  make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));

	FS.update_path(fname, "$game_config$", "game.ltx");
	pGameIni = xr_new<CInifile>(fname, TRUE);
	CHECK_OR_EXIT(!pGameIni->sections().empty(),
				  make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));
}

void CXRay::InitConsole()
{
	Msg("Initializing Console...");

#ifdef DEDICATED_SERVER
		Console = xr_new<CTextConsole>();
#else
		Console = xr_new<CConsole>();
#endif

	Console->Initialize();

	if (strstr(Core.Params, "-ltx "))
	{
		string64 c_name;
		(void)sscanf(strstr(Core.Params, "-ltx ") + 5, "%[^ ] ", c_name);
		strcpy_s(Console->ConfigFile, c_name);
		Msg("Execute custom game settings file: %s", c_name);
	}
	else
	{
		strcpy_s(Console->ConfigFile, sizeof(Console->ConfigFile), "user_game_settings");
		Msg("Execute game settings file: %s", Console->ConfigFile);
	}
}

void CXRay::InitInput()
{
	BOOL bCaptureInput = !strstr(Core.Params, "-i");
	if (g_dedicated_server)
		bCaptureInput = FALSE;

	pInput = xr_new<CInput>(bCaptureInput);
}

void CXRay::destroyInput()
{
	xr_delete(pInput);
}

void CXRay::InitSound()
{
	Msg("Initializing Sound...");
	CSound_manager_interface::_create(u64(Device.m_hWnd));
}

void CXRay::destroySound()
{
	CSound_manager_interface::_destroy();
}

void CXRay::destroySettings()
{
	xr_delete(pSettings);
	xr_delete(pGameIni);
}

void CXRay::destroyConsole()
{
	Console->Destroy();
	xr_delete(Console);
}

void CXRay::execUserScript()
{
	Console->Execute("unbindall");
	Console->ExecuteScript(Console->ConfigFile);
}

void CXRay::ProcessEventLoop()
{
	Device.PrepareEventLoop();
	Device.StartEventLoop();
	Device.EndEventLoop();
}

void CXRay::Destroy()
{
	xr_delete(g_SpatialSpacePhysic);
	xr_delete(g_SpatialSpace);
	DEL_INSTANCE(g_pGamePersistent);
	Event.Dump();

	destroySound();
	destroyInput();

	destroySettings();

	LALib.OnDestroy();

	destroyConsole();

	GameStateManager.Destroy();

	LoadingScreen.Destroy();

	FontManager.Destroy();

	Device.seqFrameMT.Remove(&SoundProcessor);

	Device.Destroy();
	Sheduler.Destroy();
#ifdef DEBUG_MEMORY_MANAGER
	extern void dbg_dump_leaks_prepare();
	if (Memory.debug_mode)
		dbg_dump_leaks_prepare();
#endif // DEBUG_MEMORY_MANAGER
	UnloadLibraries();

	DebugUI->Destroy();
	delete DebugUI;

	Core.Destroy();
}

typedef void DUMMY_STUFF(const void*, const u32&, void*);
XRCORE_API DUMMY_STUFF* g_temporary_stuff;
void CXRay::DecodeResources()
{
	Msg("[CXRay]: Initializing Universal Resource Auto-Decoder...");
	g_temporary_stuff = &DecodeGameResources;
}
//////////////////////////////////////////////////////////////////////////
