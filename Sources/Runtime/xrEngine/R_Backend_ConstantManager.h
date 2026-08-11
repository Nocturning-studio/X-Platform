// CConstantManager.h
#pragma once
#include "r_constants.h"   // R_constant, R_constant_load ещЄ нужны

class ENGINE_API CConstantManager
{
public:
    CConstantManager();
    ~CConstantManager() {}

    void ResetDirty();
    void ForceDirty();
    void Flush();

    void SetConstant(R_constant* C, const fmat4x4& A);
    void SetConstant(R_constant* C, const fvec4& A);
    void SetConstant(R_constant* C, float x, float y, float z, float w);

    // ћассивы (установка элемента e)
    void SetArrayConstant(R_constant* C, u32 e, const fmat4x4& A);
    void SetArrayConstant(R_constant* C, u32 e, const fvec4& A);
    void SetArrayConstant(R_constant* C, u32 e, float x, float y, float z, float w);

private:
    // ¬нутренние методы дл€ работы с конкретным массивом (пиксельным или вершинным)
    void SetConstantInternal(R_constant* C, R_constant_load& L, const fmat4x4& A, bool isPixel);
    void SetConstantInternal(R_constant* C, R_constant_load& L, const fvec4& A, bool isPixel);
    void SetArrayConstantInternal(R_constant* C, R_constant_load& L, u32 e, const fmat4x4& A, bool isPixel);
    void SetArrayConstantInternal(R_constant* C, R_constant_load& L, u32 e, const fvec4& A, bool isPixel);

    // ƒанные
    ALIGN(16) fvec4 m_pixelData[256];
    ALIGN(16) fvec4 m_vertexData[256];

    bool  m_pixelDirty;
    bool  m_vertexDirty;
    u32   m_pixelDirtyLo;   // минимальный индекс изменившегос€ регистра
    u32   m_pixelDirtyHi;   // максимальный индекс + 1 (диапазон [lo, hi))
    u32   m_vertexDirtyLo;
    u32   m_vertexDirtyHi;
};
