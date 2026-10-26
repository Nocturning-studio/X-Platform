////////////////////////////////////////////////////////////////////////////////
// Created: 23.10.2026 0:36:54
// Author: NS_Deathman
// File: GameModule.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrFramework/IGameModule.h>
////////////////////////////////////////////////////////////////////////////////
class CGameModule final : public IGameModule
{
public:
    // IGameModule
    const char* GetName() const override { return "xrGame (STALKER SHOC)"; }

    TGameObjectFactoryCreate* GetGameObjectCreate()  const override;
    TGameObjectFactoryDestroy* GetGameObjectDestroy() const override;

    void RegisterConsoleCommands() override;
    void RegisterInputCommands()   override;

    IAppContext* CreateGameContext() override;
    void DestroyGameContext(IAppContext*) override;

    ILoadingScreen* CreateLoadingScreen() override;
    void DestroyLoadingScreen(ILoadingScreen*) override;
};
////////////////////////////////////////////////////////////////////////////////
