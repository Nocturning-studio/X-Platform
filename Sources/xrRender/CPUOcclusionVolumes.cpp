////////////////////////////////////////////////////////////////////////////////
// Created: 14.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "Stdafx.h"
#include "CPUOcclusion.h"
#include "CPUOcclusionShaders.h"
#include "../xrEngine/du_cone.h"
#include "../xrEngine/du_sphere.h"
#include "../xrEngine/du_sphere_part.h"
////////////////////////////////////////////////////////////////////////////////
void CPUOcclusion::CreateLightPointGeometry()
{
    std::vector<SoftX::VertexInput> vertices;
    vertices.reserve(DU_SPHERE_NUMVERTEX);
    for (int i = 0; i < DU_SPHERE_NUMVERTEX; ++i)
    {
        SoftX::VertexInput vi;
        fvec3 vertex = du_sphere_vertices[i];
        vi.Position = float3(vertex.x, vertex.y, vertex.z);
        vi.Normal = float3(0, 0, 0);
        vi.Color = float4(0, 0, 0, 0);
        vi.UV = float2(0, 0);
        vertices.push_back(vi);
    }

    std::vector<uint> indices;
    indices.reserve(DU_SPHERE_NUMFACES * 3);
    for (int i = 0; i < DU_SPHERE_NUMFACES * 3; ++i)
        indices.push_back((uint)du_sphere_faces[i]);

    m_lightPointVB = std::make_unique<SoftX::VertexBuffer>(vertices);
    m_lightPointIB = std::make_unique<SoftX::IndexBuffer>(indices);

    Msg("[CPU-OCC] Point geometry created: %u vertices, %u indices", m_lightPointVB->Size(), m_lightPointIB->Size());
}

void CPUOcclusion::CreateLightOmniPartGeometry()
{
    std::vector<SoftX::VertexInput> vertices;
    vertices.reserve(DU_SPHERE_PART_NUMVERTEX);
    for (int i = 0; i < DU_SPHERE_PART_NUMVERTEX; ++i)
    {
        SoftX::VertexInput vi;
        fvec3 vertex = du_sphere_part_vertices[i];
        vi.Position = float3(vertex.x, vertex.y, vertex.z);
        vi.Normal = float3(0, 0, 0);
        vi.Color = float4(0, 0, 0, 0);
        vi.UV = float2(0, 0);
        vertices.push_back(vi);
    }

    std::vector<uint> indices;
    indices.reserve(DU_SPHERE_PART_NUMFACES * 3);
    for (int i = 0; i < DU_SPHERE_PART_NUMFACES * 3; ++i)
        indices.push_back((uint)du_sphere_part_faces[i]);

    m_lightOmniPartVB = std::make_unique<SoftX::VertexBuffer>(vertices);
    m_lightOmniPartIB = std::make_unique<SoftX::IndexBuffer>(indices);

    Msg("[CPU-OCC] Omni geometry created: %u vertices, %u indices", m_lightOmniPartVB->Size(), m_lightOmniPartIB->Size());
}

void CPUOcclusion::CreateLightSpotGeometry()
{
    std::vector<SoftX::VertexInput> vertices;
    vertices.reserve(DU_CONE_NUMVERTEX);
    for (int i = 0; i < DU_CONE_NUMVERTEX; ++i)
    {
        SoftX::VertexInput vi;
        fvec3 vertex = du_cone_vertices[i];
        vi.Position = float3(vertex.x, vertex.y, vertex.z);
        vi.Normal = float3(0, 0, 0);
        vi.Color = float4(0, 0, 0, 0);
        vi.UV = float2(0, 0);
        vertices.push_back(vi);
    }

    std::vector<uint> indices;
    indices.reserve(DU_CONE_NUMFACES * 3);
    for (int i = 0; i < DU_CONE_NUMFACES * 3; ++i)
        indices.push_back((uint)du_cone_faces[i]);

    m_lightSpotVB = std::make_unique<SoftX::VertexBuffer>(vertices);
    m_lightSpotIB = std::make_unique<SoftX::IndexBuffer>(indices);

    Msg("[CPU-OCC] Spot geometry created: %u vertices, %u indices", m_lightSpotVB->Size(), m_lightSpotIB->Size());
}

SoftX::OcclusionQuery::queryID CPUOcclusion::AddLightVolume(light* L)
{
    if (!L)
    {
        Msg("[CPU-OCC] AddLightVolume: light is null");
        return 0;
    }
       
    if (!m_activeQuery)
    {
        Msg("[CPU-OCC] AddLightVolume: m_activeQuery is null");
        return 0;
    }

    SoftX::VertexBuffer* vb = nullptr;
    SoftX::IndexBuffer* ib = nullptr;

    switch (L->LightFlags.type)
    {
    case IRender_Light::REFLECTED:
    case IRender_Light::POINT:
        vb = m_lightPointVB.get();
        ib = m_lightPointIB.get();
        break;
    case IRender_Light::SPOT:
        vb = m_lightSpotVB.get();
        ib = m_lightSpotIB.get();
        break;
    case IRender_Light::OMNIPART:
        vb = m_lightOmniPartVB.get();
        ib = m_lightOmniPartIB.get();
        break;
    default:
        Msg("[CPU-OCC] AddLightVolume: L->LightFlags.type is unknown %d", L->LightFlags.type);
        return 0;
    }

    if (!vb || !ib)
    {
        Msg("[CPU-OCC] AddLightVolume: null vb or ib for light type %d", L->LightFlags.type);
        return 0;
    }

    fmat4x4 world = L->get_transform();
    fmat4x4 mvp = Fidentity;
    mvp.mul(m_currentViewProj, world);   // world * viewProj
    SoftX::ConstantBuffer cb(&mvp, sizeof(mvp));

    m_activeQuery->SetVertexBuffer(*vb);
    m_activeQuery->SetIndexBuffer(*ib);
    m_activeQuery->SetConstantBuffer(cb);
    m_activeQuery->SetVertexShader(LightVolumeQueryVS);
    m_activeQuery->SetDepthFunc(SoftX::ComparisonFunc::Less);
    m_activeQuery->SetCullMode(SoftX::CullMode::None);

    return m_activeQuery->DrawIndexed();
}

void CPUOcclusion::ResetOcclusionVolumes()
{
    m_lightPointVB.reset();
    m_lightPointIB.reset();
    m_lightSpotVB.reset();
    m_lightSpotIB.reset();
    m_lightOmniPartVB.reset();
    m_lightOmniPartIB.reset();
}
////////////////////////////////////////////////////////////////////////////////
