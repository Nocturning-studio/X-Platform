////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
struct DX9Texture
{
	IDirect3DTexture9* tex2D = nullptr;
	IDirect3DCubeTexture9* texCube = nullptr;
	IDirect3DSurface9* surface = nullptr;
	RHI_Format format = RHI_Format::Unknown;
	uint32_t width = 0;
	uint32_t height = 0;
	bool isRenderTarget = false;
	bool isDepthStencil = false;
};

D3DFORMAT RHIToD3DFormat(RHI_Format fmt);
D3DTEXTUREADDRESS RHIAddressToD3D(RHI_TextureAddress addr);
D3DTEXTUREFILTERTYPE RHIFilterToD3D(RHI_Filter f);
size_t GetPixelSize(RHI_Format fmt);

RHI_Format D3DFormatToRHI(D3DFORMAT fmt);
////////////////////////////////////////////////////////////////////////////////
