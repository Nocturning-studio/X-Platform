////////////////////////////////////////////////////////////////////////////////
// Created: 18.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "Stdafx.h"
#include "SoftXAABBOcclusion.h"
#include "SoftXOcclusionCore.h"
#include "../xrCDB/xrCDB.h" // для Fbox3, fvec3

SoftXAABBOcclusion::ProjectedVertex SoftXAABBOcclusion::Project(const fvec3& worldPos,
                                                                const fmat4x4& viewProj) const
{
    ProjectedVertex res;
    const float x = worldPos.x, y = worldPos.y, z = worldPos.z;

    // Преобразование [x,y,z,1] * viewProj (row‑major)
    float w = x * viewProj._14 + y * viewProj._24 + z * viewProj._34 + viewProj._44;
    float cx = x * viewProj._11 + y * viewProj._21 + z * viewProj._31 + viewProj._41;
    float cy = x * viewProj._12 + y * viewProj._22 + z * viewProj._32 + viewProj._42;
    float cz = x * viewProj._13 + y * viewProj._23 + z * viewProj._33 + viewProj._43;

    if (w <= 0.0f || _abs(w) < EPS_L)
    {
        res.valid = false; // за ближней плоскостью
        return res;
    }

    float invW = 1.0f / w;
    res.x = cx * invW * 0.5f + 0.5f;   // диапазон 0..1
    res.y = cy * invW * (-0.5f) + 0.5f; // инверсия Y (как в CHOM)
    res.z = cz * invW;                   // глубина 0..1
    res.valid = true;
    return res;
}

bool SoftXAABBOcclusion::TestAABB(const Fbox3& worldAABB, const fmat4x4& viewProj, uint2 depthResolution) const
{
    if (!m_core) return true; // безопасное поведение

    SoftX::DepthBuffer* db = m_core->GetReadBuffer();
    if (!db) return true;

    // Проецируем 8 углов
    const fvec3 corners[8] = {
        {worldAABB.min.x, worldAABB.min.y, worldAABB.min.z},
        {worldAABB.min.x, worldAABB.min.y, worldAABB.max.z},
        {worldAABB.min.x, worldAABB.max.y, worldAABB.min.z},
        {worldAABB.min.x, worldAABB.max.y, worldAABB.max.z},
        {worldAABB.max.x, worldAABB.min.y, worldAABB.min.z},
        {worldAABB.max.x, worldAABB.min.y, worldAABB.max.z},
        {worldAABB.max.x, worldAABB.max.y, worldAABB.min.z},
        {worldAABB.max.x, worldAABB.max.y, worldAABB.max.z}
    };

    float minX = 1.0f, maxX = 0.0f, minY = 1.0f, maxY = 0.0f;
    float minDepth = 1.0f;
    bool anyValid = false;

    for (int i = 0; i < 8; ++i)
    {
        ProjectedVertex pv = Project(corners[i], viewProj);
        if (!pv.valid) return true; // near‑plane пересечение → видим

        anyValid = true;
        if (pv.x < minX) minX = pv.x;
        if (pv.x > maxX) maxX = pv.x;
        if (pv.y < minY) minY = pv.y;
        if (pv.y > maxY) maxY = pv.y;
        if (pv.z < minDepth) minDepth = pv.z;
    }

    if (!anyValid) return true; // все вершины за near plane (не должно случаться)

    // Преобразуем в пиксельные координаты depth‑буфера
    int px0 = iFloor(minX * depthResolution.x);
    int py0 = iFloor(minY * depthResolution.y);
    int px1 = iCeil(maxX * depthResolution.x);
    int py1 = iCeil(maxY * depthResolution.y);

    // Отсечение границ
    px0 = std::max(0, px0);
    py0 = std::max(0, py0);
    px1 = std::min((int)depthResolution.x - 1, px1);
    py1 = std::min((int)depthResolution.y - 1, py1);

    // Консервативный тест: ищем хотя бы один пиксель, где AABB ближе
    for (int y = py0; y <= py1; ++y)
    {
        for (int x = px0; x <= px1; ++x)
        {
            float storedDepth = db->Read(int2(x, y));
            if (minDepth < storedDepth + 0.001f) // стандартный Less: если объект ближе
                return true;
        }
    }
    return false; // перекрыт
}

bool SoftXAABBOcclusion::TestRect(float x0, float y0, float x1, float y1, float depth, uint2 depthResolution) const
{
    if (!m_core) return true;
    SoftX::DepthBuffer* db = m_core->GetReadBuffer();
    if (!db) return true;

    // Преобразуем нормализованные координаты (0..1) в пиксельные
    int px0 = iFloor(x0 * depthResolution.x);
    int py0 = iFloor(y0 * depthResolution.y);
    int px1 = iCeil(x1 * depthResolution.x);
    int py1 = iCeil(y1 * depthResolution.y);

    // Clamp
    px0 = std::max(0, px0);
    py0 = std::max(0, py0);
    px1 = std::min((int)depthResolution.x - 1, px1);
    py1 = std::min((int)depthResolution.y - 1, py1);

    // Консервативный тест: достаточно одного пикселя с глубиной меньше depth
    for (int y = py0; y <= py1; ++y)
    {
        for (int x = px0; x <= px1; ++x)
        {
            float stored = db->Read(int2(x, y));
            if (depth < stored)    // объект ближе к камере
                return true;
        }
    }
    return false;
}

bool SoftXAABBOcclusion::TestPolygon(const sPoly& worldPoly, const fmat4x4& viewProj, uint2 depthResolution) const
{
    if (!m_core || worldPoly.empty()) return true;
    SoftX::DepthBuffer* db = m_core->GetReadBuffer();
    if (!db) return true;

    float minX = 1.0f, maxX = 0.0f, minY = 1.0f, maxY = 0.0f;
    float minDepth = 1.0f;

    for (const fvec3& v : worldPoly)
    {
        ProjectedVertex pv = Project(v, viewProj);
        if (!pv.valid) return true; // пересекает near plane → видим

        if (pv.x < minX) minX = pv.x;
        if (pv.x > maxX) maxX = pv.x;
        if (pv.y < minY) minY = pv.y;
        if (pv.y > maxY) maxY = pv.y;
        if (pv.z < minDepth) minDepth = pv.z;
    }

    return TestRect(minX, minY, maxX, maxY, minDepth, depthResolution);
}

void SoftXAABBOcclusion::Initialize(SoftXOcclusionCore* core)
{
    m_core = core;
}

void SoftXAABBOcclusion::Shutdown()
{
    m_core = nullptr;
}
////////////////////////////////////////////////////////////////////////////////
