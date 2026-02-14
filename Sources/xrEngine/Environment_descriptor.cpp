//-----------------------------------------------------------------------------
// Environment descriptor
//-----------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "Environment.h"
#include "xr_efflensflare.h"
#include "thunderbolt.h"

CEnvDescriptor::CEnvDescriptor(shared_str const& identifier) : m_identifier(identifier)
{
	exec_time = 0.0f;
	exec_time_loaded = 0.0f;

	clouds_color.set(1, 1, 1, 1);
	sky_color.set(1, 1, 1);
	sky_rotation = 0.0f;

	far_plane = 400.0f;

	fog_color.set(1, 1, 1);
	fog_density = 0.0f;
	fog_sky_influence = 0.0f;
	vertical_fog_intensity = 0.0001f;
	vertical_fog_density = 0.0001f;
	vertical_fog_height = 0.8f;

	rain_density = 0.0f;
	rain_color.set(0, 0, 0);

	bolt_period = 0.0f;
	bolt_duration = 0.0f;

	wind_turbulence = 0.0f;
	wind_strength = 0.0f;
	wind_direction = 0.0f;
	wind_gusting = 0.0f;

	ambient.set(0, 0, 0);
	hemi_color.set(1, 1, 1, 1);
	sun_color.set(1, 1, 1);
	sun_dir.set(0, -1, 0);

	m_fSunShaftsIntensity = 0;
	m_fWaterIntensity = 1;

	lens_flare_id = "";
	tb_id = "";

	m_fTreeAmplitude = 0.005f;
	m_fTreeSpeed = 1.00f;
	m_fTreeRotation = 10.0f;
	m_fTreeWave.set(.1f, .01f, .11f);

	m_SepiaColor.set(1.0f, 1.0f, 1.0f);
	m_SepiaPower = 0.0f;
	m_VignettePower = 0.2f;

	env_ambient = NULL;
}

