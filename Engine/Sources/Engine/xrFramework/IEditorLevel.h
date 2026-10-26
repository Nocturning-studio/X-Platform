////////////////////////////////////////////////////////////////////////////////
// Created: 26.10.2026 22:08:33
// Author: NS_Deathman
// File: IEditorLevel.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "ILevel.h"
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API IEditorLevel : public ILevel
{
public:
    virtual shared_str Name() const = 0;
    virtual void NewLevel() = 0;
    virtual BOOL LoadEditor(LPCSTR path) = 0;
    virtual BOOL SaveEditor(LPCSTR path) = 0;

    virtual CObject* SpawnObject(LPCSTR section, const fvec3& pos, const fvec3& rot) = 0;
    virtual void DeleteObject(CObject* obj) = 0;

    virtual void SetSelection(CObject* obj) = 0;
    virtual CObject* GetSelection() const = 0;
    virtual const xr_vector<CObject*>& GetObjects() const = 0;
};
////////////////////////////////////////////////////////////////////////////////
