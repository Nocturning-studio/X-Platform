#pragma once
#include "stdafx.h"
#include "ColorMapManager.h"

class CEffectorsManager : public IEffectorsManager
{
  public:
	CEffectorsManager();
	~CEffectorsManager();

	virtual void set_blur(float f)
	{
		param_blur = f;
	}
	virtual void set_gray(float f)
	{
		param_gray = f;
	}
	virtual void set_duality_h(float f)
	{
		param_duality_h = _abs(f);
	}
	virtual void set_duality_v(float f)
	{
		param_duality_v = _abs(f);
	}
	virtual void set_noise(float f)
	{
		param_noise = f;
	}
	virtual void set_noise_scale(float f)
	{
		param_noise_scale = f;
	}
	virtual void set_noise_fps(float f)
	{
		param_noise_fps = _abs(f) + EPS_S;
	}
	virtual void set_color_base(u32 f)
	{
		param_color_base = f;
	}
	virtual void set_color_gray(u32 f)
	{
		param_color_gray = f;
	}
	virtual void set_color_add(u32 f)
	{
		param_color_add = f;
	}
	virtual void set_cm_imfluence(float f)
	{
		param_color_map_influence = f;
	}
	virtual void set_cm_interpolate(float f)
	{
		param_color_map_interpolate = f;
	}
	virtual void set_cm_textures(const shared_str& tex0, const shared_str& tex1)
	{
		color_map_manager.SetTextures(tex0, tex1);
	}
	virtual void set_radiation_intensity(float f)
	{
		param_radiation_intensity = f;
	}

	virtual float get_blur()
	{
		return param_blur;
	}
	virtual float get_gray()
	{
		return param_gray;
	}
	virtual float get_duality_h()
	{
		return param_duality_h;
	}
	virtual float get_duality_v()
	{
		return param_duality_v;
	}
	virtual float get_noise()
	{
		return param_noise;
	}
	virtual float get_noise_scale()
	{
		return param_noise_scale;
	}
	virtual float get_noise_fps()
	{
		return param_noise_fps;
	}
	virtual float get_color_base()
	{
		return param_color_base;
	}
	virtual float get_color_gray()
	{
		return param_color_gray;
	}
	virtual float get_color_add()
	{
		return param_color_add;
	}
	virtual float get_cm_imfluence()
	{
		return param_color_map_influence;
	}
	virtual float get_cm_interpolate()
	{
		return param_color_map_interpolate;
	}
	virtual float get_radiation_intensity()
	{
		return param_radiation_intensity;
	}

  private:
	// Параметры постобработки
	float param_blur;
	float param_gray;
	float param_duality_h;
	float param_duality_v;
	float param_noise;
	float param_noise_scale;
	float param_noise_fps;

	u32 param_color_base;
	u32 param_color_gray;
	u32 param_color_add;

	// Параметры шума
	float im_noise_time;
	u32 im_noise_shift_w;
	u32 im_noise_shift_h;

	// Color Mapping
	ColorMapManager color_map_manager;
	float param_color_map_influence;
	float param_color_map_interpolate;

	// Radiation
	float param_radiation_intensity;
};
