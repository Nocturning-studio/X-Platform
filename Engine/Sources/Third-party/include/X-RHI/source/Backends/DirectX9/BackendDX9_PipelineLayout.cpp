////////////////////////////////////////////////////////////////////////////////
// Created: 12.10.2026 21:07:00
// Author: NS_Deathman
// File: BackendDX9_PipelineLayout.cpp
// Nocturning studio for X-RHI
////////////////////////////////////////////////////////////////////////////////
#include <source/pch.h>
#include "BackendDX9.h"
////////////////////////////////////////////////////////////////////////////////
RHI_PipelineLayoutHandle CRenderBackendDX9::CreatePipelineLayout(const RHI_PipelineLayoutDesc& desc)
{
    if (!m_pDevice)
        return {};

    DX9PipelineLayout* lay = new DX9PipelineLayout;
    lay->desc = desc;
    return m_pipelineLayouts.Allocate(lay);
}

void CRenderBackendDX9::DestroyPipelineLayout(RHI_PipelineLayoutHandle handle)
{
    DX9PipelineLayout* lay = m_pipelineLayouts.Release(handle);
    if (!lay)
    {
        if (handle.IsValid() && handle.id < m_pipelineLayouts.Data().size())
            Print("! [DX9] DestroyPipelineLayout: double free of RHI_PipelineLayoutHandle(id=%u), ignoring", handle.id);
        return;
    }
    delete lay;
}
////////////////////////////////////////////////////////////////////////////////
