////////////////////////////////////////////////////////////////////////////////
// Created: 04.10.2026 15:06:35
// Author: NS_Deathman
// File: OutputStage.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "IRenderStage.h"
#include "DeferredLightingResources.h"
////////////////////////////////////////////////////////////////////////////////
class CFrameOutputStage final : public IRenderStage
{
public:
    explicit CFrameOutputStage(CDeferredLightingResources& res) : m_res(res) {}

    std::string_view GetName() const override { return "FrameOutputStage"; }

    void OnAttach() override;
    void OnDeviceReset() override;
    void Execute() override;
    void OnDetach() override;

private:
    CDeferredLightingResources& m_res;

    CShaderPass m_pass;
    bool m_pass_valid = false;
};
////////////////////////////////////////////////////////////////////////////////
