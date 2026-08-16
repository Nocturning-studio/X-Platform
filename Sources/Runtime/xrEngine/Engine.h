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
#include "EventAPI.h"
#include "xrBind_PSGP.h"
#include "LevelManager.h"
#include "FontManager.h"
#include "debug_ui.h"
#include "pure.h"
#include "WindowManager.h"
#include "TimeManager.h"
#include "ThreadManager.h"
#include "RenderView.h"
#include "ResourceManager.h"
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API CSheduler;
class ENGINE_API CLevelLoadingScreen;
class ENGINE_API CGameStateManager;
class ENGINE_API CStats;
class ENGINE_API CResourceManager;
////////////////////////////////////////////////////////////////////////////////
extern "C"
{
	typedef DLL_API DLL_Pure* __cdecl Factory_Create(CLASS_ID CLS_ID);
	typedef DLL_API void __cdecl Factory_Destroy(DLL_Pure* O);
};

extern "C"
{
	typedef void __cdecl VTPause(void);
	typedef void __cdecl VTResume(void);
};

struct ENGINE_API CEngineEvents
{
	CRegistrator<pureFrame> Frame;
	CRegistrator<pureRender> Render;
	CRegistrator<pureAppActivate> AppActivate;
	CRegistrator<pureAppDeactivate> AppDeactivate;
	CRegistrator<pureAppStart> AppStart;
	CRegistrator<pureAppEnd> AppEnd;
	CRegistrator<pureDeviceReset> DeviceReset;
};

class ENGINE_API CEngine
{
  public:
	typedef fastdelegate::FastDelegate0<bool> LOADING_EVENT;

  private:
	HMODULE hGame;
	HMODULE hRender;
	HMODULE hTuner;
	HMODULE hOptick;

	bool m_bLoaded;

	xr_list<LOADING_EVENT> m_loading_events;

  public:
	Factory_Create* pCreate;
	Factory_Destroy* pDestroy;

	BOOL tune_enabled;
	VTPause* tune_pause;
	VTResume* tune_resume;

	CEventAPI Event;
	CLevelManager LevelManager;
	CFontManager FontManager;
	CDebugUI DebugUI;
	CWindowManager WindowManager;
	CTimeManager TimeManager;
	CThreadManager ThreadManager;
	CEngineEvents Events;
	CRenderView RenderView; 

	CGameStateManager* GameStateManager;
	CLevelLoadingScreen* LoadingScreen;
	CSheduler* Sheduler;
	CStats* Statistic;
	CResourceManager* ResourceManager;

	public:
	CEngine();
	~CEngine();

	void Run();

	bool IsLoaded()
	{
		return m_bLoaded;
	}

	void SetLoaded()
	{
		m_bLoaded = TRUE;
	}

	void SetUnloaded()
	{
		m_bLoaded = FALSE;
	}

	xr_list<LOADING_EVENT> GetLoadingEvents()
	{
		return m_loading_events;
	}

	void AddLoadingEvent(LOADING_EVENT _Event)
	{
		m_loading_events.push_back(_Event);
	}

	void AddLoadingEventFront(LOADING_EVENT _Event)
	{
		m_loading_events.push_front(_Event);
	}

	void PopLoadingEvent()
	{
		m_loading_events.pop_front();
	}

	private:
	bool Initialize();
	void ProcessEventLoop();
	void ProcessFrame();
	bool CheckLoadingEvents();
	void UpdateGameLogic();
	void Destroy();
};
////////////////////////////////////////////////////////////////////////////////
extern ENGINE_API CEngine* g_Engine;

#define Engine (*g_Engine)

#define NEW_INSTANCE(a) Engine.pCreate(a)
#define DEL_INSTANCE(a)                                                                                                \
	{                                                                                                                  \
		Engine.pDestroy(a);                                                                                            \
		a = NULL;                                                                                                      \
	}
////////////////////////////////////////////////////////////////////////////////
