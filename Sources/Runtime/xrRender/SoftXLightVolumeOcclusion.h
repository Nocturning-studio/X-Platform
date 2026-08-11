////////////////////////////////////////////////////////////////////////////////
// Created: 18.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <SoftX/include/SoftX.h>
#include "../xrEngine/du_cone.h"
#include "../xrEngine/du_sphere.h"
#include "../xrEngine/du_sphere_part.h"
////////////////////////////////////////////////////////////////////////////////
class light;
class SoftXOcclusionCore;

class SoftXLightVolumeOcclusion
{
public:
    SoftXLightVolumeOcclusion() = default;
    ~SoftXLightVolumeOcclusion() = default;

    void Initialize(SoftXOcclusionCore* core);
    void Shutdown();

    void BeginQueries(const fmat4x4& viewProj, const SoftX::Viewport& viewport);
    void EndQueries();
    bool IsQueryReady() const;
    void ResetPendingQuery();
    uint32_t GetVisibleSamples(uint32_t queryId) const;

    SoftX::OcclusionQuery::queryID AddVolume(light* L);
    SoftX::OcclusionQuery* GetPendingQuery() const { return m_pendingQuery; }

private:
    SoftXOcclusionCore* m_core = nullptr;

    // Геометрия объёмов источников
    std::unique_ptr<SoftX::VertexBuffer> m_pointVB;
    std::unique_ptr<SoftX::IndexBuffer>  m_pointIB;
    std::unique_ptr<SoftX::VertexBuffer> m_spotVB;
    std::unique_ptr<SoftX::IndexBuffer>  m_spotIB;
    std::unique_ptr<SoftX::VertexBuffer> m_omniVB;
    std::unique_ptr<SoftX::IndexBuffer>  m_omniIB;

    // Пул запросов
    static constexpr int QUERY_POOL_SIZE = 2;
    std::vector<std::unique_ptr<SoftX::OcclusionQuery>> m_queryPool;
    int m_currentQueryIndex = 0;
    SoftX::OcclusionQuery* m_activeQuery = nullptr;
    SoftX::OcclusionQuery* m_pendingQuery = nullptr;

    // Текущие параметры камеры
    fmat4x4 m_currentViewProj;
    SoftX::Viewport m_currentViewport;

    void CreateGeometries();
    void DestroyGeometries();
};
////////////////////////////////////////////////////////////////////////////////
