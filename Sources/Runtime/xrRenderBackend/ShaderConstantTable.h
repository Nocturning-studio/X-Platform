////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "ShaderConstant.h"
////////////////////////////////////////////////////////////////////////////////
constexpr u16 RC_dest_pixel_bit = (1 << 0);
constexpr u16 RC_dest_vertex_bit = (1 << 1);
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CShaderConstantTable
{
public:
	// Разобрать CTAB-секцию из байткода. destination — RC_dest_vertex / RC_dest_pixel.
	BOOL Parse(const void* bytecode, UINT bytecodeSize, u16 destination);

	// Слить в себя другую таблицу (для объединения VS и PS).
	void Merge(const CShaderConstantTable& other);

	void Clear();

	// Поиск по имени.
	const CShaderConstant* Find(LPCSTR name) const;

	const xr_vector<CShaderConstant>& All() const { return m_entries; }
	bool empty() const { return m_entries.empty(); }

private:
	xr_vector<CShaderConstant> m_entries;
};
////////////////////////////////////////////////////////////////////////////////
