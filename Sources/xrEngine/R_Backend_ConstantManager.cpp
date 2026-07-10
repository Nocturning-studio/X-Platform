#include "stdafx.h"
#include "r_constants.h"
#include <d3d9.h>

CConstantManager::CConstantManager()
{
    memset(m_pixelData, 0, sizeof(m_pixelData));
    memset(m_vertexData, 0, sizeof(m_vertexData));
    m_pixelDirty = false;
    m_vertexDirty = false;
    m_pixelDirtyLo = 256;
    m_pixelDirtyHi = 0;
    m_vertexDirtyLo = 256;
    m_vertexDirtyHi = 0;
}

void CConstantManager::ResetDirty()
{
    m_pixelDirty = false;
    m_vertexDirty = false;
    m_pixelDirtyLo = 256;
    m_pixelDirtyHi = 0;
    m_vertexDirtyLo = 256;
    m_vertexDirtyHi = 0;
}

void CConstantManager::ForceDirty()
{
    m_pixelDirty = true;
    m_vertexDirty = true;
    m_pixelDirtyLo = 0;
    m_pixelDirtyHi = 256;
    m_vertexDirtyLo = 0;
    m_vertexDirtyHi = 256;
}

static void MarkDirtyRange(bool& dirtyFlag, u32& lo, u32& hi, u32 start, u32 count)
{
    if (start < lo) lo = start;
    u32 end = start + count;
    if (end > hi) hi = end;
    dirtyFlag = true;
}

void CConstantManager::SetConstantInternal(R_constant* C, R_constant_load& L, const fmat4x4& A, bool isPixel)
{
    VERIFY(C->type == RC_float);
    fvec4* data = isPixel ? m_pixelData : m_vertexData;
    bool& dirty = isPixel ? m_pixelDirty : m_vertexDirty;
    u32& lo = isPixel ? m_pixelDirtyLo : m_vertexDirtyLo;
    u32& hi = isPixel ? m_pixelDirtyHi : m_vertexDirtyHi;

    switch (L.size_class)
    {
    case RC_2x4:
        data[L.offset].set(A._11, A._21, A._31, A._41);
        data[L.offset + 1].set(A._12, A._22, A._32, A._42);
        MarkDirtyRange(dirty, lo, hi, L.offset, 2);
        break;
    case RC_3x4:
        data[L.offset].set(A._11, A._21, A._31, A._41);
        data[L.offset + 1].set(A._12, A._22, A._32, A._42);
        data[L.offset + 2].set(A._13, A._23, A._33, A._43);
        MarkDirtyRange(dirty, lo, hi, L.offset, 3);
        break;
    case RC_4x4:
        data[L.offset].set(A._11, A._21, A._31, A._41);
        data[L.offset + 1].set(A._12, A._22, A._32, A._42);
        data[L.offset + 2].set(A._13, A._23, A._33, A._43);
        data[L.offset + 3].set(A._14, A._24, A._34, A._44);
        MarkDirtyRange(dirty, lo, hi, L.offset, 4);
        break;
    default:
        FATAL("Invalid constant run-time-type");
    }
}

void CConstantManager::SetConstant(R_constant* C, const fmat4x4& A)
{
    if (!C) return;
    if (C->destination & RC_dest_pixel)  SetConstantInternal(C, C->ps, A, true);
    if (C->destination & RC_dest_vertex) SetConstantInternal(C, C->vs, A, false);
}

void CConstantManager::SetConstantInternal(R_constant* C, R_constant_load& L, const fvec4& A, bool isPixel)
{
    VERIFY(C->type == RC_float);
    fvec4* data = isPixel ? m_pixelData : m_vertexData;
    bool& dirty = isPixel ? m_pixelDirty : m_vertexDirty;
    u32& lo = isPixel ? m_pixelDirtyLo : m_vertexDirtyLo;
    u32& hi = isPixel ? m_pixelDirtyHi : m_vertexDirtyHi;

    if (L.size_class == RC_1x1)
    {
        data[L.offset].set(A.x, 0.0f, 0.0f, 0.0f);
        MarkDirtyRange(dirty, lo, hi, L.offset, 1);
    }
    else if (L.size_class == RC_1x4)
    {
        data[L.offset] = A;
        MarkDirtyRange(dirty, lo, hi, L.offset, 1);
    }
    else FATAL("Unexpected class for vector constant");
}

void CConstantManager::SetConstant(R_constant* C, const fvec4& A)
{
    if (!C) return;
    if (C->destination & RC_dest_pixel)  SetConstantInternal(C, C->ps, A, true);
    if (C->destination & RC_dest_vertex) SetConstantInternal(C, C->vs, A, false);
}

