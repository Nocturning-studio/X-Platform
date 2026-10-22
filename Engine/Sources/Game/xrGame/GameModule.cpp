////////////////////////////////////////////////////////////////////////////////
// Created: 23.10.2026 0:37:05
// Author: NS_Deathman
// File: GameModule.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "GameModule.h"
#include "LoadingScreen.h"
#include <xrFramework/std_classes.h>
#include <xrFramework/IGame_Persistent.h>
////////////////////////////////////////////////////////////////////////////////
extern void CCC_RegisterCommands();
extern void CCC_RegisterInput();
////////////////////////////////////////////////////////////////////////////////
extern "C"
{
    DLL_API DLL_Pure* __cdecl GameObjectFactory_Create(CLASS_ID clsid);
    DLL_API void __cdecl GameObjectFactory_Destroy(DLL_Pure* O);
}
////////////////////////////////////////////////////////////////////////////////
TGameObjectFactoryCreate* CGameModule::GetGameObjectCreate() const
{
    return &GameObjectFactory_Create;
}

TGameObjectFactoryDestroy* CGameModule::GetGameObjectDestroy() const
{
    return &GameObjectFactory_Destroy;
}

void CGameModule::RegisterConsoleCommands()
{
    CCC_RegisterCommands();
}

void CGameModule::RegisterInputCommands()
{
    CCC_RegisterInput();
}

IGame_Persistent* CGameModule::CreatePersistent()
{
    return (IGame_Persistent*)GameObjectFactory_Create(CLSID_GAME_PERSISTANT);
}

void CGameModule::DestroyPersistent(IGame_Persistent* p)
{
    if (p) GameObjectFactory_Destroy(p);
}

ILoadingScreen* CGameModule::CreateLoadingScreen()
{
    return xr_new<CLoadingScreen>();
}

void CGameModule::DestroyLoadingScreen(ILoadingScreen* ls)
{
    if (ls) ls->Destroy();
}

extern "C"
{
    DLL_API IGameModule* __cdecl xrGame_GetModule()
    {
        static CGameModule s_module;
        return &s_module;
    }
}
////////////////////////////////////////////////////////////////////////////////
