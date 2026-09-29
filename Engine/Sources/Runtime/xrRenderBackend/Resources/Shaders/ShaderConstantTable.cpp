////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "ShaderConstantTable.h"
////////////////////////////////////////////////////////////////////////////////
namespace
{

	EConstantClass ConvertClass(uint16_t v)
	{
		switch (v)
		{
		case D3DXPC_SCALAR: return EConstantClass::Scalar;
		case D3DXPC_VECTOR: return EConstantClass::Vector;
		case D3DXPC_MATRIX_ROWS: return EConstantClass::Matrix4x4; // уточняется в Parse по Rows/Columns
		case D3DXPC_MATRIX_COLUMNS: return EConstantClass::Matrix4x4;
		case D3DXPC_STRUCT: return EConstantClass::Struct;
		case D3DXPC_OBJECT: return EConstantClass::Object;
		default: return EConstantClass::Unknown;
		}
	}

	// Точный класс матрицы зависит от Rows/Columns в D3DXSHADER_TYPEINFO.
	EConstantClass ConvertMatrixClass(uint16_t rows, uint16_t columns)
	{
		if (rows == 3 && columns == 4) return EConstantClass::Matrix3x4;
		if (rows == 4 && columns == 4) return EConstantClass::Matrix4x4;
		if (rows == 2 && columns == 4) return EConstantClass::Matrix2x4;
		return EConstantClass::Unknown;
	}

	EConstantType ConvertType(uint16_t v)
	{
		switch (v)
		{
		case D3DXPT_VOID: return EConstantType::Void;
		case D3DXPT_BOOL: return EConstantType::Bool;
		case D3DXPT_INT: return EConstantType::Int;
		case D3DXPT_FLOAT: return EConstantType::Float;
		case D3DXPT_STRING: return EConstantType::String;
		case D3DXPT_TEXTURE: return EConstantType::Texture;
		default: return EConstantType::Unknown;
		}
	}

	EConstantRegister ConvertRegisterSet(uint16_t v)
	{
		switch (v)
		{
		case D3DXRS_FLOAT4: return EConstantRegister::Float4;
		case D3DXRS_INT4: return EConstantRegister::Int4;
		case D3DXRS_BOOL: return EConstantRegister::Bool;
		case D3DXRS_SAMPLER: return EConstantRegister::Sampler;
		default: return EConstantRegister::Unknown;
		}
	}

	// Разбирает CTAB-секцию токен-стрима SM2/SM3.
	// D3DXFindShaderComment сам проходит по токенам до END и возвращает
	// указатель на payload после FourCC.
	BOOL ParseBytecode(const void* bytecode, 
				       uint16_t destination, 
				       xr_vector<CShaderConstantEntry>& outConstants, 
				       xr_vector<CShaderSamplerEntry>& outSamplers)
	{
		if (!bytecode)
			return FALSE;

		const DWORD* tokens = static_cast<const DWORD*>(bytecode);

		LPCVOID ctabData = nullptr;
		UINT ctabSize = 0;
		HRESULT hr = D3DXFindShaderComment(tokens, MAKEFOURCC('C', 'T', 'A', 'B'),
			&ctabData, &ctabSize);
		if (FAILED(hr) || !ctabData)
			return FALSE;

		if (ctabSize < sizeof(D3DXSHADER_CONSTANTTABLE))
			return FALSE;

		const auto* table = static_cast<const D3DXSHADER_CONSTANTTABLE*>(ctabData);

		if (table->ConstantInfo + table->Constants * sizeof(D3DXSHADER_CONSTANTINFO) > ctabSize)
			return FALSE;

		const BYTE* base = static_cast<const BYTE*>(ctabData);
		const auto* info = reinterpret_cast<const D3DXSHADER_CONSTANTINFO*>(base + table->ConstantInfo);

		for (UINT i = 0; i < table->Constants; ++i, ++info)
		{
			if (info->TypeInfo + sizeof(D3DXSHADER_TYPEINFO) > ctabSize)
				continue;

			const auto* ti = reinterpret_cast<const D3DXSHADER_TYPEINFO*>(base + info->TypeInfo);
			const char* name = info->Name ? reinterpret_cast<const char*>(base + info->Name) : "";

			if (info->RegisterSet == D3DXRS_SAMPLER)
			{
				CShaderSamplerEntry s;
				s.name = name;
				s.type = ConvertType(static_cast<uint16_t>(ti->Type));

				// D3D9-стейдж: PS — r_index (0-15), VS — D3DVERTEXTEXTURESAMPLER0 + r_index.
				const uint32_t r = info->RegisterIndex;
				s.dx9Stage = (destination & RC_dest_pixel_bit)
					? r
					: (D3DVERTEXTEXTURESAMPLER0 + r);

				outSamplers.push_back(std::move(s));
				continue;
			}

			CShaderConstantEntry c;
			c.name = name;
			c.registerSet = ConvertRegisterSet(static_cast<uint16_t>(info->RegisterSet));
			c.type = ConvertType(static_cast<uint16_t>(ti->Type));

			if (ti->Class == D3DXPC_MATRIX_ROWS || ti->Class == D3DXPC_MATRIX_COLUMNS)
				c.cls = ConvertMatrixClass(static_cast<uint16_t>(ti->Rows), static_cast<uint16_t>(ti->Columns));
			else
				c.cls = ConvertClass(static_cast<uint16_t>(ti->Class));

			c.registerCount = info->RegisterCount;

			if (destination & RC_dest_vertex_bit)
				c.vsRegister = info->RegisterIndex;
			if (destination & RC_dest_pixel_bit)
				c.psRegister = info->RegisterIndex;

			outConstants.push_back(std::move(c));
		}

		return TRUE;
	}

} // namespace

