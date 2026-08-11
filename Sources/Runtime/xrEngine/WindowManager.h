#pragma once
#include "stdafx.h"

class ENGINE_API CWindowManager
{
public:
    CWindowManager();
    ~CWindowManager();

    void Initialize();
    void Destroy();
    void Apply();               // применить накопленные параметры
    void Reset() { Apply(); }   // совместимость

    bool ProcessMessages();

    // Геттеры
    HWND  GetHandle() const { return m_hWnd; }
    bool  IsWindowed() const { return m_bWindowed; }
    u32   GetRefreshRate() const { return m_RefreshRate; }
    fvec2 GetResolution() const { return { (float)m_width, (float)m_height }; }
    u32   GetWidth()  const { return m_width; }
    u32   GetHeight() const { return m_height; }

    // Сеттеры (накапливают состояние, Apply() применит)
    void SetWindowed(bool bWindowed) { m_bWindowed = bWindowed; }
    void SetRefreshRate(u32 rate) { m_RefreshRate = rate; }
    void SetResolution(ivec2 res) { SetResolution((u32)res.x, (u32)res.y); }
    void SetResolution(u32 w, u32 h);
    void UpdateSize(u32 w, u32 h) { SetResolution(w, h); }   // для обратной совместимости

    void CenterWindow();

private:
    HWND      m_hWnd;
    HINSTANCE m_hInstance;
    u32       m_width;
    u32       m_height;
    bool      m_bWindowed;
    u32       m_RefreshRate;
    int       m_PositionX;
    int       m_PositionY;
    bool      m_bQuitRequested;
    bool      m_bInitialized;

    // Стиль окна (borderless)
    DWORD GetStyle() const { return WS_POPUP | WS_VISIBLE; }

    // Внутренние хелперы
    void RegisterWindowClass();
    void CreateGameWindow();
    void UpdateWindowAttributes();

    // Центрирование: вычисляет координаты относительно рабочей области монитора
    void ComputeCenteredPosition(HMONITOR hMonitor, int winW, int winH, int& outX, int& outY);
    // Сохраняет текущую позицию окна (только если окно существует и не полноэкранное)
    void SaveWindowPosition();
    // Устанавливает стиль и расширенный стиль, затем заставляет систему пересчитать неклиентскую область
    void ApplyWindowStyle(DWORD exStyle = 0);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
