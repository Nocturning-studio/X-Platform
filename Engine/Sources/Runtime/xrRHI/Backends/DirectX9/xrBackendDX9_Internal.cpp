////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "xrBackendDX9.h"
////////////////////////////////////////////////////////////////////////////////
D3DFORMAT RHIToD3DFormat(RHI_Format fmt)
{
	switch(fmt)
	{
		// --- Color 8-bit ---
	case RHI_Format::RGBA8_UNORM:
	case RHI_Format::RGBA8_UNORM_SRGB: // sRGB — это sampler state, формат тот же
		return D3DFMT_A8R8G8B8;
	case RHI_Format::A8_UNORM:
		return D3DFMT_A8;
	case RHI_Format::R8_UNORM:
		return D3DFMT_L8;

		// --- Color 10-bit ---
	case RHI_Format::R10G10B10A2_UNORM:
		return D3DFMT_A2B10G10R10;

		// --- Color half-float ---
	case RHI_Format::RGBA16_FLOAT:
		return D3DFMT_A16B16G16R16F;
	case RHI_Format::RG16_FLOAT:
		return D3DFMT_G16R16F;
	case RHI_Format::R16_FLOAT:
		return D3DFMT_R16F;

		// --- Color 16-bit UNORM ---
	case RHI_Format::R16G16B16A16_UNORM:
		return D3DFMT_A16B16G16R16;

		// --- Color 32-bit float ---
	case RHI_Format::R32G32B32A32_FLOAT:
		return D3DFMT_A32B32G32R32F;
	case RHI_Format::R32_FLOAT:
		return D3DFMT_R32F;

		// --- Depth/Stencil ---
	case RHI_Format::D16_UNORM:
		return D3DFMT_D16;
	case RHI_Format::D24_UNORM_S8_UINT:
		return D3DFMT_D24S8;
	case RHI_Format::D32_FLOAT:
		return D3DFMT_D32F_LOCKABLE;

		// --- Legacy D3D9-specific ---
	case RHI_Format::D15S1:
		return D3DFMT_D15S1;
	case RHI_Format::D24X8:
		return D3DFMT_D24X8;
	case RHI_Format::D32_LOCKABLE:
		return D3DFMT_D32;

		// --- Vendor-specific ---
	case RHI_Format::D24S8_Shadow:
		return (D3DFORMAT)MAKEFOURCC('I', 'N', 'T', 'Z');
	case RHI_Format::D16_Shadow:
		return (D3DFORMAT)MAKEFOURCC('D', 'F', '1', '6');
	case RHI_Format::D24X4S4:
		return D3DFMT_D24X4S4;

		// --- Compressed ---
	case RHI_Format::BC1_UNORM:
		return D3DFMT_DXT1;
	case RHI_Format::BC3_UNORM:
		return D3DFMT_DXT5;
	case RHI_Format::BC5_UNORM:
		return (D3DFORMAT)MAKEFOURCC('A', 'T', 'I', '2');

	case RHI_Format::NULLRT:
		return (D3DFORMAT)MAKEFOURCC('N', 'U', 'L', 'L');

	default:
		return D3DFMT_UNKNOWN;
	}
}

RHI_Format D3DFormatToRHI(D3DFORMAT fmt)
{
	switch(fmt)
	{
		// --- Color 8-bit ---
	case D3DFMT_A8R8G8B8:
	case D3DFMT_X8R8G8B8:
		return RHI_Format::RGBA8_UNORM;
	case D3DFMT_A8B8G8R8:
	case D3DFMT_X8B8G8R8:
		return RHI_Format::RGBA8_UNORM;
	case D3DFMT_A8:
		return RHI_Format::A8_UNORM;
	case D3DFMT_L8:
		return RHI_Format::R8_UNORM;

		// --- Color 10-bit ---
	case D3DFMT_A2B10G10R10:
	case D3DFMT_A2R10G10B10:
		return RHI_Format::R10G10B10A2_UNORM;

		// --- Color half-float ---
	case D3DFMT_A16B16G16R16F:
		return RHI_Format::RGBA16_FLOAT;
	case D3DFMT_G16R16F:
		return RHI_Format::RG16_FLOAT;
	case D3DFMT_R16F:
		return RHI_Format::R16_FLOAT;

		// --- Color 16-bit UNORM ---
	case D3DFMT_A16B16G16R16:
		return RHI_Format::R16G16B16A16_UNORM;
	case D3DFMT_G16R16:
		return RHI_Format::RG16_FLOAT; // ближайшее

	// --- Color 32-bit float ---
	case D3DFMT_A32B32G32R32F:
		return RHI_Format::R32G32B32A32_FLOAT;
	case D3DFMT_R32F:
		return RHI_Format::R32_FLOAT;

		// --- Depth/Stencil ---
	case D3DFMT_D16:
		return RHI_Format::D16_UNORM;
	case D3DFMT_D24S8:
		return RHI_Format::D24_UNORM_S8_UINT;
	case D3DFMT_D32F_LOCKABLE:
		return RHI_Format::D32_FLOAT;

	case D3DFMT_D15S1:
		return RHI_Format::D15S1;
	case D3DFMT_D24X8:
		return RHI_Format::D24X8;
	case D3DFMT_D32:
		return RHI_Format::D32_LOCKABLE;
	case D3DFMT_D24X4S4:
		return RHI_Format::D24X4S4;

		// --- Vendor-specific ---
	case MAKEFOURCC('I', 'N', 'T', 'Z'):
		return RHI_Format::D24S8_Shadow;
	case MAKEFOURCC('D', 'F', '1', '6'):
		return RHI_Format::D16_Shadow;

		// --- Compressed ---
	case D3DFMT_DXT1:
		return RHI_Format::BC1_UNORM;
	case D3DFMT_DXT5:
		return RHI_Format::BC3_UNORM;

	default:
		if((DWORD)fmt == MAKEFOURCC('A', 'T', 'I', '2'))
			return RHI_Format::BC5_UNORM;
		return RHI_Format::Unknown;
	}
}

