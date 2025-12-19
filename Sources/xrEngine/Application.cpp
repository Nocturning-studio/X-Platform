////////////////////////////////////////////////////////////////////////////////
// Created: 14.01.2025
// Author: NSDeathman
// Refactored code: Application class realization
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Application.h"
#include "Optick_Capture.h"
#include "igame_level.h"
#include "igame_persistent.h"
#include "xr_input.h"
#include "xr_ioconsole.h"
#include "x_ray.h"
#include "std_classes.h"
#include "GameFont.h"
#include "resource.h"
#include "LightAnimLibrary.h"
#include "ispatial.h"
#include "Text_Console.h"
#include <process.h>
#include "xr_ioconsole.h"
#include "../xrDiscordAPI/DiscordAPI.h"
////////////////////////////////////////////////////////////////////////////////
#pragma todo(NSDeathman to NSDeathman : Отрефакторить)
int max_load_stage = 0;
////////////////////////////////////////////////////////////////////////////////
#pragma todo(NSDeathman to NSDeathman : Отрефакторить)
struct _SoundProcessor : public pureFrame
{
	virtual void OnFrame()
	{
		Device.Statistic->Sound.Begin();
		::Sound->update(Device.vCameraPosition, Device.vCameraDirection, Device.vCameraTop);
		Device.Statistic->Sound.End();
	}
} SoundProcessor;
////////////////////////////////////////////////////////////////////////////////
LPCSTR _GetFontTexName(LPCSTR section)
{
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

void _InitializeFont(CGameFont*& F, LPCSTR section, u32 flags)
{
	LPCSTR font_tex_name = _GetFontTexName(section);
	R_ASSERT(font_tex_name);

	if (!F)
	{
		F = xr_new<CGameFont>("font", font_tex_name, flags);
		Device.seqRender.Add(F, REG_PRIORITY_LOW - 1000);
	}
	else
		F->Initialize("font", font_tex_name);

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

CApplication::CApplication()
{
	OPTICK_EVENT("CApplication::CApplication");

	ll_dwReference = 0;

	// events
	eQuit = Engine.Event.Handler_Attach("KERNEL:quit", this);
	eStart = Engine.Event.Handler_Attach("KERNEL:start", this);
	eStartLoad = Engine.Event.Handler_Attach("KERNEL:load", this);
	eDisconnect = Engine.Event.Handler_Attach("KERNEL:disconnect", this);

	// levels
	Level_Current = 0;
	Level_Scan();

	// Font
	pFontSystem = NULL;

	// Register us
	Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 1000);

	if (psDeviceFlags.test(mtSound))
		Device.seqFrameMT.Add(&SoundProcessor);
	else
		Device.seqFrame.Add(&SoundProcessor);

	//Console->Show();

	DiscordAPI.Init();

#ifdef ENABLE_PROFILING
	OptickCapture.Initialize();
#endif

	// App Title
	app_title[0] = '\0';
}

CApplication::~CApplication()
{
	OPTICK_EVENT("CApplication::~CApplication");

	Console->Hide();

#ifdef ENABLE_PROFILING
	OptickCapture.Destroy();
#endif

	// font
	Device.seqRender.Remove(pFontSystem);
	xr_delete(pFontSystem);

	Device.seqFrameMT.Remove(&SoundProcessor);
	Device.seqFrame.Remove(&SoundProcessor);
	Device.seqFrame.Remove(this);

	// events
	Engine.Event.Handler_Detach(eDisconnect, this);
	Engine.Event.Handler_Detach(eStartLoad, this);
	Engine.Event.Handler_Detach(eStart, this);
	Engine.Event.Handler_Detach(eQuit, this);
}

void CApplication::OnEvent(EVENT E, u64 P1, u64 P2)
{
	OPTICK_EVENT("CApplication::OnEvent");

	if (E == eQuit)
	{
		PostQuitMessage(0);

		for (u32 i = 0; i < Levels.size(); i++)
		{
			xr_free(Levels[i].folder);
			xr_free(Levels[i].name);
		}
	}
	else if (E == eStart)
	{
		LPSTR op_server = LPSTR(P1);
		LPSTR op_client = LPSTR(P2);
		R_ASSERT(0 == g_pGameLevel);
		R_ASSERT(0 != g_pGamePersistent);

#ifdef NO_SINGLE
		Console->Execute("main_menu on");
		if (op_server == NULL || strstr(op_server, "/deathmatch") || strstr(op_server, "/teamdeathmatch") ||
			strstr(op_server, "/artefacthunt"))
#endif
		{
			Console->Execute("main_menu off");
			Console->Hide();
			// FIX BY IXRAY (THANKS BY NSDeathman)
			// Device.Reset(false);
			//-----------------------------------------------------------
			g_pGamePersistent->PreStart(op_server);
			//-----------------------------------------------------------
			g_pGameLevel = (IGame_Level*)NEW_INSTANCE(CLSID_GAME_LEVEL);
			pApp->LoadBegin();
			Msg("\nStart level loading...");
			Msg("Start Game Persistent... ");
			g_pGamePersistent->Start(op_server);
			Msg("Start Game Level... ");
			g_pGameLevel->net_Start(op_server, op_client);
			pApp->LoadEnd();
		}
		xr_free(op_server);
		xr_free(op_client);
	}
	else if (E == eDisconnect)
	{
		if (g_pGameLevel)
		{
			// Get visibility of console BEFORE hiding, for show it again if we needed
			//bool bConsoleVisibility = Console->GetVisibility();

			//if (bConsoleVisibility)
			//	Console->Hide();

			g_pGameLevel->net_Stop();
			DEL_INSTANCE(g_pGameLevel);

			//if (bConsoleVisibility)
			//	Console->Show();

			if ((FALSE == Engine.Event.Peek("KERNEL:quit")) && (FALSE == Engine.Event.Peek("KERNEL:start")))
			{
				Console->Execute("main_menu off");
				Console->Execute("main_menu on");
			}
		}
		R_ASSERT(0 != g_pGamePersistent);
		g_pGamePersistent->Disconnect();
	}
}

static CTimer phase_timer;
extern ENGINE_API BOOL g_appLoaded = FALSE;

void CApplication::LoadBegin()
{
	OPTICK_EVENT("CApplication::LoadBegin");

	ll_dwReference++;
	if (1 == ll_dwReference)
	{
		g_appLoaded = FALSE;

#ifndef DEDICATED_SERVER
		_InitializeFont(pFontSystem, "ui_font_graffiti19_russian", 0);

		ll_hGeom.create(FVF::F_TL, RenderBackend.Vertex.Buffer(), RenderBackend.QuadIB);
		sh_progress.create("hud\\default", "ui\\ui_load");
		ll_hGeom2.create(FVF::F_TL, RenderBackend.Vertex.Buffer(), NULL);
#endif
		phase_timer.Start();
		load_stage = 0;
	}
}

void CApplication::LoadEnd()
{
	OPTICK_EVENT("CApplication::LoadEnd");

	ll_dwReference--;

	if (0 == ll_dwReference)
	{
		g_pGamePersistent->LoadTitle("st_loading_end");
		Msg("* phase time: %d ms", phase_timer.GetElapsed_ms());
		Msg("* phase cmem: %d K", Memory.mem_usage() / 1024);
		Console->Execute("stat_memory");
		g_appLoaded = TRUE;
		//		DUMP_PHASE;
	}
}

void CApplication::destroy_loading_shaders()
{
	OPTICK_EVENT("CApplication::destroy_loading_shaders");

	hLevelLogo.destroy();
	sh_progress.destroy();
	//.	::Sound->mute			(false);
}

u32 calc_progress_color(u32 idx, u32 total, int stage, int max_stage)
{
	if (idx > (total / 2))
		idx = total - idx;

	float kk = (float(stage + 1) / float(max_stage)) * (total / 2.0f);
	float f = 1 / (expf((float(idx) - kk) * 0.5f) + 1.0f);

	return color_argb_f(f, 1.0f, 1.0f, 1.0f);
}

void CApplication::LoadDraw()
{
	OPTICK_EVENT("CApplication::LoadDraw");

	if (g_appLoaded)
		return;

	Device.dwFrame += 1;

	if (!Device.Begin())
		return;

	if (g_dedicated_server)
		Console->OnRender();
	else
		load_draw_internal();

	Device.End();
}

void CApplication::LoadTitleInt(LPCSTR str)
{
	OPTICK_EVENT("CApplication::LoadTitleInt");

	load_stage++;

	//VERIFY(ll_dwReference);
	VERIFY(str && xr_strlen(str) < 256);
	strcpy_s(app_title, str);
	Msg("* phase time: %d ms", phase_timer.GetElapsed_ms());
	phase_timer.Start();
	Msg("* phase cmem: %d K", Memory.mem_usage() / 1024);
	//.	Console->Execute			("stat_memory");
	Log(app_title);

	if (g_pGamePersistent->GameType() == 1 && strstr(Core.Params, "alife"))
		max_load_stage = 26;
	else
		max_load_stage = 16;

	LoadDraw();
}

void CApplication::LoadSwitch()
{
}

void CApplication::SetLoadLogo(ref_shader NewLoadLogo)
{
	OPTICK_EVENT("CApplication::SetLoadLogo");

	hLevelLogo = NewLoadLogo;
	//	R_ASSERT(0);
};

// Sequential
void CApplication::OnFrame()
{
	OPTICK_EVENT("CApplication::OnFrame");

	Engine.Event.OnFrame();
	g_SpatialSpace->update();
	g_SpatialSpacePhysic->update();

	if (g_pGameLevel)
		g_pGameLevel->SoundEvent_Dispatch();

	if (!g_dedicated_server)
		DiscordAPI.Update();

	if (g_dedicated_server)
		Console->OnFrame();

#ifdef ENABLE_PROFILING
	OptickCapture.OnFrame();
#endif
}

void CApplication::Level_Append(LPCSTR folder)
{
	OPTICK_EVENT("CApplication::Level_Append");

	string_path N1, N2, N3, N4;
	strconcat(sizeof(N1), N1, folder, "level");
	strconcat(sizeof(N2), N2, folder, "level.ltx");
	strconcat(sizeof(N3), N3, folder, "level.geom");
	strconcat(sizeof(N4), N4, folder, "level.cform");
	if (FS.exist("$game_levels$", N1) && FS.exist("$game_levels$", N2) && FS.exist("$game_levels$", N3) &&
		FS.exist("$game_levels$", N4))
	{
		sLevelInfo LI;
		LI.folder = xr_strdup(folder);
		LI.name = 0;
		Levels.push_back(LI);
	}
}

void CApplication::Level_Scan()
{
	OPTICK_EVENT("CApplication::Level_Scan");

#pragma todo("container is created in stack!")
	xr_vector<char*>* folder = FS.file_list_open("$game_levels$", FS_ListFolders | FS_RootOnly);
	R_ASSERT(folder && folder->size());
	for (u32 i = 0; i < folder->size(); i++)
		Level_Append((*folder)[i]);
	FS.file_list_close(folder);
#ifdef DEBUG
	folder = FS.file_list_open("$game_levels$", "$debug$\\", FS_ListFolders | FS_RootOnly);
	if (folder)
	{
		string_path tmp_path;
		for (u32 i = 0; i < folder->size(); i++)
		{
			strconcat(sizeof(tmp_path), tmp_path, "$debug$\\", (*folder)[i]);
			Level_Append(tmp_path);
		}

		FS.file_list_close(folder);
	}
#endif
}

void CApplication::Level_Set(u32 L)
{
	OPTICK_EVENT("CApplication::Level_Set");

	if (L >= Levels.size())
		return;
	Level_Current = L;
	FS.get_path("$level$")->_set(Levels[L].folder);

	string_path temp;
	string_path temp2;
	strconcat(sizeof(temp), temp, "intro\\intro_", Levels[L].folder);
	temp[xr_strlen(temp) - 1] = 0;
	if (FS.exist(temp2, "$game_textures$", temp, ".dds"))
		hLevelLogo.create("font", temp);
	else
		hLevelLogo.create("font", "intro\\intro_no_start_picture");
}

int CApplication::Level_ID(LPCSTR name)
{
	OPTICK_EVENT("CApplication::Level_ID");

	char buffer[256];
	strconcat(sizeof(buffer), buffer, name, "\\");
	for (u32 I = 0; I < Levels.size(); I++)
	{
		if (0 == xr_stricmp(buffer, Levels[I].folder))
			return int(I);
	}
	return -1;
}

#pragma optimize("g", off)
void CApplication::load_draw_internal()
{
	OPTICK_EVENT("CApplication::load_draw_internal");

	if (!sh_progress)
	{
		RenderBackend.Clear(0, 0, CLEAR_RENDERTARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1, 0);
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
	Fvector2 k;
	k.set(float(_w) / bw, float(_h) / bh);

	RenderBackend.set_Shader(sh_progress);
	CTexture* T = RenderBackend.get_ActiveTexture(0);
	Fvector2 tsz;
	tsz.set((float)T->get_Width(), (float)T->get_Height());
	Frect back_text_coords;
	Frect back_coords;
	Fvector2 back_size;

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
	pv = (FVF::TL*)RenderBackend.Vertex.Lock(4, ll_hGeom.stride(), Offset);
	pv->set(back_coords.lt.x, back_coords.rb.y, C, back_text_coords.lt.x, back_text_coords.rb.y);
	pv++;
	pv->set(back_coords.lt.x, back_coords.lt.y, C, back_text_coords.lt.x, back_text_coords.lt.y);
	pv++;
	pv->set(back_coords.rb.x, back_coords.rb.y, C, back_text_coords.rb.x, back_text_coords.rb.y);
	pv++;
	pv->set(back_coords.rb.x, back_coords.lt.y, C, back_text_coords.rb.x, back_text_coords.lt.y);
	pv++;
	RenderBackend.Vertex.Unlock(4, ll_hGeom.stride());

	RenderBackend.set_Geometry(ll_hGeom);
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

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
	pv = (FVF::TL*)RenderBackend.Vertex.Lock(2 * (v_cnt + 1), ll_hGeom2.stride(), Offset);
	FVF::TL* _pv = pv;
	float pos_delta = back_coords.width() / v_cnt;
	float tc_delta = back_text_coords.width() / v_cnt;
	u32 clr = C;

	for (u32 idx = 0; idx < v_cnt + 1; ++idx)
	{
		clr = calc_progress_color(idx, v_cnt, load_stage, max_load_stage);
		pv->set(back_coords.lt.x + pos_delta * idx + offs, back_coords.rb.y + offs, 0 + EPS_S, 1, clr,
				back_text_coords.lt.x + tc_delta * idx, back_text_coords.rb.y);
		pv++;
		pv->set(back_coords.lt.x + pos_delta * idx + offs, back_coords.lt.y + offs, 0 + EPS_S, 1, clr,
				back_text_coords.lt.x + tc_delta * idx, back_text_coords.lt.y);
		pv++;
	}
	VERIFY(u32(pv - _pv) == 2 * (v_cnt + 1));
	RenderBackend.Vertex.Unlock(2 * (v_cnt + 1), ll_hGeom2.stride());

	RenderBackend.set_Geometry(ll_hGeom2);
	RenderBackend.Render(D3DPT_TRIANGLESTRIP, Offset, 2 * v_cnt);

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
		r.rb.add(r.lt, Fvector2().set(512, 256));
		r.lt.mul(k);
		r.rb.mul(k);
		pv = (FVF::TL*)RenderBackend.Vertex.Lock(4, ll_hGeom.stride(), Offset);
		pv->set(r.lt.x, r.rb.y, C, 0, 1);
		pv++;
		pv->set(r.lt.x, r.lt.y, C, 0, 0);
		pv++;
		pv->set(r.rb.x, r.rb.y, C, 1, 1);
		pv++;
		pv->set(r.rb.x, r.lt.y, C, 1, 0);
		pv++;
		RenderBackend.Vertex.Unlock(4, ll_hGeom.stride());

		RenderBackend.set_Shader(hLevelLogo);
		RenderBackend.set_Geometry(ll_hGeom);
		RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}
