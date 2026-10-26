////////////////////////////////////////////////////////////////////////////////
// Created: 26.10.2026 22:58:32
// Author: NS_Deathman
// File: EditorModule.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "EditorModule.h"
#include "EditorContext.h"
#include "EditorLevel.h"
#include "EditorGameAPI.h"
////////////////////////////////////////////////////////////////////////////////
extern "C" DLL_API IGameModule * __cdecl xrGame_GetModule();   // из xrGame.dll

CEditorModule::CEditorModule()
{
    CEditorGameAPI::Load();   // получаем SEditorGameAPI
    m_gameModule = xrGame_GetModule();
    R_ASSERT2(m_gameModule, "xrGame.dll missing xrGame_GetModule");
    Msg("* [EditorModule] Wrapping '%s'", m_gameModule->GetName());
}

CEditorModule::~CEditorModule()
{
    m_gameModule = nullptr;
    CEditorGameAPI::Unload();
}

TGameObjectFactoryCreate* CEditorModule::GetGameObjectCreate() const 
{ 
    return m_gameModule->GetGameObjectCreate(); 
}
TGameObjectFactoryDestroy* CEditorModule::GetGameObjectDestroy() const 
{ 
    return m_gameModule->GetGameObjectDestroy(); 
}
void CEditorModule::RegisterConsoleCommands() 
{ 
    m_gameModule->RegisterConsoleCommands(); 
}
void CEditorModule::RegisterInputCommands() 
{ 
    m_gameModule->RegisterInputCommands(); 
}

IAppContext* CEditorModule::CreateEditorContext() 
{ 
    return xr_new<CEditorContext>(); 
}
IEditorLevel* CEditorModule::CreateEditorLevel() 
{ 
    return xr_new<CEditorLevel>(); 
}
////////////////////////////////////////////////////////////////////////////////
