////////////////////////////////////////////////////////////////////////////////
// Created: 23.10.2026 0:18:55
// Author: NS_Deathman
// File: CDllGameModule.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "IGameModule.h"
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API CDllGameModule final : public IGameModule
{
public:
    explicit CDllGameModule(const char* dllName);
    ~CDllGameModule() override;

    CDllGameModule(const CDllGameModule&) = delete;
    CDllGameModule& operator=(const CDllGameModule&) = delete;

    // IGameModule
    const char* GetName() const override;
    TGameObjectFactoryCreate* GetGameObjectCreate()  const override;
    TGameObjectFactoryDestroy* GetGameObjectDestroy() const override;
    void RegisterConsoleCommands() override;
    void RegisterInputCommands()   override;
    IGame_Persistent* CreatePersistent() override;
    void DestroyPersistent(IGame_Persistent* p) override;
    ILoadingScreen* CreateLoadingScreen() override;
    void DestroyLoadingScreen(ILoadingScreen* ls) override;

    void OnFrameworkShutdown() override;

private:
    xr_string m_dllName;
    HMODULE m_handle = nullptr;
    IGameModule* m_instance = nullptr;
};
////////////////////////////////////////////////////////////////////////////////