void CConstantManager::SetConstant(R_constant* C, float x, float y, float z, float w)
{
    fvec4 tmp;
    tmp.set(x, y, z, w);
    SetConstant(C, tmp);
}

void CConstantManager::SetArrayConstantInternal(R_constant* C, R_constant_load& L, u32 e, const fmat4x4& A, bool isPixel)
{
    VERIFY(C->type == RC_float);
    fvec4* data = isPixel ? m_pixelData : m_vertexData;
    bool& dirty = isPixel ? m_pixelDirty : m_vertexDirty;
    u32& lo = isPixel ? m_pixelDirtyLo : m_vertexDirtyLo;
    u32& hi = isPixel ? m_pixelDirtyHi : m_vertexDirtyHi;

    u32 base;
    u32 count = 0;
    switch (L.size_class)
    {
    case RC_2x4: base = L.offset + 2 * e; count = 2;
        data[base].set(A._11, A._21, A._31, A._41);
        data[base + 1].set(A._12, A._22, A._32, A._42);
        break;
    case RC_3x4: base = L.offset + 3 * e; count = 3;
        data[base].set(A._11, A._21, A._31, A._41);
        data[base + 1].set(A._12, A._22, A._32, A._42);
        data[base + 2].set(A._13, A._23, A._33, A._43);
        break;
    case RC_4x4: base = L.offset + 4 * e; count = 4;
        data[base].set(A._11, A._21, A._31, A._41);
        data[base + 1].set(A._12, A._22, A._32, A._42);
        data[base + 2].set(A._13, A._23, A._33, A._43);
        data[base + 3].set(A._14, A._24, A._34, A._44);
        break;
    default: FATAL("Invalid constant array type"); return;
    }
    MarkDirtyRange(dirty, lo, hi, base, count);
}

void CConstantManager::SetArrayConstantInternal(R_constant* C, R_constant_load& L, u32 e, const fvec4& A, bool isPixel)
{
    VERIFY(C->type == RC_float);
    VERIFY(L.cls == RC_1x4);
    fvec4* data = isPixel ? m_pixelData : m_vertexData;
    bool& dirty = isPixel ? m_pixelDirty : m_vertexDirty;
    u32& lo = isPixel ? m_pixelDirtyLo : m_vertexDirtyLo;
    u32& hi = isPixel ? m_pixelDirtyHi : m_vertexDirtyHi;

    u32 base = L.offset + e;
    data[base] = A;
    MarkDirtyRange(dirty, lo, hi, base, 1);
}

void CConstantManager::SetArrayConstant(R_constant* C, u32 e, const fmat4x4& A)
{
    if (!C) return;
    if (C->destination & RC_dest_pixel)  SetArrayConstantInternal(C, C->ps, e, A, true);
    if (C->destination & RC_dest_vertex) SetArrayConstantInternal(C, C->vs, e, A, false);
}

void CConstantManager::SetArrayConstant(R_constant* C, u32 e, const fvec4& A)
{
    if (!C) return;
    if (C->destination & RC_dest_pixel)  SetArrayConstantInternal(C, C->ps, e, A, true);
    if (C->destination & RC_dest_vertex) SetArrayConstantInternal(C, C->vs, e, A, false);
}

void CConstantManager::SetArrayConstant(R_constant* C, u32 e, float x, float y, float z, float w)
{
    fvec4 tmp; tmp.set(x, y, z, w);
    SetArrayConstant(C, e, tmp);
}

void CConstantManager::Flush()
{
    IDirect3DDevice9Ex* device = RenderBackend.GetDevice();
    if (!device) return;

    if (m_pixelDirty && m_pixelDirtyLo < m_pixelDirtyHi)
    {
        u32 count = m_pixelDirtyHi - m_pixelDirtyLo;
        R_ASSERT(count < 32);
        CHK_DX(device->SetPixelShaderConstantF(m_pixelDirtyLo, (float*)&m_pixelData[m_pixelDirtyLo], count));
        m_pixelDirty = false;
        m_pixelDirtyLo = 256;
        m_pixelDirtyHi = 0;
    }

    if (m_vertexDirty && m_vertexDirtyLo < m_vertexDirtyHi)
    {
        u32 count = m_vertexDirtyHi - m_vertexDirtyLo;
        CHK_DX(device->SetVertexShaderConstantF(m_vertexDirtyLo, (float*)&m_vertexData[m_vertexDirtyLo], count));
        m_vertexDirty = false;
        m_vertexDirtyLo = 256;
        m_vertexDirtyHi = 0;
    }
}
