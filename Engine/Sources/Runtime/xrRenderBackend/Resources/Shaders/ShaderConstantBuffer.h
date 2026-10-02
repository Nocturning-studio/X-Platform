////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrRHI/xrRHI.h>
////////////////////////////////////////////////////////////////////////////////
class CShaderConstantTable;
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CShaderConstantBuffer
{
  public:
	void Reset();

	bool SetFloat(LPCSTR name, float v, const CShaderConstantTable& table);
	bool SetVector(LPCSTR name, const fvec4& v, const CShaderConstantTable& table);
	bool SetMatrix(LPCSTR name, const fmat4x4& m, const CShaderConstantTable& table);
	bool SetMatrixArray(LPCSTR name, uint32_t index, const fmat4x4& m, const CShaderConstantTable& table);

	// Отправить грязные диапазоны в RHI.
	void Flush(IRenderBackend& rhi);

  private:
	void MarkDirty(bool pixel, uint32_t lo, uint32_t hi);

	// Массивы значений. 256 float4-регистров — максимум SM3.0.
	ALIGN(16) fvec4 m_vsData[256];
	ALIGN(16) fvec4 m_psData[256];

	bool m_vsDirty = false;
	bool m_psDirty = false;
	uint32_t m_vsDirtyLo = 256;
	uint32_t m_vsDirtyHi = 0;
	uint32_t m_psDirtyLo = 256;
	uint32_t m_psDirtyHi = 0;
};
////////////////////////////////////////////////////////////////////////////////
