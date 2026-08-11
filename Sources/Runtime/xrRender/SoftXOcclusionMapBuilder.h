////////////////////////////////////////////////////////////////////////////////
// Created: 17.06.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <SoftX/include/SoftX.h>
////////////////////////////////////////////////////////////////////////////////
class CHOM;
class SoftXOcclusionCore;

class SoftXOcclusionMapBuilder
{
public:
    SoftXOcclusionMapBuilder() = default;
    ~SoftXOcclusionMapBuilder() = default;

    // Загрузить геометрию из CHOM (вершины + индексы)
    void Load(const CHOM& hom);

    // Освободить ресурсы
    void Unload();

    bool IsLoaded() const { return m_loaded; }

    // Привязать ядро SoftX
    void SetCore(SoftXOcclusionCore* core) { m_core = core; }

    // Асинхронно заполнить depth-буфер (write-буфер ядра)
    void BuildAsync(const fmat4x4& viewProj);

    u32 GetVertexCount() const { return m_vertexCount; }
    u32 GetIndexCount()  const { return m_indexCount; }

private:
    SoftXOcclusionCore* m_core = nullptr;

    // Геометрия окклюдеров HOM
    std::unique_ptr<SoftX::VertexBuffer> m_occluderVB;
    std::unique_ptr<SoftX::IndexBuffer>  m_occluderIB;

    u32 m_vertexCount = 0;
    u32 m_indexCount  = 0;
    bool m_loaded     = false;

    // Вспомогательные функции для извлечения геометрии из CDB
    void ExtractGeometry(const CDB::MODEL* model,
                         xr_vector<fvec3>& outVertices,
                         xr_vector<u16>& outIndices) const;
};
////////////////////////////////////////////////////////////////////////////////
