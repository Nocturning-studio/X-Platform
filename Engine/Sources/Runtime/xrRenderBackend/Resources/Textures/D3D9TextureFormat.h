////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "TextureFormat.h"
////////////////////////////////////////////////////////////////////////////////
// Конвертирует прокси-формат движка в нативный D3DFORMAT.
// Возвращает D3DFMT_UNKNOWN, если прямого аналога в D3D9 нет
// (например, BC5_UNORM без расширения ATI/3Dc).
XRRB_API D3DFORMAT ToD3DFormat(ETextureFormat fmt);

// Обратная конверсия. Нужна для диагностики, проверки device caps
// и (в будущем) для RHI-слоя.
XRRB_API ETextureFormat FromD3DFormat(D3DFORMAT fmt);

// SRGB в D3D9 — это НЕ формат, а состояние сэмплера (D3DSAMP_SRGBTEXTURE).
// Формат-флаг ниже говорит "эта текстура должна читаться как sRGB".
inline bool IsSRGBFormat(ETextureFormat fmt)
{
	return fmt == ETextureFormat::R8G8B8A8_UNORM_SRGB || fmt == ETextureFormat::B8G8R8A8_UNORM_SRGB;
}

// Вспомогательное: сообщает, является ли формат depth/stencil.
// Полезно для валидации перед созданием render target.
inline bool IsDepthFormat(ETextureFormat fmt)
{
	switch(fmt)
	{
	case ETextureFormat::D16_UNORM:
	case ETextureFormat::D24_UNORM_S8_UINT:
	case ETextureFormat::D32_FLOAT:
		return true;
	default:
		return false;
	}
}
////////////////////////////////////////////////////////////////////////////////
