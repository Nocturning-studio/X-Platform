////////////////////////////////////////////////////////////////////////////////
// Created: 18.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "Stdafx.h"
#include "SoftXLightVolumeOcclusion.h"
#include "SoftXOcclusionCore.h"
#include "light.h"
////////////////////////////////////////////////////////////////////////////////
static void FillGeometry(
    const fvec3* sourceVerts, u32 vertCount,
    const u16* sourceFaces, u32 faceCount,
    std::unique_ptr<SoftX::VertexBuffer>& outVB,
    std::unique_ptr<SoftX::IndexBuffer>& outIB,
    const char* label)
{
    std::vector<SoftX::Vertex> vertices;
    vertices.reserve(vertCount);
    for (u32 i = 0; i < vertCount; ++i)
    {
        SoftX::Vertex vi;
        const fvec3& v = sourceVerts[i];
        vi.Position = float3(v.x, v.y, v.z);
        vertices.push_back(vi);
    }

    std::vector<uint> indices;
    indices.reserve(faceCount * 3);
    for (u32 i = 0; i < faceCount * 3; ++i)
        indices.push_back((uint)sourceFaces[i]);

    outVB = std::make_unique<SoftX::VertexBuffer>(vertices);
    outIB = std::make_unique<SoftX::IndexBuffer>(indices);

    Msg("[LightVolOcc] %s geometry: %u verts, %u indices", label, outVB->Size(), outIB->Size());
}

void SoftXLightVolumeOcclusion::CreateGeometries()
{
    FillGeometry(du_sphere_vertices, DU_SPHERE_NUMVERTEX, du_sphere_faces, DU_SPHERE_NUMFACES, m_pointVB, m_pointIB, "Point");
    FillGeometry(du_cone_vertices, DU_CONE_NUMVERTEX, du_cone_faces, DU_CONE_NUMFACES, m_spotVB, m_spotIB, "Spot");
    FillGeometry(du_sphere_part_vertices, DU_SPHERE_PART_NUMVERTEX, du_sphere_part_faces, DU_SPHERE_PART_NUMFACES, m_omniVB, m_omniIB, "Omni");
}

void SoftXLightVolumeOcclusion::DestroyGeometries()
{
    m_pointVB.reset(); m_pointIB.reset();
    m_spotVB.reset();  m_spotIB.reset();
    m_omniVB.reset();  m_omniIB.reset();
}

// ---------------------------------------------------------------------------------------
void SoftXLightVolumeOcclusion::Initialize(SoftXOcclusionCore* core)
{
    m_core = core;
    m_queryPool.clear();
    m_queryPool.reserve(QUERY_POOL_SIZE);
    for (int i = 0; i < QUERY_POOL_SIZE; ++i)
        m_queryPool.emplace_back(std::make_unique<SoftX::OcclusionQuery>());
    m_currentQueryIndex = 0;
    m_activeQuery = nullptr;
    m_pendingQuery = nullptr;

    CreateGeometries();
}

void SoftXLightVolumeOcclusion::Shutdown()
{
    DestroyGeometries();
    m_queryPool.clear();
    m_activeQuery = nullptr;
    m_pendingQuery = nullptr;
    m_core = nullptr;
}

// ---------------------------------------------------------------------------------------
void SoftXLightVolumeOcclusion::BeginQueries(const fmat4x4& viewProj, const SoftX::Viewport& viewport)
{
    if (!m_core) return;
    m_currentViewProj = viewProj;
    m_currentViewport = viewport;

    m_activeQuery = m_queryPool[m_currentQueryIndex].get();
    m_currentQueryIndex = (m_currentQueryIndex + 1) % QUERY_POOL_SIZE;

    if (!m_activeQuery->IsReady()) m_activeQuery->Flush();
    m_activeQuery->SetDepthBuffer(m_core->GetWriteBuffer());
    m_activeQuery->SetViewport(m_currentViewport);
    m_activeQuery->Begin();
}

void SoftXLightVolumeOcclusion::EndQueries()
{
    if (m_activeQuery) { m_activeQuery->End(); m_pendingQuery = m_activeQuery; m_activeQuery = nullptr; }
}

bool SoftXLightVolumeOcclusion::IsQueryReady() const { return m_pendingQuery && m_pendingQuery->IsReady(); }
void SoftXLightVolumeOcclusion::ResetPendingQuery() { m_pendingQuery = nullptr; }

uint32_t SoftXLightVolumeOcclusion::GetVisibleSamples(uint32_t queryId) const
{
    if (!m_pendingQuery || !m_pendingQuery->IsReady()) return 0xfffffffe;
    uint v = 0;
    return m_pendingQuery->GetResult(queryId, &v) ? v : 0;
}

// ---------------------------------------------------------------------------------------
SoftX::Interpolant LightVolumeQueryVS(const SoftX::Vertex& input, const SoftX::ConstantBuffer& cb)
{
    const fmat4x4& mvp = *static_cast<const fmat4x4*>(cb.Data());
    float4 pos = float4(input.Position.x, input.Position.y, input.Position.z, 1.0f);

    float4 clip;
    clip.x = pos.x * mvp._11 + pos.y * mvp._21 + pos.z * mvp._31 + mvp._41;
    clip.y = pos.x * mvp._12 + pos.y * mvp._22 + pos.z * mvp._32 + mvp._42;
    clip.z = pos.x * mvp._13 + pos.y * mvp._23 + pos.z * mvp._33 + mvp._43;
    clip.w = pos.x * mvp._14 + pos.y * mvp._24 + pos.z * mvp._34 + mvp._44;

    SoftX::Interpolant output;
    output.ClipSpacePosition = clip;
    return output;
}

SoftX::OcclusionQuery::queryID SoftXLightVolumeOcclusion::AddVolume(light* L)
{
    if (!L || !m_activeQuery) return 0;

    SoftX::VertexBuffer* vb = nullptr; SoftX::IndexBuffer* ib = nullptr;
    switch (L->LightFlags.type) {
    case IRender_Light::REFLECTED:
    case IRender_Light::POINT:     vb = m_pointVB.get(); ib = m_pointIB.get(); break;
    case IRender_Light::SPOT:      vb = m_spotVB.get();  ib = m_spotIB.get();  break;
    case IRender_Light::OMNIPART:  vb = m_omniVB.get();  ib = m_omniIB.get();  break;
    default: return 0;
    }
    if (!vb || !ib) return 0;

    fmat4x4 mvp; mvp.mul(m_currentViewProj, L->get_transform());
    SoftX::ConstantBuffer cb(&mvp, sizeof(mvp));

    m_activeQuery->SetVertexBuffer(*vb); m_activeQuery->SetIndexBuffer(*ib);
    m_activeQuery->SetConstantBuffer(cb);
    m_activeQuery->SetVertexShader(LightVolumeQueryVS);
    m_activeQuery->SetDepthFunc(SoftX::ComparisonFunc::Less);
    m_activeQuery->SetCullMode(SoftX::CullMode::None);
    return m_activeQuery->DrawIndexed();
}
////////////////////////////////////////////////////////////////////////////////
