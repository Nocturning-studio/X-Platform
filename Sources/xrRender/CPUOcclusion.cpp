////////////////////////////////////////////////////////////////////////////////
// Created: 14.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "CPUOcclusion.h"
#include "SoftXOcclusionCore.h"
#include "SoftXOcclusionMapBuilder.h"
#include "HOM.h"
////////////////////////////////////////////////////////////////////////////////
inline bool SaveDepthBufferToTGA(const SoftX::DepthBuffer& depthBuffer, const char* filename)
{
    const uint width = depthBuffer.Width();
    const uint height = depthBuffer.Height();
    if (width == 0 || height == 0) return false;

    std::ofstream file(filename, std::ios::binary);
    if (!file) return false;

    uint8_t header[18] = {};
    header[2] = 3;
    header[12] = static_cast<uint8_t>(width & 0xFF);
    header[13] = static_cast<uint8_t>((width >> 8) & 0xFF);
    header[14] = static_cast<uint8_t>(height & 0xFF);
    header[15] = static_cast<uint8_t>((height >> 8) & 0xFF);
    header[16] = 8;
    header[17] = 0x20;
    file.write(reinterpret_cast<const char*>(header), sizeof(header));

    std::vector<uint8_t> row(width);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float depth = depthBuffer.Read(int2(x, y));
            uint8_t c = static_cast<uint8_t>(AfterMath::clamp(depth, 0.0f, 1.0f) * 255.0f + 0.5f);
            row[x] = c;
        }
        file.write(reinterpret_cast<const char*>(row.data()), width);
    }
    return file.good();
}

// ---------------------------------------------------------------------------------------
// Конструктор / деструктор
// ---------------------------------------------------------------------------------------
CPUOcclusion::CPUOcclusion() {}
CPUOcclusion::~CPUOcclusion() { Unload(); }

// ---------------------------------------------------------------------------------------
// Загрузка / выгрузка
// ---------------------------------------------------------------------------------------
void CPUOcclusion::Load(const CHOM& hom)
{
    Unload();

    // Загружаем геометрию в SoftXOcclusionMapBuilder
    m_occlusionMap.Load(hom);
    if (!m_occlusionMap.IsLoaded())
        return;

    // Получаем сырые данные из CHOM для D3D9-отладки
    const CDB::MODEL* model = hom.get_occluder_model();

    xr_vector<fvec3> vertices;
    xr_vector<u16> indices;

    const u32 vertCount = model->get_verts_count();
    const u32 triCount = model->get_tris_count();

    const fvec3* srcVerts = model->get_verts();
    vertices.assign(srcVerts, srcVerts + vertCount);

    const CDB::TRI* srcTris = model->get_tris();
    indices.reserve(triCount * 3);
    for (u32 i = 0; i < triCount; ++i)
    {
        const CDB::TRI& tri = srcTris[i];
        R_ASSERT(tri.verts[0] <= 0xFFFF && tri.verts[1] <= 0xFFFF && tri.verts[2] <= 0xFFFF);
        indices.push_back((u16)tri.verts[0]);
        indices.push_back((u16)tri.verts[1]);
        indices.push_back((u16)tri.verts[2]);
    }

    m_vertexCount = (u32)vertices.size();
    m_indexCount = (u32)indices.size();

    if (m_vertexCount == 0 || m_indexCount == 0)
    {
        Msg("! [CPUOcclusion] HOM model is empty.");
        return;
    }

    InitializeSoftX();

    // D3D9 отладка
    {
        const u32 vbSize = m_vertexCount * sizeof(fvec3);
        R_CHK(HW.GetDevice()->CreateVertexBuffer(vbSize, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &m_VB, nullptr));
        fvec3* pData = nullptr;
        R_CHK(m_VB->Lock(0, 0, (void**)&pData, 0));
        std::memcpy(pData, vertices.data(), vbSize);
        R_CHK(m_VB->Unlock());
    }
    {
        const u32 ibSize = m_indexCount * sizeof(u16);
        R_CHK(HW.GetDevice()->CreateIndexBuffer(ibSize, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_IB, nullptr));
        u16* pData = nullptr;
        R_CHK(m_IB->Lock(0, 0, (void**)&pData, 0));
        std::memcpy(pData, indices.data(), ibSize);
        R_CHK(m_IB->Unlock());
    }

    static D3DVERTEXELEMENT9 dwDecl[] = {
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

    ShutdownSoftX();
}

// ---------------------------------------------------------------------------------------
// D3D9 отладка
// ---------------------------------------------------------------------------------------
void CPUOcclusion::DrawDebug()
{
    OPTICK_EVENT("CPUOcclusion::DrawDebug()");
    if (!m_loaded) return;

    RenderBackendLegacy.set_Geometry(m_geom);
    RenderBackendLegacy.set_transform_world(Fidentity);
    RenderBackendLegacy.set_Shader(m_shader);
    RenderBackendLegacy.set_CullMode(CULL_DISABLE);

    const u32 primCount = m_indexCount / 3;
    RenderBackendLegacy.Render(D3DPT_TRIANGLELIST, 0, 0, m_vertexCount, 0, primCount);
}

// ---------------------------------------------------------------------------------------
// SoftX инициализация / деинициализация
// ---------------------------------------------------------------------------------------
void CPUOcclusion::InitializeSoftX()
{
    ShutdownSoftX();
    m_core = std::make_unique<SoftXOcclusionCore>();
    m_core->Initialize(512);

    m_occlusionMap.SetCore(m_core.get());
    m_lightOcc.Initialize(m_core.get());
}

void CPUOcclusion::ShutdownSoftX()
{
    m_lightOcc.Shutdown();
    if (m_core) m_core->Shutdown();
    m_core.reset();
}

// ---------------------------------------------------------------------------------------
// Глубина
// ---------------------------------------------------------------------------------------
void CPUOcclusion::SaveDepthBuffer(const SoftX::DepthBuffer& depthBuffer, const char* filename)
{
    SaveDepthBufferToTGA(depthBuffer, filename);
}

void CPUOcclusion::SaveDepthBuffer()
{
    if (m_core)
        SaveDepthBufferToTGA(*m_core->GetReadBuffer(), "output.tga");
}
////////////////////////////////////////////////////////////////////////////////
