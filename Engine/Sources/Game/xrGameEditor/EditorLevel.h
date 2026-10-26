////////////////////////////////////////////////////////////////////////////////
// Created: 26.10.2026 23:05:20
// Author: NS_Deathman
// File: EditorLevel.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrFramework/IEditorLevel.h>
////////////////////////////////////////////////////////////////////////////////
class CSE_Abstract;
////////////////////////////////////////////////////////////////////////////////
class CEditorLevel final : public IEditorLevel
{
public:
    CEditorLevel();
    ~CEditorLevel() override;

    // ILevel pure virtuals
    shared_str Name() const override { return m_level_name; }
    void GetLevelInfo(CServerInfo*) override {}

    BOOL net_Start(LPCSTR, LPCSTR) override { return FALSE; }
    void net_Load(LPCSTR) override {}
    void net_Save(LPCSTR) override {}
    void net_Stop() override;
    void net_Update() override {}
    void Load_GameSpecific_CFORM(CDB::TRI*, u32) override {}

    // ILevel virtuals
    BOOL Load(u32) override { return FALSE; }
    void OnEvent(EVENT E, u64 P1, u64 P2) override {}
    void OnFrame() override;
    void OnRender() override;

    // IEditorLevel
    void NewLevel() override;
    BOOL LoadEditor(LPCSTR path) override;
    BOOL SaveEditor(LPCSTR path) override;
    CObject* SpawnObject(LPCSTR section, const fvec3& pos, const fvec3& rot) override;
    void DeleteObject(CObject* obj) override;
    void SetSelection(CObject* obj) override { m_selection = obj; }
    CObject* GetSelection() const override { return m_selection; }
    const xr_vector<CObject*>& GetObjects() const override { return m_scene_objects; }

private:
    void ClearLevel();

    xr_vector<CObject*> m_scene_objects;
    CObject* m_selection = nullptr;
    shared_str m_level_name = "untitled";
    u16 m_next_object_id = 1;
};
////////////////////////////////////////////////////////////////////////////////