D3DTEXTUREADDRESS RHIAddressToD3D(RHI_TextureAddress addr)
{
	switch(addr)
	{
	case RHI_TextureAddress::Wrap:
		return D3DTADDRESS_WRAP;
	case RHI_TextureAddress::Mirror:
		return D3DTADDRESS_MIRROR;
	case RHI_TextureAddress::Clamp:
		return D3DTADDRESS_CLAMP;
	case RHI_TextureAddress::Border:
		return D3DTADDRESS_BORDER;
	case RHI_TextureAddress::MirrorOnce:
		return D3DTADDRESS_MIRRORONCE;
	default:
		return D3DTADDRESS_WRAP;
	}
}

D3DTEXTUREFILTERTYPE RHIFilterToD3D(RHI_Filter f)
{
	switch(f)
	{
	case RHI_Filter::Point:
		return D3DTEXF_POINT;
	case RHI_Filter::Linear:
		return D3DTEXF_LINEAR;
	case RHI_Filter::Anisotropic:
		return D3DTEXF_ANISOTROPIC;
	case RHI_Filter::PyramidalQuad:
		return D3DTEXF_PYRAMIDALQUAD;
	case RHI_Filter::GaussianQuad:
		return D3DTEXF_GAUSSIANQUAD;
	default:
		return D3DTEXF_POINT;
	}
}

size_t GetPixelSize(RHI_Format fmt)
{
	switch(fmt)
	{
	case RHI_Format::RGBA8_UNORM:
	case RHI_Format::RGBA8_UNORM_SRGB:
	case RHI_Format::R10G10B10A2_UNORM:
	case RHI_Format::R32_FLOAT:
		return 4;
	case RHI_Format::A8_UNORM:
	case RHI_Format::R8_UNORM:
		return 1;

	case RHI_Format::RGBA16_FLOAT:
	case RHI_Format::R16G16B16A16_UNORM:
		return 8;
	case RHI_Format::RG16_FLOAT:
		return 4;
	case RHI_Format::R16_FLOAT:
		return 2;

	case RHI_Format::R32G32B32A32_FLOAT:
		return 16;

	default:
		return 0;
	}
}

BYTE ToD3DDeclType(RHI_VertexElementType t)
{
	switch (t)
	{
	case RHI_VertexElementType::Float1:
		return D3DDECLTYPE_FLOAT1;
	case RHI_VertexElementType::Float2:
		return D3DDECLTYPE_FLOAT2;
	case RHI_VertexElementType::Float3:
		return D3DDECLTYPE_FLOAT3;
	case RHI_VertexElementType::Float4:
		return D3DDECLTYPE_FLOAT4;
	case RHI_VertexElementType::UByte4:
		return D3DDECLTYPE_UBYTE4;
	case RHI_VertexElementType::UByte4N:
		return D3DDECLTYPE_UBYTE4N;
	case RHI_VertexElementType::Short2:
		return D3DDECLTYPE_SHORT2;
	case RHI_VertexElementType::Short2N:
		return D3DDECLTYPE_SHORT2N;
	case RHI_VertexElementType::Short4:
		return D3DDECLTYPE_SHORT4;
	case RHI_VertexElementType::Short4N:
		return D3DDECLTYPE_SHORT4N;
		// UInt1..4 не имеют аналога в D3D9 — это задел под DX10+.
	default:
		return D3DDECLTYPE_UNUSED;
	}
}

BYTE ToD3DDeclUsage(RHI_VertexElementSemantic s)
{
	switch (s)
	{
	case RHI_VertexElementSemantic::Position:
		return D3DDECLUSAGE_POSITION;
	case RHI_VertexElementSemantic::Normal:
		return D3DDECLUSAGE_NORMAL;
	case RHI_VertexElementSemantic::Tangent:
		return D3DDECLUSAGE_TANGENT;
	case RHI_VertexElementSemantic::Binormal:
		return D3DDECLUSAGE_BINORMAL;
	case RHI_VertexElementSemantic::TexCoord:
		return D3DDECLUSAGE_TEXCOORD;
	case RHI_VertexElementSemantic::Color:
		return D3DDECLUSAGE_COLOR;
	case RHI_VertexElementSemantic::BlendWeight:
		return D3DDECLUSAGE_BLENDWEIGHT;
	case RHI_VertexElementSemantic::BlendIndices:
		return D3DDECLUSAGE_BLENDINDICES;
	case RHI_VertexElementSemantic::PositionT:
		return D3DDECLUSAGE_POSITIONT;
	default:
		return D3DDECLUSAGE_POSITION;
	}
}

D3DPRIMITIVETYPE ToD3DPrimitive(RHI_Topology t)
{
	switch (t)
	{
	case RHI_Topology::PointList:
		return D3DPT_POINTLIST;
	case RHI_Topology::LineList:
		return D3DPT_LINELIST;
	case RHI_Topology::LineStrip:
		return D3DPT_LINESTRIP;
	case RHI_Topology::TriangleList:
		return D3DPT_TRIANGLELIST;
	case RHI_Topology::TriangleStrip:
		return D3DPT_TRIANGLESTRIP;
	case RHI_Topology::TriangleFan:
		return D3DPT_TRIANGLEFAN;
	}
	return D3DPT_TRIANGLELIST;
}
////////////////////////////////////////////////////////////////////////////////
