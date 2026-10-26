////////////////////////////////////////////////////////////////////////////////
// Created: 26.10.2026 22:42:00
// Author: NS_Deathman
// File: EditorHost.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "EditorHost.h"
#include "xrFramework/CDllGameModule.h" 
////////////////////////////////////////////////////////////////////////////////
CEditorHost::CEditorHost() : m_gameModule(nullptr)
{
}

CEditorHost::~CEditorHost()
{
    xr_delete(m_gameModule);
    m_gameModule = nullptr;
}

IGameModule* CEditorHost::GetGameModule()
{
    if (!m_gameModule)
        m_gameModule = xr_new<CDllGameModule>("xrGameEditor.dll", "xrEditor_GetModule");
    return m_gameModule;
}


void CEditorHost::ProcessStartupCommands()
{

}

bool CEditorHost::OnPostInitialize()
{
    m_level = Engine.CreateEditorLevel();
    if (!m_level) 
    { 
        Msg("! [Editor] Game module did not provide editor level"); 
        return false; 
    }
    m_level->NewLevel();
    return true;
}

void CEditorHost::OnFrame()
{

}
////////////////////////////////////////////////////////////////////////////////
