////////////////////////////////////////////////////////////////////////////////
// Created: 14.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <SoftX/include/SoftX.h>
#include "SoftXOcclusionCore.h"
#include "SoftXOcclusionMapBuilder.h"
#include "SoftXLightVolumeOcclusion.h"
#include "SoftXAABBOcclusion.h"
////////////////////////////////////////////////////////////////////////////////
class CHOM;
class light;
class light_Package;

class CPUOcclusion
{
public:
    CPUOcclusion();
    ~CPUOcclusion();

    void Load(const CHOM& hom);
    void Unload();
    void DrawDebug();
    bool IsLoaded() const { return m_loaded; }

    void __stdcall BuildDepthBuffer() { m_occlusionMap.Build(); }
    void WaitForBuildAndSwap() { if (m_core) m_core->WaitForBuildAndSwap(); }

    void BeginOcclusionQueries(const fmat4x4& viewProj, const SoftX::Viewport& viewport) { m_lightOcc.BeginQueries(viewProj, viewport); }
    void EndOcclusionQueries() { m_lightOcc.EndQueries(); }
    bool IsQueryReady() const { return m_lightOcc.IsQueryReady(); }
    void ResetPendingQuery() { m_lightOcc.ResetPendingQuery(); }
    uint32_t GetVisibleSamples(uint32_t id) const { return m_lightOcc.GetVisibleSamples(id); }
    SoftX::OcclusionQuery::queryID AddLightVolume(light* L) { return m_lightOcc.AddVolume(L); }
    SoftX::OcclusionQuery* GetPendingQuery() const { return m_lightOcc.GetPendingQuery(); }

    bool TestAABB(const Fbox3& box, const fmat4x4& VP = Engine.RenderView.ViewProjection) const
    {
        if (!m_core || !m_core->IsReadBufferReady()) return true;
        uint2 resolution = m_core->GetDepthResolution();
        return m_aabbOcc.TestAABB(box, VP, resolution);
    }

    bool TestAABB(const fvec3& min, const fvec3 max, const fmat4x4& VP = Engine.RenderView.ViewProjection) const
    {
        if (!m_core || !m_core->IsReadBufferReady()) return true;
        uint2 resolution = m_core->GetDepthResolution();
        Fbox3 AABB;
        AABB.set(min, max);
        return m_aabbOcc.TestAABB(AABB, VP, resolution);
    }

    bool TestRect(float x0, float y0, float x1, float y1, float depth) const
    {
        if (!m_core || !m_core->IsReadBufferReady()) return true;
        uint2 resolution = m_core->GetDepthResolution();
        return m_aabbOcc.TestRect(x0, y0, x1, y1, depth, resolution);
    }

    bool TestPolygon(const sPoly& worldPoly, const fmat4x4& viewProj = Engine.RenderView.ViewProjection) const
    {
        if (!m_core || !m_core->IsReadBufferReady()) return true;
        uint2 resolution = m_core->GetDepthResolution();
        return m_aabbOcc.TestPolygon(worldPoly, viewProj, resolution);
    }

    void SaveDepthBuffer(const SoftX::DepthBuffer& buf, const char* fname);
    void SaveDepthBuffer();

private:
    // D3D9 отладка
    IDirect3DVertexBuffer9* m_VB = nullptr;
    IDirect3DIndexBuffer9* m_IB = nullptr;
    ref_geom m_geom; ref_shader m_shader;
    u32 m_vertexCount = 0, m_indexCount = 0;
    bool m_loaded = false;

    // Компоненты
    std::unique_ptr<SoftXOcclusionCore> m_core;
    SoftXOcclusionMapBuilder m_occlusionMap;
    SoftXLightVolumeOcclusion m_lightOcc;
    SoftXAABBOcclusion m_aabbOcc;

    void InitializeSoftX();
    void ShutdownSoftX();
};
////////////////////////////////////////////////////////////////////////////////
