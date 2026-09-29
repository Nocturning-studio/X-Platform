////////////////////////////////////////////////////////////////////////////////
// Created: 25.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
enum class ESamplerFilter : uint8_t
{
	Point,
	Linear,
	Anisotropic,
};

enum class ESamplerAddress : uint8_t
{
	Wrap,
	Clamp,
	Mirror,
	Border,
	MirrorOnce,
};

struct CSamplerDesc
{
	ESamplerFilter  minFilter = ESamplerFilter::Linear;
	ESamplerFilter  magFilter = ESamplerFilter::Linear;
	ESamplerFilter  mipFilter = ESamplerFilter::Linear;
	ESamplerAddress addressU = ESamplerAddress::Wrap;
	ESamplerAddress addressV = ESamplerAddress::Wrap;
	ESamplerAddress addressW = ESamplerAddress::Wrap;
	uint32_t             maxAnisotropy = 1;
	uint32_t             borderColor = 0;
	float           mipLodBias = 0.0f;

	static CSamplerDesc Default() { return {}; }

	static CSamplerDesc Point()
	{
		CSamplerDesc d;
		d.minFilter = d.magFilter = d.mipFilter = ESamplerFilter::Point;
		return d;
	}

	static CSamplerDesc Linear()
	{
		CSamplerDesc d;
		return d;
	}

	static CSamplerDesc Anisotropic(uint32_t maxAniso = 16)
	{
		CSamplerDesc d;
		d.minFilter = ESamplerFilter::Anisotropic;
		d.magFilter = ESamplerFilter::Anisotropic;
		d.mipFilter = ESamplerFilter::Linear;
		d.maxAnisotropy = maxAniso;
		return d;
	}

	static CSamplerDesc Clamp()
	{
		CSamplerDesc d;
		d.addressU = d.addressV = d.addressW = ESamplerAddress::Clamp;
		return d;
	}
};

void ApplySamplerDesc(IDirect3DDevice9* device, uint32_t stage, const CSamplerDesc& desc);
////////////////////////////////////////////////////////////////////////////////
