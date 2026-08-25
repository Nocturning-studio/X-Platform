////////////////////////////////////////////////////////////////////////////////
// Authors:
//			Oles (Oles Shishkovtsov)
//			AlexMX (Alexander Maksimchuk)
//
// Engine class realization
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "GameStateManager.h"
#include "xrSheduler.h"
#include "igame_level.h"
#include "igame_persistent.h"
#include "xr_input.h"
#include "xr_ioconsole.h"
#include "Engine.h"
#include "std_classes.h"
#include "GameFont.h"
#include "resource.h"
#include "LightAnimLibrary.h"
#include "ispatial.h"
#include "Text_Console.h"
#include <process.h>
#include "build_identificator.h"
#include "LogoWindow.h"
#include "LevelLoadingScreen.h"
#include "render.h"
#include "xrBind_PSGP.h"
#include "SDL3/SDL.h"
//////////////////////////////////////////////////////////////////////////
#define TRIVIAL_ENCRYPTOR_DECODER
#include "trivial_encryptor.h"
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
ENGINE_API CEngine* g_Engine;
//////////////////////////////////////////////////////////////////////////
extern CRenderDevice Device;
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
		Engine.Statistic->Sound.Begin();
		::Sound->update(Engine.RenderView.Position, Engine.RenderView.Direction, Engine.RenderView.Top);
		Engine.Statistic->Sound.End();
	}
} SoundProcessor;
//////////////////////////////////////////////////////////////////////////
// Resource auto-decoder hooks
typedef void DUMMY_STUFF(const void*, const u32&, void*);
XRCORE_API DUMMY_STUFF* g_temporary_stuff;
//////////////////////////////////////////////////////////////////////////

// Log callback to route SDL3 messages to console/logs
static void LogCallbackSDL3(void* userdata, int category, SDL_LogPriority priority, const char* message)
{
	(void)userdata;

	pcstr category_str = "unknown";
	switch (category)
	{
	case SDL_LOG_CATEGORY_APPLICATION:
		category_str = "app";
		break;
	case SDL_LOG_CATEGORY_ERROR:
		category_str = "error";
		break;
	case SDL_LOG_CATEGORY_ASSERT:
		category_str = "assert";
		break;
	case SDL_LOG_CATEGORY_SYSTEM:
		category_str = "system";
		break;
	case SDL_LOG_CATEGORY_AUDIO:
		category_str = "audio";
		break;
	case SDL_LOG_CATEGORY_VIDEO:
		category_str = "video";
		break;
	case SDL_LOG_CATEGORY_RENDER:
		category_str = "render";
		break;
	case SDL_LOG_CATEGORY_INPUT:
		category_str = "input";
		break;
	case SDL_LOG_CATEGORY_TEST:
		category_str = "test";
		break;
	case SDL_LOG_CATEGORY_GPU:
		category_str = "gpu";
		break;
	}

	char console_mark = '?';
	pcstr priority_str = "unknown";
	switch (priority)
	{
	case SDL_LOG_PRIORITY_TRACE:
		priority_str = "trace";
		console_mark = '%';
		break;
	case SDL_LOG_PRIORITY_VERBOSE:
		priority_str = "verbose";
		console_mark = '%';
		break;
	case SDL_LOG_PRIORITY_DEBUG:
		priority_str = "debug";
		console_mark = '#';
		break;
	case SDL_LOG_PRIORITY_INFO:
		priority_str = "info";
		console_mark = '*';
		break;
	case SDL_LOG_PRIORITY_WARN:
		priority_str = "warn";
		console_mark = '~';
		break;
	case SDL_LOG_PRIORITY_ERROR:
		priority_str = "error";
		console_mark = '!';
		break;
	case SDL_LOG_PRIORITY_CRITICAL:
		priority_str = "critical";
		console_mark = '$';
		break;
	}

	Msg("%c [SDL3][%s][%s] %s", console_mark, category_str, priority_str, message);
}

static void InitSDL3()
{
	SDL_SetLogOutputFunction(LogCallbackSDL3, nullptr);

	SDL_InitFlags sdl_flags = SDL_INIT_VIDEO;

	R_ASSERT3(SDL_Init(sdl_flags), "Failed to initialize SDL3", SDL_GetError());
}

static void ShutdownSDL3()
{
	SDL_Quit();
}

CEngine::CEngine()
{
	hGame = 0;
	hRender = 0;
	hTuner = 0;
	hOptick = 0;
	pCreate = 0;
	pDestroy = 0;
	tune_pause = dummy;
	tune_resume = dummy;
	m_bLoaded = FALSE;
}

CEngine::~CEngine()
{
}

