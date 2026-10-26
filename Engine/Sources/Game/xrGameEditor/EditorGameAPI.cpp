////////////////////////////////////////////////////////////////////////////////
// Created: 26.10.2026 22:47:14
// Author: NS_Deathman
// File: EditorGameAPI.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "EditorGameAPI.h"
#include <xrGame/editor/GameEditorAPI.h>
////////////////////////////////////////////////////////////////////////////////
extern "C" DLL_API SEditorGameAPI* __cdecl xrGame_GetEditorAPI();

namespace
{
	SEditorGameAPI* g_api = nullptr;
}

void CEditorGameAPI::Load()
{
	g_api = xrGame_GetEditorAPI();
	R_ASSERT2(g_api, "xrGame.dll missing xrGame_GetEditorAPI");
	R_ASSERT2(g_api->version == 1, "SEditorGameAPI version mismatch");
}

void CEditorGameAPI::Unload() { g_api = nullptr; }

CSE_Abstract* CEditorGameAPI::CreateServerObject(LPCSTR section)
{
	return g_api->CreateServerObject(section);
}
void CEditorGameAPI::DestroyServerObject(CSE_Abstract* e)
{
	g_api->DestroyServerObject(e);
}
void CEditorGameAPI::InitGameGlobals() 
{ 
	g_api->InitGameGlobals(); 
}
void CEditorGameAPI::CleanGameGlobals() 
{ 
	g_api->CleanGameGlobals(); 
}
void CEditorGameAPI::RegisterModel(IRender_Visual* V) 
{ 
	g_api->RegisterModel(V); 
}
float CEditorGameAPI::MtlTransparent(u32 mtl_idx) 
{ 
	return g_api->MtlTransparent(mtl_idx); 
}
////////////////////////////////////////////////////////////////////////////////
