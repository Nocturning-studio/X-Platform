////////////////////////////////////////////////////////////////////////////////
// Created: 05.10.2026 14:06:44
// Author: NS_Deathman
// File: IndirectLightingStage.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "IRenderStage.h"
#include "DeferredLightingResources.h"
////////////////////////////////////////////////////////////////////////////////
class CIndirectLightingStage final : public IRenderStage
{
public:
    explicit CIndirectLightingStage(CDeferredLightingResources& res) : m_res(res) {}

    std::string_view GetName() const override { return "IndirectLightingStage"; }

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
