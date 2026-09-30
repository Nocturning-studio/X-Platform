////////////////////////////////////////////////////////////////////////////////
// Created: 30.09.2026 13:16:39
// Author: NS_Deathman
// File: Texture.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "Texture.h"
////////////////////////////////////////////////////////////////////////////////
CTexture::~CTexture()
{
	DestroyRHI();
}

bool CTexture::Create(IRenderBackend& rhi, const RHI_TextureDesc& desc)
{
	DestroyRHI();

	m_desc = desc;
	m_rhi = &rhi;

	m_rhiHandle = rhi.CreateTexture(desc, nullptr);
	if (!m_rhiHandle.IsValid())
	{
		Msg("! [CTexture] CreateTexture failed for '%s' (%ux%u, fmt=%u)", desc.debugName ? desc.debugName : "<unnamed>",
																		  desc.width, 
																		  desc.height, 
																		  (uint32_t)desc.format);
		m_rhi = nullptr;
		return false;
	}
	return true;
}

RHI_RenderTargetView CTexture::CreateRTV(uint32_t mip, uint32_t face) const
{
	if (!m_rhi || !m_rhiHandle.IsValid()) return {};
	if (!(m_desc.usage & RHI_TexUsage_RenderTarget))
	{
		Msg("! [CTexture] CreateRTV: '%s' is not a render target", m_desc.debugName ? m_desc.debugName : "<unnamed>");
		return {};
	}
	return m_rhi->CreateRTV(m_rhiHandle, mip, face);
}

RHI_DepthStencilView CTexture::CreateDSV(uint32_t mip, uint32_t face) const
{
	if (!m_rhi || !m_rhiHandle.IsValid()) return {};
	if (!(m_desc.usage & RHI_TexUsage_DepthStencil))
	{
		Msg("! [CTexture] CreateDSV: '%s' is not depth/stencil", m_desc.debugName ? m_desc.debugName : "<unnamed>");
		return {};
	}
	return m_rhi->CreateDSV(m_rhiHandle, mip, face);
}

void CTexture::Bind(IRenderBackend& rhi, uint32_t slot) const
{
	rhi.SetShaderResource(slot, m_rhiHandle);
}

void CTexture::DestroyRHI()
{
	if (m_rhi && m_rhiHandle.IsValid())
		m_rhi->DestroyTexture(m_rhiHandle);
	m_rhiHandle = {};
	m_rhi = nullptr;
}
////////////////////////////////////////////////////////////////////////////////
