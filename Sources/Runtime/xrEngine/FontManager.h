////////////////////////////////////////////////////////////////////////////////
// Refactored: Font Manager
// Merged SFontManager and CFontManager logic
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "stdafx.h"
#include "GameFont.h"
#include "pure.h"

class ENGINE_API CFontManager : public pureDeviceReset
{
  public:
	// Вектор указателей на указатели шрифтов (для массовой обработки)
	typedef xr_vector<CGameFont**> FONTS_VEC;
	typedef FONTS_VEC::iterator FONTS_VEC_IT;
	FONTS_VEC m_all_fonts;

	// --- Игровые шрифты (Public для доступа из Game DLL) ---
	CGameFont* pFontMedium;
	CGameFont* pFontDI;
	CGameFont* pFontArial14;
	CGameFont* pFontGraffiti19Russian;
	CGameFont* pFontGraffiti22Russian;
	CGameFont* pFontLetterica16Russian;
	CGameFont* pFontLetterica18Russian;
	CGameFont* pFontGraffiti32Russian;
	CGameFont* pFontGraffiti50Russian;
	CGameFont* pFontLetterica25;
	CGameFont* pFontStat;

	// --- Системный шрифт (Engine usage) ---
	// Он отделен от общего списка, так как рендерится через seqRender
  private:
	CGameFont* pSystemFont;

	// Внутренние методы
	void InitializeFonts();
	void InitializeFont(CGameFont*& F, LPCSTR section, u32 flags = 0);
	LPCSTR GetFontTexName(LPCSTR section);

  public:
	CFontManager();
	~CFontManager();

	void Initialize();
	void Destroy();

	// Очистка буфера системного шрифта перед кадром
	void OnFrame();

	// Рендер игровых шрифтов (вызывается из UI/GamePersistent)
	void Render();

	// Обработка потери устройства (Alt+Tab, смена разрешения)
	virtual void OnDeviceReset();

	// Доступ к системному шрифту
	CGameFont* GetSystemFont() const
	{
		return pSystemFont;
	}
};
