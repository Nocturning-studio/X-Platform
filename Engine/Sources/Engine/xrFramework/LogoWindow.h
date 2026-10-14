#pragma once
#include "SDL3/SDL.h"

class LogoWindow
{
private:
	SDL_Window* m_window = nullptr;
	SDL_Renderer* m_renderer = nullptr;
	SDL_Texture* m_texture = nullptr;
	bool m_visible = false;

public:
	LogoWindow();
	~LogoWindow();

	void Show();
	void Hide();

	void Pump();
};
