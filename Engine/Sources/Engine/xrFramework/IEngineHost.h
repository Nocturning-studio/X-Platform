////////////////////////////////////////////////////////////////////////////////
// Created: 14.10.2026 20:41:02
// Author: NS_Deathman
// File: IEngineHost.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "xrFrameworkAPI.h"
#include "GameObjectFactory.h"
////////////////////////////////////////////////////////////////////////////////
class IAppContext;
class ILoadingScreen;
class IGameModule;
////////////////////////////////////////////////////////////////////////////////
// Хост-приложение. Реализуется в Game.exe / Editor.exe.
// xrFramework ничего не знает о конкретном приложении — он лишь вызывает хуки.
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API IEngineHost
{
public:
    virtual ~IEngineHost() = default;

    virtual const char* GetApplicationName() const = 0;
    virtual const char* GetApplicationSaveName() const = 0;
    virtual const char* GetSplashImagePath() const { return "splash.png"; }

    // Хост возвращает игровой модуль. Может быть nullptr (bare framework).
    // Game.exe / Editor.exe возвращают CDllGameModule (или in-process реализацию).
    virtual IGameModule* GetGameModule() { return nullptr; }

    virtual const char* GetRenderDllName() const { return "xrRender.dll"; }

    // Lifecycle
    virtual bool OnPreInitialize() { return true; }
    virtual bool OnPostInitialize() { return true; }
    virtual void ProcessStartupCommands() {}
    virtual void OnFrame() {}
    virtual void OnPreDestroy() {}
};
////////////////////////////////////////////////////////////////////////////////
