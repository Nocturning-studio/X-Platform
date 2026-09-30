////////////////////////////////////////////////////////////////////////////////
// Created: 30.09.2026 13:16:39
// Author: NS_Deathman
// File: Texture.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "Texture.h"
////////////////////////////////////////////////////////////////////////////////
namespace
{
	// ETextureFormat -> RHI_Format.
	//
	// ВНИМАНИЕ: пространства имён ETextureFormat и RHI_Format разъезжаются по
	// семантике. D3D9-формат A8R8G8B8 (в терминах RHI — RGBA8_UNORM) в
	// ETextureFormat называется B8G8R8A8_UNORM (потому что ETextureFormat
	// именует по байтам в памяти, а не по регистрам).
	//
	// Здесь мы делаем маппинг по СЕМАНТИКЕ (то есть так, как эти форматы
	// используются в шейдерах), а не по порядку байтов. Если у пользователя
	// получится несоответствие — правим имена в одном из enum'ов.
	RHI_Format ToRHIFormat(ETextureFormat fmt)
	{
		switch (fmt)
		{
		case ETextureFormat::Unknown:              return RHI_Format::Unknown;

			// --- Color ---
		case ETextureFormat::R8G8B8A8_UNORM:       return RHI_Format::RGBA8_UNORM;
		case ETextureFormat::R8G8B8A8_UNORM_SRGB:  return RHI_Format::RGBA8_UNORM; // FIXME: sRGB-варианта нет в RHI
		case ETextureFormat::B8G8R8A8_UNORM:       return RHI_Format::RGBA8_UNORM;
		case ETextureFormat::B8G8R8A8_UNORM_SRGB:  return RHI_Format::RGBA8_UNORM; // FIXME: sRGB-варианта нет в RHI

		case ETextureFormat::R10G10B10A2_UNORM:    return RHI_Format::Unknown;      // FIXME: добавить в RHI
		case ETextureFormat::R16G16B16A16_FLOAT:   return RHI_Format::RGBA16_FLOAT;
		case ETextureFormat::R16G16B16A16_UNORM:   return RHI_Format::Unknown;      // FIXME: добавить в RHI
		case ETextureFormat::R32G32B32A32_FLOAT:   return RHI_Format::Unknown;      // FIXME: добавить в RHI

		case ETextureFormat::R32_FLOAT:            return RHI_Format::Unknown;      // FIXME: добавить в RHI
		case ETextureFormat::R16_FLOAT:            return RHI_Format::R16_FLOAT;
		case ETextureFormat::R8_UNORM:             return RHI_Format::R8_UNORM;

			// --- Depth/Stencil ---
		case ETextureFormat::D24_UNORM_S8_UINT:    return RHI_Format::D24_UNORM_S8_UINT;
		case ETextureFormat::D32_FLOAT:            return RHI_Format::D32_FLOAT;
		case ETextureFormat::D16_UNORM:            return RHI_Format::D16_UNORM;

			// --- Compressed ---
			// FIXME: в RHI нет BC-форматов. Добавить, когда будет загрузчик сжатых
			// текстур. Пока возвращаем Unknown — на этих форматах создание текстуры
			// завершится провалом.
		case ETextureFormat::BC1_UNORM:            return RHI_Format::Unknown;
		case ETextureFormat::BC3_UNORM:            return RHI_Format::Unknown;
		case ETextureFormat::BC5_UNORM:            return RHI_Format::Unknown;
		}
		return RHI_Format::Unknown;
	}

	// CTextureDesc -> RHI_TextureDesc.
	RHI_TextureDesc ToRHITextureDesc(const CTextureDesc& desc, RHI_Format fmt)
	{
		RHI_TextureDesc r{};
		r.width = desc.width;
		r.height = desc.height;
		r.depth = desc.depth;
		r.mipLevels = desc.mips;
		r.format = fmt;
		r.isRenderTarget = (desc.usage & TexUsage_RenderTarget) != 0;
		r.isDepthStencil = (desc.usage & TexUsage_DepthStencil) != 0;
		r.isCubeMap = (desc.dim == ETextureDim::Cube);
		return r;
	}

} // namespace

////////////////////////////////////////////////////////////////////////////////

CTexture::~CTexture()
{
	DestroyRHI();
}

bool CTexture::Create(IRenderBackend& rhi, const CTextureDesc& desc)
{
	// На случай пересоздания — подчистить старый хэндл.
	DestroyRHI();

	const RHI_Format fmt = ToRHIFormat(desc.format);
	if (fmt == RHI_Format::Unknown)
	{
		Msg("! [CTexture] Unsupported format %u for '%s'",
			uint32_t(desc.format),
			desc.debugName ? desc.debugName : "<unnamed>");
		return false;
	}

	m_desc = desc;
	m_rhi = &rhi;

	const RHI_TextureDesc rhiDesc = ToRHITextureDesc(desc, fmt);
	m_rhiHandle = rhi.CreateTexture(rhiDesc, nullptr);

	if (!m_rhiHandle.IsValid())
	{
		Msg("! [CTexture] CreateTexture failed for '%s' (%ux%u, fmt=%u)",
			desc.debugName ? desc.debugName : "<unnamed>",
			desc.width, desc.height, uint32_t(desc.format));
		m_rhi = nullptr;
		return false;
	}

	return true;
}

RHI_RenderTargetView CTexture::CreateRTV(uint32_t mip, uint32_t face) const
{
	if (!m_rhi || !m_rhiHandle.IsValid())
		return RHI_RenderTargetView{};

	if (!(m_desc.usage & TexUsage_RenderTarget))
	{
		Msg("! [CTexture] CreateRTV: texture '%s' was not created as render target",
			m_desc.debugName ? m_desc.debugName : "<unnamed>");
		return RHI_RenderTargetView{};
	}

	return m_rhi->CreateRTV(m_rhiHandle, mip, face);
}

RHI_DepthStencilView CTexture::CreateDSV(uint32_t mip, uint32_t face) const
{
	if (!m_rhi || !m_rhiHandle.IsValid())
		return RHI_DepthStencilView{};

	if (!(m_desc.usage & TexUsage_DepthStencil))
	{
		Msg("! [CTexture] CreateDSV: texture '%s' was not created as depth/stencil",
			m_desc.debugName ? m_desc.debugName : "<unnamed>");
		return RHI_DepthStencilView{};
	}

	return m_rhi->CreateDSV(m_rhiHandle, mip, face);
}

void CTexture::Bind(IRenderBackend& rhi, uint32_t slot) const
{
	if (!m_rhiHandle.IsValid())
	{
		rhi.SetShaderResource(slot, RHI_TextureHandle{});
		return;
	}
	rhi.SetShaderResource(slot, m_rhiHandle);
}

void CTexture::DestroyRHI()
{
	if (m_rhi && m_rhiHandle.IsValid())
	{
		m_rhi->DestroyTexture(m_rhiHandle);
	}
	m_rhiHandle = {};
	m_rhi = nullptr;
}
////////////////////////////////////////////////////////////////////////////////
