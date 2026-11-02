////////////////////////////////////////////////////////////////////////////////
// Created: 02.10.2026 15:07:42
// Author: NS_Deathman
// File: xrBackendDX9_Samplers.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "xrBackendDX9.h"
////////////////////////////////////////////////////////////////////////////////
namespace
{

D3DTEXTUREFILTERTYPE ToD3DFilter(RHI_Filter f)
{
	switch(f)
	{
	case RHI_Filter::None:
		return D3DTEXF_NONE;
	case RHI_Filter::Point:
		return D3DTEXF_POINT;
	case RHI_Filter::Linear:
		return D3DTEXF_LINEAR;
	case RHI_Filter::Anisotropic:
		return D3DTEXF_ANISOTROPIC;
	case RHI_Filter::PyramidalQuad:
		return D3DTEXF_PYRAMIDALQUAD;
	case RHI_Filter::GaussianQuad:
		return D3DTEXF_GAUSSIANQUAD;
	}
	return D3DTEXF_LINEAR;
}

D3DTEXTUREADDRESS ToD3DAddress(RHI_TextureAddress a)
{
	switch(a)
	{
	case RHI_TextureAddress::Wrap:
		return D3DTADDRESS_WRAP;
	case RHI_TextureAddress::Mirror:
		return D3DTADDRESS_MIRROR;
	case RHI_TextureAddress::Clamp:
		return D3DTADDRESS_CLAMP;
	case RHI_TextureAddress::Border:
		return D3DTADDRESS_BORDER;
	case RHI_TextureAddress::MirrorOnce:
		return D3DTADDRESS_MIRRORONCE;
	}
	return D3DTADDRESS_WRAP;
}

// D3D9 stage: PS uses raw 0..15, VS uses D3DVERTEXTEXTURESAMPLER0 + 0..3.
inline DWORD ToD3DStage(uint32_t slot)
{
	return (slot < 16) ? slot : (D3DVERTEXTEXTURESAMPLER0 + (slot - 16));
}

} // namespace
////////////////////////////////////////////////////////////////////////////////

void CRenderBackendDX9::SetSampler(uint32_t slot, const RHI_SamplerDesc& desc)
{
	if(!m_pDevice)
		return;

	const DWORD stage = ToD3DStage(slot);

	m_pDevice->SetSamplerState(stage, D3DSAMP_MINFILTER, ToD3DFilter(desc.minFilter));
	m_pDevice->SetSamplerState(stage, D3DSAMP_MAGFILTER, ToD3DFilter(desc.magFilter));
	m_pDevice->SetSamplerState(stage, D3DSAMP_MIPFILTER, ToD3DFilter(desc.mipFilter));

	m_pDevice->SetSamplerState(stage, D3DSAMP_ADDRESSU, ToD3DAddress(desc.addressU));
	m_pDevice->SetSamplerState(stage, D3DSAMP_ADDRESSV, ToD3DAddress(desc.addressV));
	m_pDevice->SetSamplerState(stage, D3DSAMP_ADDRESSW, ToD3DAddress(desc.addressW));

	m_pDevice->SetSamplerState(stage, D3DSAMP_MAXANISOTROPY, desc.maxAnisotropy);

	m_pDevice->SetSamplerState(stage, D3DSAMP_BORDERCOLOR, desc.borderColor);

	const DWORD bias = static_cast<DWORD>(desc.mipLodBias * 256.0f);
	m_pDevice->SetSamplerState(stage, D3DSAMP_MIPMAPLODBIAS, bias);
}
////////////////////////////////////////////////////////////////////////////////
