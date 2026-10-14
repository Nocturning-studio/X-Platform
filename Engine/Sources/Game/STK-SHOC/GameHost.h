////////////////////////////////////////////////////////////////////////////////
// Created: 14.10.2026 21:31:24
// Author: NS_Deathman
// File: GameHost.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "xrEngine/IEngineHost.h"
////////////////////////////////////////////////////////////////////////////////
class CGameHost final : public IEngineHost
{
public:
    pcstr GetApplicationName()     const override { return "S.T.A.L.K.E.R.: Shadow Of Chernobyl"; }
    pcstr GetApplicationSaveName() const override { return "stalker_shoc"; }
    pcstr GetGameDllName()         const override { return "xrGame.dll"; }
    pcstr GetRenderDllName()       const override { return "xrRender.dll"; }
    pcstr GetSplashImagePath()     const override { return "game_splash.png"; }

    bool OnPreInitialize() override;
    bool OnPostInitialize() override;
    void ProcessStartupCommands() override;
    void OnFrame() override;
    void OnPreDestroy() override;
};
////////////////////////////////////////////////////////////////////////////////
