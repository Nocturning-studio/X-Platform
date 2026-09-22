////////////////////////////////////////////////////////////////////////////////
// Created: 22.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
enum class EConstantRegister : u16
{
	Float4 = 0,
	Int4 = 1,
	Bool = 2,
	Sampler = 3,
	Unknown = 0xFFFF,
};

enum class EConstantClass : u16
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

enum class EConstantType : u16
{
	Void = 0,
	Bool = 1,
	Int = 2,
	Float = 3,
	String = 4,
	Texture = 5,
	Unknown = 0xFFFF,
};

struct CShaderConstant
{
	xr_string name;
	EConstantRegister registerSet = EConstantRegister::Unknown;
	EConstantClass cls = EConstantClass::Unknown;
	EConstantType type = EConstantType::Unknown;

	// Индекс регистра в каждом стейдже (UINT_MAX = не используется в этом стейдже).
	u32 vsRegister = u32(-1);
	u32 psRegister = u32(-1);

	// Сколько float4-регистров занимает (обычно 1..4).
	u16 registerCount = 0;
};
////////////////////////////////////////////////////////////////////////////////
