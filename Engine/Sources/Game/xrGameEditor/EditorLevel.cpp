////////////////////////////////////////////////////////////////////////////////
// Created: 26.10.2026 23:05:44
// Author: NS_Deathman
// File: EditorLevel.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "EditorLevel.h"
#include "EditorGameAPI.h"
#include <xrFramework/Engine.h>
#include <xrRender/render.h>
#include <xrFramework/xr_object.h>
#include <xrFramework/xr_object_list.h>
#include <xrGame/xrServer_Objects.h>
////////////////////////////////////////////////////////////////////////////////
CEditorLevel::CEditorLevel() 
{ 
    Msg("* [EditorLevel] Constructed"); 
}
CEditorLevel::~CEditorLevel() 
{ 
    Msg("* [EditorLevel] Destroyed"); 
}

void CEditorLevel::NewLevel()
{
    ClearLevel();
    m_level_name = "untitled";
    bReady = true;

    Fbox bounds;
    bounds.set(-16384, -16384, -16384, 16384, 16384, 16384);
    if (!g_SpatialSpace->m_root)
        g_SpatialSpace->initialize(bounds);
    if (!g_SpatialSpacePhysic->m_root)
        g_SpatialSpacePhysic->initialize(bounds);

    Render->NewLevel();

    Engine.Events.Render.Add(this);
    Engine.Events.Frame.Add(this);
    Msg("* [EditorLevel] New scene");
}

void CEditorLevel::net_Stop()
{
    Engine.Events.Render.Remove(this);
    Engine.Events.Frame.Remove(this);

    ClearLevel();
    Render->DestroyLevel();

    bReady = false;
}

void CEditorLevel::ClearLevel()
{
    m_selection = nullptr;
    for (CObject* obj : m_scene_objects)
        if (obj) Objects.ForceDestroy(obj);
    m_scene_objects.clear();
    m_next_object_id = 1;
}

void CEditorLevel::OnFrame()
{
    if (!bReady) return;
    Objects.Update(false);
}

void CEditorLevel::OnRender()
{
    if (!bReady) return;

    for (CObject* obj : m_scene_objects)
    {
        if (!obj || obj->getDestroy() || !obj->getVisible()) continue;
        obj->renderable_Render();
    }

#ifndef DEDICATED_SERVER
    Render->Calculate();
    Render->Render();
#endif
}

CObject* CEditorLevel::SpawnObject(LPCSTR section, const fvec3& pos, const fvec3& rot)
{
    CSE_Abstract* E = CEditorGameAPI::CreateServerObject(section);
    if (!E) { Msg("! [EditorLevel] Section '%s' not found", section); return nullptr; }

    E->s_name = section;
    E->set_name_replace("");
    E->s_gameid = 0;
    E->o_Position = pos;
    E->o_Angle = rot;
    E->s_RP = 0xff;
    E->ID = m_next_object_id++;
    E->ID_Parent = 0xffff;
    E->ID_Phantom = 0xffff;
    E->RespawnTime = 0;
    E->s_flags.assign(M_SPAWN_OBJECT_LOCAL);

    CObject* O = Objects.Create(*E->s_name);
    if (!O)
    {
        CEditorGameAPI::DestroyServerObject(E);
        return nullptr;
    }

    if (!O->net_Spawn(E))
    {
        O->net_Destroy();
        Objects.ForceDestroy(O);
        CEditorGameAPI::DestroyServerObject(E);
        return nullptr;
    }

    CEditorGameAPI::DestroyServerObject(E);
    m_scene_objects.push_back(O);
    return O;
}

void CEditorLevel::DeleteObject(CObject* obj)
{
    if (!obj) return;
    if (m_selection == obj) m_selection = nullptr;

    auto it = std::find(m_scene_objects.begin(), m_scene_objects.end(), obj);
    if (it != m_scene_objects.end()) m_scene_objects.erase(it);

    Objects.ForceDestroy(obj);
}

BOOL CEditorLevel::LoadEditor(LPCSTR path) 
{ 
    Msg("LoadEditor('%s') stub", path); return FALSE; 
}
BOOL CEditorLevel::SaveEditor(LPCSTR path) 
{ 
    Msg("SaveEditor('%s') stub", path); return FALSE; 
}
////////////////////////////////////////////////////////////////////////////////
