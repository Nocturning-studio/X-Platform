////////////////////////////////////////////////////////////////////////////////
// Created: 26.10.2026 23:01:22
// Author: NS_Deathman
// File: EditorContext.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrFramework/IAppContext.h>
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API IRender_Visual;
class ENGINE_API CGameFont;
////////////////////////////////////////////////////////////////////////////////
class CEditorContext final : public IAppContext
{
public:
    CEditorContext();
    ~CEditorContext() override;

    void OnAppStart() override;
    void OnAppEnd()   override;
    void OnFrame()    override;

    void OnAppActivate()   override {}
    void OnAppDeactivate() override {}

    void LoadTitle(LPCSTR) override {}
    bool CanBePaused() override { return false; }

    void  RegisterModel(IRender_Visual* V) override;
    float MtlTransparent(u32 mtl_idx) override;
    void  Statistics(CGameFont*) override {}

    bool OnRenderPPUI_query() override { return false; }
    void OnRenderPPUI_main()  override {}
    void OnRenderPPUI_PP()    override {}
};
////////////////////////////////////////////////////////////////////////////////
