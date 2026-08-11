#include "stdafx.h"
#include "EffectorsManager.h"

CEffectorsManager::CEffectorsManager()
{
	// Инициализация значений по умолчанию
	param_blur = 0.f;
	param_gray = 0.f;
	param_noise = 0.f;
	param_duality_h = 0.f;
	param_duality_v = 0.f;
	param_noise_fps = 25.f;
	param_noise_scale = 1.f;

	im_noise_time = 1.0f / 100.0f;
	im_noise_shift_w = 0;
	im_noise_shift_h = 0;

	param_color_base = color_rgba(127, 127, 127, 0);
	param_color_gray = color_rgba(85, 85, 85, 0);
	param_color_add = color_rgba(0, 0, 0, 0);

	param_color_map_influence = 0.0f;
	param_color_map_interpolate = 0.0f;

	param_radiation_intensity = 0.0f;
}

CEffectorsManager::~CEffectorsManager()
{
}
