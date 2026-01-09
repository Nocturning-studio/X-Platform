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
	Fvector2 GetResolution() const
	{
		return {(float)m_width, (float)m_height};
	}
	void UpdateSize(u32 w, u32 h)
	{
		m_width = w;
		m_height = h;
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
