////////////////////////////////////////////////////////////////////////////////
// Created: 14.01.2025
// Author: NSDeathman
// Refactored code: Application class realization
////////////////////////////////////////////////////////////////////////////////
#ifndef APPLICATION_INCLUDED
#define APPLICATION_INCLUDED
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "EventAPI.h"
#include "pure.h"
#include "device.h"
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API CApplication : public pureFrame, public IEventReceiver
{
  private:
	string256 app_title;

	ref_shader hLevelLogo;
	ref_geom ll_hGeom;
	ref_geom ll_hGeom2;

	ref_shader sh_progress;
	int load_stage;

	u32 ll_dwReference;

	EVENT eQuit;
	EVENT eStart;
	EVENT eStartLoad;
	EVENT eDisconnect;

	void Level_Append(LPCSTR lname);

  public:
	CGameFont* pFontSystem;

	// Levels
	struct sLevelInfo
	{
		char* folder;
		char* name;
	};
	xr_vector<sLevelInfo> Levels;
	u32 Level_Current;
	void Level_Scan();
	int Level_ID(LPCSTR name);
	void Level_Set(u32 ID);

	// Loading
	void LoadBegin();
	void LoadEnd();
	void LoadTitleInt(LPCSTR str);
	void SetLoadLogo(ref_shader NewLoadLogo);
	void LoadSwitch();
	void LoadDraw();

	virtual void OnEvent(EVENT E, u64 P1, u64 P2);

	// Other
	CApplication();
	~CApplication();

	virtual void OnFrame();
	void load_draw_internal();
	void destroy_loading_shaders();
};
////////////////////////////////////////////////////////////////////////////////
extern ENGINE_API CApplication* pApp;
////////////////////////////////////////////////////////////////////////////////
#endif //APPLICATION_INCLUDED
////////////////////////////////////////////////////////////////////////////////
