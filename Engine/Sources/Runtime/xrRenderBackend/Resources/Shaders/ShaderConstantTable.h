////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "ShaderConstant.h"
////////////////////////////////////////////////////////////////////////////////
constexpr uint16_t RC_dest_pixel_bit = (1 << 0);
constexpr uint16_t RC_dest_vertex_bit = (1 << 1);
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CShaderConstantTable
{
  public:
	// Разобрать CTAB-секцию из байткода. destination — RC_dest_vertex / RC_dest_pixel.
	BOOL Parse(const void* bytecode, UINT bytecodeSize, uint16_t destination);

	// Слить в себя другую таблицу (для объединения VS и PS).
	void Merge(const CShaderConstantTable& other);

	void Clear();

	const xr_vector<CShaderConstantEntry>& All() const { return m_constants; }
	const xr_vector<CShaderSamplerEntry>& Samplers() const { return m_samplers; }

	// Поиск по имени.
	const CShaderConstantEntry* FindConstant(LPCSTR name) const;
	const CShaderSamplerEntry* FindSampler(LPCSTR name) const;

	bool empty() const { return m_constants.empty() && m_samplers.empty(); }

  private:
	xr_vector<CShaderConstantEntry> m_constants;
	xr_vector<CShaderSamplerEntry> m_samplers;
};
////////////////////////////////////////////////////////////////////////////////
