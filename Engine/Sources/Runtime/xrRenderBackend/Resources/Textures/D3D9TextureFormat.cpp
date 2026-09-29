////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "D3D9TextureFormat.h"
////////////////////////////////////////////////////////////////////////////////

// Соглашение о порядке именования:
//
//   - DXGI/DX11:    название пишется в порядке БАЙТОВ слева направо,
//                   начиная с младшего адреса в памяти.
//                   R8G8B8A8_UNORM => байты: R, G, B, A.
//
//   - D3D9:         название читается как 32-битный DWORD в порядке
//                   AA-RR-GG-BB для A8R8G8B8 (little-endian на x86).
//                   D3DFMT_A8R8G8B8 => DWORD 0xAARRGGBB => байты: B, G, R, A.
//
// Соответствие:
//   R8G8B8A8_UNORM  <=>  D3DFMT_A8B8G8R8    (байты: R, G, B, A)
//   B8G8R8A8_UNORM  <=>  D3DFMT_A8R8G8B8    (байты: B, G, R, A)

D3DFORMAT ToD3DFormat(ETextureFormat fmt)
{
	switch(fmt)
	{
	case ETextureFormat::Unknown:
		return D3DFMT_UNKNOWN;

		// --- Color ---
	case ETextureFormat::R8G8B8A8_UNORM:
	case ETextureFormat::R8G8B8A8_UNORM_SRGB:
		// SRGB на уровне формата в D3D9 нет — флаг обрабатывается
		// через D3DSAMP_SRGBTEXTURE при биндинге.
		return D3DFMT_A8B8G8R8;

	case ETextureFormat::B8G8R8A8_UNORM:
	case ETextureFormat::B8G8R8A8_UNORM_SRGB:
		return D3DFMT_A8R8G8B8;

	case ETextureFormat::R10G10B10A2_UNORM:
		return D3DFMT_A2B10G10R10;

	case ETextureFormat::R16G16B16A16_FLOAT:
		return D3DFMT_A16B16G16R16F;

	case ETextureFormat::R16G16B16A16_UNORM:
		return D3DFMT_A16B16G16R16;

	case ETextureFormat::R32G32B32A32_FLOAT:
		return D3DFMT_A32B32G32R32F;

	case ETextureFormat::R32_FLOAT:
		return D3DFMT_R32F;

	case ETextureFormat::R16_FLOAT:
		return D3DFMT_R16F;

	case ETextureFormat::R8_UNORM:
		// Ближайший аналог в D3D9 — L8. В шейдере разворачивается
		// в (L, L, L, 1). Если нужен чистый alpha-only — использyй A8,
		// но для R8_UNORM по смыслу именно L8.
		return D3DFMT_L8;

		// --- Depth/Stencil ---
	case ETextureFormat::D24_UNORM_S8_UINT:
		return D3DFMT_D24S8;

	case ETextureFormat::D32_FLOAT:
		// D32F_LOCKABLE работает и как shader resource, и как depth buffer.
		// Обычный D3DFMT_D32 — только depth, без чтения из шейдера.
		return D3DFMT_D32F_LOCKABLE;

	case ETextureFormat::D16_UNORM:
		return D3DFMT_D16;

		// --- Compressed ---
	case ETextureFormat::BC1_UNORM:
		return D3DFMT_DXT1;

	case ETextureFormat::BC3_UNORM:
		return D3DFMT_DXT5;

	case ETextureFormat::BC5_UNORM:
		// В D3D9 нет "официального" BC5. Обычно используется
		// ATI2/3Dc (двухканальный сжатый формат). Работает на GPU
		// с поддержкой ATI_dx10 или NVIDIA, но не гарантирован
		// везде. Если устройство не поддерживает — вернётся
		// D3DERR_INVALIDCALL при Create*.
		return static_cast<D3DFORMAT>(MAKEFOURCC('A', 'T', 'I', '2'));

	default:
		return D3DFMT_UNKNOWN;
	}
}

////////////////////////////////////////////////////////////////////////////////

ETextureFormat FromD3DFormat(D3DFORMAT fmt)
{
	switch(fmt)
	{
	case D3DFMT_UNKNOWN:
		return ETextureFormat::Unknown;

		// --- Color ---
	case D3DFMT_A8B8G8R8:
		return ETextureFormat::R8G8B8A8_UNORM;
	case D3DFMT_A8R8G8B8:
		return ETextureFormat::B8G8R8A8_UNORM;
	case D3DFMT_X8R8G8B8:
		return ETextureFormat::B8G8R8A8_UNORM; // без alpha — мапим в ближайшее
	case D3DFMT_X8B8G8R8:
		return ETextureFormat::R8G8B8A8_UNORM;

	case D3DFMT_A2B10G10R10:
		return ETextureFormat::R10G10B10A2_UNORM;
	case D3DFMT_A2R10G10B10:
		return ETextureFormat::R10G10B10A2_UNORM; // порядок каналов другой, но точность та же

	case D3DFMT_A16B16G16R16F:
		return ETextureFormat::R16G16B16A16_FLOAT;
	case D3DFMT_A16B16G16R16:
		return ETextureFormat::R16G16B16A16_UNORM;
	case D3DFMT_A32B32G32R32F:
		return ETextureFormat::R32G32B32A32_FLOAT;

	case D3DFMT_R32F:
		return ETextureFormat::R32_FLOAT;
	case D3DFMT_R16F:
		return ETextureFormat::R16_FLOAT;
	case D3DFMT_L8:
		return ETextureFormat::R8_UNORM;

		// --- Depth/Stencil ---
	case D3DFMT_D24S8:
		return ETextureFormat::D24_UNORM_S8_UINT;
	case D3DFMT_D32F_LOCKABLE:
		return ETextureFormat::D32_FLOAT;
	case D3DFMT_D16:
		return ETextureFormat::D16_UNORM;

		// --- Compressed ---
	case D3DFMT_DXT1:
		return ETextureFormat::BC1_UNORM;
	case D3DFMT_DXT5:
		return ETextureFormat::BC3_UNORM;

	default:
		break;
	}

	if(static_cast<DWORD>(fmt) == MAKEFOURCC('A', 'T', 'I', '2'))
		return ETextureFormat::BC5_UNORM;

	return ETextureFormat::Unknown;
}
////////////////////////////////////////////////////////////////////////////////
