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
	IDirect3DVolumeTexture9* tex3D = nullptr;
	IDirect3DSurface9* surface = nullptr;

	RHI_Format format = RHI_Format::Unknown;
	RHI_TextureDim dim = RHI_TextureDim::Tex2D;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t depth = 1;
	uint32_t mipLevels = 1;
	uint32_t arraySize = 1;
	uint32_t sampleCount = 1;
	uint32_t usage = RHI_TexUsage_None;

	bool isRenderTarget = false;
	bool isDepthStencil = false;

	IDirect3DBaseTexture9* GetBase() const
	{
		if(tex2D)
			return tex2D;
		if(texCube)
			return texCube;
		if(tex3D)
			return tex3D;
		return nullptr;
	}
};

struct DX9Buffer
{
	IDirect3DVertexBuffer9* vb = nullptr;
	IDirect3DIndexBuffer9* ib = nullptr;

	RHI_BufferDesc desc{};
	D3DPOOL pool = D3DPOOL_DEFAULT;
	bool locked = false;
	bool isIndex = false;
};

struct DX9InputLayout
{
	IDirect3DVertexDeclaration9* decl = nullptr;
	RHI_InputLayoutDesc desc;
};

struct DX9Shader
{
	IDirect3DVertexShader9* vs = nullptr;
	IDirect3DPixelShader9* ps = nullptr;
	bool isVertex = false;
};

D3DFORMAT RHIToD3DFormat(RHI_Format fmt);
D3DTEXTUREADDRESS RHIAddressToD3D(RHI_TextureAddress addr);
D3DTEXTUREFILTERTYPE RHIFilterToD3D(RHI_Filter f);
size_t GetPixelSize(RHI_Format fmt);

RHI_Format D3DFormatToRHI(D3DFORMAT fmt);
////////////////////////////////////////////////////////////////////////////////
