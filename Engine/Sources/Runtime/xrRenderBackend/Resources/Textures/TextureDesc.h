////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "TextureFormat.h"
////////////////////////////////////////////////////////////////////////////////
enum class ETextureDim : uint8_t
{
	Tex1D,
	Tex2D,
	Tex3D,
	Cube
};

enum ETextureUsage : uint32_t
{
	TexUsage_None = 0,
	TexUsage_ShaderResource = 1u << 0,
	TexUsage_RenderTarget = 1u << 1,
	TexUsage_DepthStencil = 1u << 2,
	TexUsage_Unordered = 1u << 3,
	TexUsage_CPUReadable = 1u << 4,
	TexUsage_CPUWritable = 1u << 5,
	TexUsage_GenerateMips = 1u << 6,
};

struct CTextureDesc
{
	ETextureDim dim = ETextureDim::Tex2D;
	uint32_t width = 1;
	uint32_t height = 1;
	uint32_t depth = 1;
	uint32_t mips = 1;
	uint32_t arraySize = 1;
	uint32_t sampleCount = 1;
	uint32_t sampleQuality = 0;
	ETextureFormat format = ETextureFormat::R8G8B8A8_UNORM;
	uint32_t usage = TexUsage_ShaderResource;

	const char* debugName = nullptr;

	static CTextureDesc RenderTarget(uint32_t w, uint32_t h, ETextureFormat fmt, uint32_t mips = 1)
	{
		CTextureDesc d;
		d.width = w;
		d.height = h;
		d.mips = mips;
		d.format = fmt;
		d.usage = TexUsage_RenderTarget | TexUsage_ShaderResource;
		return d;
	}

	static CTextureDesc DepthStencil(uint32_t w, uint32_t h, ETextureFormat fmt = ETextureFormat::D24_UNORM_S8_UINT)
	{
		CTextureDesc d;
		d.width = w;
		d.height = h;
		d.format = fmt;
		d.usage = TexUsage_DepthStencil;
		return d;
	}
};
////////////////////////////////////////////////////////////////////////////////
