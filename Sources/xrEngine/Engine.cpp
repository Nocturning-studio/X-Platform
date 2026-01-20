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
#include "Engine.h"
#include "std_classes.h"
#include "GameFont.h"
#include "resource.h"
#include "LightAnimLibrary.h"
#include "ispatial.h"
#include "Text_Console.h"
#include <process.h>
#include "../xrDiscordAPI/DiscordAPI.h"
#include "build_identificator.h"
#include "LogoWindow.h"
//////////////////////////////////////////////////////////////////////////
#define TRIVIAL_ENCRYPTOR_DECODER
#include "trivial_encryptor.h"
#include <xrCPU_Pipe.h>
//////////////////////////////////////////////////////////////////////////
ENGINE_API CEngine* g_Engine;
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
		::Sound->update(Engine.RenderView.Position, Engine.RenderView.Direction, Engine.RenderView.Top);
		Device.Statistic->Sound.End();
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
	Debug._initialize(false);
#else  // DEDICATED_SERVER
	Debug._initialize(true);
	g_dedicated_server = true;
#endif // DEDICATED_SERVER

	// 3. Декодер ресурсов (Anti-piracy stub logic)
	Msg("[CEngine]: Initializing Universal Resource Auto-Decoder...");
	g_temporary_stuff = &DecodeGameResources;

	// 4. Build Info
	ComputeBuildIdentificator();
	PrintBuildIdentificator();

	// 5. Инициализация ядра (xrCore)
	Core.Initialize("X-Ray Engine", "xray_engine");

	FPU::m24r();

	// 6. Инициализация настроек (Settings / INI)
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

	Msg("Initializing Engine...");

	// 7. Math extensions & Sheduler
	xrBind_PSGP(&PSGP, true);

	Msg("Initializing Engine Sheduler...");
	Sheduler.Initialize();

#ifdef DEBUG
	msCreate("game");
#endif

	WindowManager.Initialize();

	TimeManager.Initialize();

	// 8. Device Base Init (без создания окна/контекста, только структуры)
	Device.Initialize();

	// 9. Input System
	{
		BOOL bCaptureInput = !strstr(Core.Params, "-i");
		if (g_dedicated_server)
			bCaptureInput = FALSE;

		pInput = xr_new<CInput>(bCaptureInput);
		pInput->Initialize();
	}

	// 10. Console
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

	// 11. Load Libraries (DLLs)
	{
		Msg("Initializing Engine API...");

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

#ifdef ENABLE_PROFILING
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

		// Discord
		LPCSTR DiscordAPI_name = "xrDiscordAPI.dll";
		Log("Loading DLL:", DiscordAPI_name);
		hDiscordAPI = LoadLibrary(DiscordAPI_name);
		R_ASSERT2(DiscordAPI_name, "! Can't load discord api");
	}

	// 12. Exec Scripts & Sound Init
	{
		Console->Execute("unbindall");
		Console->ExecuteScript(Console->ConfigFile);

		Msg("Initializing Sound...");
		CSound_manager_interface::_create(u64(WindowManager.GetHandle()));
	}

	DebugUI.Initialize();

	// 13. Handle Command Line
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

	// 14. Final Systems Create

	ThreadManager.Initialize();

	Device.Create();

	LALib.OnCreate();

	GameStateManager.Initialize();

	Msg("Initializing Font Manager...");
	FontManager.Initialize();

	Msg("Scanning levels...");
	LevelManager.Scan();

	Engine.ThreadManager.seqFrameMT.Add(&SoundProcessor);

	g_pGamePersistent = (IGame_Persistent*)NEW_INSTANCE(CLSID_GAME_PERSISTANT);
	g_pGamePersistent->Initialize();

	g_SpatialSpace = xr_new<ISpatial_DB>();
	g_SpatialSpacePhysic = xr_new<ISpatial_DB>();

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
	Device.Statistic->EngineTOTAL.Begin();

	// Логика "первого кадра" или загрузки
	// В оригинале ProcessLoading просто вызывал seqFrame один раз и ставил флаг
	if (!IsLoaded())
	{
		Events.Frame.Process(rp_Frame);
		SetLoaded();
	}
	else
	{
		// Основной апдейт игровых систем (Actor, Level, Weather и т.д.)
		Events.Frame.Process(rp_Frame);
	}

	Device.Statistic->EngineTOTAL.End();
}

bool CEngine::CheckLoadingEvents()
{
	if (g_loading_events.empty())
		return false;

	// Выполняем одно событие загрузки (например, загрузка текстуры)
	if (g_loading_events.front()())
		g_loading_events.pop_front();

	// Рисуем экран загрузки
	LoadingScreen.ForceRender();

	return true; // Кадр обработан, дальше идти не надо
}

void CEngine::ProcessFrame()
{
	OPTICK_THREAD("X-Ray Primary Thread");
	OPTICK_FRAME("X-Ray Primary Thread");
	PROFILE_FUNCTION();

	// 1. Проверка готовности устройства
	if (!Device.b_is_Ready)
	{
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
	Device.PrepareEventLoop();

	// Основной цикл теперь выглядит так:
	while (true)
	{
		// 1. Обработка сообщений ОС (Window Manager)
		if (!WindowManager.ProcessMessages())
			break; // Если вернул false -> WM_QUIT -> выходим

		// 2. Игровой кадр
		ProcessFrame();
	}

	Device.EndEventLoop();
}

void CEngine::Destroy()
{
	// 1. Очистка игровых сущностей и пространств
	xr_delete(g_SpatialSpacePhysic);
	xr_delete(g_SpatialSpace);
	DEL_INSTANCE(g_pGamePersistent);

	// 2. События
	Event.Dump();

	// 3. Звук и Ввод
	CSound_manager_interface::_destroy();
	xr_delete(pInput);

	// 4. Настройки
	xr_delete(pSettings);
	xr_delete(pGameIni);

	LALib.OnDestroy();

	// 5. Консоль
	Console->Destroy();
	xr_delete(Console);

	FontManager.Destroy();

	GameStateManager.Destroy();

	DebugUI.Destroy();

	WindowManager.Destroy();

	// 6. Device & Scheduler
	TimeManager.Destroy();

	Device.Destroy();

	ThreadManager.seqFrameMT.Remove(&SoundProcessor);

	ThreadManager.Destroy();

	Sheduler.Destroy();

#ifdef DEBUG_MEMORY_MANAGER
	extern void dbg_dump_leaks_prepare();
	if (Memory.debug_mode)
		dbg_dump_leaks_prepare();
#endif // DEBUG_MEMORY_MANAGER

	// 7. Выгрузка библиотек
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

	// Очистка системных ресурсов, которые были загружены библиотеками
	Event._destroy();
	XRC.r_clear_compact();

	// 8. Ядро
	Core.Destroy();
}

void CEngine::Run()
{
	Initialize();
	ProcessEventLoop();
	Destroy();
}
