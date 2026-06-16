////////////////////////////////////////////////////////////////////////////////
// Created: 14.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <SoftX/include/SoftX.h>
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

    void SwapDepthBuffers();
    void WaitForBuildAndSwap();
    void BuildDepthBuffer(const fmat4x4& viewProj);
    void SaveDepthBuffer(const SoftX::DepthBuffer& depthBuffer, const char* filename);
    void SaveDepthBuffer();

    void DebugRenderLightVolumes(const light_Package& package, const fmat4x4& VP);

    void BeginOcclusionQueries(const fmat4x4& viewProj, const SoftX::Viewport& viewport);
    void EndOcclusionQueries();
    bool IsQueryReady() const;
    void ResetPendingQuery();
    uint32_t GetVisibleSamples(uint32_t queryId) const;

    SoftX::OcclusionQuery::queryID AddLightVolume(light* L);

    SoftX::OcclusionQuery* GetPendingQuery() const { return m_pendingQuery; }

private:
    IDirect3DVertexBuffer9* m_VB = nullptr;
    IDirect3DIndexBuffer9* m_IB = nullptr;
    ref_geom m_geom;            // обёртка над VB/IB + декларация
    ref_shader m_shader;        // простой шейдер

    u32 m_vertexCount = 0;
    u32 m_indexCount = 0;
    bool m_loaded = false;

    std::unique_ptr<SoftX::Device> m_softDevice;

    std::unique_ptr<SoftX::VertexBuffer> m_softOccluderVB;
    std::unique_ptr<SoftX::IndexBuffer> m_softOccluderIB;

    std::unique_ptr<SoftX::VertexBuffer> m_lightPointVB;
    std::unique_ptr<SoftX::IndexBuffer>  m_lightPointIB;
    std::unique_ptr<SoftX::VertexBuffer> m_lightSpotVB;
    std::unique_ptr<SoftX::IndexBuffer>  m_lightSpotIB;
    std::unique_ptr<SoftX::VertexBuffer> m_lightOmniPartVB;
    std::unique_ptr<SoftX::IndexBuffer>  m_lightOmniPartIB;

    static constexpr int QUERY_POOL_SIZE = 2;
    std::vector<std::unique_ptr<SoftX::OcclusionQuery>> m_queryPool;
    int m_currentQueryIndex = 0;
    SoftX::OcclusionQuery* m_activeQuery = nullptr;
    SoftX::OcclusionQuery* m_pendingQuery = nullptr;

    fmat4x4 m_currentViewProj;
    SoftX::Viewport m_currentViewport;

    std::unique_ptr<SoftX::DepthBuffer> m_softDepthBuffer[2];
    int m_writeIdx = 0;   // индекс буфера для записи (build)
    int m_readIdx = 1;   // индекс буфера для чтения (query)

    // Асинхронная задача заполнения
    std::future<void> m_buildFuture;

    void InitializeSoftX(const xr_vector<fvec3>& vertices, const xr_vector<u16>& indices);
    void ShutdownSoftX();

    void CreateLightPointGeometry();
    void CreateLightSpotGeometry();
    void CreateLightOmniPartGeometry();
    void ResetOcclusionVolumes();
};
////////////////////////////////////////////////////////////////////////////////
