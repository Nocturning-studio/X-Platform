////////////////////////////////////////////////////////////////////////////////
// Created: 18.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "Stdafx.h"
#include "SoftXAABBOcclusion.h"
#include "SoftXOcclusionCore.h"
#include "../xrCDB/xrCDB.h"
////////////////////////////////////////////////////////////////////////////////
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
    PROFILE_FUNCTION();

    if (!m_core) return true; // безопасное поведение

    std::shared_ptr<SoftX::DepthBuffer> db = m_core->GetReadBuffer();
    if (!db) return true;

    // --- 1. Проецируем углы, получаем экранный rect и минимальную глубину ---
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
        if (!pv.valid) return true;   // пересекает near plane – считаем видимым

        anyValid = true;
        if (pv.x < minX) minX = pv.x;
        if (pv.x > maxX) maxX = pv.x;
        if (pv.y < minY) minY = pv.y;
        if (pv.y > maxY) maxY = pv.y;
        if (pv.z < minDepth) minDepth = pv.z;
    }

    if (!anyValid) return true;

    // --- 2. Преобразуем в пиксельные координаты уровня 0 ---
    int px0 = iFloor(minX * depthResolution.x);
    int py0 = iFloor(minY * depthResolution.y);
    int px1 = iCeil(maxX * depthResolution.x);
    int py1 = iCeil(maxY * depthResolution.y);

    px0 = std::max(0, px0);
    py0 = std::max(0, py0);
    px1 = std::min((int)depthResolution.x - 1, px1);
    py1 = std::min((int)depthResolution.y - 1, py1);

    // Защита от вырожденных случаев
    if (px0 > px1 || py0 > py1) return true;

    int rectW = px1 - px0 + 1;
    int rectH = py1 - py0 + 1;

    // --- 3. Выбираем уровень мип-цепочки, на котором rect ≤ 2×2 ---
    uint level = 0;
    uint maxMip = db->MipCount() - 1;
    while (level < maxMip && (rectW > 2 || rectH > 2))
    {
        rectW = (rectW + 1) / 2;   // консервативное масштабирование ширины
        rectH = (rectH + 1) / 2;
        px0 /= 2;
        py0 /= 2;
        px1 = px0 + rectW - 1;
        py1 = py0 + rectH - 1;
        ++level;
    }

    // Корректируем границы, чтобы не выйти за размер мипа
    uint2 mipSize = db->MipSize(level);
    px0 = std::max(0, px0);
    py0 = std::max(0, py0);
    px1 = std::min((int)mipSize.x - 1, px1);
    py1 = std::min((int)mipSize.y - 1, py1);

    // --- 4. Проверяем несколько пикселей на этом уровне ---
    for (int y = py0; y <= py1; ++y)
    {
        for (int x = px0; x <= px1; ++x)
        {
            float hiZDepth = db->Read(int2(x, y), level);
            // Так как HiZ хранит минимум, условие minDepth < hiZDepth
            // означает, что хотя бы один исходный пиксель AABB ближе, чем occluder.
            // Следовательно, объект не полностью перекрыт – видим.
            if (minDepth < hiZDepth)
                return true;
        }
    }
    return false; // AABB полностью перекрыт окклюдерами
}

bool SoftXAABBOcclusion::TestRect(float x0, float y0, float x1, float y1, float depth, uint2 depthResolution) const
{
    PROFILE_FUNCTION();

    if (!m_core) return true;
    std::shared_ptr<SoftX::DepthBuffer> db = m_core->GetReadBuffer();
    if (!db) return true;

    // 1. Пиксельные координаты на уровне 0
    int px0 = iFloor(x0 * depthResolution.x);
    int py0 = iFloor(y0 * depthResolution.y);
    int px1 = iCeil(x1 * depthResolution.x);
    int py1 = iCeil(y1 * depthResolution.y);

    // Защита от вырожденного прямоугольника
    if (px0 > px1 || py0 > py1)
        return true;

    // Clamp границ в размер буфера
    px0 = std::max(0, px0);
    py0 = std::max(0, py0);
    px1 = std::min((int)depthResolution.x - 1, px1);
    py1 = std::min((int)depthResolution.y - 1, py1);

    int rectW = px1 - px0 + 1;
    int rectH = py1 - py0 + 1;

    // 2. Выбор уровня мип-цепочки (консервативно уменьшаем до ≤2×2)
    uint level = 0;
    const uint maxMip = db->MipCount() - 1;
    while (level < maxMip && (rectW > 2 || rectH > 2))
    {
        rectW = (rectW + 1) / 2;  // ширина после уменьшения вдвое (округляем вверх)
        rectH = (rectH + 1) / 2;
        px0 /= 2;
        py0 /= 2;
        px1 = px0 + rectW - 1;
        py1 = py0 + rectH - 1;
        ++level;
    }

    // 3. Коррекция границ под размер выбранного мипа
    const uint2 mipSize = db->MipSize(level);
    px0 = std::max(0, px0);
    py0 = std::max(0, py0);
    px1 = std::min((int)mipSize.x - 1, px1);
    py1 = std::min((int)mipSize.y - 1, py1);

    // 4. Консервативный тест: если хотя бы в одном пикселе Hi‑Z
    //    глубина объекта меньше сохранённой (т.е. объект ближе) – видим.
    for (int y = py0; y <= py1; ++y)
    {
        for (int x = px0; x <= px1; ++x)
        {
            float hiZDepth = db->Read(int2(x, y), level);
            if (depth < hiZDepth)   // объект ближе, чем окклюдер
                return true;
        }
    }
    return false;   // полностью перекрыт
}

bool SoftXAABBOcclusion::TestPolygon(const sPoly& worldPoly, const fmat4x4& viewProj, uint2 depthResolution) const
{
    PROFILE_FUNCTION();

    if (!m_core || worldPoly.empty()) return true;
    std::shared_ptr<SoftX::DepthBuffer> db = m_core->GetReadBuffer();
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
