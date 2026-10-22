////////////////////////////////////////////////////////////////////////////////
// Created: 23.10.2026 0:19:10
// Author: NS_Deathman
// File: CDllGameModule.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "CDllGameModule.h"
////////////////////////////////////////////////////////////////////////////////
namespace
{
    using FnGetModule = IGameModule * (*)();
}
////////////////////////////////////////////////////////////////////////////////
CDllGameModule::CDllGameModule(pcstr dllName) : m_dllName(dllName ? dllName : "")
{
    R_ASSERT2(!m_dllName.empty(), "CDllGameModule: empty DLL name");

    m_handle = LoadLibraryA(m_dllName.c_str());
    R_ASSERT3(m_handle, "Cannot load game module", m_dllName.c_str());

    auto getModule = (FnGetModule)GetProcAddress(m_handle, "xrGame_GetModule");
    R_ASSERT3(getModule, "Game module missing xrGame_GetModule export", m_dllName.c_str());

    m_instance = getModule();
    R_ASSERT2(m_instance, "xrGame_GetModule returned nullptr");

    Msg("* [GameModule] Loaded '%s' (%s)", m_dllName.c_str(), m_instance->GetName());
}

CDllGameModule::~CDllGameModule()
{
    OnFrameworkShutdown();
}

pcstr CDllGameModule::GetName() const
{
    return m_instance ? m_instance->GetName() : "";
}

TGameObjectFactoryCreate* CDllGameModule::GetGameObjectCreate() const
{
    return m_instance ? m_instance->GetGameObjectCreate() : nullptr;
}

TGameObjectFactoryDestroy* CDllGameModule::GetGameObjectDestroy() const
{
    return m_instance ? m_instance->GetGameObjectDestroy() : nullptr;
}

void CDllGameModule::RegisterConsoleCommands()
{
    if (m_instance) m_instance->RegisterConsoleCommands();
}

void CDllGameModule::RegisterInputCommands()
{
    if (m_instance) m_instance->RegisterInputCommands();
}

IGame_Persistent* CDllGameModule::CreatePersistent()
{
    return m_instance ? m_instance->CreatePersistent() : nullptr;
}

void CDllGameModule::DestroyPersistent(IGame_Persistent* p)
{
    if (m_instance) m_instance->DestroyPersistent(p);
}

ILoadingScreen* CDllGameModule::CreateLoadingScreen()
{
    return m_instance ? m_instance->CreateLoadingScreen() : nullptr;
}

void CDllGameModule::DestroyLoadingScreen(ILoadingScreen* ls)
{
    if (m_instance) m_instance->DestroyLoadingScreen(ls);
}

void CDllGameModule::OnFrameworkShutdown()
{
    m_instance = nullptr;
    if (m_handle)
    {
        FreeLibrary(m_handle);
        m_handle = nullptr;
        Msg("* [GameModule] Unloaded '%s'", m_dllName.c_str());
    }
}
////////////////////////////////////////////////////////////////////////////////
