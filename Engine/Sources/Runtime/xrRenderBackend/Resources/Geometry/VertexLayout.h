////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
////////////////////////////////////////////////////////////////////////////////
// Соответствует D3DDECLTYPE_* в D3D9 и DXGI_FORMAT в DX12 (маппинг делается в бэкенде).
enum class EVertexElementType : uint32_t
{
	Unknown = 0,
	Float1, Float2, Float3, Float4,
	UByte4, UByte4N,
	Short2, Short2N, Short4, Short4N,
	// DX10+ - в D3D9 не поддерживаются
	UInt1, UInt2, UInt3, UInt4,
};

// Соответствует D3DDECLUSAGE_* в D3D9 и семантическим строкам ("POSITION", "TEXCOORD"...) в DX12.
enum class EVertexElementSemantic : uint32_t
{
	Position = 0,
	Normal,
	Tangent,
	Binormal,
	TexCoord,
	Color,
	BlendWeight,
	BlendIndices,
	PositionT,
};

enum class EVertexInputRate : uint32_t
{
	PerVertex = 0,
	PerInstance,
};

struct CVertexElement
{
	uint32_t                    stream = 0;
	uint32_t                    offset = 0;
	EVertexElementType     type = EVertexElementType::Float3;
	EVertexElementSemantic semantic = EVertexElementSemantic::Position;
	uint32_t                    semanticIndex = 0;
	EVertexInputRate       inputRate = EVertexInputRate::PerVertex;
	uint32_t                    instanceStepRate = 0; // имеет смысл только для PerInstance
};

inline uint32_t VertexElementTypeSize(EVertexElementType t)
{
	switch (t)
	{
	case EVertexElementType::Float1:  return 4;
	case EVertexElementType::Float2:  return 8;
	case EVertexElementType::Float3:  return 12;
	case EVertexElementType::Float4:  return 16;
	case EVertexElementType::UByte4:  return 4;
	case EVertexElementType::UByte4N: return 4;
	case EVertexElementType::Short2:  return 4;
	case EVertexElementType::Short2N: return 4;
	case EVertexElementType::Short4:  return 8;
	case EVertexElementType::Short4N: return 8;
	case EVertexElementType::UInt1:   return 4;
	case EVertexElementType::UInt2:   return 8;
	case EVertexElementType::UInt3:   return 12;
	case EVertexElementType::UInt4:   return 16;
	default:                          return 0;
	}
}

// Описание входного лэйаута. В DX12 маппится в D3D12_INPUT_LAYOUT_DESC,
// в D3D9 — в массив D3DVERTEXELEMENT9.
struct CVertexLayoutDesc
{
	xr_vector<CVertexElement> elements;

	// Stride для конкретного stream'а (наибольший offset+size).
	uint32_t ComputeStreamStride(uint32_t stream) const
	{
		uint32_t maxEnd = 0;
		for (const auto& e : elements)
		{
			if (e.stream != stream) continue;
			const uint32_t end = e.offset + VertexElementTypeSize(e.type);
			if (end > maxEnd) maxEnd = end;
		}
		return maxEnd;
	}

	xr_vector<uint32_t> ComputeStreamStrides() const
	{
		uint32_t streams = 0;
		for (const auto& e : elements)
			if (e.stream + 1 > streams) streams = e.stream + 1;

		xr_vector<uint32_t> strides(streams, 0);
		for (const auto& e : elements)
		{
			const uint32_t end = e.offset + VertexElementTypeSize(e.type);
			if (end > strides[e.stream]) strides[e.stream] = end;
		}
		return strides;
	}
};
////////////////////////////////////////////////////////////////////////////////
