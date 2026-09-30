////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <d3d9.h>
#include "xrRenderBackendAPI.h"
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CSurface
{
public:
    CSurface() = default;
    ~CSurface() = default;

    CSurface(const CSurface&) = delete;
    CSurface& operator=(const CSurface&) = delete;
    CSurface(CSurface&& o) noexcept : m_d3d9Surface(o.m_d3d9Surface) { o.m_d3d9Surface = nullptr; }
    CSurface& operator=(CSurface&& o) noexcept
    {
        if (this != &o)
        {
            RELEASE(m_d3d9Surface);
            m_d3d9Surface = o.m_d3d9Surface;
            o.m_d3d9Surface = nullptr;
        }
        return *this;
    }

    bool IsValid() const { return m_d3d9Surface != nullptr; }

    IDirect3DSurface9* GetD3D9() const { return m_d3d9Surface; }

    // D3D12_CPU_DESCRIPTOR_HANDLE GetDX12RTV() const;
    // D3D12_CPU_DESCRIPTOR_HANDLE GetDX12DSV() const;

    static CSurface CreateD3D9(IDirect3DSurface9* s)
    {
        CSurface r;
        r.m_d3d9Surface = s;
        return r;
    }

private:
    IDirect3DSurface9* m_d3d9Surface = nullptr;
};
////////////////////////////////////////////////////////////////////////////////
