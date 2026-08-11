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
//////////////////////////////////////////////////////////////////////////
#define TRIVIAL_ENCRYPTOR_DECODER
#include "trivial_encryptor.h"
//////////////////////////////////////////////////////////////////////////
#include "xrBind_PSGP.h"
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
	// 1. Предварительная настройка интерфейса (Splash Screen)
	auto Logo = xr_make_unique<LogoWindow>();
	Logo->Show();

	// 2. Настройка Debug систем
#ifndef DEDICATED_SERVER
	Debug.Initialize(false);
#else  // DEDICATED_SERVER
	Debug.Initialize(true);
	g_dedicated_server = true;
#endif // DEDICATED_SERVER

	// 3. Декодер ресурсов
	Msg("[CEngine]: Initializing Universal Resource Auto-Decoder...");
	g_temporary_stuff = &DecodeGameResources;

	// 4. Build Info
	InitializeGlobalBuildID();

	// 5. Инициализация ядра (xrCore)
	Core.Initialize("X-Ray Engine", "xray_engine");

	// 6. Инициализация настроек (Settings / INI)
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

	// 7. Math extensions & Sheduler
	xrBind_PSGP(&PSGP, true);

	Msg("Initializing Engine Sheduler...");
	Sheduler = xr_new<CSheduler>();
	Sheduler->Initialize();

#ifdef DEBUG
	msCreate("game");
#endif

	WindowManager.Initialize();

	TimeManager.Initialize();

	Statistic = xr_new<CStats>();
	Statistic->Initialize();

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

	{
		// Render
		Msg("Initializing Renderer...");
		LPCSTR render_name = "xrRender.dll";
		Log("Loading DLL:", render_name);
		hRender = LoadLibrary(render_name);
		R_ASSERT2(hRender, "! Can't load renderer");

		// Game
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

		LPCSTR g_name = "OptickCore.dll";
		Log("Loading DLL:", g_name);
		hOptick = LoadLibrary(g_name);
		if (0 == hOptick)
		{
			R_CHK(GetLastError());
			Msg("Optick is not installed");
		}
#endif
	}

	{
		Msg("Initializing Sound...");
		CSound_manager_interface::_create(u64(WindowManager.GetHandle()));
	}

	{
		// Loads after xrGame and xrRender are register they commands
		Msg("Loading user settings");
		Console->ExecuteScript(Console->ConfigFile);
	}

	DebugUI.Initialize();

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

	ThreadManager.Initialize();

	ResourceManager = xr_new<CResourceManager>();

	Device.Initialize();
	Device.Create();

	LALib.OnCreate();

	GameStateManager = xr_new<CGameStateManager>();
	GameStateManager->Initialize();

	Msg("Initializing Font Manager...");
	FontManager.Initialize();

	Msg("Scanning levels...");
	LevelManager.Scan();

	Engine.ThreadManager.LegacyFrameMT.Add(&SoundProcessor);

	g_pGamePersistent = (IGame_Persistent*)NEW_INSTANCE(CLSID_GAME_PERSISTANT);
	g_pGamePersistent->Initialize();

	g_SpatialSpace = xr_new<ISpatial_DB>();
	g_SpatialSpacePhysic = xr_new<ISpatial_DB>();

	 LoadingScreen = xr_new<CLevelLoadingScreen>();

	// 15. Show Window
	Memory.mem_usage();

	Logo->Hide();

	//ShowWindow(Engine.WindowManager.GetHandle(), SW_SHOWNORMAL);

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

	// 1. Проверка готовности устройства
	if (!Device.b_is_Ready)
	{
		OPTICK_EVENT("Waiting for Device.b_is_Ready");
		Sleep(100);
		return;
	}

	// 2. Начало отсчета времени кадра
	TimeManager.Update();		// Расчет DeltaTime
	TimeManager.OnFrameStart(); // Засекаем время для лимитера

	// 3. Сбор статистики (включаем если нужно)
	// psDeviceFlags обычно глобальна или доступна через Device
	if (psDeviceFlags.test(rsStatistic))
		g_bEnableStatGather = TRUE;
	else
		g_bEnableStatGather = FALSE;

	// 4. Блокирующие события загрузки (прерывают кадр)
	if (CheckLoadingEvents())
		return;

	// 5. Обновление игровой логики (Input, AI, Game)
	UpdateGameLogic();

	// 6. Precache (Прогрев рендера вращением камеры)
	// Сама реализация вращения пока остается в Device, но вызываем мы её отсюда
	if (Device.dwPrecacheFrame)
		Device.PreCache();

	// 7. Расчет камеры и матриц (View * Projection)
	// Делаем это ПОСЛЕ логики (где камера могла сдвинуться) и ПЕРЕД рендером
	RenderView.UpdateViewProjection();

	// 8. Запуск тяжелых задач в потоках (Скелет, Физика, Распаковка)
	// Они работают параллельно с рендером (или рендер ждет их, зависит от реализации RenderFrame)
	ThreadManager.SignalFrameStart();

	// 9. Рендер сцены
#ifndef DEDICATED_SERVER
	Device.RenderFrame();
#endif

	// 10. Сохранение состояния камеры (для интерполяции в след. кадре)
	RenderView.SaveState();

	// 11. Синхронизация: ждем завершения всех потоков перед следующим кадром
	ThreadManager.WaitForFrameEnd();

	// 12. Лимитер FPS (усыпляем поток, если слишком быстро)
	TimeManager.DoFrameLimit();

	// 13. Экономия энергии при свернутом окне
	if (!Device.b_is_Active)
		Sleep(1);
}

void CEngine::ProcessEventLoop()
{
	Msg("Preparing event loop...");
	Events.AppStart.Process(rp_AppStart);
	Engine.SetUnloaded();

	// Основной цикл теперь выглядит так:
	while (true)
	{
		// 1. Обработка сообщений ОС (Window Manager)
		if (!WindowManager.ProcessMessages())
			break; // Если вернул false -> WM_QUIT -> выходим

		// 2. Игровой кадр
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

	Core.Destroy();
}

void CEngine::Run()
{
	Initialize();
	ProcessEventLoop();
	Destroy();
}
