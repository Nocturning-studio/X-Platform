////////////////////////////////////////////////////////////////////////////////
// Created: 25.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "ShaderSampler.h"
////////////////////////////////////////////////////////////////////////////////
namespace
{

	D3DTEXTUREFILTERTYPE ToD3DFilter(ESamplerFilter f)
	{
		switch (f)
		{
		case ESamplerFilter::Point:       return D3DTEXF_POINT;
		case ESamplerFilter::Linear:      return D3DTEXF_LINEAR;
		case ESamplerFilter::Anisotropic: return D3DTEXF_ANISOTROPIC;
		}
		return D3DTEXF_LINEAR;
	}

	D3DTEXTUREADDRESS ToD3DAddress(ESamplerAddress a)
	{
		switch (a)
		{
		case ESamplerAddress::Wrap:       return D3DTADDRESS_WRAP;
		case ESamplerAddress::Clamp:      return D3DTADDRESS_CLAMP;
		case ESamplerAddress::Mirror:     return D3DTADDRESS_MIRROR;
		case ESamplerAddress::Border:     return D3DTADDRESS_BORDER;
		case ESamplerAddress::MirrorOnce: return D3DTADDRESS_MIRRORONCE;
		}
		return D3DTADDRESS_WRAP;
	}

} // namespace

void ApplySamplerDesc(IDirect3DDevice9* device, uint32_t stage, const CSamplerDesc& desc)
{
	CHK_DX(device->SetSamplerState(stage, D3DSAMP_MINFILTER, ToD3DFilter(desc.minFilter)));
	CHK_DX(device->SetSamplerState(stage, D3DSAMP_MAGFILTER, ToD3DFilter(desc.magFilter)));
	CHK_DX(device->SetSamplerState(stage, D3DSAMP_MIPFILTER, ToD3DFilter(desc.mipFilter)));

	CHK_DX(device->SetSamplerState(stage, D3DSAMP_ADDRESSU, ToD3DAddress(desc.addressU)));
	CHK_DX(device->SetSamplerState(stage, D3DSAMP_ADDRESSV, ToD3DAddress(desc.addressV)));
	CHK_DX(device->SetSamplerState(stage, D3DSAMP_ADDRESSW, ToD3DAddress(desc.addressW)));

	if (desc.minFilter == ESamplerFilter::Anisotropic ||
		desc.magFilter == ESamplerFilter::Anisotropic)
	{
		CHK_DX(device->SetSamplerState(stage, D3DSAMP_MAXANISOTROPY, desc.maxAnisotropy));
	}

	if (desc.addressU == ESamplerAddress::Border ||
		desc.addressV == ESamplerAddress::Border ||
		desc.addressW == ESamplerAddress::Border)
	{
		CHK_DX(device->SetSamplerState(stage, D3DSAMP_BORDERCOLOR, desc.borderColor));
	}

	if (desc.mipLodBias != 0.0f)
	{
		const DWORD bias = static_cast<DWORD>(desc.mipLodBias * 256.0f);
		CHK_DX(device->SetSamplerState(stage, D3DSAMP_MIPMAPLODBIAS, bias));
	}
}
////////////////////////////////////////////////////////////////////////////////
