////////////////////////////////////////////////////////////////////////////////
//  X-Platform — расширение функциональности X-Ray Engine
//
//  Автор:        NSDeathman
//  Студия:       Nocturning Studio
//
//  SPDX-FileCopyrightText: 2026 Nocturning Studio и участники X-Platform
//  SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
//
//  Этот файл распространяется на условиях PolyForm Noncommercial 1.0.0.
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <X-RHI/include/XRHI.h>
////////////////////////////////////////////////////////////////////////////////

struct IDirect3DDevice9Ex;

// ============================================================================
// CStateCache
// ============================================================================
//
// Самостоятельный кеш D3D9-состояний для legacy-фасада (CRenderBackendFacade).
//
// Контракт:
//   - SetDevice() вызывается при создании/сбросе устройства.
//   - Invalidate() — после Reset/пересоздания (сбрасывает и значения,
//     и validity-флаги; Get* после этого возвращают D3D9-дефолты).
//   - Set*State() применяют diff относительно внутреннего кеша; если кеш
//     невалиден — форсированно пишут все поля (учтите это при переключении
//     на ручное управление device->SetRenderState: после такой записи
//     зовите Invalidate() либо используйте только методы кеша).
//
// ============================================================================
class ENGINE_API CStateCache
{
public:
    CStateCache() = default;
    ~CStateCache() = default;

    CStateCache(const CStateCache&) = delete;
    CStateCache& operator=(const CStateCache&) = delete;

    // =====================================================================
    // Device binding
    // =====================================================================
    void SetDevice(IDirect3DDevice9Ex* device);
    IDirect3DDevice9Ex* GetDevice() const { return m_pDevice; }

    // Сброс всех кешей. После вызова Get* возвращают дефолты D3D9.
    void Invalidate();

    // =====================================================================
    // RHI-native setters (агрегированные)
    // =====================================================================
    void SetBlendState(const RHI_BlendState& state);
    const RHI_BlendState& GetBlendState() const { return m_blendCache; }

    void SetDepthStencilState(const RHI_DepthStencilState& state);
    const RHI_DepthStencilState& GetDepthStencilState() const { return m_depthCache; }

    void SetRasterizerState(const RHI_RasterizerState& state);
    const RHI_RasterizerState& GetRasterizerState() const { return m_rasterCache; }

    // =====================================================================
    // Scalar states
    // =====================================================================
    void SetStencilRef(uint8_t ref);
    uint8_t GetStencilRef() const { return m_stencilRef; }

    // Blend factor: ARGB, 0xAARRGGBB — как у D3DRS_BLENDFACTOR.
    void SetBlendFactor(uint32_t argb);
    void SetBlendFactor(const fvec4& rgba);
    uint32_t GetBlendFactorARGB() const { return m_blendFactor; }

    // =====================================================================
    // Dynamic rasterizer
    // =====================================================================
    void SetViewport(const RHI_Viewport& vp);
    const RHI_Viewport& GetViewport() const { return m_viewportCache; }

    void SetScissorRect(const RHI_Rect* rect);
    bool GetScissorRect(RHI_Rect& out) const;

    // =====================================================================
    // Convenience setters (меняют одно поле поверх текущего состояния)
    // =====================================================================
    void SetBlend(bool enable, RHI_Blend src, RHI_Blend dst);
    void SetBlendEx(bool enable, RHI_Blend src, RHI_Blend dst, RHI_BlendOp op);

    void SetStencil(bool enable,
        RHI_CmpFunc func,
        uint8_t ref, uint8_t mask, uint8_t writemask,
        RHI_StencilOp fail, RHI_StencilOp pass, RHI_StencilOp zfail);

    void SetColorWriteEnable(uint8_t mask);
    void SetDepthWriteEnable(bool enable);
    void SetCullMode(RHI_CullMode mode);

    // =====================================================================
    // Getters
    // =====================================================================
    bool GetBlendEnable() const { return m_blendCache.enable; }
    RHI_Blend GetSrcBlend() const { return m_blendCache.srcColor; }
    RHI_Blend GetDstBlend() const { return m_blendCache.dstColor; }
    RHI_CullMode GetCullMode() const { return m_rasterCache.cullMode; }

private:
    IDirect3DDevice9Ex* m_pDevice = nullptr;

    // ---- Cached state values (D3D9-дефолты) ----
    RHI_BlendState        m_blendCache{};
    RHI_DepthStencilState m_depthCache{};
    RHI_RasterizerState   m_rasterCache{};
    RHI_Viewport          m_viewportCache{};
    RHI_Rect              m_scissorCache{};

    uint8_t  m_stencilRef = 0;
    uint32_t m_blendFactor = 0xFFFFFFFF; // (1,1,1,1) — D3D9 default

    // ---- Cache validity ----
    bool m_blendValid = false;
    bool m_depthValid = false;
    bool m_rasterValid = false;
    bool m_viewportValid = false;
    bool m_scissorValid = false;
    bool m_scissorEnabled = false;
    bool m_stencilRefValid = false;
    bool m_blendFactorValid = false;
};
////////////////////////////////////////////////////////////////////////////////
