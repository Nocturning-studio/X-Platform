#pragma once

#include "stdafx.h"
#include "GameFont.h"

class ENGINE_API CLevelLoadingScreen
{
  private:
	// Ресурсы рендера
	ref_shader hLevelLogo;
	ref_geom ll_hGeom;
	ref_geom ll_hGeom2;
	ref_shader sh_progress;

	// Шрифты и текст
	CGameFont* pFontSystem;
	string256 app_title;

	// Состояние
	bool bIsActive;
	int load_stage;
	int max_load_stage;
	u32 ll_dwReference; // Счетчик ссылок (для вложенных вызовов)

	// Таймер фаз загрузки
	CTimer phase_timer;

	// Вспомогательные методы
	void InitializeFont();
	u32 CalcProgressColor(u32 idx, u32 total, int stage, int max_stage);
	void DrawInternal();

  public:
	CLevelLoadingScreen();
	~CLevelLoadingScreen();

	void Destroy();

	// Управление жизненным циклом экрана
	void Show();
	void Hide();
	void ForceRender(); // Принудительная отрисовка (LoadDraw)

	// Обновление данных
	void SetTitle(LPCSTR str);
	void UpdateLevelLogo(); // Подтянет логотип текущего уровня из LevelManager

	bool IsActive() const
	{
		return bIsActive;
	}
};

extern ENGINE_API BOOL g_appLoaded;
