////////////////////////////////////////////////////////////////////////////////
// Created: 30.09.2026 14:36:17
// Author: NS_Deathman
// File: xrRHI_Buffers.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
enum RHI_BufferUsage : uint32_t
{
	RHI_BufferUsage_Default = 0,
	RHI_BufferUsage_Immutable = 1u << 0,
	RHI_BufferUsage_Dynamic = 1u << 1,
	RHI_BufferUsage_Staging = 1u << 2,
	RHI_BufferUsage_CPUReadable = 1u << 3,
};

enum class RHI_IndexFormat : uint32_t
{
	UInt16 = 0,
	UInt32,
};

inline uint32_t RHI_IndexFormatSize(RHI_IndexFormat f)
{
	return (f == RHI_IndexFormat::UInt32) ? 4u : 2u;
}

struct RHI_BufferDesc
{
	uint32_t sizeBytes = 0;
	uint32_t stride = 0;								   // VB only: размер одной вершины
	RHI_IndexFormat indexFormat = RHI_IndexFormat::UInt16; // IB only
	uint32_t usage = RHI_BufferUsage_Default;
	const char* debugName = nullptr;

	// ---- Фабрики ----

	static RHI_BufferDesc Vertex(uint32_t sizeBytes,
								 uint32_t stride,
								 uint32_t usage = RHI_BufferUsage_Default)
	{
		RHI_BufferDesc d;
		d.sizeBytes = sizeBytes;
		d.stride = stride;
		d.usage = usage;
		return d;
	}

	static RHI_BufferDesc Index(uint32_t sizeBytes,
								RHI_IndexFormat fmt = RHI_IndexFormat::UInt16,
								uint32_t usage = RHI_BufferUsage_Default)
	{
		RHI_BufferDesc d;
		d.sizeBytes = sizeBytes;
		d.indexFormat = fmt;
		d.usage = usage;
		return d;
	}
};
////////////////////////////////////////////////////////////////////////////////
