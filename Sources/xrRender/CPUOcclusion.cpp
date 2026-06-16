////////////////////////////////////////////////////////////////////////////////
// Created: 14.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "CPUOcclusion.h"
#include "CPUOcclusionShaders.h"
#include "HOM.h"
////////////////////////////////////////////////////////////////////////////////
static void ExtractGeometryFromCDB(const CDB::MODEL* model,
                                   xr_vector<fvec3>& outVertices,
                                   xr_vector<u16>& outIndices)
{
    VERIFY(model);
    const u32 vertCount = model->get_verts_count();
    const u32 triCount = model->get_tris_count();

    // Копируем все вершины
    const fvec3* srcVerts = model->get_verts();
    outVertices.assign(srcVerts, srcVerts + vertCount);

    // Копируем индексы (u32 -> u16, с проверкой диапазона)
    const CDB::TRI* srcTris = model->get_tris();
    outIndices.reserve(triCount * 3);
    for (u32 i = 0; i < triCount; ++i)
    {
        const CDB::TRI& tri = srcTris[i];
        // Убеждаемся, что индексы помещаются в 16 бит
        R_ASSERT(tri.verts[0] <= 0xFFFF && tri.verts[1] <= 0xFFFF && tri.verts[2] <= 0xFFFF);
        outIndices.push_back((u16)tri.verts[0]);
        outIndices.push_back((u16)tri.verts[1]);
        outIndices.push_back((u16)tri.verts[2]);
    }
}

static SoftX::VertexBuffer CreateSoftXVertexBuffer(const xr_vector<fvec3>& verts)
{
    std::vector<SoftX::VertexInput> vbData;
    vbData.reserve(verts.size());
    for (const fvec3& v : verts)
    {
        SoftX::VertexInput vi;
        vi.Position = float3(v.x, v.y, v.z);
        vi.Normal = float3(0, 0, 0);
        vi.Color = float4(0, 0, 0, 0);
        vi.UV = float2(0, 0);
        vbData.push_back(vi);
    }
    return SoftX::VertexBuffer(vbData);
}

static SoftX::IndexBuffer CreateSoftXIndexBuffer(const xr_vector<u16>& indices)
{
    std::vector<uint> idx32;
    idx32.reserve(indices.size());
    for (u16 i : indices)
        idx32.push_back(i);
    return SoftX::IndexBuffer(idx32);
}

inline bool SaveDepthBufferToTGA(const SoftX::DepthBuffer& depthBuffer, const char* filename)
{
    const uint width = depthBuffer.Width();
    const uint height = depthBuffer.Height();
    if (width == 0 || height == 0)
        return false;

    std::ofstream file(filename, std::ios::binary);
    if (!file)
        return false;

    // ----- TGA header (18 bytes) -----
    uint8_t header[18] = {};
    header[2] = 3;                      // image type: uncompressed grayscale
    header[12] = static_cast<uint8_t>(width & 0xFF);
    header[13] = static_cast<uint8_t>((width >> 8) & 0xFF);
    header[14] = static_cast<uint8_t>(height & 0xFF);
    header[15] = static_cast<uint8_t>((height >> 8) & 0xFF);
    header[16] = 8;                     // bits per pixel
    header[17] = 0x20;                  // image descriptor: origin = top-left
    file.write(reinterpret_cast<const char*>(header), sizeof(header));

    // ----- Image data (rows top to bottom) -----
    std::vector<uint8_t> row(width);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float depth = depthBuffer.Read(int2(x, y));
            // Clamp to [0,1] and scale to 0..255
            uint8_t c = static_cast<uint8_t>(
                AfterMath::clamp(depth, 0.0f, 1.0f) * 255.0f + 0.5f);
            row[x] = c;
        }
        file.write(reinterpret_cast<const char*>(row.data()), width);
    }

    return file.good();
}

void CPUOcclusion::SaveDepthBuffer(const SoftX::DepthBuffer& depthBuffer, const char* filename)
{
    SaveDepthBufferToTGA(depthBuffer, filename);
}

void CPUOcclusion::SaveDepthBuffer()
{
    SaveDepthBufferToTGA(*m_softDepthBuffer[m_readIdx], "output.tga");
}

CPUOcclusion::CPUOcclusion()
{
}

CPUOcclusion::~CPUOcclusion()
{
    Unload();
}