////////////////////////////////////////////////////////////////////////////////

BOOL CShaderConstantTable::Parse(const void* bytecode, UINT /*bytecodeSize*/, uint16_t destination)
{
	Clear();

	if (!ParseBytecode(bytecode, destination, m_constants, m_samplers))
		return FALSE;

	std::sort(m_constants.begin(), m_constants.end(),
		[](const CShaderConstantEntry& a, const CShaderConstantEntry& b)
		{ return xr_strcmp(a.name.c_str(), b.name.c_str()) < 0; });

	std::sort(m_samplers.begin(), m_samplers.end(),
		[](const CShaderSamplerEntry& a, const CShaderSamplerEntry& b)
		{ return xr_strcmp(a.name.c_str(), b.name.c_str()) < 0; });

	return TRUE;
}

void CShaderConstantTable::Merge(const CShaderConstantTable& other)
{
	for (const auto& src : other.m_constants)
	{
		auto it = std::lower_bound(m_constants.begin(), m_constants.end(), src.name.c_str(),
			[](const CShaderConstantEntry& e, LPCSTR n)
			{
				return xr_strcmp(e.name.c_str(), n) < 0;
			});

		if (it != m_constants.end() && xr_strcmp(it->name.c_str(), src.name.c_str()) == 0)
		{
			// Уже есть — дополняем регистрами из второго стейджа.
			if (src.vsRegister != uint32_t(-1))
				it->vsRegister = src.vsRegister;
			if (src.psRegister != uint32_t(-1))
				it->psRegister = src.psRegister;
		}
		else
		{
			m_constants.insert(it, src);
		}
	}

	for (const auto& src : other.m_samplers)
		m_samplers.push_back(src);

	std::sort(m_samplers.begin(), m_samplers.end(),
		[](const CShaderSamplerEntry& a, const CShaderSamplerEntry& b)
		{ return xr_strcmp(a.name.c_str(), b.name.c_str()) < 0; });
}

void CShaderConstantTable::Clear()
{
	m_constants.clear();
	m_samplers.clear();
}

const CShaderConstantEntry* CShaderConstantTable::FindConstant(LPCSTR name) const
{
	if (!name)
		return nullptr;

	auto it = std::lower_bound(m_constants.begin(), m_constants.end(), name,
		[](const CShaderConstantEntry& e, LPCSTR n)
		{
			return xr_strcmp(e.name.c_str(), n) < 0;
		});

	if (it != m_constants.end() && xr_strcmp(it->name.c_str(), name) == 0)
		return &(*it);

	return nullptr;
}

const CShaderSamplerEntry* CShaderConstantTable::FindSampler(LPCSTR name) const
{
	if (!name)
		return nullptr;

	auto it = std::lower_bound(m_samplers.begin(), m_samplers.end(), name,
		[](const CShaderSamplerEntry& e, LPCSTR n)
		{ return xr_strcmp(e.name.c_str(), n) < 0; });

	if (it != m_samplers.end() && xr_strcmp(it->name.c_str(), name) == 0)
		return &(*it);

	return nullptr;
}
////////////////////////////////////////////////////////////////////////////////