bool CEngine::Initialize()
{
	// Предварительная настройка интерфейса (Splash Screen)
	auto Logo = xr_make_unique<LogoWindow>();
	Logo->Show();

	// Настройка Debug систем
#ifndef DEDICATED_SERVER
	Debug.Initialize(false);
#else  // DEDICATED_SERVER
	Debug.Initialize(true);
	g_dedicated_server = true;
#endif // DEDICATED_SERVER

	// Декодер ресурсов
	Msg("[CEngine]: Initializing Universal Resource Auto-Decoder...");
	g_temporary_stuff = &DecodeGameResources;

	// Build Info
	InitializeGlobalBuildID();

	// Инициализация ядра (xrCore)
	Core.Initialize("X-Ray Engine", "xray_engine");

	// Called after Core.Initialize() to ensure the SDL3 messages would end up in the .log files
	InitSDL3();

	// Инициализация настроек (Settings / INI)
	{
		Msg("Initializing Settings...");
		string_path fname;
		FS.update_path(fname, "$game_config$", "system.ltx");
		pSettings = xr_new<CInifile>(fname, TRUE);
		R_ASSERT2(!pSettings->sections().empty(), make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));

		FS.update_path(fname, "$game_config$", "game.ltx");
		pGameIni = xr_new<CInifile>(fname, TRUE);
		R_ASSERT2(!pGameIni->sections().empty(), make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));
	}

	Msg("Initializing Engine...");

	// Math extensions & Sheduler
	xrBind_PSGP(&PSGP, true);

	Msg("Initializing Engine Sheduler...");
	Sheduler = xr_new<CSheduler>();
	Sheduler->Initialize();

#ifdef DEBUG
	msCreate("game");
#endif

	TimeManager.Initialize();
	ThreadManager.Initialize();

	Statistic = xr_new<CStats>();
	Statistic->Initialize();

	Logo->Hide();
	WindowManager.Initialize();

	{
		BOOL bCaptureInput = !strstr(Core.Params, "-i");
		if (g_dedicated_server)
			bCaptureInput = FALSE;

		pInput = xr_new<CInput>(bCaptureInput);
		pInput->Initialize();
	}

	{
		Msg("Initializing Console...");
#ifdef DEDICATED_SERVER
		Console = xr_new<CTextConsole>();
#else
		Console = xr_new<CConsole>();
#endif
		Console->Initialize();
	}

	{
		// Render
		{
			LPCSTR render_name = "xrRender.dll";
			Log("Loading DLL:", render_name);
			hRender = LoadLibrary(render_name);
			R_ASSERT2(hRender, "! Can't load renderer");
		}

		// Game
		{
			LPCSTR g_name = "xrGame.dll";
			Msg("Loading DLL: %s", g_name);
			hGame = LoadLibrary(g_name);

			if (0 == hGame)
				R_CHK(GetLastError());

			R_ASSERT2(hGame, "Game DLL raised exception during loading or there is no game DLL at all");

			pCreate = (Factory_Create*)GetProcAddress(hGame, "xrFactory_Create");
			R_ASSERT2(pCreate, "Error in xrFactory_Create");

			pDestroy = (Factory_Destroy*)GetProcAddress(hGame, "xrFactory_Destroy");
			R_ASSERT2(pDestroy, "Error in xrFactory_Destroy");
		}

#ifndef MASTER_GOLD
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

		{
			LPCSTR g_name = "OptickCore.dll";
			Log("Loading DLL:", g_name);
			hOptick = LoadLibrary(g_name);
			if (0 == hOptick)
			{
				R_CHK(GetLastError());
				Msg("Optick is not installed");
			}
		}
#endif
	}

	{
		// Loads after xrGame and xrRender are register they commands
		Msg("Loading user settings");
		Console->ExecuteScript(Console->ConfigFile);
	}

	{
		Msg("Initializing Sound...");
		CSound_manager_interface::_create(u64(WindowManager.GetHandle()));
	}

	ResourceManager = xr_new<CResourceManager>();

	Statistic = xr_new<CStats>();
	Statistic->Initialize();

	Device.Initialize();
	Render->Initialize();
	DebugUI.Initialize();

	LALib.OnCreate();

	GameStateManager = xr_new<CGameStateManager>();
	GameStateManager->Initialize();

	FontManager.Initialize();

	LevelManager.Scan();

	Engine.ThreadManager.LegacyFrameMT.Add(&SoundProcessor);

	g_pGamePersistent = (IGame_Persistent*)NEW_INSTANCE(CLSID_GAME_PERSISTANT);
	g_pGamePersistent->Initialize();

	g_SpatialSpace = xr_new<ISpatial_DB>();
	g_SpatialSpacePhysic = xr_new<ISpatial_DB>();

	LoadingScreen = xr_new<CLevelLoadingScreen>();
	LoadingScreen->Show();
	g_pGamePersistent->LoadTitle("st_loading_game");
	LoadingScreen->ForceRender();

	Memory.mem_usage();

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

	g_pGamePersistent->LoadTitle("st_loading_shaders");
	LoadingScreen->ForceRender();
	Render->Create();
	LoadingScreen->Hide();

	return true;
}

