////////////////////////////////////////////////////////////////////////////////
// Created: 14.10.2026 21:31:33
// Author: NS_Deathman
// File: GameHost.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "GameHost.h"
////////////////////////////////////////////////////////////////////////////////
bool CGameHost::OnPreInitialize()
{
    return true;
}

bool CGameHost::OnPostInitialize()
{
    return true;
}

void CGameHost::ProcessStartupCommands()
{
    if (LPCSTR p = strstr(Core.Params, "-start "))
        Console->Execute(p + 1);

    if (LPCSTR p = strstr(Core.Params, "-load "))
        Console->Execute(p + 1);

    if (strstr(Core.Params, "-load_last_save"))
        Console->Execute("load_last_save");

    if (strstr(Core.Params, "-load_last_quick_save"))
        Console->Execute("load_last_quick_save");
}

void CGameHost::OnFrame()
{
}

void CGameHost::OnPreDestroy()
{
}
////////////////////////////////////////////////////////////////////////////////
