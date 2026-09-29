////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "Texture.h"
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CD3D9Texture : public CTexture
{
public:
	CD3D9Texture() = default;
	~CD3D9Texture() override;

	HRESULT Create(IDirect3DDevice9Ex * device, const CTextureDesc & desc);

	IDirect3DBaseTexture9* GetD3D9Texture() const override { return m_texture; }
	IDirect3DSurface9* GetD3D9Surface(uint32_t mip, uint32_t face) const override;

	void Bind(IDirect3DDevice9Ex* device, uint32_t slot) const override;
	CSurface CreateRenderTargetView(uint32_t mip, uint32_t face) const override;
	CSurface CreateDepthStencilView(uint32_t mip, uint32_t face) const override;

	void    OnDeviceLost() override;
	HRESULT OnDeviceReset(IDirect3DDevice9Ex * device) override;

private:
	void ReleaseSurfaces();
	uint32_t  FlatIndex(uint32_t mip, uint32_t face) const;

private:
	IDirect3DBaseTexture9* m_texture = nullptr;

	mutable xr_vector<IDirect3DSurface9*> m_surfaces;

	uint32_t m_faces = 1;
};
////////////////////////////////////////////////////////////////////////////////
