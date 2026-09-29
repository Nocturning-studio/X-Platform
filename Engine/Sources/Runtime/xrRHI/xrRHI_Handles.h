////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
constexpr uint32_t InvalidHandleId = 0xFFFFFFFF;

struct RHI_TextureHandle
{
	uint32_t id = InvalidHandleId;
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
	uint32_t id = InvalidHandleId;
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
	uint32_t id = InvalidHandleId;
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
	uint32_t id = InvalidHandleId;
	bool IsValid() const
	{
		return id != InvalidHandleId;
	}
	bool operator==(const RHI_ConstantBufferHandle& other) const
	{
		return id == other.id;
	}
};
////////////////////////////////////////////////////////////////////////////////
