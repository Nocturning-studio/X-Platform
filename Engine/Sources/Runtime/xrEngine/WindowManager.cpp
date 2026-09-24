#include "stdafx.h"
#include "WindowManager.h"
#include "device.h"
#include "resource.h"
#include "xr_ioc_cmd.h"
#include "SDL3/SDL.h"

// -----------------------------------------------------------------------------------
// Construction / Destruction
// -----------------------------------------------------------------------------------
CWindowManager::CWindowManager()
	: m_hWnd(nullptr), m_hInstance(nullptr),
	  m_width(640), m_height(480),
	  m_bWindowed(true),
	  m_RefreshRate(0),
	  m_PositionX(CW_USEDEFAULT), m_PositionY(CW_USEDEFAULT),
	  m_bQuitRequested(false),
	  m_pSdlWindow(nullptr),
	  m_WindowTitle("S.T.A.L.K.E.R.: Shadow Of Chernobyl"),
	  m_bInitialized(false)
{
}

CWindowManager::~CWindowManager() {}

// -----------------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------------

// -----------------------------------------------------------------------------------
// Private helpers
// -----------------------------------------------------------------------------------

void CWindowManager::Initialize()
{
	Msg("Initializing Window Manager...");

	InitializeWindow();

	m_bInitialized = true;
}

void CWindowManager::InitializeWindow()
{
	SDL_Log("Creating game window");

	SDL_WindowFlags sdl_window_flags = SDL_WINDOW_MOUSE_GRABBED | SDL_WINDOW_BORDERLESS;

	m_pSdlWindow = SDL_CreateWindow(
		m_WindowTitle,
		m_width, m_height,
		sdl_window_flags);

	R_ASSERT3(m_pSdlWindow, "Failed to create SDL3 window", SDL_GetError());

	SDL_Log("Getting HWND handle from game window");

	SDL_PropertiesID window_props = SDL_GetWindowProperties(m_pSdlWindow);

	void* sdl_hwnd = SDL_GetPointerProperty(window_props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);

	R_ASSERT3(sdl_hwnd, "Failed to get HWND from SDL3 window", SDL_GetError());

	m_hWnd = (HWND)sdl_hwnd;

	SDL_Log("Created SDL3 window: window 0x%p, hwnd 0x%p", m_pSdlWindow, m_hWnd);
}

void CWindowManager::Destroy()
{
	if(m_hWnd)
	{
		m_hWnd = nullptr;
	}

	if(m_pSdlWindow)
	{
		SDL_DestroyWindow(m_pSdlWindow);
		m_pSdlWindow = nullptr;
	}
}

void CWindowManager::Apply()
{
	if(!m_pSdlWindow)
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s: the m_pSdlWindow is null!", __FUNCTION__);
		return;
	}

	SDL_SetWindowSize(m_pSdlWindow, m_width, m_height);
	SDL_SetWindowFullscreen(m_pSdlWindow, !m_bWindowed);

	CenterWindow();

	R_ASSERT3(SDL_SyncWindow(m_pSdlWindow), "SDL_SyncWindow timeout", SDL_GetError());

	int w{};
	int h{};
	R_ASSERT3(SDL_GetWindowSize(m_pSdlWindow, &w, &h), "SDL_GetWindowSize failed", SDL_GetError());
	m_width = w;
	m_height = h;
}

bool CWindowManager::ProcessMessages()
{
	static bool minimized = false;
	static bool focused = false;

	SDL_Event event;
	while(SDL_PollEvent(&event))
	{
		if(SDL_GetLogPriority(SDL_LOG_CATEGORY_APPLICATION) <= SDL_LOG_PRIORITY_VERBOSE)
		{
			string512 event_desc;

			SDL_GetEventDescription(&event, event_desc, sizeof(event_desc));

			SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Received event %s", event_desc);
		}

		switch(event.type)
		{
		case SDL_EVENT_QUIT:
			m_bQuitRequested = true;
			break;
		case SDL_EVENT_WINDOW_MINIMIZED:
			minimized = true;
			break;
		case SDL_EVENT_WINDOW_RESTORED:
			minimized = false;
			break;
		case SDL_EVENT_WINDOW_FOCUS_GAINED:
			focused = true;
			break;
		case SDL_EVENT_WINDOW_FOCUS_LOST:
			focused = false;
			break;
		}
	}

	bool device_should_be_active = !minimized && focused;

	Device.SetActivate(device_should_be_active);

	return !m_bQuitRequested;
}

void CWindowManager::CenterWindow()
{
	if(!m_pSdlWindow)
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s: the m_pSdlWindow is null!", __FUNCTION__);
		return;
	}

	SDL_SetWindowPosition(m_pSdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}
