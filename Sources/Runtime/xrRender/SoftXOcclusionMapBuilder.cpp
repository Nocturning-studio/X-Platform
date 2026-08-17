////////////////////////////////////////////////////////////////////////////////
// Created: 17.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "SoftXOcclusionMapBuilder.h"
#include "SoftXOcclusionCore.h"
#include "HOM.h"
////////////////////////////////////////////////////////////////////////////////
static SoftX::VertexBuffer CreateVertexBuffer(const xr_vector<fvec3>& verts)
{
    std::vector<SoftX::Vertex> vbData;
    vbData.reserve(verts.size());
    for (const fvec3& v : verts)
    {
        SoftX::Vertex vi;
        vi.Position = float3(v.x, v.y, v.z);
        vbData.push_back(vi);
    }
    return SoftX::VertexBuffer(vbData);
}

static SoftX::IndexBuffer CreateIndexBuffer(const xr_vector<u16>& indices)
{
    std::vector<uint> idx32;
    idx32.reserve(indices.size());
    for (u16 i : indices)
        idx32.push_back(i);
    return SoftX::IndexBuffer(idx32);
}

void SoftXOcclusionMapBuilder::ExtractGeometry(const CDB::MODEL* model,
    xr_vector<fvec3>& outVertices,
    xr_vector<u16>& outIndices) const
{
    VERIFY(model);
    const u32 vertCount = model->get_verts_count();
    const u32 triCount = model->get_tris_count();

    const fvec3* srcVerts = model->get_verts();
    outVertices.assign(srcVerts, srcVerts + vertCount);

    const CDB::TRI* srcTris = model->get_tris();
    outIndices.reserve(triCount * 3);
    for (u32 i = 0; i < triCount; ++i)
    {
        const CDB::TRI& tri = srcTris[i];
        R_ASSERT(tri.verts[0] <= 0xFFFF && tri.verts[1] <= 0xFFFF && tri.verts[2] <= 0xFFFF);
        outIndices.push_back((u16)tri.verts[0]);
        outIndices.push_back((u16)tri.verts[1]);
        outIndices.push_back((u16)tri.verts[2]);
    }
}

// ---------------------------------------------------------------------------------------
void SoftXOcclusionMapBuilder::Load(const CHOM& hom)
{
    Unload();

    const CDB::MODEL* model = hom.get_occluder_model();
    if (!model)
    {
        Msg("! [SoftXOcclusionMapBuilder] HOM model is null.");
        return;
    }

    xr_vector<fvec3> vertices;
    xr_vector<u16> indices;
    ExtractGeometry(model, vertices, indices);

    u32 vertexCount = (u32)vertices.size();   // локальные переменные
    u32 indexCount = (u32)indices.size();

    if (vertexCount == 0 || indexCount == 0)
    {
        Msg("! [SoftXOcclusionMapBuilder] HOM model is empty.");
        return;
    }

    m_occluderVB = std::make_unique<SoftX::VertexBuffer>(CreateVertexBuffer(vertices));
    m_occluderIB = std::make_unique<SoftX::IndexBuffer>(CreateIndexBuffer(indices));

    m_vertexCount = vertexCount;
    m_indexCount = indexCount;
    m_loaded = true;

    Msg("* [SoftXOcclusionMapBuilder] Loaded %u verts, %u indices from HOM.", m_vertexCount, m_indexCount);
}

void SoftXOcclusionMapBuilder::Unload()
{
    m_occluderVB.reset();
    m_occluderIB.reset();
    m_loaded = false;
}

void SoftXOcclusionMapBuilder::Build()
{
    if (!m_core || !m_loaded)
        return;

    SoftXOcclusionCore* core = m_core;
    SoftX::VertexBuffer* vb = m_occluderVB.get();
    SoftX::IndexBuffer* ib = m_occluderIB.get();

    uint2 depthResolution = core->GetWriteBuffer()->Size();

    {
        OPTICK_EVENT("Draw occluder");
        SoftX::DeviceContext& ctx = core->GetImmediateContext();

        // Полная настройка состояний перед рисованием окклюдеров
        ctx.SetRenderTarget(nullptr, true);
        ctx.SetDepthBuffer(core->GetWriteBuffer());
        ctx.SetCullMode(SoftX::CullMode::None);
        ctx.SetFillMode(SoftX::FillMode::Solid);
        ctx.SetDepthFunc(SoftX::ComparisonFunc::Less);
        ctx.SetDepthWriteEnable(true);
        ctx.SetViewport(SoftX::Viewport(0.0f, 0.0f, (int)depthResolution.x, (int)depthResolution.y, 0.0f, 1.0f));
        ctx.SetTileSize(64);

        ctx.Clear(SoftX::ClearFlags::DepthBuffer, float4(), 1.0f);

        auto occlusionVS = [](const SoftX::Vertex& input, const SoftX::ConstantBuffer& cb, const SoftX::TextureTable&) -> SoftX::Interpolant
        {
            const fmat4x4& mvp = *static_cast<const fmat4x4*>(cb.Data());
            float x = input.Position.x, y = input.Position.y, z = input.Position.z;
            float w = x * mvp._14 + y * mvp._24 + z * mvp._34 + mvp._44;
            float outX = x * mvp._11 + y * mvp._21 + z * mvp._31 + mvp._41;
            float outY = x * mvp._12 + y * mvp._22 + z * mvp._32 + mvp._42;
            float outZ = x * mvp._13 + y * mvp._23 + z * mvp._33 + mvp._43;
            SoftX::Interpolant output;
            output.ClipSpacePosition = float4(outX, outY, outZ, w);
            return output;
        };
        ctx.SetVertexShader(occlusionVS);
        ctx.SetPixelShader([](const auto&, const auto&, const auto&) { return float4(0, 0, 0, 0); });

        SoftX::ConstantBuffer cb(&Engine.RenderView.ViewProjection, sizeof(Engine.RenderView.ViewProjection));
        ctx.SetConstantBuffer(cb);
        ctx.SetVertexBuffer(*vb);
        ctx.SetIndexBuffer(*ib);
        ctx.DrawIndexed();
    }

    {
        OPTICK_EVENT("Generate Hi-Z Chain");
        core->GetWriteBuffer()->GenerateHiZ(SoftX::DepthBuffer::HiZReduction::Min);
    }
}
////////////////////////////////////////////////////////////////////////////////
