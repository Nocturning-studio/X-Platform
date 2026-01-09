////////////////////////////////////////////////////////////////////////////////
// Created: 14.01.2025
// Author: NSDeathman
// Refactored code: X-Ray class realization
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API CGameFont;
struct xrDispatchTable;
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "GameStateManager.h"
#include "EventAPI.h"
#include "xrSheduler.h"
#include "xrCPU_Pipe.h"
#include "LevelManager.h"
#include "LevelLoadingScreen.h"
#include "FontManager.h"
#include "debug_ui.h"
#include "pure.h"
#include "WindowManager.h"
////////////////////////////////////////////////////////////////////////////////
// Class creation/destroying interface
extern "C"
{
	typedef DLL_API DLL_Pure* __cdecl Factory_Create(CLASS_ID CLS_ID);
	typedef DLL_API void __cdecl Factory_Destroy(DLL_Pure* O);
};

// Tuning interface
extern "C"
{
	typedef void __cdecl VTPause(void);
	typedef void __cdecl VTResume(void);
};

class ENGINE_API CEngine
{
  private:
	HMODULE hGame;
	HMODULE hRender;
	HMODULE hTuner;
	HMODULE hDiscordAPI;
	HMODULE hOptick;

  public:
	Factory_Create* pCreate;
	Factory_Destroy* pDestroy;

	BOOL tune_enabled;
	VTPause* tune_pause;
	VTResume* tune_resume;

	CEventAPI Event;
	CSheduler Sheduler;
	CLevelManager LevelManager;
	CLevelLoadingScreen LoadingScreen;
	CFontManager FontManager;
	CGameStateManager GameStateManager;
	CDebugUI DebugUI;
	CWindowManager WindowManager;

  public:
	// Конструктор/Деструктор
	CEngine();
	~CEngine();

	// Основные жизненные циклы (Методы, которые остались)
	bool Initialize(); // Вся инициализация (Console, Sound, Device, Input, DLLs) теперь здесь
	void ProcessEventLoop();
	void Destroy();	   // Очистка ресурсов

	void Run(); // Запуск Loop (Init -> Loop -> Destroy)
};
////////////////////////////////////////////////////////////////////////////////
extern xrDispatchTable PSGP;
////////////////////////////////////////////////////////////////////////////////
// Объявляем глобальный указатель, чтобы его видели другие .cpp файлы
extern ENGINE_API CEngine* g_Engine;

// "Обманываем" старый код, который пишет Engine.Event...
#define Engine (*g_Engine)

#define NEW_INSTANCE(a) Engine.pCreate(a)
#define DEL_INSTANCE(a)                                                                                                \
	{                                                                                                                  \
		Engine.pDestroy(a);                                                                                            \
		a = NULL;                                                                                                      \
	}
////////////////////////////////////////////////////////////////////////////////
