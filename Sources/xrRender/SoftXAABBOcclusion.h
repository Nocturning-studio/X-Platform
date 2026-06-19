////////////////////////////////////////////////////////////////////////////////
// Created: 18.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <SoftX/include/SoftX.h>
////////////////////////////////////////////////////////////////////////////////
class SoftXOcclusionCore;

class SoftXAABBOcclusion
{
public:
    SoftXAABBOcclusion() = default;
    ~SoftXAABBOcclusion() = default;

    void Initialize(SoftXOcclusionCore* core);
    void Shutdown();

    // Тест видимости AABB в мировых координатах
    // depthWidth/Height – размер depth‑буфера, viewProj – матрица View*Projection
    bool TestAABB(const Fbox3& worldAABB, const fmat4x4& viewProj, uint2 depthResolution) const;

    // Тест для экранного прямоугольника (координаты 0..1, как в CHOM)
    bool TestRect(float x0, float y0, float x1, float y1, float depth, uint2 depthResolution) const;

    // Тест для полигона в мировых координатах
    bool TestPolygon(const sPoly& worldPoly, const fmat4x4& viewProj, uint2 depthResolution) const;

private:
    SoftXOcclusionCore* m_core = nullptr;

    // Вспомогательные методы для проекции вершин
    struct ProjectedVertex
    {
        float x, y, z; // экранные 0..1 (x,y), глубина 0..1 (z)
        bool valid;     // false если за near plane
    };
    ProjectedVertex Project(const fvec3& worldPos, const fmat4x4& viewProj) const;
};
////////////////////////////////////////////////////////////////////////////////
