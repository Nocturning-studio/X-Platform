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
    m_bUseSDL3(false),
    m_pSdlWindow(nullptr),
    m_WindowTitle("S.T.A.L.K.E.R.: Shadow Of Chernobyl"),
    m_bInitialized(false)
{}

CWindowManager::~CWindowManager() {}

// -----------------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------------
void CWindowManager::InitializeWin32()
{
    m_hInstance = GetModuleHandle(nullptr);
    RegisterWindowClassWin32();
    CreateGameWindowWin32();
}

void CWindowManager::ApplyWin32()
{
    if (!m_bInitialized)
    {
        CreateGameWindowWin32();
        m_bInitialized = true;
        return;
    }
    UpdateWindowAttributesWin32();   // окно уже существует – меняем атрибуты без пересоздания
}

void CWindowManager::DestroyWin32()
{
    if (m_hWnd)
    {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
}

bool CWindowManager::ProcessMessagesWin32()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            m_bQuitRequested = true;
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return !m_bQuitRequested;
}

void CWindowManager::SetResolutionWin32(u32 w, u32 h)
{
    m_width = w;
    m_height = h;
    // Сброс позиции заставит Apply() центрировать окно при следующем вызове
    m_PositionX = CW_USEDEFAULT;
    m_PositionY = CW_USEDEFAULT;
}

void CWindowManager::CenterWindowWin32()
{
    if (!m_hWnd || !m_bWindowed)
        return;

    RECT rc;
    GetWindowRect(m_hWnd, &rc);
    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;

    HMONITOR hMon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
    int newX, newY;
    ComputeCenteredPositionWin32(hMon, winW, winH, newX, newY);

    m_PositionX = newX;
    m_PositionY = newY;

    SetWindowPos(m_hWnd, nullptr, newX, newY, 0, 0,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

// -----------------------------------------------------------------------------------
// Private helpers
// -----------------------------------------------------------------------------------
void CWindowManager::RegisterWindowClassWin32()
{
    WNDCLASS wndClass = {};
    wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndProcWin32;
    wndClass.hInstance = m_hInstance;
    wndClass.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpszClassName = "_XRAY_";

    RegisterClass(&wndClass);
}

void CWindowManager::CreateGameWindowWin32()
{
    const char* wndclass = "_XRAY_";

    DWORD style = GetStyleWin32();
    int winW = m_width, winH = m_height;   // для borderless это и есть размер окна

    int x = CW_USEDEFAULT, y = CW_USEDEFAULT;
    if (m_bWindowed && m_PositionX == CW_USEDEFAULT)
    {
        // Первое создание — центрируем по основному монитору
        HMONITOR hMon = MonitorFromPoint({ 0,0 }, MONITOR_DEFAULTTOPRIMARY);
        ComputeCenteredPositionWin32(hMon, winW, winH, x, y);
        m_PositionX = x;
        m_PositionY = y;
    }
    else if (m_bWindowed)
    {
        x = m_PositionX;
        y = m_PositionY;
    }

    m_hWnd = CreateWindowEx(
        m_bWindowed ? 0 : WS_EX_TOPMOST,   // полноэкранный – всегда поверх
        wndclass, m_WindowTitle, style,
        x, y, winW, winH,
        nullptr, nullptr, m_hInstance, nullptr);

    R_ASSERT2(m_hWnd, "Failed to create game window");

    // Реальная клиентская область (для borderless совпадает с окном)
    RECT rcClient;
    GetClientRect(m_hWnd, &rcClient);
    m_width = rcClient.right - rcClient.left;
    m_height = rcClient.bottom - rcClient.top;

    // Сохраняем фактическую позицию (чтобы при следующем Apply не центрировать, если не просили)
    if (m_bWindowed)
    {
        RECT rcWindow;
        GetWindowRect(m_hWnd, &rcWindow);
        m_PositionX = rcWindow.left;
        m_PositionY = rcWindow.top;
    }

    Msg("Window created: Handle [0x%p], %dx%d, Windowed: %s",
        m_hWnd, m_width, m_height, m_bWindowed ? "yes" : "no");
}

void CWindowManager::UpdateWindowAttributesWin32()
{
    if (!m_hWnd) return;

    // 1. Применяем стиль окна (без рамок) и расширенный стиль (TopMost для fullscreen)
    ApplyWindowStyleWin32(m_bWindowed ? 0 : WS_EX_TOPMOST);

    const int winW = m_width;
    const int winH = m_height;

    if (m_bWindowed)
    {
        // Оконный режим: всегда центрируем на текущем мониторе
        HMONITOR hMon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
        int centerX, centerY;
        ComputeCenteredPositionWin32(hMon, winW, winH, centerX, centerY);

        // Перемещаем окно в центр, не отбирая фокус
        SetWindowPos(m_hWnd, nullptr, centerX, centerY, winW, winH,
            SWP_NOZORDER | SWP_NOACTIVATE);

        // Сохраняем новую позицию (чисто информационно, для совместимости с CenterWindow())
        m_PositionX = centerX;
        m_PositionY = centerY;
    }
    else
    {
        // Полноэкранный режим: (0,0) поверх всех окон
        SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, winW, winH, SWP_SHOWWINDOW);
        // Сбрасываем оконную позицию, чтобы при возврате в окно центрироваться
        m_PositionX = CW_USEDEFAULT;
        m_PositionY = CW_USEDEFAULT;
    }

    // Синхронизируем размеры с фактической клиентской областью (на случай несоответствия)
    RECT rcClient;
    GetClientRect(m_hWnd, &rcClient);
    m_width = rcClient.right - rcClient.left;
    m_height = rcClient.bottom - rcClient.top;
}

void CWindowManager::ComputeCenteredPositionWin32(HMONITOR hMonitor, int winW, int winH, int& outX, int& outY)
{
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMonitor, &mi);
    const RECT& work = mi.rcWork;

    outX = work.left + (work.right - work.left - winW) / 2;
    outY = work.top + (work.bottom - work.top - winH) / 2;
}

