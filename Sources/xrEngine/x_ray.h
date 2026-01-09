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
////////////////////////////////////////////////////////////////////////////////
// Abstract 'Pure' class for DLL interface
class ENGINE_API DLL_Pure
{
  public:
	CLASS_ID CLS_ID;

	DLL_Pure(void* params)
	{
		CLS_ID = 0;
	};
	DLL_Pure()
	{
		CLS_ID = 0;
	};
	virtual DLL_Pure* _construct()
	{
		return this;
	}
	virtual ~DLL_Pure(){};
};

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

class ENGINE_API CXRay
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

public:
	void InitSettings();
	void InitConsole();
	void InitInput();
	void InitSound();
	
	void destroySettings();
	void destroyConsole();
	void destroyInput();
	void destroySound();

	void execUserScript();

	void ProcessEventLoop();
	void Destroy();

	void DecodeResources();

	void LoadLibraries();
	void UnloadLibraries();

	CXRay();
	~CXRay();

	void Run();
	bool Initialize();
};
////////////////////////////////////////////////////////////////////////////////
extern xrDispatchTable PSGP;
////////////////////////////////////////////////////////////////////////////////
// Объявляем глобальный указатель, чтобы его видели другие .cpp файлы
extern ENGINE_API CXRay* g_XRay;

// "Обманываем" старый код, который пишет Engine.Event...
// Теперь Engine разыменовывается в наш глобальный объект
#define Engine (*g_XRay)

#define NEW_INSTANCE(a) Engine.pCreate(a)
#define DEL_INSTANCE(a)                                                                                                \
	{                                                                                                                  \
		Engine.pDestroy(a);                                                                                   \
		a = NULL;                                                                                                      \
	}
////////////////////////////////////////////////////////////////////////////////
