////////////////////////////////////////////////////////////////////////////////
// Created: 14.10.2026 21:31:24
// Author: NS_Deathman
// File: GameHost.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "xrFramework/IEngineHost.h"
////////////////////////////////////////////////////////////////////////////////
class CDllGameModule;
////////////////////////////////////////////////////////////////////////////////
class CGameHost final : public IEngineHost
{
  public:
	CGameHost();
	~CGameHost() override;

	pcstr GetApplicationName() const override { return "S.T.A.L.K.E.R.: Shadow Of Chernobyl"; }
	pcstr GetApplicationSaveName() const override { return "stalker_shoc"; }
	pcstr GetRenderDllName() const override { return "xrRender.dll"; }
	pcstr GetSplashImagePath() const override { return "game_splash.png"; }

	IGameModule* GetGameModule() override;

	bool OnPreInitialize() override { return true; }
	bool OnPostInitialize() override { return true; }
	void ProcessStartupCommands() override;
	void OnFrame() override {}
	void OnPreDestroy() override {}

  private:
	CDllGameModule* m_gameModule = nullptr;
};
////////////////////////////////////////////////////////////////////////////////
