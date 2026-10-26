////////////////////////////////////////////////////////////////////////////////
// Created: 26.10.2026 23:01:34
// Author: NS_Deathman
// File: EditorContext.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "EditorContext.h"
#include "EditorGameAPI.h"
#include "xrFramework/Engine.h"
////////////////////////////////////////////////////////////////////////////////
CEditorContext::CEditorContext() { Msg("* [EditorContext] Constructed"); }
CEditorContext::~CEditorContext() { Msg("* [EditorContext] Destroyed"); }

void CEditorContext::OnAppStart()
{
    CEditorGameAPI::InitGameGlobals();
    m_pMainMenu = nullptr;
    Environment().load();
    // Сознательно: без без CMainMenu, без prefetch
}

void CEditorContext::OnAppEnd()
{
    // Не вызываем IAppContext::OnAppEnd() — он unload’ит Environment и DEL_INSTANCE(g_hud)
    Environment().unload();
    CEditorGameAPI::CleanGameGlobals();
}

void CEditorContext::OnFrame() { /* no-op */ }

void CEditorContext::RegisterModel(IRender_Visual* V)
{
    CEditorGameAPI::RegisterModel(V);
}

float CEditorContext::MtlTransparent(u32 mtl_idx)
{
    return CEditorGameAPI::MtlTransparent(mtl_idx);
}
////////////////////////////////////////////////////////////////////////////////
