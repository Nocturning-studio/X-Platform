////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
////////////////////////////////////////////////////////////////////////////////
enum class EConstantRegister : uint16_t
{
	Float4 = 0,
	Int4 = 1,
	Bool = 2,
	Sampler = 3,
	Unknown = 0xFFFF,
};

enum class EConstantClass : uint16_t
{
	Scalar = 0,		// 1 float4
	Vector = 1,		// 1 float4
	Matrix2x4 = 2,	// 2 float4 (транспонированная 4x2)
	Matrix3x4 = 3,	// 3 float4
	Matrix4x4 = 4,	// 4 float4
	Struct = 5,
	Object = 6,
	Unknown = 0xFFFF,
};

enum class EConstantType : uint16_t
{
	Void = 0,
	Bool = 1,
	Int = 2,
	Float = 3,
	String = 4,
	Texture = 5,
	Unknown = 0xFFFF,
};

struct CShaderSamplerEntry
{
	xr_string name;
	uint32_t dx9Stage = uint32_t(-1);      // 0-15 для PS, 257+ для VS
	EConstantType type = EConstantType::Unknown; // 2D / Cube / 3D / ...
};

struct CShaderConstantEntry
{
	xr_string name;
	EConstantRegister registerSet = EConstantRegister::Unknown;
	EConstantClass cls = EConstantClass::Unknown;
	EConstantType type = EConstantType::Unknown;

	// Индекс регистра в каждом стейдже (UINT_MAX = не используется в этом стейдже).
	uint32_t vsRegister = uint32_t(-1);
	uint32_t psRegister = uint32_t(-1);

	// Сколько float4-регистров занимает (обычно 1..4).
	uint16_t registerCount = 0;
};
////////////////////////////////////////////////////////////////////////////////
