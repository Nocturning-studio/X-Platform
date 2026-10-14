////////////////////////////////////////////////////////////////////////////////
// Created: 14.10.2026 20:41:02
// Author: NS_Deathman
// File: IEngineHost.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "xrFrameworkAPI.h"
////////////////////////////////////////////////////////////////////////////////
class IGame_Persistent;
////////////////////////////////////////////////////////////////////////////////
extern "C"
{
    typedef DLL_API DLL_Pure* __cdecl Factory_Create(CLASS_ID CLS_ID);
    typedef DLL_API void __cdecl Factory_Destroy(DLL_Pure* O);
};
////////////////////////////////////////////////////////////////////////////////
// Хост-приложение. Реализуется в Game.exe / Editor.exe.
// xrFramework ничего не знает о конкретном приложении — он лишь вызывает хуки.
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API IEngineHost
{
public:
    virtual ~IEngineHost() = default;

    // ---- Идентификация ----
    virtual pcstr GetApplicationName() const = 0;
    virtual pcstr GetApplicationSaveName() const = 0;

    // ---- Что грузить динамически ----
    virtual pcstr GetGameDllName()   const { return "xrGame.dll"; }
    virtual pcstr GetRenderDllName() const { return "xrRender.dll"; }

    virtual pcstr GetSplashImagePath() const { return "splash.png"; }

    // ---- Жизненный цикл ----
    virtual bool OnPreInitialize() { return true; }
    virtual bool OnPostInitialize() { return true; }
    virtual void ProcessStartupCommands() {}
    virtual void OnFrame() {}
    virtual void OnPreDestroy() {}

    // ---- Фабрики game-persistent ----
    // По умолчанию используется CLSID_GAME_PERSISTANT через фабрику xrGame.dll.
    // Editor.exe может переопределить.
    virtual IGame_Persistent* CreateGamePersistent(Factory_Create* factory);
    virtual void DestroyGamePersistent(IGame_Persistent* p, Factory_Destroy* factory);
};
////////////////////////////////////////////////////////////////////////////////