void CEngine::UpdateGameLogic()
{
	PROFILE_FUNCTION();

	// Профилирование логики процессора
	Statistic->EngineTOTAL.Begin();

	Events.Frame.Process(rp_Frame);
	if (!IsLoaded())
		SetLoaded();

	Statistic->EngineTOTAL.End();
}

bool CEngine::CheckLoadingEvents()
{
	if (m_loading_events.empty())
		return false;

	// Выполняем одно событие загрузки (например, загрузка текстуры)
	if (m_loading_events.front()())
		m_loading_events.pop_front();

	// Рисуем экран загрузки
	LoadingScreen->ForceRender();

	return true; // Кадр обработан, дальше идти не надо
}

void CEngine::ProcessFrame()
{
	OPTICK_FRAME("X-Ray Primary Thread");
	PROFILE_FUNCTION();

	// Проверка готовности устройства
	if (!Device.b_is_Ready)
	{
		OPTICK_EVENT("Waiting for Device.b_is_Ready");
		Sleep(100);
		return;
	}

	// Начало отсчета времени кадра
	TimeManager.Update();		// Расчет DeltaTime
	TimeManager.OnFrameStart(); // Засекаем время для лимитера

	// Сбор статистики
	if (psDeviceFlags.test(rsStatistic))
		g_bEnableStatGather = TRUE;
	else
		g_bEnableStatGather = FALSE;

	// Блокирующие события загрузки (прерывают кадр)
	if (CheckLoadingEvents())
		return;

	// Обновление игровой логики (Input, AI, Game)
	UpdateGameLogic();

	// Расчет камеры и матриц (View * Projection)
	RenderView.UpdateViewProjection();

	// Запуск тяжелых задач в потоках (Скелет, Физика, Распаковка)
	ThreadManager.SignalFrameStart();

	// Рендер сцены
#ifndef DEDICATED_SERVER
	Device.RenderFrame();
#endif

	// Сохранение состояния камеры (для интерполяции в след. кадре)
	RenderView.SaveState();

	// Синхронизация: ждем завершения всех потоков перед следующим кадром
	ThreadManager.WaitForFrameEnd();

	// Лимитер FPS (усыпляем поток, если слишком быстро)
	TimeManager.DoFrameLimit();

	// Экономия энергии при свернутом окне
	if (!Device.b_is_Active)
		Sleep(1);
}

void CEngine::ProcessEventLoop()
{
	Msg("Preparing event loop...");
	Events.AppStart.Process(rp_AppStart);
	Engine.SetUnloaded();

	while (true)
	{
		if (!WindowManager.ProcessMessages())
			break;

		ProcessFrame();
	}

	Engine.Events.AppEnd.Process(rp_AppEnd);
}

void CEngine::Destroy()
{
	xr_delete(g_SpatialSpacePhysic);
	xr_delete(g_SpatialSpace);
	DEL_INSTANCE(g_pGamePersistent);

	LoadingScreen->Destroy();

	FontManager.Destroy();

	Event.Dump();

	CSound_manager_interface::_destroy();
	xr_delete(pInput);

	xr_delete(pSettings);
	xr_delete(pGameIni);

	LALib.OnDestroy();

	xr_delete(Console);

	FontManager.Destroy();

	GameStateManager->Destroy();

	DebugUI.Destroy();

	WindowManager.Destroy();

	TimeManager.Destroy();

	Events.Render.R.clear();
	Events.AppActivate.R.clear();
	Events.AppDeactivate.R.clear();
	Events.AppStart.R.clear();
	Events.AppEnd.R.clear();
	Events.Frame.R.clear();
	Events.DeviceReset.R.clear();

	Render->Destroy();
	Device.Destroy();

	xr_delete(ResourceManager);

	ThreadManager.LegacyFrameMT.Remove(&SoundProcessor);

	ThreadManager.Destroy();

	Sheduler->Destroy();

#ifdef DEBUG_MEMORY_MANAGER
	extern void dbg_dump_leaks_prepare();
	if (Memory.debug_mode)
		dbg_dump_leaks_prepare();
#endif // DEBUG_MEMORY_MANAGER

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
	pCreate = 0;
	pDestroy = 0;

	Event._destroy();
	XRC.r_clear_compact();

	ShutdownSDL3();

	Core.Destroy();
}

void CEngine::Run()
{
	Initialize();
	ProcessEventLoop();
	Destroy();
}
