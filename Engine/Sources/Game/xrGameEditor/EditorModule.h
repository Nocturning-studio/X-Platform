////////////////////////////////////////////////////////////////////////////////
// Created: 26.10.2026 22:58:04
// Author: NS_Deathman
// File: EditorModule.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrFramework/IGameModule.h>
////////////////////////////////////////////////////////////////////////////////
class CEditorModule final : public IGameModule
{
public:
    CEditorModule();
    ~CEditorModule() override;

    const char* GetName() const override { return "xrGameEditor"; }

    TGameObjectFactoryCreate* GetGameObjectCreate()  const override;
    TGameObjectFactoryDestroy* GetGameObjectDestroy() const override;
    void RegisterConsoleCommands() override;
    void RegisterInputCommands() override;

    IAppContext* CreateGameContext() override { return nullptr; }
    void DestroyGameContext(IAppContext*) override {}

    // Editor
    IAppContext* CreateEditorContext() override;
    IEditorLevel* CreateEditorLevel() override;

    ILoadingScreen* CreateLoadingScreen() override { return nullptr; }
    void DestroyLoadingScreen(ILoadingScreen*) override {}

private:
    IGameModule* m_gameModule = nullptr;
};
////////////////////////////////////////////////////////////////////////////////
