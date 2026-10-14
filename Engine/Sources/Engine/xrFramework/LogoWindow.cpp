#include "pch.h"
#include "LogoWindow.h"
#include "Engine.h"
#include "SDL3/SDL.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

// -----------------------------------------------------------------------------------
// Вспомогательное: путь к splash-картинке относительно exe
// -----------------------------------------------------------------------------------
static bool ResolveExeRelativePath(pcstr relPath, string_path outFullPath)
{
	if (!relPath || !relPath[0])
		return false;

	string_path exePath;
	if (!GetModuleFileNameA(NULL, exePath, sizeof(exePath)))
		return false;

	// Отрезаем имя exe
	char* lastSlash = strrchr(exePath, '\\');
	if (!lastSlash)
		return false;
	*(lastSlash + 1) = '\0';

	strconcat(sizeof(string_path), outFullPath, exePath, relPath);
	return true;
}

// -----------------------------------------------------------------------------------
// Construction / Destruction
// -----------------------------------------------------------------------------------
LogoWindow::LogoWindow()
{
	// Инициализируем видео-подсистему (идемпотентно; InitSDL3 позже вызовет SDL_Init ещё раз)
	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
		{
			Msg("! LogoWindow: SDL_InitSubSystem(VIDEO) failed: %s", SDL_GetError());
			return;
		}
	}

	// Путь к сплешу запрашиваем у хоста
	pcstr splashRel = (Engine.AppHost) ? Engine.AppHost->GetSplashImagePath() : nullptr;
	if (!splashRel || !splashRel[0])
	{
		Msg("* LogoWindow: splash image disabled by host");
		return;
	}

	string_path splashFull;
	if (!ResolveExeRelativePath(splashRel, splashFull))
	{
		Msg("! LogoWindow: cannot resolve splash path '%s'", splashRel);
		return;
	}

	// Загружаем PNG через stb_image (RGBA8)
	int w = 0, h = 0, comp = 0;
	stbi_uc* pixels = stbi_load(splashFull, &w, &h, &comp, 4);
	if (!pixels)
	{
		Msg("! LogoWindow: cannot load '%s': %s", splashFull, stbi_failure_reason());
		return;
	}

	// Создаём borderless окно по размеру картинки, поверх всех
	SDL_WindowFlags flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP;
	m_window = SDL_CreateWindow("", w, h, flags);
	if (!m_window)
	{
		Msg("! LogoWindow: SDL_CreateWindow failed: %s", SDL_GetError());
		stbi_image_free(pixels);
		return;
	}

	SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	m_renderer = SDL_CreateRenderer(m_window, nullptr);
	if (!m_renderer)
	{
		Msg("! LogoWindow: SDL_CreateRenderer failed: %s", SDL_GetError());
		stbi_image_free(pixels);
		return;
	}

	m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h);
	if (!m_texture)
	{
		Msg("! LogoWindow: SDL_CreateTexture failed: %s", SDL_GetError());
		stbi_image_free(pixels);
		return;
	}

	SDL_SetTextureBlendMode(m_texture, SDL_BLENDMODE_BLEND);
	SDL_UpdateTexture(m_texture, nullptr, pixels, w * 4);
	stbi_image_free(pixels);

	// Первый кадр
	SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
	SDL_RenderClear(m_renderer);
	SDL_RenderTexture(m_renderer, m_texture, nullptr, nullptr);
	SDL_RenderPresent(m_renderer);

	SDL_ShowWindow(m_window);
	m_visible = true;

	Msg("* LogoWindow: splash '%s' shown (%dx%d)", splashFull, w, h);
}

LogoWindow::~LogoWindow()
{
	if (m_texture) { SDL_DestroyTexture(m_texture);   m_texture = nullptr; }
	if (m_renderer) { SDL_DestroyRenderer(m_renderer); m_renderer = nullptr; }
	if (m_window) { SDL_DestroyWindow(m_window);     m_window = nullptr; }
}

// -----------------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------------
void LogoWindow::Show()
{
	if (!m_window) return;
	SDL_ShowWindow(m_window);
	m_visible = true;
}

void LogoWindow::Hide()
{
	if (!m_window) return;
	SDL_HideWindow(m_window);
	m_visible = false;
}

void LogoWindow::Pump()
{
	if (!m_visible || !m_window) return;

	SDL_PumpEvents();
}
