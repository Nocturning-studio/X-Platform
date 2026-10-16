////////////////////////////////////////////////////////////////////////////////
// Created: 12.10.2026 20:48:58
// Author: NS_Deathman
// File: XRHI_PipelineLayout.h
// Nocturning studio for X-RHI
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "XRHI_Handles.h"
#include "XRHI_Types.h"
////////////////////////////////////////////////////////////////////////////////

// ============================================================================
// RHI_BindingType
// ============================================================================
enum class RHI_BindingType : uint32_t
{
	ConstantBuffer = 0,
	ShaderResource,
	UnorderedAccess,
	Sampler,
};

// ============================================================================
// RHI_ShaderStage (битовая маска)
// ============================================================================
enum RHI_ShaderStageBits : uint32_t
{
	RHI_Stage_None = 0,
	RHI_Stage_Vertex = 1u << 0,
	RHI_Stage_Pixel = 1u << 1,
	// Задел: Geometry, Hull, Domain, Compute
};

// ============================================================================
// RHI_BindingDesc
// ============================================================================
//
// Описание одного биндинга в pipeline layout.
//
// D3D12:
//   - rootConstantCount > 0, type == ConstantBuffer -> root constants (inline)
//   - иначе -> descriptor table, тип берётся из type
//
// D3D9:
//   - layout игнорируется; регистры там неявные (c#, s#, t#).
//   - но layout может использоваться для валидации в debug-сборке.
//
// ============================================================================
struct RHI_BindingDesc
{
	RHI_BindingType type = RHI_BindingType::ShaderResource;
	uint32_t shaderRegister = 0;
	uint32_t registerSpace = 0;
	uint32_t arraySize = 1;
	uint32_t shaderStages = RHI_Stage_Pixel;

	// Только для ConstantBuffer: > 0 — root constants (D3D12).
	// Измеряется в 32-битных словах (1 vec4 == 4).
	uint32_t rootConstantCount = 0;
};

// ============================================================================
// RHI_PipelineLayoutDesc
// ============================================================================
struct RHI_PipelineLayoutDesc
{
	std::vector<RHI_BindingDesc> bindings;
	const char* debugName = nullptr;

	void AddCBV(uint32_t reg, uint32_t stages, uint32_t rootConstCount = 0, uint32_t arraySize = 1)
	{
		RHI_BindingDesc b;
		b.type = RHI_BindingType::ConstantBuffer;
		b.shaderRegister = reg;
		b.shaderStages = stages;
		b.rootConstantCount = rootConstCount;
		b.arraySize = arraySize;
		bindings.push_back(b);
	}

	void AddSRV(uint32_t reg, uint32_t stages, uint32_t arraySize = 1)
	{
		RHI_BindingDesc b;
		b.type = RHI_BindingType::ShaderResource;
		b.shaderRegister = reg;
		b.shaderStages = stages;
		b.arraySize = arraySize;
		bindings.push_back(b);
	}

	void AddSampler(uint32_t reg, uint32_t stages)
	{
		RHI_BindingDesc b;
		b.type = RHI_BindingType::Sampler;
		b.shaderRegister = reg;
		b.shaderStages = stages;
		bindings.push_back(b);
	}
};
////////////////////////////////////////////////////////////////////////////////
