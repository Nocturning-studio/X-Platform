////////////////////////////////////////////////////////////////////////////////
// Refactored: Font Manager
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "FontManager.h"
#include "igame_persistent.h"

CFontManager::CFontManager()
{
	pSystemFont = nullptr;

	// Инициализируем указатели нулями
	pFontMedium = nullptr;
	pFontDI = nullptr;
	pFontArial14 = nullptr;
	pFontGraffiti19Russian = nullptr;
	pFontGraffiti22Russian = nullptr;
	pFontLetterica16Russian = nullptr;
	pFontLetterica18Russian = nullptr;
	pFontGraffiti32Russian = nullptr;
	pFontGraffiti50Russian = nullptr;
	pFontLetterica25 = nullptr;
	pFontStat = nullptr;

	// Регистрируем игровые шрифты в список для массового обновления/удаления
	m_all_fonts.push_back(&pFontMedium);
	m_all_fonts.push_back(&pFontDI);
	m_all_fonts.push_back(&pFontArial14);
	m_all_fonts.push_back(&pFontGraffiti19Russian);
	m_all_fonts.push_back(&pFontGraffiti22Russian);
	m_all_fonts.push_back(&pFontLetterica16Russian);
	m_all_fonts.push_back(&pFontLetterica18Russian);
	m_all_fonts.push_back(&pFontGraffiti32Russian);
	m_all_fonts.push_back(&pFontGraffiti50Russian);
	m_all_fonts.push_back(&pFontLetterica25);
	m_all_fonts.push_back(&pFontStat);

	// Регистрируемся на сброс устройства
	Device.seqDeviceReset.Add(this, REG_PRIORITY_HIGH);
}

CFontManager::~CFontManager()
{
	Destroy();
	Device.seqDeviceReset.Remove(this);
}

void CFontManager::Initialize()
{
	InitializeFonts();
}

LPCSTR CFontManager::GetFontTexName(LPCSTR section)
{
	// Используем улучшенную логику из "нового" класса (поддержка 2k/4k)
	static char* tex_names[] = {"texture800", "texture", "texture1600", "texture2k"};
	int def_idx = 1; // default 1024x768
	int idx = def_idx;

	u32 h = Device.dwHeight;

	if (h <= 600)
		idx = 0;
	else if (h <= 1024)
		idx = 2;
	else if (h <= 1440)
		idx = 3;
	else
		idx = 3;

	while (idx >= 0)
	{
		if (pSettings->line_exist(section, tex_names[idx]))
			return pSettings->r_string(section, tex_names[idx]);
		--idx;
	}

	return pSettings->r_string(section, tex_names[def_idx]);
}

void CFontManager::InitializeFont(CGameFont*& F, LPCSTR section, u32 flags)
{
	LPCSTR font_tex_name = GetFontTexName(section);
	R_ASSERT(font_tex_name);

	if (!F)
	{
		F = xr_new<CGameFont>("font", font_tex_name, flags);
	}
	else
	{
		F->Initialize("font", font_tex_name);
	}

#ifdef DEBUG
	F->m_font_name = section;
#endif

	if (pSettings->line_exist(section, "size"))
	{
		float sz = pSettings->r_float(section, "size");
		if (flags & CGameFont::fsDeviceIndependent)
			F->SetHeightI(sz);
		else
			F->SetHeight(sz);
	}
	if (pSettings->line_exist(section, "interval"))
		F->SetInterval(pSettings->r_fvector2(section, "interval"));
}

void CFontManager::InitializeFonts()
{
	// 1. Инициализация системного шрифта
	LPCSTR sys_font_sect = "ui_font_graffiti19_russian";
	LPCSTR sys_font_tex = GetFontTexName(sys_font_sect);

	if (!pSystemFont)
	{
		pSystemFont = xr_new<CGameFont>("font", sys_font_tex, 0);
		Device.seqRender.Add(pSystemFont, REG_PRIORITY_LOW - 1000);
	}
	else
	{
		pSystemFont->Initialize("font", sys_font_tex);
	}

	if (pSettings->line_exist(sys_font_sect, "size"))
		pSystemFont->SetHeight(pSettings->r_float(sys_font_sect, "size"));

	// 2. Инициализация игровых шрифтов (членов класса)
	InitializeFont(pFontMedium, "hud_font_medium");
	InitializeFont(pFontDI, "hud_font_di", CGameFont::fsGradient | CGameFont::fsDeviceIndependent);
	InitializeFont(pFontArial14, "ui_font_arial_14");
	InitializeFont(pFontGraffiti19Russian, "ui_font_graffiti19_russian");
	InitializeFont(pFontGraffiti22Russian, "ui_font_graffiti22_russian");
	InitializeFont(pFontLetterica16Russian, "ui_font_letterica16_russian");
	InitializeFont(pFontLetterica18Russian, "ui_font_letterica18_russian");
	InitializeFont(pFontGraffiti32Russian, "ui_font_graff_32");
	InitializeFont(pFontGraffiti50Russian, "ui_font_graff_50");
	InitializeFont(pFontLetterica25, "ui_font_letter_25");
	InitializeFont(pFontStat, "stat_font", CGameFont::fsDeviceIndependent);
}

void CFontManager::Destroy()
{
	// Удаляем системный шрифт
	if (pSystemFont)
	{
		Device.seqRender.Remove(pSystemFont);
		xr_delete(pSystemFont);
	}

	// Удаляем игровые шрифты
	FONTS_VEC_IT it = m_all_fonts.begin();
	FONTS_VEC_IT it_e = m_all_fonts.end();
	for (; it != it_e; ++it)
	{
		xr_delete(**it);
	}
}

void CFontManager::OnFrame()
{
	PROFILE_FUNCTION();

	if (pSystemFont)
		pSystemFont->Clear();
}

void CFontManager::Render()
{
	//OPTICK_EVENT("CFontManager::Render");

	// Рендерим только игровые шрифты. Системный рендерится сам через seqRender.
	FONTS_VEC_IT it = m_all_fonts.begin();
	FONTS_VEC_IT it_e = m_all_fonts.end();
	for (; it != it_e; ++it)
	{
		if (*it)
			(**it)->OnRender();
	}
}

void CFontManager::OnDeviceReset()
{
	InitializeFonts();
}
