////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ShaderConstantTable.h"
#include <algorithm>
////////////////////////////////////////////////////////////////////////////////
namespace
{

	EConstantClass ConvertClass(u16 v)
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
	EConstantClass ConvertMatrixClass(u16 rows, u16 columns)
	{
		if (rows == 3 && columns == 4) return EConstantClass::Matrix3x4;
		if (rows == 4 && columns == 4) return EConstantClass::Matrix4x4;
		if (rows == 2 && columns == 4) return EConstantClass::Matrix2x4;
		return EConstantClass::Unknown;
	}

	EConstantType ConvertType(u16 v)
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

	EConstantRegister ConvertRegisterSet(u16 v)
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
	BOOL ParseConstantsFromBytecode(const void* bytecode, u16 destination,
		xr_vector<CShaderConstant>& out)
	{
		if (!bytecode)
			return FALSE;

		const DWORD* tokens = static_cast<const DWORD*>(bytecode);

		LPCVOID ctabData = nullptr;
		UINT ctabSize = 0;
		HRESULT hr = D3DXFindShaderComment(tokens, MAKEFOURCC('C', 'T', 'A', 'B'), &ctabData, &ctabSize);
		if (FAILED(hr) || !ctabData)
		{
			Msg("! [CTAB] D3DXFindShaderComment failed (hr=0x%08X)", hr);
			return FALSE;
		}

		if (ctabSize < sizeof(D3DXSHADER_CONSTANTTABLE))
		{
			Msg("! [CTAB] CTAB too small: %u bytes", ctabSize);
			return FALSE;
		}

		const auto* table = static_cast<const D3DXSHADER_CONSTANTTABLE*>(ctabData);

		if (table->ConstantInfo + table->Constants * sizeof(D3DXSHADER_CONSTANTINFO) > ctabSize)
		{
			Msg("! [CTAB] ConstantInfo array overflows CTAB: info=%u, count=%u, size=%u",
				table->ConstantInfo, table->Constants, ctabSize);
			return FALSE;
		}

		const BYTE* base = static_cast<const BYTE*>(ctabData);
		const auto* info = reinterpret_cast<const D3DXSHADER_CONSTANTINFO*>(base + table->ConstantInfo);

		for (UINT i = 0; i < table->Constants; ++i, ++info)
		{
			// Сэмплеры — отдельная тема (sampler-state manager).
			if (info->RegisterSet == D3DXRS_SAMPLER)
				continue;

			if (info->TypeInfo + sizeof(D3DXSHADER_TYPEINFO) > ctabSize)
				continue;

			const auto* ti = reinterpret_cast<const D3DXSHADER_TYPEINFO*>(base + info->TypeInfo);

			CShaderConstant c;
			c.name = info->Name ? reinterpret_cast<const char*>(base + info->Name) : "";
			c.registerSet = ConvertRegisterSet(static_cast<u16>(info->RegisterSet));
			c.type = ConvertType(static_cast<u16>(ti->Type));

			// Класс для матриц уточняем по Rows/Columns.
			if (ti->Class == D3DXPC_MATRIX_ROWS || ti->Class == D3DXPC_MATRIX_COLUMNS)
				c.cls = ConvertMatrixClass(static_cast<u16>(ti->Rows), static_cast<u16>(ti->Columns));
			else
				c.cls = ConvertClass(static_cast<u16>(ti->Class));

			c.registerCount = info->RegisterCount;

			if (destination & RC_dest_vertex_bit)
				c.vsRegister = info->RegisterIndex;
			if (destination & RC_dest_pixel_bit)
				c.psRegister = info->RegisterIndex;

			out.push_back(std::move(c));
		}

		return TRUE;
	}

} // namespace

////////////////////////////////////////////////////////////////////////////////

BOOL CShaderConstantTable::Parse(const void* bytecode, UINT /*bytecodeSize*/, u16 destination)
{
	Clear();

	if (!ParseConstantsFromBytecode(bytecode, destination, m_entries))
		return FALSE;

	std::sort(m_entries.begin(), m_entries.end(),
		[](const CShaderConstant& a, const CShaderConstant& b)
		{
			return xr_strcmp(a.name.c_str(), b.name.c_str()) < 0;
		});

	return TRUE;
}

void CShaderConstantTable::Merge(const CShaderConstantTable& other)
{
	for (const auto& src : other.m_entries)
	{
		auto it = std::lower_bound(m_entries.begin(), m_entries.end(), src.name.c_str(),
			[](const CShaderConstant& e, LPCSTR n)
			{
				return xr_strcmp(e.name.c_str(), n) < 0;
			});

		if (it != m_entries.end() && xr_strcmp(it->name.c_str(), src.name.c_str()) == 0)
		{
			// Уже есть — дополняем регистрами из второго стейджа.
			if (src.vsRegister != u32(-1))
				it->vsRegister = src.vsRegister;
			if (src.psRegister != u32(-1))
				it->psRegister = src.psRegister;
		}
		else
		{
			m_entries.insert(it, src);
		}
	}
}

void CShaderConstantTable::Clear()
{
	m_entries.clear();
}

const CShaderConstant* CShaderConstantTable::Find(LPCSTR name) const
{
	if (!name)
		return nullptr;

	auto it = std::lower_bound(m_entries.begin(), m_entries.end(), name,
		[](const CShaderConstant& e, LPCSTR n)
		{
			return xr_strcmp(e.name.c_str(), n) < 0;
		});

	if (it != m_entries.end() && xr_strcmp(it->name.c_str(), name) == 0)
		return &(*it);

	return nullptr;
}
////////////////////////////////////////////////////////////////////////////////
