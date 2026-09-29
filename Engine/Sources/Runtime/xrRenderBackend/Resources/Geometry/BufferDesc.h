////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
////////////////////////////////////////////////////////////////////////////////

enum EBufferUsageFlag : uint32_t
{
	BufferUsage_Default = 0,
	BufferUsage_Immutable = 1u << 0, // не меняется после создания (DX12: D3D12_HEAP_TYPE_DEFAULT)
	BufferUsage_Dynamic = 1u << 1,	 // частая запись с CPU (D3D9: D3DUSAGE_DYNAMIC + DEFAULT)
	BufferUsage_Staging = 1u << 2,	 // CPU-readback / upload heap (DX12: UPLOAD/READBACK)
	BufferUsage_CPUReadable = 1u << 3,
};

enum class EIndexFormat : uint32_t
{
	UInt16 = 0,
	UInt32,
};

struct CVertexBufferDesc
{
	uint32_t sizeBytes = 0;
	uint32_t stride = 0; // размер одной вершины (для SetStreamSource)
	uint32_t usage = BufferUsage_Default;
	const char* debugName = nullptr;
};

struct CIndexBufferDesc
{
	uint32_t sizeBytes = 0;
	EIndexFormat format = EIndexFormat::UInt16;
	uint32_t usage = BufferUsage_Default;
	const char* debugName = nullptr;
};

inline uint32_t IndexFormatSize(EIndexFormat f)
{
	return (f == EIndexFormat::UInt32) ? 4u : 2u;
}
////////////////////////////////////////////////////////////////////////////////
