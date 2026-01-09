#include "stdafx.h"
#include "WindowManager.h"
#include "device.h"	  // Нужно для доступа к Device (пока глобальному)
#include "resource.h" // Для иконок (IDI_ICON1)
#include "xr_ioc_cmd.h"

CWindowManager::CWindowManager() : m_hWnd(NULL), m_hInstance(NULL), m_width(640), m_height(480), m_bQuitRequested(false)
{
}

CWindowManager::~CWindowManager()
{
}

void CWindowManager::Initialize()
{
	Msg("Initializing Window Manager...");
	m_hInstance = GetModuleHandle(NULL);

	RegisterWindowClass();
	CreateGameWindow();
}

void CWindowManager::RegisterWindowClass()
{
	const char* wndclass = "_XRAY_";

	WNDCLASS wndClass = {0};
	wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW; // Стандартные стили для игр
	wndClass.lpfnWndProc = WndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = m_hInstance;
	wndClass.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = wndclass;

	RegisterClass(&wndClass);
}

void CWindowManager::CreateGameWindow()
{
	const char* wndclass = "_XRAY_";
	const char* title = "S.T.A.L.K.E.R.: Shadow Of Chernobyl"; // Можно вынести в настройки

	DWORD dwWindowStyle = WS_BORDER | WS_DLGFRAME | WS_VISIBLE;

	RECT rc;
	SetRect(&rc, 0, 0, m_width, m_height);
	AdjustWindowRect(&rc, dwWindowStyle, FALSE);

	// Создаем окно. Пока ставим CW_USEDEFAULT, потом можно добавить центрирование
	m_hWnd = CreateWindow(wndclass, title, dwWindowStyle, CW_USEDEFAULT, CW_USEDEFAULT, (rc.right - rc.left),
						  (rc.bottom - rc.top), 0L, 0, m_hInstance, 0L);

	R_ASSERT2(m_hWnd, "Failed to create game window");

	// Сохраняем реальные размеры клиентской области
	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);
	m_width = rcClient.right - rcClient.left;
	m_height = rcClient.bottom - rcClient.top;

	Msg("Window created: Handle [0x%p], Resolution [%dx%d]", m_hWnd, m_width, m_height);
}

void CWindowManager::Destroy()
{
	if (m_hWnd)
	{
		DestroyWindow(m_hWnd);
		m_hWnd = NULL;
	}
	UnregisterClass("_XRAY_", m_hInstance);
}

bool CWindowManager::ProcessMessages()
{
	MSG msg;
	// Обрабатываем ВСЕ сообщения в очереди
	while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
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

// Старая логика WndProc, перенесенная сюда
LRESULT CALLBACK CWindowManager::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ACTIVATE: {
		// Пока мы зависим от глобального Device, вызываем его метод здесь
		// В будущем можно сделать Callback из CWindowManager
		Device.OnWM_Activate(wParam, lParam);
		break;
	}
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
		// Используем консоль для корректного завершения
		if (Console)
			Console->Execute("quit");
		return 0;
	case WM_KEYDOWN:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
