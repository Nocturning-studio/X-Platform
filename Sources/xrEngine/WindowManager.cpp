#include "stdafx.h"
#include "WindowManager.h"
#include "device.h"
#include "resource.h"
#include "xr_ioc_cmd.h"

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
    m_bInitialized(false)
{}

CWindowManager::~CWindowManager() {}

// -----------------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------------
void CWindowManager::Initialize()
{
    Msg("Initializing Window Manager...");
    m_hInstance = GetModuleHandle(nullptr);
    RegisterWindowClass();
    CreateGameWindow();
    m_bInitialized = true;
}

void CWindowManager::Apply()
{
    if (!m_bInitialized)
    {
        CreateGameWindow();
        m_bInitialized = true;
        return;
    }
    UpdateWindowAttributes();   // окно уже существует – меняем атрибуты без пересоздания
}

void CWindowManager::Destroy()
{
    if (m_hWnd)
    {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
}

bool CWindowManager::ProcessMessages()
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

void CWindowManager::SetResolution(u32 w, u32 h)
{
    m_width = w;
    m_height = h;
    // Сброс позиции заставит Apply() центрировать окно при следующем вызове
    m_PositionX = CW_USEDEFAULT;
    m_PositionY = CW_USEDEFAULT;
}

void CWindowManager::CenterWindow()
{
    if (!m_hWnd || !m_bWindowed)
        return;

    RECT rc;
    GetWindowRect(m_hWnd, &rc);
    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;

    HMONITOR hMon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
    int newX, newY;
    ComputeCenteredPosition(hMon, winW, winH, newX, newY);

    m_PositionX = newX;
    m_PositionY = newY;

    SetWindowPos(m_hWnd, nullptr, newX, newY, 0, 0,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

// -----------------------------------------------------------------------------------
// Private helpers
// -----------------------------------------------------------------------------------
void CWindowManager::RegisterWindowClass()
{
    WNDCLASS wndClass = {};
    wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndProc;
    wndClass.hInstance = m_hInstance;
    wndClass.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpszClassName = "_XRAY_";

    RegisterClass(&wndClass);
}

void CWindowManager::CreateGameWindow()
{
    const char* wndclass = "_XRAY_";
    const char* title = "S.T.A.L.K.E.R.: Shadow Of Chernobyl";

    DWORD style = GetStyle();
    int winW = m_width, winH = m_height;   // для borderless это и есть размер окна

    int x = CW_USEDEFAULT, y = CW_USEDEFAULT;
    if (m_bWindowed && m_PositionX == CW_USEDEFAULT)
    {
        // Первое создание — центрируем по основному монитору
        HMONITOR hMon = MonitorFromPoint({ 0,0 }, MONITOR_DEFAULTTOPRIMARY);
        ComputeCenteredPosition(hMon, winW, winH, x, y);
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
        wndclass, title, style,
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

void CWindowManager::UpdateWindowAttributes()
{
    if (!m_hWnd) return;

    // 1. Применяем стиль окна (без рамок) и расширенный стиль (TopMost для fullscreen)
    ApplyWindowStyle(m_bWindowed ? 0 : WS_EX_TOPMOST);

    const int winW = m_width;
    const int winH = m_height;

    if (m_bWindowed)
    {
        // Оконный режим: всегда центрируем на текущем мониторе
        HMONITOR hMon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
        int centerX, centerY;
        ComputeCenteredPosition(hMon, winW, winH, centerX, centerY);

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

void CWindowManager::ComputeCenteredPosition(HMONITOR hMonitor, int winW, int winH, int& outX, int& outY)
{
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMonitor, &mi);
    const RECT& work = mi.rcWork;

    outX = work.left + (work.right - work.left - winW) / 2;
    outY = work.top + (work.bottom - work.top - winH) / 2;
}

void CWindowManager::SaveWindowPosition()
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

void CWindowManager::ApplyWindowStyle(DWORD exStyle)
{
    SetWindowLongPtr(m_hWnd, GWL_STYLE, GetStyle());
    SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, exStyle);

    // Заставляем систему пересчитать неклиентскую область (она исчезнет/появится)
    SetWindowPos(m_hWnd, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

// -----------------------------------------------------------------------------------
// Static window procedure
// -----------------------------------------------------------------------------------
LRESULT CALLBACK CWindowManager::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_ACTIVATE:
        Device.OnWM_Activate(wParam, lParam);
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
