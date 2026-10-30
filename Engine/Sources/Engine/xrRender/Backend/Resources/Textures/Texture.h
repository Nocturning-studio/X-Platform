////////////////////////////////////////////////////////////////////////////////
// Author: NS_Deathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrRender/Backend/Resources/SharedResource.h>
#include <xrRender/Backend/Resources/ResourceState.h>
////////////////////////////////////////////////////////////////////////////////
class XRRENDER_API CTexture : public CRefCountedResource
{
  public:
	CTexture() = default;
	~CTexture() override;

	CTexture(const CTexture&) = delete;
	CTexture& operator=(const CTexture&) = delete;

	bool Create(IRenderBackend& rhi, const RHI_TextureDesc& desc);

	const RHI_TextureDesc& GetDesc() const { return m_desc; }

	RHI_TextureHandle GetRHIHandle() const { return m_rhiHandle; }

	RHI_RenderTargetView CreateRTV(uint32_t mip = 0, uint32_t face = 0) const;
	RHI_DepthStencilView CreateDSV(uint32_t mip = 0, uint32_t face = 0) const;

	void Bind(IRenderBackend& rhi, uint32_t slot) const;

	void Transition(EResourceState /*state*/) {}

  private:
	void DestroyRHI();

	RHI_TextureDesc m_desc;
	IRenderBackend* m_rhi = nullptr;
	RHI_TextureHandle m_rhiHandle{};
};
////////////////////////////////////////////////////////////////////////////////
using ref_texture = CSharedPtr<CTexture>;
////////////////////////////////////////////////////////////////////////////////
