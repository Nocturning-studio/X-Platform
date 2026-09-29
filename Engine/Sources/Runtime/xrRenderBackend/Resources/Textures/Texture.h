////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrRenderBackend/Resources/DeviceResource.h>
#include <xrRenderBackend/Resources/ResourceState.h>
#include "TextureDesc.h"
#include "Surface.h"
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CTexture : public CDeviceResource
{
  public:
	virtual ~CTexture() = default;

	const CTextureDesc& GetDesc() const { return m_desc; }

	virtual IDirect3DBaseTexture9* GetD3D9Texture() const = 0;
	virtual IDirect3DSurface9* GetD3D9Surface(uint32_t mip, uint32_t face) const = 0;

	// virtual ID3D12Resource* GetNativeResource() const = 0;
	// virtual uint32_t GetSRVSlot() const = 0;

	virtual void Bind(IDirect3DDevice9Ex* device, uint32_t slot) const = 0;
	virtual void Transition(EResourceState /*state*/) {}

	virtual CSurface CreateRenderTargetView(uint32_t mip = 0, uint32_t face = 0) const = 0;
	virtual CSurface CreateDepthStencilView(uint32_t mip = 0, uint32_t face = 0) const = 0;

	// --- Device lost / reset ---
	virtual void OnDeviceLost() = 0;
	virtual HRESULT OnDeviceReset(IDirect3DDevice9Ex* device) = 0;

  protected:
	CTextureDesc m_desc;
};

////////////////////////////////////////////////////////////////////////////////
class CTexture;
using ref_texture = CSharedPtr<CTexture>;
////////////////////////////////////////////////////////////////////////////////
