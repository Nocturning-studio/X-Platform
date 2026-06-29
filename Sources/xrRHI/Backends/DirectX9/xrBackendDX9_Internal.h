#pragma once
#include "../../xrRHI.h"
#include <d3d9.h>
#include <d3dx9.h>

RHI_BEGIN

struct DX9Texture
{
    IDirect3DTexture9* tex2D = nullptr;
    IDirect3DCubeTexture9* texCube = nullptr;
    IDirect3DSurface9* surface = nullptr;
    RHI_Format format = RHI_Format::Unknown;
    u32 width = 0;
    u32 height = 0;
    bool isRenderTarget = false;
    bool isDepthStencil = false;
};

struct DX9Sampler
{
    SamplerDesc desc;
};

D3DFORMAT RHIToD3DFormat(RHI_Format fmt);
D3DTEXTUREADDRESS RHIAddressToD3D(RHI_TextureAddress addr);
D3DTEXTUREFILTERTYPE RHIFilterToD3D(RHI_Filter f);
size_t GetPixelSize(RHI_Format fmt);

RHI_Format D3DFormatToRHI(D3DFORMAT fmt);

RHI_END