void CWindowManager::SaveWindowPositionWin32()
{
    if (!m_hWnd)
        return;

    // Сохраняем, только если окно сейчас не в полноэкранном состоянии
    // (проверяем по текущему расширенному стилю)
    LONG_PTR exStyle = GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOPMOST)
        return;   // полноэкранный – не сохраняем

    RECT rc;
    if (GetWindowRect(m_hWnd, &rc))
    {
        m_PositionX = rc.left;
        m_PositionY = rc.top;
    }
}

void CWindowManager::ApplyWindowStyleWin32(DWORD exStyle)
{
    SetWindowLongPtr(m_hWnd, GWL_STYLE, GetStyleWin32());
    SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, exStyle);

    // Заставляем систему пересчитать неклиентскую область (она исчезнет/появится)
    SetWindowPos(m_hWnd, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

// -----------------------------------------------------------------------------------
// Static window procedure
// -----------------------------------------------------------------------------------
LRESULT CALLBACK CWindowManager::WndProcWin32(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_ACTIVATE:
        {
            u16 fActive = LOWORD(wParam);
            BOOL fMinimized = (BOOL)HIWORD(wParam);
            BOOL bActive = ((fActive != WA_INACTIVE) && (!fMinimized)) ? TRUE : FALSE;

            Device.SetActivate(bActive);
        }
        break;
    case WM_SETCURSOR:
        return 1;
    case WM_SYSCOMMAND:
        switch (wParam)
        {
        case SC_MOVE:
        case SC_SIZE:
        case SC_MAXIMIZE:
        case SC_MONITORPOWER:
            return 1;
        }
        break;
    case WM_CLOSE:
        if (Console)
            Console->Execute("quit");
        return 0;

        // Для borderless-окна: разрешаем перетаскивание за любое место клиентской области
    case WM_NCHITTEST:
    {
        LRESULT hit = DefWindowProc(hWnd, uMsg, wParam, lParam);
        if (hit == HTCLIENT)
            return HTCAPTION;
        return hit;
    }
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void CWindowManager::Initialize(bool use_sdl3)
{
    Msg("Initializing Window Manager...");

    m_bUseSDL3 = use_sdl3;

    if (m_bUseSDL3)
        InitializeSDL3();
    else
        InitializeWin32();

    m_bInitialized = true;
}

void CWindowManager::Destroy()
{
    if (m_bUseSDL3)
        DestroySDL3();
    else
        DestroyWin32();
}

void CWindowManager::Apply()
{
    if (m_bUseSDL3)
        ApplySDL3();
    else
        ApplyWin32();
}

bool CWindowManager::ProcessMessages()
{
    if (m_bUseSDL3)
        return ProcessMessagesSDL3();
    else
        return ProcessMessagesWin32();
}

void CWindowManager::SetResolution(u32 w, u32 h)
{
    if (m_bUseSDL3)
        SetResolutionSDL3(w, h);
    else
        SetResolutionWin32(w, h);
}

void CWindowManager::CenterWindow()
{
    if (m_bUseSDL3)
        CenterWindowSDL3();
    else
        CenterWindowWin32();
}

void CWindowManager::InitializeSDL3()
{
    SDL_Log("Creating game window");

    SDL_WindowFlags sdl_window_flags = SDL_WINDOW_MOUSE_GRABBED;

    m_pSdlWindow = SDL_CreateWindow(
        m_WindowTitle,
        m_width, m_height,
        sdl_window_flags
    );

    R_ASSERT3(m_pSdlWindow, "Failed to create SDL3 window", SDL_GetError());

    SDL_Log("Getting HWND handle from game window");

    SDL_PropertiesID window_props = SDL_GetWindowProperties(m_pSdlWindow);

    void* sdl_hwnd = SDL_GetPointerProperty(window_props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);

    R_ASSERT3(sdl_hwnd, "Failed to get HWND from SDL3 window", SDL_GetError());

    m_hWnd = (HWND)sdl_hwnd;

    SDL_Log("Created SDL3 window: window 0x%p, hwnd 0x%p", m_pSdlWindow, m_hWnd);
}

void CWindowManager::DestroySDL3()
{
    if (m_hWnd)
    {
        m_hWnd = nullptr;
    }

    if (m_pSdlWindow)
    {
        SDL_DestroyWindow(m_pSdlWindow);
        m_pSdlWindow = nullptr;
    }
}

void CWindowManager::ApplySDL3()
{
    if (!m_pSdlWindow)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s: the m_pSdlWindow is null!", __FUNCTION__);
        return;
    }

    SDL_SetWindowSize(m_pSdlWindow, m_width, m_height);
    SDL_SetWindowFullscreen(m_pSdlWindow, !m_bWindowed);

    CenterWindowSDL3();

    R_ASSERT3(SDL_SyncWindow(m_pSdlWindow), "SDL_SyncWindow timeout", SDL_GetError());

    int w{};
    int h{};
    R_ASSERT3(SDL_GetWindowSize(m_pSdlWindow, &w, &h), "SDL_GetWindowSize failed", SDL_GetError());
    m_width = w;
    m_height = h;
}

bool CWindowManager::ProcessMessagesSDL3()
{
    static bool minimized = false;
    static bool focused = false;

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (SDL_GetLogPriority(SDL_LOG_CATEGORY_APPLICATION) <= SDL_LOG_PRIORITY_VERBOSE)
        {
            string512 event_desc;

            SDL_GetEventDescription(&event, event_desc, sizeof(event_desc));

            SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Received event %s", event_desc);
        }

        switch (event.type)
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

void CWindowManager::SetResolutionSDL3(u32 w, u32 h)
{
    m_width = w;
    m_height = h;
}

void CWindowManager::CenterWindowSDL3()
{
    if (!m_pSdlWindow)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s: the m_pSdlWindow is null!", __FUNCTION__);
        return;
    }

    SDL_SetWindowPosition(m_pSdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}
