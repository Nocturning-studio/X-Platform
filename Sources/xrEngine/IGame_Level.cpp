#include "stdafx.h"
#include "igame_level.h"
#include "igame_persistent.h"

#include "x_ray.h"
#include "std_classes.h"
#include "customHUD.h"
#include "render.h"
#include "gamefont.h"
#include "xrLevel.h"
#include "CameraManager.h"
#include "../xrEngine/xr_ioconsole.h"

ENGINE_API IGame_Level* g_pGameLevel = NULL;

IGame_Level::IGame_Level()
{
	m_pCameras = xr_new<CCameraManager>(true);
	g_pGameLevel = this;
	pLevel = NULL;
	bReady = false;
	pCurrentEntity = NULL;
	pCurrentViewEntity = NULL;
	pHUD = NULL;
}

#include "resourcemanager.h"

IGame_Level::~IGame_Level()
{
	if (strstr(Core.Params, "-nes_texture_storing"))
		Device.Resources->StoreNecessaryTextures();

	Device.Resources->DeferredUnloadLevelTextures(pLevel->fname());

	xr_delete(pLevel);

	// Render-level unload
	g_pGamePersistent->LoadTitle("st_start_level_unloading");
	Render->level_Unload();

	g_pGamePersistent->LoadTitle("st_unloading_env_mods");
	g_pGamePersistent->Environment().mods_unload();

	g_pGamePersistent->LoadTitle("st_unloading_weathers");
	g_pGamePersistent->Environment().unload();

	xr_delete(m_pCameras);

	// Unregister
	Device.seqRender.Remove(this);
	Device.seqFrame.Remove(this);

	CCameraManager::ResetPP();
}

void IGame_Level::net_Stop()
{
	for (int i = 0; i < 6; i++)
		Objects.Update(true);
	// Destroy all objects
	Objects.Unload();
	IR_Release();

	bReady = false;
}

//-------------------------------------------------------------------------------------------
extern CStatTimer tscreate;

BOOL IGame_Level::Load(u32 dwNum)
{
	// Initialize level data
	pApp->Level_Set(dwNum);
	string_path temp;
	if (!FS.exist(temp, "$level$", "level.ltx"))
		Debug.fatal(DEBUG_INFO, "Can't find level configuration file '%s'.", temp);
	pLevel = xr_new<CInifile>(temp);

	// Open
	g_pGamePersistent->LoadTitle("st_opening_stream");
	IReader* LL_Stream = FS.r_open("$level$", "level");
	IReader& fs = *LL_Stream;

	// Header
	hdrLEVEL H;
	fs.r_chunk_safe(fsL_HEADER, &H, sizeof(H));
	R_ASSERT2(XRCL_PRODUCTION_VERSION == H.XRLC_version, "Incompatible level version.");

	// CForms
	g_pGamePersistent->LoadTitle("st_loading_cform");
	ObjectSpace.Load();
	pApp->LoadSwitch();

	// HUD + Environment
	if (g_hud)
		pHUD = g_hud;
	else
		pHUD = (CCustomHUD*)NEW_INSTANCE(CLSID_HUDMANAGER);

	// Render-level Load
	g_pGamePersistent->LoadTitle("st_start_loading_level");
	Render->level_Load(LL_Stream);
	tscreate.FrameEnd();

	// Objects
	g_pGamePersistent->LoadTitle("st_loading_weathers");
	g_pGamePersistent->Environment().load();

	g_pGamePersistent->LoadTitle("st_loading_env_mods");
	g_pGamePersistent->Environment().mods_load();

	g_pGamePersistent->LoadTitle("st_loading_level_specific_ambients");
	g_pGamePersistent->Environment().load_level_specific_ambients();

	R_ASSERT(Load_GameSpecific_Before());
	Objects.Load();
	R_ASSERT(Load_GameSpecific_After());

	// Done
	FS.r_close(LL_Stream);

	bReady = true;

	if (!g_dedicated_server)
		IR_Capture();

#ifndef DEDICATED_SERVER
	Device.seqRender.Add(this);
#endif

	Device.seqFrame.Add(this);

	return TRUE;
}

void IGame_Level::OnRender()
{
	OPTICK_EVENT("IGame_Level::OnRender");

#ifndef DEDICATED_SERVER
		Render->Calculate();
		Render->Render();
#endif
}

void IGame_Level::OnFrame()
{
	OPTICK_EVENT("IGame_Level::OnFrame");

	// Update all objects
	VERIFY(bReady);
	Objects.Update(false);
	pHUD->OnFrame();

	// Ambience
	if (Sounds_Random.size() && (Device.dwTimeGlobal > Sounds_Random_dwNextTime))
	{
		Sounds_Random_dwNextTime = Device.dwTimeGlobal + ::Random.randI(10000, 20000);
		Fvector pos;
		pos.random_dir().normalize().mul(::Random.randF(30, 100)).add(Device.vCameraPosition);
		int id = ::Random.randI(Sounds_Random.size());
		if (Sounds_Random_Enabled)
		{
			Sounds_Random[id].play_at_pos(0, pos, 0);
			Sounds_Random[id].set_volume(1.f);
			Sounds_Random[id].set_range(10, 200);
		}
	}
}

// ==================================================================================================

void CServerInfo::AddItem(LPCSTR name_, LPCSTR value_, u32 color_)
{
	shared_str s_name(name_);
	AddItem(s_name, value_, color_);
}

void CServerInfo::AddItem(shared_str& name_, LPCSTR value_, u32 color_)
{
	SItem_ServerInfo it;
	strcpy_s(it.name, name_.c_str());
	strcat_s(it.name, " = ");
	strcat_s(it.name, value_);
	it.color = color_;

	if (data.size() < max_item)
	{
		data.push_back(it);
	}
}
