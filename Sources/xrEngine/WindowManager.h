#pragma once
#include "stdafx.h"

class ENGINE_API CWindowManager
{
  public:
	CWindowManager();
	~CWindowManager();

	void Initialize();
	void Destroy();

	// Обработка очереди сообщений. Возвращает false, если пришел сигнал выхода.
	bool ProcessMessages();

	// Геттеры
	HWND GetHandle() const
	{
		return m_hWnd;
	}
	float2 GetResolution() const
	{
		return {(float)m_width, (float)m_height};
	}
	void UpdateSize(u32 w, u32 h)
	{
		m_width = w;
		m_height = h;
	}
	void SetResolution(int2 Resolution)
	{
		m_width = Resolution.x;
		m_height = Resolution.y;
	}
	void SetResolution(u32 ResolutionX, u32 ResolutionY)
	{
		m_width = ResolutionX;
		m_height = ResolutionY;
	}
	u32 GetWidth()
	{
		return m_width;
	}
	u32 GetHeight()
	{
		return m_height;
	}

  private:
	HWND m_hWnd;
	HINSTANCE m_hInstance;
	u32 m_width;
	u32 m_height;
	bool m_bQuitRequested;

	// Статическая оконная процедура для WinAPI
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Приватные хелперы
	void RegisterWindowClass();
	void CreateGameWindow();
};
