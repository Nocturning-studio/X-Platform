////////////////////////////////////////////////////////////////////////////////
// Created: 30.09.2026 14:38:42
// Author: NS_Deathman
// File: xrRHI_InputLayout.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
enum class RHI_VertexElementType : uint32_t
{
	Unknown = 0,
	Float1,
	Float2,
	Float3,
	Float4,
	UByte4,
	UByte4N,
	Short2,
	Short2N,
	Short4,
	Short4N,
	// DX10+ — в D3D9 не поддерживаются
	UInt1,
	UInt2,
	UInt3,
	UInt4,
};

enum class RHI_VertexElementSemantic : uint32_t
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

enum class RHI_VertexInputRate : uint32_t
{
	PerVertex = 0,
	PerInstance,
};

struct RHI_VertexElement
{
	uint32_t stream = 0;
	uint32_t offset = 0;
	RHI_VertexElementType type = RHI_VertexElementType::Float3;
	RHI_VertexElementSemantic semantic = RHI_VertexElementSemantic::Position;
	uint32_t semanticIndex = 0;
	RHI_VertexInputRate inputRate = RHI_VertexInputRate::PerVertex;
	uint32_t instanceStepRate = 0; // имеет смысл только для PerInstance
};

inline uint32_t RHI_VertexElementTypeSize(RHI_VertexElementType t)
{
	switch(t)
	{
	case RHI_VertexElementType::Float1:
		return 4;
	case RHI_VertexElementType::Float2:
		return 8;
	case RHI_VertexElementType::Float3:
		return 12;
	case RHI_VertexElementType::Float4:
		return 16;
	case RHI_VertexElementType::UByte4:
		return 4;
	case RHI_VertexElementType::UByte4N:
		return 4;
	case RHI_VertexElementType::Short2:
		return 4;
	case RHI_VertexElementType::Short2N:
		return 4;
	case RHI_VertexElementType::Short4:
		return 8;
	case RHI_VertexElementType::Short4N:
		return 8;
	case RHI_VertexElementType::UInt1:
		return 4;
	case RHI_VertexElementType::UInt2:
		return 8;
	case RHI_VertexElementType::UInt3:
		return 12;
	case RHI_VertexElementType::UInt4:
		return 16;
	default:
		return 0;
	}
}

struct RHI_InputLayoutDesc
{
	xr_vector<RHI_VertexElement> elements;

	// Stride для конкретного stream'а (наибольший offset+size).
	uint32_t ComputeStreamStride(uint32_t stream) const
	{
		uint32_t maxEnd = 0;
		for(const auto& e : elements)
		{
			if(e.stream != stream)
				continue;
			const uint32_t end = e.offset + RHI_VertexElementTypeSize(e.type);
			if(end > maxEnd)
				maxEnd = end;
		}
		return maxEnd;
	}

	xr_vector<uint32_t> ComputeStreamStrides() const
	{
		uint32_t streams = 0;
		for(const auto& e : elements)
			if(e.stream + 1 > streams)
				streams = e.stream + 1;

		xr_vector<uint32_t> strides(streams, 0);
		for(const auto& e : elements)
		{
			const uint32_t end = e.offset + RHI_VertexElementTypeSize(e.type);
			if(end > strides[e.stream])
				strides[e.stream] = end;
		}
		return strides;
	}
};
////////////////////////////////////////////////////////////////////////////////
