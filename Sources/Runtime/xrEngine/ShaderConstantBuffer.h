////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
class CShaderConstantTable;
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API CShaderConstantBuffer
{
public:
	void Reset();		// сбросить dirty-флаги (например, после device reset)

	bool SetFloat(LPCSTR name, float v, const CShaderConstantTable& table);
	bool SetVector(LPCSTR name, const fvec4& v, const CShaderConstantTable& table);
	bool SetMatrix(LPCSTR name, const fmat4x4& m, const CShaderConstantTable& table);
	bool SetMatrixArray(LPCSTR name, u32 index, const fmat4x4& m, const CShaderConstantTable& table);

	// Отправить грязные диапазоны в device.
	void Flush(IDirect3DDevice9* device);

private:
	void MarkDirty(bool pixel, u32 lo, u32 hi);

	// Массивы значений. 256 float4-регистров — максимум SM3.0.
	ALIGN(16) fvec4 m_vsData[256];
	ALIGN(16) fvec4 m_psData[256];

	bool m_vsDirty = false;
	bool m_psDirty = false;
	u32 m_vsDirtyLo = 256;
	u32 m_vsDirtyHi = 0;
	u32 m_psDirtyLo = 256;
	u32 m_psDirtyHi = 0;
};
////////////////////////////////////////////////////////////////////////////////
