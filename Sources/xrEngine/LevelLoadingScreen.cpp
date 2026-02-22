#include "stdafx.h"
#include "LevelLoadingScreen.h"
#include "Engine.h"			  // Для доступа к Engine.LevelManager
#include "igame_persistent.h" // Для g_pGamePersistent
#include "xr_ioconsole.h"

// Глобальная переменная для совместимости, если она используется где-то еще
ENGINE_API BOOL g_appLoaded;

// Вспомогательная функция для выбора текстуры шрифта (скрыта в cpp)
static LPCSTR _GetFontTexName(LPCSTR section)
{
	static char* tex_names[] = {"texture800", "texture", "texture1600", "texture2k"};
	int def_idx = 1;
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

CLevelLoadingScreen::CLevelLoadingScreen()
{
	pFontSystem = nullptr;
	bIsActive = false;
	load_stage = 0;
	max_load_stage = 16;
	ll_dwReference = 0;
	app_title[0] = 0;
}

CLevelLoadingScreen::~CLevelLoadingScreen()
{
}

void CLevelLoadingScreen::Destroy()
{
	if (pFontSystem)
	{
		Engine.Events.Render.Remove(pFontSystem);
		xr_delete(pFontSystem);
	}
	hLevelLogo.destroy();
	sh_progress.destroy();
}

void CLevelLoadingScreen::InitializeFont()
{
	LPCSTR section = "ui_font_graffiti19_russian";
	LPCSTR font_tex_name = _GetFontTexName(section);
	R_ASSERT(font_tex_name);

	if (!pFontSystem)
	{
		pFontSystem = xr_new<CGameFont>("font", font_tex_name, 0);
		Engine.Events.Render.Add(pFontSystem, REG_PRIORITY_LOW - 1000);
	}
	else
	{
		pFontSystem->Initialize("font", font_tex_name);
	}

	if (pSettings->line_exist(section, "size"))
	{
		float sz = pSettings->r_float(section, "size");
		pFontSystem->SetHeight(sz);
	}
	if (pSettings->line_exist(section, "interval"))
		pFontSystem->SetInterval(pSettings->r_fvector2(section, "interval"));
}

void CLevelLoadingScreen::Show()
{
	ll_dwReference++;
	if (1 == ll_dwReference)
	{
		bIsActive = true;
		g_appLoaded = FALSE;

#ifndef DEDICATED_SERVER
		InitializeFont();

		// Инициализация шейдеров и геометрии
		ll_hGeom.create(FVF::F_TL, RenderBackendLegacy.Vertex.Buffer(), RenderBackendLegacy.QuadIB);
		sh_progress.create("hud\\default", "ui\\ui_load");
		ll_hGeom2.create(FVF::F_TL, RenderBackendLegacy.Vertex.Buffer(), NULL);

		// Обновляем логотип уровня
		UpdateLevelLogo();
#endif
		phase_timer.Start();
		load_stage = 0;
	}
}

void CLevelLoadingScreen::Hide()
{
	ll_dwReference--;
	if (0 == ll_dwReference)
	{
		bIsActive = false;
		if (g_pGamePersistent)
			g_pGamePersistent->LoadTitle("st_loading_end");

		Msg("* phase time: %d ms", phase_timer.GetElapsed_ms());
		Msg("* phase cmem: %d K", Memory.mem_usage() / 1024);
		Console->Execute("stat_memory");
		g_appLoaded = TRUE;

		// Очистка ресурсов, чтобы не висели в видеопамяти
		hLevelLogo.destroy();
		sh_progress.destroy();
	}
}

void CLevelLoadingScreen::SetTitle(LPCSTR str)
{
	load_stage++;
	VERIFY(str && xr_strlen(str) < 256);
	strcpy_s(app_title, str);

	Msg("* phase time: %d ms", phase_timer.GetElapsed_ms());
	phase_timer.Start();
	Msg("* phase cmem: %d K", Memory.mem_usage() / 1024);
	Log(app_title);

	if (g_pGamePersistent && g_pGamePersistent->GameType() == 1 && strstr(Core.Params, "alife"))
		max_load_stage = 26;
	else
		max_load_stage = 16;

	ForceRender();
}

void CLevelLoadingScreen::ForceRender()
{
	if (!bIsActive || g_appLoaded)
		return;

	Engine.TimeManager.IncreaseFrameCount();

	if (!Device.Begin())
		return;

	if (g_dedicated_server)
		Console->OnRender();
	else
		DrawInternal(); // Внутренняя отрисовка

	Device.End();
}

void CLevelLoadingScreen::UpdateLevelLogo()
{
	// Используем наш новый Engine.LevelManager
	LPCSTR folderName = Engine.LevelManager.GetCurrentLevelFolderName();

	if (!folderName)
	{
		hLevelLogo.create("font", "intro\\intro_no_start_picture");
		return;
	}

	string_path temp;
	string_path temp2;
	strconcat(sizeof(temp), temp, "intro\\intro_", folderName);

	// Убираем слеш в конце, если есть
	size_t len = xr_strlen(temp);
	if (len > 0 && temp[len - 1] == '\\')
		temp[len - 1] = 0;

	if (FS.exist(temp2, "$game_textures$", temp, ".dds"))
		hLevelLogo.create("font", temp);
	else
		hLevelLogo.create("font", "intro\\intro_no_start_picture");
}

u32 CLevelLoadingScreen::CalcProgressColor(u32 idx, u32 total, int stage, int max_stage)
{
	if (idx > (total / 2))
		idx = total - idx;
	float kk = (float(stage + 1) / float(max_stage)) * (total / 2.0f);
	float f = 1 / (expf((float(idx) - kk) * 0.5f) + 1.0f);
	return color_argb_f(f, 1.0f, 1.0f, 1.0f);
}

#pragma optimize("g", off)
void CLevelLoadingScreen::DrawInternal()
{
	//OPTICK_EVENT("CApplication::load_draw_internal");

	if (!sh_progress)
	{
		RenderBackendLegacy.Clear(0, 0, CLEAR_RENDERTARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1, 0);
		return;
	}
	// Draw logo
	u32 Offset;
	u32 C = 0xffffffff;
	u32 _w = Device.dwWidth;
	u32 _h = Device.dwHeight;
	FVF::TL* pv = NULL;

	// progress
	float bw = 1024.0f;
	float bh = 768.0f;
	float2 k;
	k.set(float(_w) / bw, float(_h) / bh);

	RenderBackendLegacy.set_Shader(sh_progress);
	CTexture* T = RenderBackendLegacy.get_ActiveTexture(0);
	float2 tsz;
	tsz.set((float)T->get_Width(), (float)T->get_Height());
	Frect back_text_coords;
	Frect back_coords;
	float2 back_size;

	// progress background
	static float offs = -0.5f;

	back_size.set(1024, 768);
	back_text_coords.lt.set(0, 0);
	back_text_coords.rb.add(back_text_coords.lt, back_size);
	back_coords.lt.set(offs, offs);
	back_coords.rb.add(back_coords.lt, back_size);

	back_coords.lt.mul(k);
	back_coords.rb.mul(k);

	back_text_coords.lt.x /= tsz.x;
	back_text_coords.lt.y /= tsz.y;
	back_text_coords.rb.x /= tsz.x;
	back_text_coords.rb.y /= tsz.y;
	pv = (FVF::TL*)RenderBackendLegacy.Vertex.Lock(4, ll_hGeom.stride(), Offset);
	pv->set(back_coords.lt.x, back_coords.rb.y, C, back_text_coords.lt.x, back_text_coords.rb.y);
	pv++;
	pv->set(back_coords.lt.x, back_coords.lt.y, C, back_text_coords.lt.x, back_text_coords.lt.y);
	pv++;
	pv->set(back_coords.rb.x, back_coords.rb.y, C, back_text_coords.rb.x, back_text_coords.rb.y);
	pv++;
	pv->set(back_coords.rb.x, back_coords.lt.y, C, back_text_coords.rb.x, back_text_coords.lt.y);
	pv++;
	RenderBackendLegacy.Vertex.Unlock(4, ll_hGeom.stride());

	RenderBackendLegacy.set_Geometry(ll_hGeom);
	RenderBackendLegacy.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

	// progress bar
	back_size.set(268, 37);
	back_text_coords.lt.set(0, 768);
	back_text_coords.rb.add(back_text_coords.lt, back_size);
	back_coords.lt.set(379, 726);
	back_coords.rb.add(back_coords.lt, back_size);

	back_coords.lt.mul(k);
	back_coords.rb.mul(k);

	back_text_coords.lt.x /= tsz.x;
	back_text_coords.lt.y /= tsz.y;
	back_text_coords.rb.x /= tsz.x;
	back_text_coords.rb.y /= tsz.y;

	u32 v_cnt = 40;
	pv = (FVF::TL*)RenderBackendLegacy.Vertex.Lock(2 * (v_cnt + 1), ll_hGeom2.stride(), Offset);
	FVF::TL* _pv = pv;
	float pos_delta = back_coords.width() / v_cnt;
	float tc_delta = back_text_coords.width() / v_cnt;
	u32 clr = C;

	for (u32 idx = 0; idx < v_cnt + 1; ++idx)
	{
		clr = CalcProgressColor(idx, v_cnt, load_stage, max_load_stage);
		pv->set(back_coords.lt.x + pos_delta * idx + offs, back_coords.rb.y + offs, 0 + EPS_S, 1, clr,
				back_text_coords.lt.x + tc_delta * idx, back_text_coords.rb.y);
		pv++;
		pv->set(back_coords.lt.x + pos_delta * idx + offs, back_coords.lt.y + offs, 0 + EPS_S, 1, clr,
				back_text_coords.lt.x + tc_delta * idx, back_text_coords.lt.y);
		pv++;
	}
	VERIFY(u32(pv - _pv) == 2 * (v_cnt + 1));
	RenderBackendLegacy.Vertex.Unlock(2 * (v_cnt + 1), ll_hGeom2.stride());

	RenderBackendLegacy.set_Geometry(ll_hGeom2);
	RenderBackendLegacy.Render(D3DPT_TRIANGLESTRIP, Offset, 2 * v_cnt);

	// Draw title
	VERIFY(pFontSystem);
	pFontSystem->Clear();
	pFontSystem->SetColor(color_rgba(157, 140, 120, 255));
	pFontSystem->SetAligment(CGameFont::alCenter);
	pFontSystem->OutI(0.f, 0.815f, app_title);
	pFontSystem->OnRender();

	// draw level-specific screenshot
	if (hLevelLogo)
	{
		Frect r;
		r.lt.set(257, 369);
		r.lt.x += offs;
		r.lt.y += offs;
		r.rb.add(r.lt, float2().set(512, 256));
		r.lt.mul(k);
		r.rb.mul(k);
		pv = (FVF::TL*)RenderBackendLegacy.Vertex.Lock(4, ll_hGeom.stride(), Offset);
		pv->set(r.lt.x, r.rb.y, C, 0, 1);
		pv++;
		pv->set(r.lt.x, r.lt.y, C, 0, 0);
		pv++;
		pv->set(r.rb.x, r.rb.y, C, 1, 1);
		pv++;
		pv->set(r.rb.x, r.lt.y, C, 1, 0);
		pv++;
		RenderBackendLegacy.Vertex.Unlock(4, ll_hGeom.stride());

		RenderBackendLegacy.set_Shader(hLevelLogo);
		RenderBackendLegacy.set_Geometry(ll_hGeom);
		RenderBackendLegacy.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}
