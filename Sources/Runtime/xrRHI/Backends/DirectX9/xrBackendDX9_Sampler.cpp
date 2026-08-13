#include "pch.h"
#include "xrBackendDX9.h"

RHI_BEGIN

SamplerHandle CRenderBackendDX9::AllocSamplerHandle(DX9Sampler* sampler)
{
	u32 index;
	if (!m_FreeSamplerIndices.empty())
	{
		index = m_FreeSamplerIndices.top();
		m_FreeSamplerIndices.pop();
		m_Samplers[index] = sampler;
	}
	else
	{
		index = static_cast<u32>(m_Samplers.size());
		m_Samplers.push_back(sampler);
	}
	return SamplerHandle{index};
}

DX9Sampler* CRenderBackendDX9::GetSampler(SamplerHandle handle)
{
	if (!handle.IsValid() || handle.id >= m_Samplers.size())
		return nullptr;
	return m_Samplers[handle.id];
}

void CRenderBackendDX9::FreeSamplerHandle(SamplerHandle handle)
{
	if (!handle.IsValid() || handle.id >= m_Samplers.size())
		return;
	DX9Sampler* samp = m_Samplers[handle.id];
	if (!samp)
	{
		Print("! [DX9] Double free of SamplerHandle(id=%u) detected, ignoring.", handle.id);
		return;
	}
	delete samp;
	m_Samplers[handle.id] = nullptr;
	m_FreeSamplerIndices.push(handle.id);
}

SamplerHandle CRenderBackendDX9::CreateSampler(const SamplerDesc& desc)
{
	DX9Sampler* impl = new DX9Sampler;
	impl->desc = desc;
	return AllocSamplerHandle(impl);
}

void CRenderBackendDX9::DestroySampler(SamplerHandle handle)
{
	DX9Sampler* samp = GetSampler(handle);
	if (!samp)
	{
		Print("! [DX9] DestroySampler: invalid or already destroyed handle (id=%u).", handle.id);
		return;
	}
	FreeSamplerHandle(handle);
}

void CRenderBackendDX9::ApplySampler(u32 slot, const SamplerDesc& desc)
{
	if (!m_pDevice)
		return;

	D3DTEXTUREFILTERTYPE minFilter = RHIFilterToD3D(desc.minFilter);
	D3DTEXTUREFILTERTYPE magFilter = RHIFilterToD3D(desc.magFilter);
	D3DTEXTUREFILTERTYPE mipFilter =
		(desc.mipFilter == RHI_Filter::None) ? D3DTEXF_NONE : RHIFilterToD3D(desc.mipFilter);

	m_pDevice->SetSamplerState(slot, D3DSAMP_MINFILTER, minFilter);
	m_pDevice->SetSamplerState(slot, D3DSAMP_MAGFILTER, magFilter);
	m_pDevice->SetSamplerState(slot, D3DSAMP_MIPFILTER, mipFilter);
	m_pDevice->SetSamplerState(slot, D3DSAMP_ADDRESSU, RHIAddressToD3D(desc.addressU));
	m_pDevice->SetSamplerState(slot, D3DSAMP_ADDRESSV, RHIAddressToD3D(desc.addressV));
	m_pDevice->SetSamplerState(slot, D3DSAMP_ADDRESSW, RHIAddressToD3D(desc.addressW));
	m_pDevice->SetSamplerState(slot, D3DSAMP_MAXANISOTROPY, desc.maxAnisotropy);
	m_pDevice->SetSamplerState(slot, D3DSAMP_MIPMAPLODBIAS, *((LPDWORD)(&desc.mipLODBias)));

	if (desc.addressU == RHI_TextureAddress::Border || desc.addressV == RHI_TextureAddress::Border ||
		desc.addressW == RHI_TextureAddress::Border)
	{
		D3DCOLOR borderColor =
			D3DCOLOR_COLORVALUE(desc.borderColor.x, desc.borderColor.y, desc.borderColor.z, desc.borderColor.w);
		m_pDevice->SetSamplerState(slot, D3DSAMP_BORDERCOLOR, borderColor);
	}
}

void CRenderBackendDX9::ApplyDefaultSampler(u32 slot)
{
	m_pDevice->SetSamplerState(slot, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	m_pDevice->SetSamplerState(slot, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	m_pDevice->SetSamplerState(slot, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	m_pDevice->SetSamplerState(slot, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	m_pDevice->SetSamplerState(slot, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	m_pDevice->SetSamplerState(slot, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
	m_pDevice->SetSamplerState(slot, D3DSAMP_MAXANISOTROPY, 1);
	m_pDevice->SetSamplerState(slot, D3DSAMP_MIPMAPLODBIAS, 0);
}

RHI_END
