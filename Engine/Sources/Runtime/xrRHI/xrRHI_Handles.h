#pragma once

#include "framework.h"
#include "xrRHI_Internal.h"

constexpr u32 InvalidHandleId = 0xFFFFFFFF;

struct RHI_TextureHandle
{
	u32 id = InvalidHandleId;
	bool IsValid() const
	{
		return id != InvalidHandleId;
	}
	bool operator==(const RHI_TextureHandle& other) const
	{
		return id == other.id;
	}
};

struct RHI_SamplerHandle
{
	u32 id = InvalidHandleId;
	bool IsValid() const
	{
		return id != InvalidHandleId;
	}
	bool operator==(const RHI_SamplerHandle& other) const
	{
		return id == other.id;
	}
};

struct RHI_ShaderHandle
{
	u32 id = InvalidHandleId;
	bool IsValid() const
	{
		return id != InvalidHandleId;
	}
	bool operator==(const RHI_ShaderHandle& other) const
	{
		return id == other.id;
	}
};

struct RHI_ConstantBufferHandle
{
	u32 id = InvalidHandleId;
	bool IsValid() const
	{
		return id != InvalidHandleId;
	}
	bool operator==(const RHI_ConstantBufferHandle& other) const
	{
		return id == other.id;
	}
};
