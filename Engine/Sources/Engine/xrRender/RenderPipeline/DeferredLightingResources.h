////////////////////////////////////////////////////////////////////////////////
// Created: 04.10.2026 18:57:57
// Author: NS_Deathman
// File: DefferedLightingResources.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
class CDeferredLightingResources
{
public:
    CDeferredLightingResources() = default;
    ~CDeferredLightingResources();

    CDeferredLightingResources(const CDeferredLightingResources&) = delete;
    CDeferredLightingResources& operator=(const CDeferredLightingResources&) = delete;

    void Create();
    void Destroy();

    void OnDeviceReset();

    ref_texture FrameColor;
    ref_texture DepthStencil;
    ref_texture GBuffer[3];

    RHI_RenderTargetView FrameColorRTV;
    RHI_RenderTargetView GBufferRTV[3];
    RHI_DepthStencilView DepthDSV;

    uint32_t Width()  const { return m_width; }
    uint32_t Height() const { return m_height; }

private:
    void CreateTextures();
    void CreateViews();
    void DestroyViews();

    uint32_t m_width = 0;
    uint32_t m_height = 0;
};
////////////////////////////////////////////////////////////////////////////////
