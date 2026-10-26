////////////////////////////////////////////////////////////////////////////////
// Created: 14.10.2026 21:31:33
// Author: NS_Deathman
// File: GameHost.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "GameHost.h"
#include "xrFramework/CDllGameModule.h" 
////////////////////////////////////////////////////////////////////////////////
CGameHost::CGameHost() : m_gameModule(nullptr)
{
}

CGameHost::~CGameHost()
{
    xr_delete(m_gameModule);
    m_gameModule = nullptr;
}

IGameModule* CGameHost::GetGameModule()
{
    if (!m_gameModule)
        m_gameModule = xr_new<CDllGameModule>("xrGame.dll");
    return m_gameModule;
}

void CGameHost::ProcessStartupCommands()
{
    if (LPCSTR p = strstr(Core.Params, "-start "))
        Engine.Console->Execute(p + 1);

    if (LPCSTR p = strstr(Core.Params, "-load "))
        Engine.Console->Execute(p + 1);

    if (strstr(Core.Params, "-load_last_save"))
        Engine.Console->Execute("load_last_save");

    if (strstr(Core.Params, "-load_last_quick_save"))
        Engine.Console->Execute("load_last_quick_save");
}
////////////////////////////////////////////////////////////////////////////////
