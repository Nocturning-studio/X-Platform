#include "pch.h"
#include "xrBackendDX9.h"

RHI_BEGIN

D3DFORMAT RHIToD3DFormat(RHI_Format fmt)
{
	switch (fmt)
	{
	case RHI_Format::RGBA8_UNORM:	return D3DFMT_A8R8G8B8;
	case RHI_Format::A8_UNORM:		return D3DFMT_A8;
	case RHI_Format::R8_UNORM:		return D3DFMT_L8;
	case RHI_Format::RGBA16_FLOAT:	return D3DFMT_A16B16G16R16F;
	case RHI_Format::RG16_FLOAT:	return D3DFMT_G16R16F;
	case RHI_Format::R16_FLOAT:		return D3DFMT_R16F;
	case RHI_Format::D16_UNORM:		return D3DFMT_D16;
	case RHI_Format::D24_UNORM_S8_UINT:	return D3DFMT_D24S8;
	case RHI_Format::D32_FLOAT:		return D3DFMT_D32F_LOCKABLE;
	case RHI_Format::D15S1:			return D3DFMT_D15S1;
	case RHI_Format::D24X8:			return D3DFMT_D24X8;
	case RHI_Format::D32_LOCKABLE:	return D3DFMT_D32;
	case RHI_Format::D24S8_Shadow:	return (D3DFORMAT)MAKEFOURCC('I', 'N', 'T', 'Z');
	case RHI_Format::D16_Shadow:	return (D3DFORMAT)MAKEFOURCC('D', 'F', '1', '6');
	case RHI_Format::D24X4S4:		return D3DFMT_D24X4S4;
    case RHI_Format::NULLRT:        return (D3DFORMAT)MAKEFOURCC('N', 'U', 'L', 'L');
	default:						return D3DFMT_UNKNOWN;
	}
}

RHI_Format D3DFormatToRHI(D3DFORMAT fmt)
{
    switch (fmt)
    {
    // Стандартные форматы бэкбуфера/текстур
    case D3DFMT_A8R8G8B8:       return RHI_Format::RGBA8_UNORM;
    case D3DFMT_X8R8G8B8:       return RHI_Format::RGBA8_UNORM;
    case D3DFMT_R5G6B5:         return RHI_Format::Unknown;
    case D3DFMT_X1R5G5B5:       return RHI_Format::Unknown;
    case D3DFMT_A1R5G5B5:       return RHI_Format::Unknown;
    case D3DFMT_A4R4G4B4:       return RHI_Format::Unknown;
    case D3DFMT_R3G3B2:         return RHI_Format::Unknown;
    case D3DFMT_A8:             return RHI_Format::A8_UNORM;
    case D3DFMT_A8R3G3B2:       return RHI_Format::Unknown;
    case D3DFMT_X4R4G4B4:       return RHI_Format::Unknown;
    case D3DFMT_A2B10G10R10:    return RHI_Format::Unknown;
    case D3DFMT_A8B8G8R8:       return RHI_Format::Unknown;
    case D3DFMT_G16R16:         return RHI_Format::RG16_FLOAT;
    case D3DFMT_A16B16G16R16:   return RHI_Format::RGBA16_FLOAT;
    case D3DFMT_A16B16G16R16F:  return RHI_Format::RGBA16_FLOAT;
    case D3DFMT_G16R16F:        return RHI_Format::RG16_FLOAT;
    case D3DFMT_R16F:           return RHI_Format::R16_FLOAT;
    case D3DFMT_R32F:           return RHI_Format::Unknown;
    case D3DFMT_A32B32G32R32F:  return RHI_Format::Unknown;

    // Luminance/Alpha (устаревшие)
    case D3DFMT_L8:             return RHI_Format::R8_UNORM;
    case D3DFMT_A8L8:           return RHI_Format::Unknown;
    case D3DFMT_A4L4:           return RHI_Format::Unknown;

    // Depth/Stencil
    case D3DFMT_D16:            return RHI_Format::D16_UNORM;
    case D3DFMT_D24S8:          return RHI_Format::D24_UNORM_S8_UINT;
    case D3DFMT_D24X8:          return RHI_Format::D24X8;
    case D3DFMT_D24X4S4:        return RHI_Format::D24X4S4;
    case D3DFMT_D32:            return RHI_Format::D32_LOCKABLE;
    case D3DFMT_D32F_LOCKABLE:  return RHI_Format::D32_FLOAT;
    case D3DFMT_D15S1:          return RHI_Format::D15S1;
    case D3DFMT_D16_LOCKABLE:   return RHI_Format::Unknown;

    // FourCC форматы (теневые карты)
    case MAKEFOURCC('I', 'N', 'T', 'Z'): return RHI_Format::D24S8_Shadow;
    case MAKEFOURCC('D', 'F', '1', '6'): return RHI_Format::D16_Shadow;
    case MAKEFOURCC('D', 'F', '2', '4'): return RHI_Format::Unknown;

    // Сжатые форматы
    case D3DFMT_DXT1:           return RHI_Format::Unknown;
    case D3DFMT_DXT2:           return RHI_Format::Unknown;
    case D3DFMT_DXT3:           return RHI_Format::Unknown;
    case D3DFMT_DXT4:           return RHI_Format::Unknown;
    case D3DFMT_DXT5:           return RHI_Format::Unknown;

    // Прочие
    case D3DFMT_UYVY:           return RHI_Format::Unknown;
    case D3DFMT_YUY2:           return RHI_Format::Unknown;
    case D3DFMT_MULTI2_ARGB8:   return RHI_Format::Unknown;

    // Неизвестный формат
    case D3DFMT_UNKNOWN:
    default:                    return RHI_Format::Unknown;
    }
}

D3DTEXTUREADDRESS RHIAddressToD3D(RHI_TextureAddress addr) {
	switch (addr) {
	case RHI_TextureAddress::Wrap:       return D3DTADDRESS_WRAP;
	case RHI_TextureAddress::Mirror:     return D3DTADDRESS_MIRROR;
	case RHI_TextureAddress::Clamp:      return D3DTADDRESS_CLAMP;
	case RHI_TextureAddress::Border:     return D3DTADDRESS_BORDER;
	case RHI_TextureAddress::MirrorOnce: return D3DTADDRESS_MIRRORONCE;
	default:                             return D3DTADDRESS_WRAP;
	}
}

D3DTEXTUREFILTERTYPE RHIFilterToD3D(RHI_Filter f)
{
	switch (f)
	{
	case RHI_Filter::Point:         return D3DTEXF_POINT;
	case RHI_Filter::Linear:        return D3DTEXF_LINEAR;
	case RHI_Filter::Anisotropic:   return D3DTEXF_ANISOTROPIC;
	case RHI_Filter::PyramidalQuad: return D3DTEXF_PYRAMIDALQUAD;
	case RHI_Filter::GaussianQuad:  return D3DTEXF_GAUSSIANQUAD;
	default:                        return D3DTEXF_POINT;
	}
}

size_t GetPixelSize(RHI_Format fmt) {
	switch (fmt) {
	case RHI_Format::RGBA8_UNORM:  return 4;
	case RHI_Format::A8_UNORM:     return 1;
	case RHI_Format::R8_UNORM:     return 1;
	case RHI_Format::RGBA16_FLOAT: return 8;
	case RHI_Format::RG16_FLOAT:   return 4;
	case RHI_Format::R16_FLOAT:    return 2;
	default:                       return 0;
	}
}

RHI_END
