////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
struct RHIDeviceCaps
{
	// ---- Идентификация адаптера ----
	uint32_t VendorId = 0;
	uint32_t DeviceId = 0;
	std::string Description; // например "AMD Radeon RX 6800"

	// ---- Текущий режим рабочего стола (при создании устройства) ----
	uint32_t DisplayWidth = 0;
	uint32_t DisplayHeight = 0;
	uint32_t DisplayRefreshRate = 0;
	RHI_Format DisplayFormat = RHI_Format::Unknown;

	// ---- Максимальные размеры ресурсов ----
	uint32_t MaxTextureWidth = 0;
	uint32_t MaxTextureHeight = 0;
	uint32_t MaxVolumeExtent = 0;

	// ---- MRT ----
	uint32_t MaxSimultaneousRTs = 0;

	// ---- Шейдеры ----
	bool HasVertexShader = false;
	bool HasPixelShader = false;
	uint32_t VertexShaderMajor = 0;
	uint32_t VertexShaderMinor = 0;
	uint32_t PixelShaderMajor = 0;
	uint32_t PixelShaderMinor = 0;
	uint32_t MaxVertexShaderConst = 0;

	uint32_t VertexCacheMethod = 0;
	uint32_t VertexCacheSize = 16;

	// ---- Depth/Stencil ----
	bool HasDepthStencil = false;

	// ---- Фильтрация ----
	uint32_t MaxAnisotropy = 1;

	// ---- Текстурные стадии ----
	uint32_t MaxTextureBlendStages = 0;
	uint32_t MaxSimultaneousTextures = 0;

	// ---- Аппаратные возможности ----
	bool HardwareTnL = false;
	bool SupportsPureDevice = false;
	bool SupportsNonPow2Textures = false;
};
////////////////////////////////////////////////////////////////////////////////
