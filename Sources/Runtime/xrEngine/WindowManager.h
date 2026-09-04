#pragma once
#include "stdafx.h"
#include "SDL3/SDL.h"

class ENGINE_API CWindowManager
{
public:
    CWindowManager();
    ~CWindowManager();

    void Initialize(bool use_sdl3);
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
    pcstr     m_WindowTitle;

    // Стиль окна (borderless)
    DWORD GetStyleWin32() const { return WS_POPUP | WS_VISIBLE; }

    // Внутренние хелперы
    void RegisterWindowClassWin32();
    void CreateGameWindowWin32();
    void UpdateWindowAttributesWin32();

    // Центрирование: вычисляет координаты относительно рабочей области монитора
    void ComputeCenteredPositionWin32(HMONITOR hMonitor, int winW, int winH, int& outX, int& outY);
    // Сохраняет текущую позицию окна (только если окно существует и не полноэкранное)
    void SaveWindowPositionWin32();
    // Устанавливает стиль и расширенный стиль, затем заставляет систему пересчитать неклиентскую область
    void ApplyWindowStyleWin32(DWORD exStyle = 0);

    static LRESULT CALLBACK WndProcWin32(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    bool m_bUseSDL3;

    SDL_Window* m_pSdlWindow;

    void InitializeWin32();
    void InitializeSDL3();
    void DestroyWin32();
    void DestroySDL3();
    void ApplyWin32();
    void ApplySDL3();
    bool ProcessMessagesWin32();
    bool ProcessMessagesSDL3();
    void SetResolutionWin32(u32 w, u32 h);
    void SetResolutionSDL3(u32 w, u32 h);
    void CenterWindowWin32();
    void CenterWindowSDL3();
};
