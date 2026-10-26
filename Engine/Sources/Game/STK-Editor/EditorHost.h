////////////////////////////////////////////////////////////////////////////////
// Created: 26.10.2026 22:42:00
// Author: NS_Deathman
// File: EditorHost.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "xrFramework/IEngineHost.h"
////////////////////////////////////////////////////////////////////////////////
class CDllGameModule;
////////////////////////////////////////////////////////////////////////////////
class CEditorHost final : public IEngineHost
{
  public:
	CEditorHost();
	~CEditorHost() override;

	pcstr GetApplicationName() const override { return "S.T.A.L.K.E.R. Editor"; }
	pcstr GetApplicationSaveName() const override { return "stalker_editor"; }
	pcstr GetRenderDllName() const override { return "xrRender.dll"; }
	pcstr GetSplashImagePath() const override { return "editor_splash.png"; }

	IGameModule* GetGameModule() override;

	bool OnPreInitialize() override { return true; }
	bool OnPostInitialize() override;
	void ProcessStartupCommands() override;
	void OnFrame() override;
	void OnPreDestroy() override {}

  private:
	CDllGameModule* m_gameModule = nullptr;
	IEditorLevel* m_level = nullptr;
};
////////////////////////////////////////////////////////////////////////////////