void CPUOcclusion::Load(const CHOM& hom)
{
    Unload(); // сброс предыдущего, если был

    const CDB::MODEL* OccluderModel = hom.get_occluder_model();

    // Проверяем, что HOM загружен и содержит модель
    if (!OccluderModel)
    {
        Msg("! [CPUOcclusion] HOM model is null, cannot load occluder geometry.");
        return;
    }

    // Извлекаем геометрию
    xr_vector<fvec3> vertices;
    xr_vector<u16> indices;
    ExtractGeometryFromCDB(OccluderModel, vertices, indices);

    m_vertexCount = (u32)vertices.size();
    m_indexCount = (u32)indices.size();

    if (m_vertexCount == 0 || m_indexCount == 0)
    {
        Msg("! [CPUOcclusion] HOM model is empty.");
        return;
    }

    InitializeSoftX(vertices, indices);

    // ── Создание D3D9 вершинного буфера ─────────────────────
    {
        const u32 vbSize = m_vertexCount * sizeof(fvec3); // только позиция
        R_CHK(HW.GetDevice()->CreateVertexBuffer(
            vbSize,
            D3DUSAGE_WRITEONLY,
            0,
            D3DPOOL_DEFAULT,
            &m_VB,
            nullptr
        ));
        fvec3* pData = nullptr;
        R_CHK(m_VB->Lock(0, 0, (void**)&pData, 0));
        std::memcpy(pData, vertices.data(), vbSize);
        R_CHK(m_VB->Unlock());
    }

    // ── Создание D3D9 индексного буфера ─────────────────────
    {
        const u32 ibSize = m_indexCount * sizeof(u16);
        R_CHK(HW.GetDevice()->CreateIndexBuffer(
            ibSize,
            D3DUSAGE_WRITEONLY,
            D3DFMT_INDEX16,
            D3DPOOL_DEFAULT,
            &m_IB,
            nullptr
        ));
        u16* pData = nullptr;
        R_CHK(m_IB->Lock(0, 0, (void**)&pData, 0));
        std::memcpy(pData, indices.data(), ibSize);
        R_CHK(m_IB->Unlock());
    }

    // ── Декларация вершин (только позиция) и создание ref_geom ─
    static D3DVERTEXELEMENT9 dwDecl[] =
    {
        { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        D3DDECL_END()
    };
    m_geom.create(dwDecl, m_VB, m_IB);

    m_shader.create("sun_occluder");

    m_loaded = true;
    Msg("* [CPUOcclusion] Loaded %u verts, %u indices from HOM.", m_vertexCount, m_indexCount);
}

void CPUOcclusion::Unload()
{
    m_geom.destroy();
    _RELEASE(m_IB);
    _RELEASE(m_VB);
    m_shader.destroy();
    m_loaded = false;
}

void CPUOcclusion::DrawDebug()
{
    OPTICK_EVENT("CPUOcclusion::DrawDebug()");

    if (!m_loaded)
        return;

    RenderBackendLegacy.set_Geometry(m_geom);
    RenderBackendLegacy.set_transform_world(Fidentity);
    RenderBackendLegacy.set_Shader(m_shader);
    RenderBackendLegacy.set_CullMode(CULL_DISABLE);

    const u32 primCount = m_indexCount / 3;
    RenderBackendLegacy.Render(D3DPT_TRIANGLELIST, 0, 0, m_vertexCount, 0, primCount);
}

void CPUOcclusion::InitializeSoftX(const xr_vector<fvec3>& vertices,
                                   const xr_vector<u16>& indices)
{
    ShutdownSoftX();

    uint DEPTH_MAP_SIZE = 512;

    SoftX::PresentParameters params;

    params.BackBufferSize = uint2(1, 1);
    params.hDeviceWindow = nullptr;
    params.Windowed = true;
    params.Headless = true;

    m_softDevice = std::make_unique<SoftX::Device>(params);

    SoftX::DeviceContext& ctx = m_softDevice->GetImmediateContext();
    
    m_queryPool.clear();
    m_queryPool.reserve(QUERY_POOL_SIZE);
    for (int i = 0; i < QUERY_POOL_SIZE; ++i)
        m_queryPool.emplace_back(std::make_unique<SoftX::OcclusionQuery>());
    m_currentQueryIndex = 0;
    m_activeQuery = nullptr;
    m_pendingQuery = nullptr;

    m_softDepthBuffer[0] = std::make_unique<SoftX::DepthBuffer>(uint2(DEPTH_MAP_SIZE, DEPTH_MAP_SIZE));
    m_softDepthBuffer[1] = std::make_unique<SoftX::DepthBuffer>(uint2(DEPTH_MAP_SIZE, DEPTH_MAP_SIZE));
    m_writeIdx = 0;
    m_readIdx = 1;

    ctx.SetRenderTarget(nullptr, true);
    ctx.SetDepthBuffer(m_softDepthBuffer[m_writeIdx].get());

    ctx.SetCullMode(SoftX::CullMode::None);
    ctx.SetFillMode(SoftX::FillMode::Solid);
    ctx.SetDepthFunc(SoftX::ComparisonFunc::Less);
    ctx.SetDepthWriteEnable(true);
    ctx.SetViewport(SoftX::Viewport(0.0f, 0.0f, DEPTH_MAP_SIZE, DEPTH_MAP_SIZE, 0.0f, 1.0f));
    ctx.SetTileSize(64); // размер тайла для тайлового рендера SoftX

    m_softOccluderVB = std::make_unique<SoftX::VertexBuffer>(CreateSoftXVertexBuffer(vertices));
    m_softOccluderIB = std::make_unique<SoftX::IndexBuffer>(CreateSoftXIndexBuffer(indices));

    CreateLightPointGeometry();
    CreateLightSpotGeometry();
    CreateLightOmniPartGeometry();
}

void CPUOcclusion::ShutdownSoftX()
{
    ResetOcclusionVolumes();

    m_softOccluderVB.reset();
    m_softOccluderIB.reset();
    m_softDepthBuffer[0].reset();
    m_softDepthBuffer[1].reset();
    if (m_buildFuture.valid())
        m_buildFuture.wait();
    m_softDevice.reset();
    m_queryPool.clear();
    m_activeQuery = nullptr;
    m_pendingQuery = nullptr;
}

void CPUOcclusion::SwapDepthBuffers()
{
    std::swap(m_writeIdx, m_readIdx);
}

void CPUOcclusion::WaitForBuildAndSwap()
{
    if (m_buildFuture.valid())
    {
        m_buildFuture.wait();
        SwapDepthBuffers();
    }
}

void CPUOcclusion::BuildDepthBuffer(const fmat4x4& viewProj)
{
    if (!m_softDevice) return;

    // Дожидаемся предыдущей задачи, если ещё не завершилась
    if (m_buildFuture.valid())
        m_buildFuture.wait();

    // Запускаем заполнение в фоне
    m_buildFuture = std::async(std::launch::async, [this, viewProj]()
        {
            SoftX::DeviceContext& ctx = m_softDevice->GetImmediateContext();

            ctx.SetRenderTarget(nullptr, true);
            ctx.SetDepthBuffer(m_softDepthBuffer[m_writeIdx].get());

            ctx.ClearDepth(1.0f);

            auto occlusionVS = [](const SoftX::VertexInput& input, const SoftX::ConstantBuffer& cb, const SoftX::TextureTable&) -> SoftX::VertexOutput
            {
                const fmat4x4& mvp = *static_cast<const fmat4x4*>(cb.Data());
                float x = input.Position.x, y = input.Position.y, z = input.Position.z;
                float w = x * mvp._14 + y * mvp._24 + z * mvp._34 + mvp._44;
                float outX = x * mvp._11 + y * mvp._21 + z * mvp._31 + mvp._41;
                float outY = x * mvp._12 + y * mvp._22 + z * mvp._32 + mvp._42;
                float outZ = x * mvp._13 + y * mvp._23 + z * mvp._33 + mvp._43;
                SoftX::VertexOutput output;
                output.Position = float4(outX, outY, outZ, w);
                return output;
            };
            ctx.SetVertexShader(occlusionVS);
            ctx.SetPixelShader([](const auto&, const auto&, const auto&) { return float4(0, 0, 0, 0); });

            SoftX::ConstantBuffer cb(&viewProj, sizeof(viewProj));
            ctx.SetConstantBuffer(cb);
            ctx.SetVertexBuffer(*m_softOccluderVB);
            ctx.SetIndexBuffer(*m_softOccluderIB);
            ctx.DrawIndexed();
        });
}

void CPUOcclusion::DebugRenderLightVolumes(const light_Package& package, const fmat4x4& VP)
{
    if (!m_softDevice) return;

    SoftX::DeviceContext& ctx = m_softDevice->GetImmediateContext();

    ctx.ClearDepth(1.0f);
    ctx.SetRenderTarget(nullptr, false);
    ctx.SetDepthBuffer(m_softDepthBuffer[m_writeIdx].get());
    ctx.SetDepthWriteEnable(true);
    ctx.SetDepthFunc(SoftX::ComparisonFunc::Less);
    ctx.SetCullMode(SoftX::CullMode::None);

    auto dummyPS = [](const SoftX::VertexOutput&, const SoftX::ConstantBuffer&, const SoftX::TextureTable&) -> float4 {
        return float4(0, 0, 0, 0);
    };
    ctx.SetPixelShader(dummyPS);

    // Вершинный шейдер БЕЗ инверсии w – как в BuildDepthBuffer
    auto occlusionVS = [](const SoftX::VertexInput& input, const SoftX::ConstantBuffer& cb, const SoftX::TextureTable&) -> SoftX::VertexOutput
    {
        const fmat4x4& mvp = *static_cast<const fmat4x4*>(cb.Data());
        const float x = input.Position.x;
        const float y = input.Position.y;
        const float z = input.Position.z;

        float w = x * mvp._14 + y * mvp._24 + z * mvp._34 + mvp._44;
        float outX = x * mvp._11 + y * mvp._21 + z * mvp._31 + mvp._41;
        float outY = x * mvp._12 + y * mvp._22 + z * mvp._32 + mvp._42;
        float outZ = x * mvp._13 + y * mvp._23 + z * mvp._33 + mvp._43;

        SoftX::VertexOutput o;
        o.Position = float4(outX, outY, outZ, w);
        o.Color = float4(0, 0, 0, 0);
        o.Normal = float3(0, 0, 0);
        o.UV = float2(0, 0);
        return o;
    };
    ctx.SetVertexShader(occlusionVS);

    SoftX::Viewport fullVP(0.0f, 0.0f, (float)m_softDepthBuffer[m_writeIdx]->Width(), (float)m_softDepthBuffer[m_writeIdx]->Height(), 0.0f, 1.0f);
    ctx.SetViewport(fullVP);

    // Построим frustum из VP для теста видимости
    CFrustum frustum;
    frustum.CreateFromMatrix((fmat4x4)VP, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);   // все плоскости, кроме ближней (она не критична)

    auto draw_list = [&](const xr_vector<light*>& lights, const char* name)
    {
        u32 drawn = 0;
        for (light* L : lights)
        {
            if (!L) continue;

            // Определяем геометрию
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
            default: continue;
            }
            if (!vb || !ib) continue;

            // Проверяем пересечение bounding‑сферы с frustum
            u32 testMask = 0xFFFFFFFF;             // все плоскости активны
            EFC_Visible vis = frustum.testSphere(L->spatial.sphere.P, L->spatial.sphere.R, testMask);
            if (vis == fcvNone)
                continue;

            Msg("[DEBUG-VISIBLE] Light type=%d, pos=(%.1f,%.1f,%.1f), radius=%.1f, name=%s",
                L->LightFlags.type,
                L->spatial.sphere.P.x, L->spatial.sphere.P.y, L->spatial.sphere.P.z,
                L->spatial.sphere.R, name);

            // Реальная матрица
            L->transform_calc();
            fmat4x4 world = L->get_transform();
            fmat4x4 mvp = Fidentity;
            mvp.mul(VP, world);
            SoftX::ConstantBuffer cb(&mvp, sizeof(mvp));

            auto printMat = [](const char* name, const fmat4x4& m)
            {
                Msg("[DEBUG] %s:", name);
                Msg("  [%.4f, %.4f, %.4f, %.4f]", m._11, m._12, m._13, m._14);
                Msg("  [%.4f, %.4f, %.4f, %.4f]", m._21, m._22, m._23, m._24);
                Msg("  [%.4f, %.4f, %.4f, %.4f]", m._31, m._32, m._33, m._34);
                Msg("  [%.4f, %.4f, %.4f, %.4f]", m._41, m._42, m._43, m._44);
            };
            printMat("world", world);
            printMat("VP", VP);
            printMat("mvp (world*VP)", mvp);

            ctx.SetVertexBuffer(*vb);
            ctx.SetIndexBuffer(*ib);
            ctx.SetConstantBuffer(cb);
            ctx.DrawIndexed();
            ++drawn;
        }
        Msg("[DEBUG] Drew %u lights from '%s'", drawn, name);
    };

    draw_list(package.v_point, "point");
    draw_list(package.v_spot, "spot");
    draw_list(package.v_shadowed, "shadowed");

    Msg("Processed light sources count : %d",
        package.v_point.size() + package.v_spot.size() + package.v_shadowed.size());

    SaveDepthBufferToTGA(*m_softDepthBuffer[m_writeIdx], "LightVolumes.tga");
}
////////////////////////////////////////////////////////////////////////////////