#define C_CHECK(C)                                                                                                     \
	if (C.x < 0 || C.x > 2 || C.y < 0 || C.y > 2 || C.z < 0 || C.z > 2)                                                \
	{                                                                                                                  \
		Msg("! Invalid '%s' in env-section '%s'", #C, m_identifier.c_str());                                           \
	}

float CEnvDescriptor::GetFloatIfExist(LPCSTR line_name, float default_value, CInifile& config)
{
	if (config.line_exist(m_identifier.c_str(), line_name))
		return config.r_float(m_identifier.c_str(), line_name);
	else
		return default_value;
}

float3 CEnvDescriptor::GetRGBColorIfExist(LPCSTR line_name, float3 default_value, CInifile& config)
{
	if (config.line_exist(m_identifier.c_str(), line_name))
		return config.r_fvector3(m_identifier.c_str(), line_name);
	else
		return default_value;
}

float4 CEnvDescriptor::GetRGBAColorIfExist(LPCSTR line_name, float4 default_value, CInifile& config)
{
	if (config.line_exist(m_identifier.c_str(), line_name))
		return config.r_fvector4(m_identifier.c_str(), line_name);
	else
		return default_value;
}

LPCSTR CEnvDescriptor::GetStringIfExist(LPCSTR line_name, LPCSTR default_value, CInifile& config)
{
	if (config.line_exist(m_identifier.c_str(), line_name))
		return config.r_string(m_identifier.c_str(), line_name);
	else
		return default_value;
}

void CEnvDescriptor::load(CEnvironment& environment, CInifile& config)
{
	float3 NULL_COLOR;
	NULL_COLOR.set(NULL, NULL, NULL);

	float3 FULL_COLOR;
	FULL_COLOR.set(1.0f, 1.0f, 1.0f);

	float4 FULL_COLOR_RGBA;
	FULL_COLOR_RGBA.set(1.0f, 1.0f, 1.0f, 1.0f);

	int3 tm = {0, 0, 0};
	sscanf(m_identifier.c_str(), "%d:%d:%d", &tm.x, &tm.y, &tm.z);
	R_ASSERT3(	(tm.x >= 0) && 
				(tm.x < 24) && 
				(tm.y >= 0) && 
				(tm.y < 60) && 
				(tm.z >= 0) && 
				(tm.z < 60),
				"Incorrect weather time", 
				m_identifier.c_str());
	exec_time = tm.x * 3600.f + tm.y * 60.f + tm.z;
	exec_time_loaded = exec_time;

	string_path st, st_env;
	strcpy(st, config.r_string(m_identifier.c_str(), "sky_texture"));
	strconcat(sizeof(st_env), st_env, st, "#irradiance");
	sky_texture_name = st;
	sky_texture_irradiance_name = st_env;
	clouds_texture_name = GetStringIfExist("clouds_texture", "sky\\sky_oblaka", config);
	LPCSTR cldclr = GetStringIfExist("clouds_color", "0, 0, 0, 0", config);

	lut_texture_name = GetStringIfExist("lut_texture", "lut\\lut_neutral", config);

	float multiplier = 0, save = 0;
	sscanf(cldclr, "%f,%f,%f,%f,%f", &clouds_color.x, &clouds_color.y, &clouds_color.z, &clouds_color.w, &multiplier);
	save = clouds_color.w;
	clouds_color.w = save;

	sky_color = GetRGBColorIfExist("sky_color", FULL_COLOR, config);
	sky_rotation = deg2rad(GetFloatIfExist("sky_rotation", 0.0f, config));

	far_plane = GetFloatIfExist("far_plane", 50.0f, config);
	fog_color = GetRGBColorIfExist("fog_color", FULL_COLOR, config);
	fog_density = GetFloatIfExist("fog_density", 0.0f, config);
	fog_sky_influence = GetFloatIfExist("fog_sky_influence", 0.0f, config);

	vertical_fog_intensity = GetFloatIfExist("vertical_fog_intensity", 0.0001f, config);
	vertical_fog_density = GetFloatIfExist("vertical_fog_density", 0.0001f, config);
	vertical_fog_height = GetFloatIfExist("vertical_fog_height", 0.8f, config);

	rain_density = GetFloatIfExist("rain_density", 0.0f, config);
	rain_color = GetRGBColorIfExist("rain_color", FULL_COLOR, config);

	// Попытка найти ссылку на пресет ветра
	bool bWindLoaded = false;
	if (config.line_exist(m_identifier.c_str(), "wind_profile"))
	{
		shared_str wind_profile = config.r_string(m_identifier.c_str(), "wind_profile");
		CEnvWind* pWind = environment.AppendEnvWind(wind_profile);

		if (pWind)
		{
			wind_strength = pWind->m_wind_strength;
			wind_direction = pWind->m_wind_direction;
			wind_gusting = pWind->m_wind_gusting;
			wind_velocity = pWind->m_wind_velocity;
			wind_tilt = pWind->m_wind_tilt;

			bWindLoaded = true;
		}
		else
		{
			Msg("! Invalid wind_profile '%s' in weather section '%s'. Fallback to legacy params.", wind_profile.c_str(),
				m_identifier.c_str());
		}
	}

	// Fallback: Если профиль не задан или не найден, читаем по-старому из текущей секции
	if (!bWindLoaded)
	{
		wind_strength = GetFloatIfExist("wind_strength", 0.35f, config);
		wind_direction = deg2rad(GetFloatIfExist("wind_direction", 0.0f, config));
		wind_gusting = GetFloatIfExist("wind_gusting", 1.0f, config);
		wind_velocity = GetFloatIfExist("wind_velocity", 1.0f, config);
		wind_tilt = GetFloatIfExist("wind_tilt", 0.0f, config);
	}

	ambient = GetRGBColorIfExist("ambient_color", FULL_COLOR, config);
	hemi_color = GetRGBAColorIfExist("hemisphere_color", FULL_COLOR_RGBA, config);
	ambient_brightness = GetFloatIfExist("ambient_brightness", 1.0f, config);
	sun_color = GetRGBColorIfExist("sun_color", NULL_COLOR, config);

	lens_flare_id = environment.eff_LensFlare->AppendDef(environment, environment.m_suns_config,
														 config.r_string(m_identifier.c_str(), "sun"));

	tb_id = environment.eff_Thunderbolt->AppendDef(environment, environment.m_thunderbolt_collections_config,
												   environment.m_thunderbolts_config,
												   config.r_string(m_identifier.c_str(), "thunderbolt_collection"));

	bolt_period = (tb_id.size()) ? config.r_float(m_identifier.c_str(), "thunderbolt_period") : 0.f;
	bolt_duration = (tb_id.size()) ? config.r_float(m_identifier.c_str(), "thunderbolt_duration") : 0.f;
	env_ambient = config.line_exist(m_identifier.c_str(), "ambient")
					  ? environment.AppendEnvAmb(config.r_string(m_identifier.c_str(), "ambient"))
					  : 0;


	m_fSunShaftsIntensity = GetFloatIfExist("sun_shafts_intensity", 0.0f, config);
	m_fWaterIntensity = GetFloatIfExist("water_intensity", 0.0f, config);

	m_fTreeAmplitude = GetFloatIfExist("trees_amplitude", 0.005f, config);
	m_fTreeSpeed = GetFloatIfExist("trees_speed", 1.00f, config);
	m_fTreeRotation = GetFloatIfExist("trees_rotation", 10.0f, config);
	m_fTreeWave.set(.1f, .01f, .11f);
	m_fTreeWave = GetRGBColorIfExist("trees_wave", m_fTreeWave, config);

	m_SepiaColor = GetRGBColorIfExist("sepia_color", NULL_COLOR, config);
	m_SepiaPower = GetFloatIfExist("sepia_power", 0.0f, config);
	m_VignettePower = GetFloatIfExist("vignette_power", 0.0f, config);

	C_CHECK(clouds_color);
	C_CHECK(sky_color);
	C_CHECK(fog_color);
	C_CHECK(rain_color);
	C_CHECK(ambient);
	C_CHECK(hemi_color);
	C_CHECK(sun_color);

	on_device_create();
}

void CEnvDescriptor::on_device_create()
{
	if (sky_texture_name.size())
		sky_texture.create(sky_texture_name.c_str());
	if (sky_texture_irradiance_name.size())
		sky_texture_irradiance.create(sky_texture_irradiance_name.c_str());
	if (clouds_texture_name.size())
		clouds_texture.create(clouds_texture_name.c_str());
	if (lut_texture_name.size())
		lut_texture.create(lut_texture_name.c_str());
}

void CEnvDescriptor::on_device_destroy()
{
	sky_texture.destroy();
	sky_texture_irradiance.destroy();
	clouds_texture.destroy();
	lut_texture.destroy();
}

CEnvDescriptor* CEnvironment::create_descriptor(shared_str const& identifier, CInifile* config)
{
	CEnvDescriptor* result = xr_new<CEnvDescriptor>(identifier);
	if (config)
		result->load(*this, *config);
	return (result);
}