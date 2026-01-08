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
#include "Application.h"
#include "debug_ui.h"
//////////////////////////////////////////////////////////////////////////
ENGINE_API CDebugUI* DebugUI = nullptr;
//////////////////////////////////////////////////////////////////////////
#define TRIVIAL_ENCRYPTOR_DECODER
#include "trivial_encryptor.h"

typedef void DUMMY_STUFF(const void*, const u32&, void*);
XRCORE_API DUMMY_STUFF* g_temporary_stuff;
//////////////////////////////////////////////////////////////////////////
extern CRenderDevice Device;
//////////////////////////////////////////////////////////////////////////
ENGINE_API CApplication* pApp = NULL;
ENGINE_API CInifile* pGameIni = NULL;
ENGINE_API bool g_bBenchmark = false;
//////////////////////////////////////////////////////////////////////////
CXRay::CXRay()
{
	m_bIntroState = TRUE;
}

void CXRay::InitEngine()
{
	InitSettings();

	Engine.Initialize();

	while (GetIntroState())
		Sleep(100);

	Device.Initialize();

	InitInput();

	InitConsole();
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

void CXRay::destroyEngine()
{
	Device.Destroy();
	Engine.Destroy();
}

void CXRay::execUserScript()
{
	Console->Execute("unbindall");
	Console->ExecuteScript(Console->ConfigFile);
}

void slowdownthread(void*)
{
	OPTICK_EVENT("X-Ray Slowdown thread");
	OPTICK_FRAME("X-Ray Slowdown thread");
	for (;;)
	{
		if (Device.Statistic->fFPS < 30)
			Sleep(1);
		if (Device.mt_bMustExit)
			return;
		if (0 == pSettings)
			return;
		if (0 == Console)
			return;
		if (0 == pInput)
			return;
		if (0 == pApp)
			return;
	}
}

void CheckPrivilegySlowdown()
{
#ifdef DEBUG
	if (strstr(Core.Params, "-slowdown"))
	{
		//Threading::SpawnThreadthread_spawn(slowdownthread, "Debug Slowdown thread", 0, 0);
	}
	if (strstr(Core.Params, "-slowdown2x"))
	{
		//Threading::SpawnThreadthread_spawn(slowdownthread, "Debug Slowdown thread 0", 0, 0);
		//Threading::SpawnThreadthread_spawn(slowdownthread, "Debug Slowdown thread 1", 0, 0);
	}
#endif // DEBUG
}

void CXRay::Startup()
{
	execUserScript();
	InitSound();

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

	ShowWindow(Device.m_hWnd, SW_SHOWNORMAL);

	DebugUI = new CDebugUI();

	Device.Create();

	DebugUI->Initialize();

	LALib.OnCreate();

	pApp = xr_new<CApplication>();

	g_pGamePersistent = (IGame_Persistent*)NEW_INSTANCE(CLSID_GAME_PERSISTANT);

	g_SpatialSpace = xr_new<ISpatial_DB>();
	g_SpatialSpacePhysic = xr_new<ISpatial_DB>();

	Memory.mem_usage();
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
	xr_delete(pApp);
	Engine.Event.Dump();

	// Destroying
	destroySound();
	destroyInput();

	if (!g_bBenchmark)
		destroySettings();

	LALib.OnDestroy();

	if (!g_bBenchmark)
		destroyConsole();
	else
		Console->Destroy();

	destroyEngine();

	DebugUI->Destroy();
	delete DebugUI;
}

// -------------------------------------------------------------------------------------------------
// Universal Encryption Auto-Detection
// -------------------------------------------------------------------------------------------------

// State cache: -1 = unknown, 0 = WW, 1 = RU
static int g_last_successful_profile = -1;

// Wrapper function to determine valid keys for FS archives
static void UniversalDecodingWrapper(const void* source, const u32& size, void* destination)
{
	// 1. Short path: Block too small for analysis or impact.
	if (size < 4)
	{
		if (g_last_successful_profile == trivial_encryptor::PROFILE_RU)
			trivial_encryptor::decode_rus(source, size, destination);
		else
			trivial_encryptor::decode_ww(source, size, destination);
		return;
	}

	u8 probe_buffer[16];
	size_t probe_len = (size < 16) ? size : 16;

	// Heuristic validator: Header usually contains file count/uncompressed size.
	// Limits raised to ~128MB to support large mod archives while filtering garbage (>3GB).
	auto is_valid_header = [](u32 value) -> bool { return (value > 0) && (value < 128000000); };

	// 2. Sticky Logic: Try previously successful profile first to avoid fluctuation on large blocks
	if (g_last_successful_profile != -1)
	{
		std::memcpy(probe_buffer, source, probe_len);
		if (g_last_successful_profile == trivial_encryptor::PROFILE_RU)
			trivial_encryptor::decode_rus(probe_buffer, u32(probe_len), probe_buffer);
		else
			trivial_encryptor::decode_ww(probe_buffer, u32(probe_len), probe_buffer);

		u32 check_val = *((u32*)probe_buffer);

		if (is_valid_header(check_val))
		{
			// Confirmed valid again - proceed
			if (g_last_successful_profile == trivial_encryptor::PROFILE_RU)
				trivial_encryptor::decode_rus(source, size, destination);
			else
				trivial_encryptor::decode_ww(source, size, destination);
			return;
		}

		// Validation failed, reset cache
		Msg("![AutoDecoder]: Cached profile failed (Val: %u). Resetting detection.", check_val);
		g_last_successful_profile = -1;
	}

	// 3. Full Auto-Detection
	Msg("[AutoDecoder]: Detecting profile for block size %u...", size);

	// Test Worldwide
	std::memcpy(probe_buffer, source, probe_len);
	trivial_encryptor::decode_ww(probe_buffer, u32(probe_len), probe_buffer);
	u32 val_ww = *((u32*)probe_buffer);
	bool ww_ok = is_valid_header(val_ww);

	// Test Russian
	std::memcpy(probe_buffer, source, probe_len);
	trivial_encryptor::decode_rus(probe_buffer, u32(probe_len), probe_buffer);
	u32 val_ru = *((u32*)probe_buffer);
	bool ru_ok = is_valid_header(val_ru);

	// Decision Matrix
	if (ww_ok && !ru_ok)
	{
		Msg("[AutoDecoder]: Detected WORLDWIDE (Val: %u).", val_ww);
		g_last_successful_profile = trivial_encryptor::PROFILE_WW;
		trivial_encryptor::decode_ww(source, size, destination);
	}
	else if (!ww_ok && ru_ok)
	{
		Msg("[AutoDecoder]: Detected RUSSIAN (Val: %u).", val_ru);
		g_last_successful_profile = trivial_encryptor::PROFILE_RU;
		trivial_encryptor::decode_rus(source, size, destination);
	}
	else if (ww_ok && ru_ok)
	{
		// Ambiguous: pick smallest reasonable number
		if (val_ww < val_ru)
		{
			Msg("[AutoDecoder]: Ambiguous. Guessing WW (%u vs %u)", val_ww, val_ru);
			g_last_successful_profile = trivial_encryptor::PROFILE_WW;
			trivial_encryptor::decode_ww(source, size, destination);
		}
		else
		{
			Msg("[AutoDecoder]: Ambiguous. Guessing RU (%u vs %u)", val_ru, val_ww);
			g_last_successful_profile = trivial_encryptor::PROFILE_RU;
			trivial_encryptor::decode_rus(source, size, destination);
		}
	}
	else
	{
		// Critical failure. Fallback to WW default.
		Msg("![AutoDecoder]: CRITICAL WARNING! Unknown format (WW: %u, RU: %u).", val_ww, val_ru);
		trivial_encryptor::decode_ww(source, size, destination);
	}
}

void CXRay::DecodeResources()
{
	Msg("[CXRay]: Initializing Universal Resource Auto-Decoder...");
	g_temporary_stuff = &UniversalDecodingWrapper;
}

//////////////////////////////////////////////////////////////////////////
