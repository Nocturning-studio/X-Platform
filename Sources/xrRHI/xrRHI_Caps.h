#pragma once
#include "framework.h"
#include "xrRHI_Internal.h"

RHI_BEGIN

struct RHIDeviceCaps
{
    // ---- Идентификация адаптера ----
    u32         VendorId = 0;
    u32         DeviceId = 0;
    std::string Description;          // например "AMD Radeon RX 6800"

    // ---- Текущий режим рабочего стола (при создании устройства) ----
    u32         DisplayWidth = 0;
    u32         DisplayHeight = 0;
    u32         DisplayRefreshRate = 0;
    RHI_Format  DisplayFormat = RHI_Format::Unknown;

    // ---- Максимальные размеры ресурсов ----
    u32 MaxTextureWidth = 0;
    u32 MaxTextureHeight = 0;
    u32 MaxVolumeExtent = 0;

    // ---- MRT ----
    u32 MaxSimultaneousRTs = 0;

    // ---- Шейдеры ----
    bool HasVertexShader = false;
    bool HasPixelShader = false;
    u32  VertexShaderMajor = 0;
    u32  VertexShaderMinor = 0;
    u32  PixelShaderMajor = 0;
    u32  PixelShaderMinor = 0;
    u32  MaxVertexShaderConst = 0;

    u32  VertexCacheMethod = 0;
    u32  VertexCacheSize = 16;

    // ---- Depth/Stencil ----
    bool HasDepthStencil = false;

    // ---- Фильтрация ----
    u32  MaxAnisotropy = 1;

    // ---- Текстурные стадии ----
    u32 MaxTextureBlendStages = 0;
    u32 MaxSimultaneousTextures = 0;

    // ---- Аппаратные возможности ----
    bool HardwareTnL = false;
    bool SupportsPureDevice = false;
    bool SupportsNonPow2Textures = false;
};

RHI_END
