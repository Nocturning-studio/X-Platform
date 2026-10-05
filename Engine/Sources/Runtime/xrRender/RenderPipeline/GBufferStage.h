////////////////////////////////////////////////////////////////////////////////
// Created: 04.10.2026 18:54:41
// Author: NS_Deathman
// File: GBufferStage.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "IRenderStage.h"
#include "DeferredLightingResources.h"
////////////////////////////////////////////////////////////////////////////////
class CGBufferStage : public IRenderStage
{
public:
    explicit CGBufferStage(CDeferredLightingResources& res) : m_res(res) {}

    std::string_view GetName() const override { return "GBufferStage"; }

    void OnAttach() override {}
    void OnDeviceReset() override {}
    void Execute() override;
    void OnDetach() override {}

private:
    void BindTargets();
    void ClearTargets();
    void RenderScene();

    CDeferredLightingResources& m_res;
};
////////////////////////////////////////////////////////////////////////////////
