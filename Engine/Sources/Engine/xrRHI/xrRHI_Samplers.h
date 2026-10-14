////////////////////////////////////////////////////////////////////////////////
// Created: 02.10.2026 15:05:03
// Author: NS_Deathman
// File: xrRHI_Samplers.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "xrRHI_Types.h"
////////////////////////////////////////////////////////////////////////////////
struct RHI_SamplerDesc
{
	RHI_Filter minFilter = RHI_Filter::Linear;
	RHI_Filter magFilter = RHI_Filter::Linear;
	RHI_Filter mipFilter = RHI_Filter::Linear;
	RHI_TextureAddress addressU = RHI_TextureAddress::Wrap;
	RHI_TextureAddress addressV = RHI_TextureAddress::Wrap;
	RHI_TextureAddress addressW = RHI_TextureAddress::Wrap;
	uint32_t maxAnisotropy = 1;
	uint32_t borderColor = 0; // 0xAARRGGBB
	float mipLodBias = 0.0f;

	constexpr bool operator==(const RHI_SamplerDesc& o) const noexcept
	{
		return minFilter == o.minFilter &&
			   magFilter == o.magFilter &&
			   mipFilter == o.mipFilter &&
			   addressU == o.addressU &&
			   addressV == o.addressV &&
			   addressW == o.addressW &&
			   maxAnisotropy == o.maxAnisotropy &&
			   borderColor == o.borderColor &&
			   mipLodBias == o.mipLodBias;
	}
	constexpr bool operator!=(const RHI_SamplerDesc& o) const noexcept { return !(*this == o); }

	// ---- Presets ----

	static RHI_SamplerDesc Default() { return {}; }

	static RHI_SamplerDesc Point()
	{
		RHI_SamplerDesc d;
		d.minFilter = d.magFilter = d.mipFilter = RHI_Filter::Point;
		return d;
	}

	static RHI_SamplerDesc Linear()
	{
		RHI_SamplerDesc d;
		d.minFilter = d.magFilter = d.mipFilter = RHI_Filter::Linear;
		return d;
	}

	static RHI_SamplerDesc Anisotropic(uint32_t maxAniso = 16)
	{
		RHI_SamplerDesc d;
		d.minFilter = RHI_Filter::Anisotropic;
		d.magFilter = RHI_Filter::Anisotropic;
		d.mipFilter = RHI_Filter::Linear;
		d.maxAnisotropy = maxAniso;
		return d;
	}

	static RHI_SamplerDesc Clamp()
	{
		RHI_SamplerDesc d;
		d.addressU = d.addressV = d.addressW = RHI_TextureAddress::Clamp;
		return d;
	}
};
////////////////////////////////////////////////////////////////////////////////
