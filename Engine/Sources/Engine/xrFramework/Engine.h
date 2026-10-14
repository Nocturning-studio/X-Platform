////////////////////////////////////////////////////////////////////////////////
//  X-Platform — расширение функциональности X-Ray Engine
//
//  Автор:        NSDeathman
//  Студия:       Nocturning Studio
//
//  SPDX-FileCopyrightText: 2026 Nocturning Studio и участники X-Platform
//  SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
//
//  Этот файл распространяется на условиях PolyForm Noncommercial 1.0.0.
//  Копирование, изменение и распространение разрешены для некоммерческих
//  целей. Публичное указание авторства в вашем проекте, документации или
//  рекламе не требуется — оно приветствуется, но не является условием
//  использования.
//
//  При распространении копий или производных работ лицензия обязывает
//  сохранять настоящие уведомления и передавать получателю текст лицензии
//  или ссылку на него. Пожалуйста, не удаляйте эту плашку — это помогает
//  сохранять историю проекта и корректно соблюдать лицензию.
//
//  Коммерческое использование без отдельного разрешения правообладателей
//  запрещено.
//
//  Полный текст лицензии:
//      https://polyformproject.org/licenses/noncommercial/1.0.0
//
//  Этот файл является source-available, а не open source.
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API CGameFont;
struct xrDispatchTable;
////////////////////////////////////////////////////////////////////////////////
class CEngine;
extern ENGINE_API CEngine* g_Engine;

#define Engine (*g_Engine)

#define NEW_INSTANCE(a) Engine.pCreate(a)
#define DEL_INSTANCE(a)     \
	{                       \
		Engine.pDestroy(a); \
		a = NULL;           \
	}
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
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
#include "RHI.h"
#include "IEngineHost.h"
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API CSheduler;
class ENGINE_API CLevelLoadingScreen;
class ENGINE_API CGameStateManager;
class ENGINE_API CStats;
class ENGINE_API CResourceManagerLegacy;
////////////////////////////////////////////////////////////////////////////////
extern "C"
{
	typedef void __cdecl VTPause(void);
	typedef void __cdecl VTResume(void);
};
////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////
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
	CResourceManagerLegacy* ResourceManager;

	CXRHISubsystem RHI;

	IEngineHost* AppHost;

  public:
	CEngine();
	~CEngine();

	void Run(IEngineHost* host);
	bool IsLoaded() { return m_bLoaded; }
	void SetLoaded() { m_bLoaded = TRUE; }
	void SetUnloaded() { m_bLoaded = FALSE; }
	xr_list<LOADING_EVENT> GetLoadingEvents() { return m_loading_events; }
	void AddLoadingEvent(LOADING_EVENT _Event) { m_loading_events.push_back(_Event); }
	void AddLoadingEventFront(LOADING_EVENT _Event) { m_loading_events.push_front(_Event); }
	void PopLoadingEvent() { m_loading_events.pop_front(); }

  private:
	bool Initialize();
	void ProcessEventLoop();
	void ProcessFrame();
	bool CheckLoadingEvents();
	void UpdateGameLogic();
	void Destroy();
};
////////////////////////////////////////////////////////////////////////////////
#include "ResourceManager.h"
////////////////////////////////////////////////////////////////////////////////
