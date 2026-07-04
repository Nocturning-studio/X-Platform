#pragma once

class ENGINE_API CEnvDescriptor
{
  public:
	float exec_time;
	float exec_time_loaded;

	shared_str sky_texture_name;
	shared_str sky_texture_irradiance_name;
	shared_str clouds_texture_name;
	shared_str lut_texture_name;

	ref_texture sky_texture;
	ref_texture sky_texture_irradiance;
	ref_texture clouds_texture;
	ref_texture lut_texture;

	fvec4 clouds_color;
	fvec3 sky_color;
	float sky_rotation;

	float far_plane;

	fvec3 fog_color;
	float fog_density;
	float fog_sky_influence;
	float vertical_fog_intensity;
	float vertical_fog_density;
	float vertical_fog_height;

	float rain_density;
	fvec3 rain_color;

	float bolt_period;
	float bolt_duration;

	float wind_turbulence;
	float wind_strength;
	float wind_direction;
	fvec3 wind_direction3D;
	float wind_gusting;
	float wind_velocity;
	float wind_tilt;

	fvec3 ambient;
	float ambient_brightness;
	fvec4 hemi_color; // w = R2 correction
	fvec3 sun_color;
	fvec3 sun_dir;
	float m_fSunShaftsIntensity;
	float m_fWaterIntensity;

	shared_str lens_flare_id;
	shared_str tb_id;

	// SkyLoader: trees wave
	float m_fTreeAmplitude;
	float m_fTreeSpeed;
	float m_fTreeRotation;
	fvec3 m_fTreeWave;

	// NSDeathman(Очередное ЧСВ в коде указало свой ник): Кинематографические инструменты
	fvec3 m_SepiaColor;
	float m_SepiaPower;
	float m_VignettePower;

	CEnvAmbient* env_ambient;

	CEnvDescriptor(shared_str const& identifier);

	float GetFloatIfExist(LPCSTR line_name, float default_value, CInifile& config);
	fvec3 GetRGBColorIfExist(LPCSTR line_name, fvec3 default_value, CInifile& config);
	fvec4 GetRGBAColorIfExist(LPCSTR line_name, fvec4 default_value, CInifile& config);
	LPCSTR GetStringIfExist(LPCSTR line_name, LPCSTR default_value, CInifile& config);

	void load(CEnvironment& environment, CInifile& config);
	void copy(const CEnvDescriptor& src)
	{
		float tm0 = exec_time;
		float tm1 = exec_time_loaded;
		*this = src;
		exec_time = tm0;
		exec_time_loaded = tm1;
	}

	void on_device_create();
	void on_device_destroy();

	shared_str m_identifier;
};