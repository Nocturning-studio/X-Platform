////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "ShaderConstantBuffer.h"
#include "ShaderConstantTable.h"
////////////////////////////////////////////////////////////////////////////////
namespace
{
// Нижние 2 бита — те же, что в старой системе.
// destination & 1 → пиксельный стейдж
// destination & 2 → вершинный стейдж
constexpr u16 RC_dest_pixel = (1 << 0);
constexpr u16 RC_dest_vertex = (1 << 1);
} // namespace

void CShaderConstantBuffer::Reset()
{
	std::memset(m_vsData, 0, sizeof(m_vsData));
	std::memset(m_psData, 0, sizeof(m_psData));
	m_vsDirty = false;
	m_psDirty = false;
	m_vsDirtyLo = 256;
	m_vsDirtyHi = 0;
	m_psDirtyLo = 256;
	m_psDirtyHi = 0;
}

void CShaderConstantBuffer::MarkDirty(bool pixel, u32 lo, u32 hi)
{
	if(pixel)
	{
		m_psDirty = true;
		if(lo < m_psDirtyLo)
			m_psDirtyLo = lo;
		if(hi > m_psDirtyHi)
			m_psDirtyHi = hi;
	}
	else
	{
		m_vsDirty = true;
		if(lo < m_vsDirtyLo)
			m_vsDirtyLo = lo;
		if(hi > m_vsDirtyHi)
			m_vsDirtyHi = hi;
	}
}

bool CShaderConstantBuffer::SetFloat(LPCSTR name, float v, const CShaderConstantTable& table)
{
	const CShaderConstant* c = table.Find(name);
	if(!c)
		return false;

	const fvec4 val{v, 0.0f, 0.0f, 0.0f};

	if(c->vsRegister != u32(-1))
	{
		m_vsData[c->vsRegister] = val;
		MarkDirty(false, c->vsRegister, c->vsRegister + 1);
	}
	if(c->psRegister != u32(-1))
	{
		m_psData[c->psRegister] = val;
		MarkDirty(true, c->psRegister, c->psRegister + 1);
	}
	return true;
}

bool CShaderConstantBuffer::SetVector(LPCSTR name, const fvec4& v, const CShaderConstantTable& table)
{
	const CShaderConstant* c = table.Find(name);
	if(!c)
		return false;

	if(c->vsRegister != u32(-1))
	{
		m_vsData[c->vsRegister] = v;
		MarkDirty(false, c->vsRegister, c->vsRegister + 1);
	}
	if(c->psRegister != u32(-1))
	{
		m_psData[c->psRegister] = v;
		MarkDirty(true, c->psRegister, c->psRegister + 1);
	}
	return true;
}

bool CShaderConstantBuffer::SetMatrix(LPCSTR name, const fmat4x4& m, const CShaderConstantTable& table)
{
	const CShaderConstant* c = table.Find(name);
	if(!c)
		return false;

	// Раскладка под D3DCOMPILE_PACK_MATRIX_ROW_MAJOR:
	// строки матрицы раскладываются в последовательные float4-регистры,
	// но записываются в транспонированном виде (как в старом CConstantManager).
	auto write = [&](fvec4* dst, u32 reg)
	{
		switch(c->cls)
		{
		case EConstantClass::Matrix2x4:
			dst[reg + 0].set(m._11, m._21, m._31, m._41);
			dst[reg + 1].set(m._12, m._22, m._32, m._42);
			return 2;
		case EConstantClass::Matrix3x4:
			dst[reg + 0].set(m._11, m._21, m._31, m._41);
			dst[reg + 1].set(m._12, m._22, m._32, m._42);
			dst[reg + 2].set(m._13, m._23, m._33, m._43);
			return 3;
		case EConstantClass::Matrix4x4:
			dst[reg + 0].set(m._11, m._21, m._31, m._41);
			dst[reg + 1].set(m._12, m._22, m._32, m._42);
			dst[reg + 2].set(m._13, m._23, m._33, m._43);
			dst[reg + 3].set(m._14, m._24, m._34, m._44);
			return 4;
		default:
			return 0;
		}
	};

	if(c->vsRegister != u32(-1))
	{
		const u32 n = write(m_vsData, c->vsRegister);
		if(n)
			MarkDirty(false, c->vsRegister, c->vsRegister + n);
	}
	if(c->psRegister != u32(-1))
	{
		const u32 n = write(m_psData, c->psRegister);
		if(n)
			MarkDirty(true, c->psRegister, c->psRegister + n);
	}
	return true;
}

bool CShaderConstantBuffer::SetMatrixArray(LPCSTR name, u32 index, const fmat4x4& m, const CShaderConstantTable& table)
{
	const CShaderConstant* c = table.Find(name);
	if(!c)
		return false;

	// Шаг между элементами массива — размер одного элемента.
	u32 stride = 0;
	switch(c->cls)
	{
	case EConstantClass::Matrix2x4:
		stride = 2;
		break;
	case EConstantClass::Matrix3x4:
		stride = 3;
		break;
	case EConstantClass::Matrix4x4:
		stride = 4;
		break;
	default:
		return false;
	}

	auto write = [&](fvec4* dst, u32 reg)
	{
		dst[reg + 0].set(m._11, m._21, m._31, m._41);
		dst[reg + 1].set(m._12, m._22, m._32, m._42);
		if(stride >= 3)
			dst[reg + 2].set(m._13, m._23, m._33, m._43);
		if(stride >= 4)
			dst[reg + 3].set(m._14, m._24, m._34, m._44);
	};

	if(c->vsRegister != u32(-1))
	{
		const u32 reg = c->vsRegister + index * stride;
		write(m_vsData, reg);
		MarkDirty(false, reg, reg + stride);
	}
	if(c->psRegister != u32(-1))
	{
		const u32 reg = c->psRegister + index * stride;
		write(m_psData, reg);
		MarkDirty(true, reg, reg + stride);
	}
	return true;
}

void CShaderConstantBuffer::Flush(IDirect3DDevice9* device)
{
	if(!device)
		return;

	if(m_vsDirty && m_vsDirtyLo < m_vsDirtyHi)
	{
		const u32 count = m_vsDirtyHi - m_vsDirtyLo;
		CHK_DX(device->SetVertexShaderConstantF(m_vsDirtyLo,
												reinterpret_cast<const float*>(&m_vsData[m_vsDirtyLo]),
												count));
		m_vsDirty = false;
		m_vsDirtyLo = 256;
		m_vsDirtyHi = 0;
	}

	if(m_psDirty && m_psDirtyLo < m_psDirtyHi)
	{
		const u32 count = m_psDirtyHi - m_psDirtyLo;
		CHK_DX(device->SetPixelShaderConstantF(m_psDirtyLo,
											   reinterpret_cast<const float*>(&m_psData[m_psDirtyLo]),
											   count));
		m_psDirty = false;
		m_psDirtyLo = 256;
		m_psDirtyHi = 0;
	}
}
////////////////////////////////////////////////////////////////////////////////
