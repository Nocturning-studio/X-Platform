#include "stdafx.h"
#pragma hdrstop

#ifndef _EDITOR
#include "render.h"
#endif

#include "Environment.h"
#include "xr_efflensflare.h"
#include "rain.h"
#include "thunderbolt.h"
#include "xrHemisphere.h"
#include "perlin.h"

#include "xr_input.h"

#include "resourcemanager.h"

#ifndef _EDITOR
#include "IGame_Level.h"
#endif

#include "D3DUtils.h"
#include "xrCore.h"

#pragma warning(push)
#pragma warning(disable : 4995)
#include <ppl.h>
#pragma warning(pop)

// #define WEATHER_LOGGING

#define DAY_LENGTH 86400.f
static const float MAX_NOISE_FREQ = 0.03f;

// real WEATHER->WFX transition time
#define WFX_TRANS_TIME 5.f

//////////////////////////////////////////////////////////////////////////
// environment
CEnvironment::CEnvironment() : CurrentEnv(0), m_ambients_config(0)
{
	bNeed_re_create_env = FALSE;
	bWFX = false;
	Current[0] = 0;
	Current[1] = 0;
	CurrentWeather = 0;
	CurrentWeatherName = 0;
	eff_Rain = 0;
	eff_LensFlare = 0;
	eff_Thunderbolt = 0;
	OnDeviceCreate(); // Этот вызов остается, но внутри будут изменения

	fGameTime = 0.f;
	fTimeFactor = 12.f;

	wind_strength_factor = 0.f;
	wind_gust_factor = 0.f;

	wind_blast_strength = 0.f;
	wind_blast_direction.set(1.f, 0.f, 0.f);

	wind_blast_strength_start_value = 0.f;
	wind_blast_strength_stop_value = 0.f;

	// fill clouds hemi verts & faces
	const fvec3* verts;
	CloudsVerts.resize(xrHemisphereVertices(2, verts));
	CopyMemory(&CloudsVerts.front(), verts, CloudsVerts.size() * sizeof(fvec3));
	const u16* indices;
	CloudsIndices.resize(xrHemisphereIndices(2, indices));
	CopyMemory(&CloudsIndices.front(), indices, CloudsIndices.size() * sizeof(u16));

	// perlin noise
	PerlinNoise1D = xr_new<CPerlinNoise1D>(Random.randI(0, 0xFFFF));
	PerlinNoise1D->SetOctaves(2);
	PerlinNoise1D->SetAmplitude(0.66666f);

	// Создаем текстуры для рендер-таргетов (они все еще нужны для шейдеров)
	tsky0 = Engine.ResourceManager->_CreateTexture("$user$sky0");
	tsky1 = Engine.ResourceManager->_CreateTexture("$user$sky1");

	tlut0 = Engine.ResourceManager->_CreateTexture("$user$lut_s0");
	tlut1 = Engine.ResourceManager->_CreateTexture("$user$lut_s1");

	string_path file_name;
	m_winds_config =
		xr_new<CInifile>(FS.update_path(file_name, "$game_config$", "environment\\winds.ltx"), TRUE, TRUE, FALSE);
	m_ambients_config =
		xr_new<CInifile>(FS.update_path(file_name, "$game_config$", "environment\\ambients.ltx"), TRUE, TRUE, FALSE);
	m_sound_channels_config = xr_new<CInifile>(
		FS.update_path(file_name, "$game_config$", "environment\\sound_channels.ltx"), TRUE, TRUE, FALSE);
	m_effects_config =
		xr_new<CInifile>(FS.update_path(file_name, "$game_config$", "environment\\effects.ltx"), TRUE, TRUE, FALSE);
	m_suns_config =
		xr_new<CInifile>(FS.update_path(file_name, "$game_config$", "environment\\suns.ltx"), TRUE, TRUE, FALSE);
	m_thunderbolt_collections_config = xr_new<CInifile>(
		FS.update_path(file_name, "$game_config$", "environment\\thunderbolt_collections.ltx"), TRUE, TRUE, FALSE);
	m_thunderbolts_config = xr_new<CInifile>(
		FS.update_path(file_name, "$game_config$", "environment\\thunderbolts.ltx"), TRUE, TRUE, FALSE);
	CInifile* config =
		xr_new<CInifile>(FS.update_path(file_name, "$game_config$", "environment\\environment.ltx"), TRUE, TRUE, FALSE);

	// params
	p_var_alt = deg2rad(config->r_float("environment", "altitude"));
	p_var_long = deg2rad(config->r_float("environment", "delta_longitude"));
	p_min_dist = std::min(.95f, config->r_float("environment", "min_dist_factor"));
	p_tilt = deg2rad(config->r_float("environment", "tilt"));
	p_second_prop = config->r_float("environment", "second_propability");
	clamp(p_second_prop, 0.f, 1.f);
	p_sky_color = config->r_float("environment", "sky_color");
	p_sun_color = config->r_float("environment", "sun_color");
	p_fog_color = config->r_float("environment", "fog_color");

	xr_delete(config);
}
CEnvironment::~CEnvironment()
{
	xr_delete(PerlinNoise1D);
	OnDeviceDestroy();

	VERIFY(m_ambients_config);
	CInifile::Destroy(m_ambients_config);
	m_ambients_config = 0;

	VERIFY(m_sound_channels_config);
	CInifile::Destroy(m_sound_channels_config);
	m_sound_channels_config = 0;

	VERIFY(m_effects_config);
	CInifile::Destroy(m_effects_config);
	m_effects_config = 0;

	VERIFY(m_suns_config);
	CInifile::Destroy(m_suns_config);
	m_suns_config = 0;

	VERIFY(m_thunderbolt_collections_config);
	CInifile::Destroy(m_thunderbolt_collections_config);
	m_thunderbolt_collections_config = 0;

	VERIFY(m_thunderbolts_config);
	CInifile::Destroy(m_thunderbolts_config);
	m_thunderbolts_config = 0;

	VERIFY(m_winds_config);
	CInifile::Destroy(m_winds_config);
	m_winds_config = 0;

	destroy_mixer();
}

void CEnvironment::Invalidate()
{
	bWFX = false;
	Current[0] = 0;
	Current[1] = 0;
	// if (eff_LensFlare)		eff_LensFlare->Invalidate();
}

float CEnvironment::TimeDiff(float prev, float cur)
{
	if (prev > cur)
		return (DAY_LENGTH - prev) + cur;
	else
		return cur - prev;
}

float CEnvironment::TimeWeight(float val, float min_t, float max_t)
{
	float weight = 0.f;
	float length = TimeDiff(min_t, max_t);
	if (!fis_zero(length, EPS))
	{
		if (min_t > max_t)
		{
			if ((val >= min_t) || (val <= max_t))
				weight = TimeDiff(min_t, val) / length;
		}
		else
		{
			if ((val >= min_t) && (val <= max_t))
				weight = TimeDiff(min_t, val) / length;
		}
		clamp(weight, 0.f, 1.f);
	}
	return weight;
}

#pragma todo("NSDeathman to NSDeathman: Сделать setenvtime для одиночной игры")
void CEnvironment::ChangeGameTime(float game_time)
{
	fGameTime = NormalizeTime(fGameTime + game_time);
};

float CEnvironment::GetGameTime()
{
	return fGameTime;
}

void CEnvironment::SetGameTime(float game_time, float time_factor)
{
	if (bWFX)
		wfx_time -= TimeDiff(fGameTime, game_time);
	fGameTime = game_time;
	fTimeFactor = time_factor;
}

float CEnvironment::NormalizeTime(float tm)
{
	if (tm < 0.f)
		return tm + DAY_LENGTH;
	else if (tm > DAY_LENGTH)
		return tm - DAY_LENGTH;
	else
		return tm;
}

void CEnvironment::SetWeather(shared_str name, bool forced)
{
	if (name.size())
	{
		EnvsMapIt it = WeatherCycles.find(name);
		if (it == WeatherCycles.end())
		{
			Msg("! Can't find weather effect with name: %s", name.c_str());
			return;
		}
		CurrentCycleName = it->first;
		if (forced)
		{
			Invalidate();
		}
		if (!bWFX)
		{
			CurrentWeather = &it->second;
			CurrentWeatherName = it->first;
		}
		if (forced)
		{
			SelectEnvs(fGameTime);
		}
#ifdef WEATHER_LOGGING
		Msg("Starting Cycle: %s [%s]", *name, forced ? "forced" : "deferred");
#endif
	}
	else
	{
#ifndef _EDITOR
		Msg("! Empty weather name");
		return;
#endif
	}
}

bool CEnvironment::SetWeatherFX(shared_str name)
{
	if (bWFX)
		return false;

	if (name.size())
	{
		EnvsMapIt it = WeatherFXs.find(name);

		if (it == WeatherFXs.end())
		{
			Msg("! Can't find weather effect with name: %s", name.c_str());
			return false;
		}

		EnvVec* PrevWeather = CurrentWeather;
		VERIFY(PrevWeather);
		CurrentWeather = &it->second;
		CurrentWeatherName = it->first;

		float rewind_tm = WFX_TRANS_TIME * fTimeFactor;
		float start_tm = fGameTime + rewind_tm;
		float current_length;
		float current_weight;
		if (Current[0]->exec_time > Current[1]->exec_time)
		{
			float x = fGameTime > Current[0]->exec_time ? fGameTime - Current[0]->exec_time
														: (DAY_LENGTH - Current[0]->exec_time) + fGameTime;
			current_length = (DAY_LENGTH - Current[0]->exec_time) + Current[1]->exec_time;
			current_weight = x / current_length;
		}
		else
		{
			current_length = Current[1]->exec_time - Current[0]->exec_time;
			current_weight = (fGameTime - Current[0]->exec_time) / current_length;
		}
		clamp(current_weight, 0.f, 1.f);

		concurrency::parallel_sort(CurrentWeather->begin(), CurrentWeather->end(), sort_env_etl_pred);
		CEnvDescriptor* C0 = CurrentWeather->at(0);
		CEnvDescriptor* C1 = CurrentWeather->at(1);
		CEnvDescriptor* CE = CurrentWeather->at(CurrentWeather->size() - 2);
		CEnvDescriptor* CT = CurrentWeather->at(CurrentWeather->size() - 1);
		C0->copy(*Current[0]);
		C0->exec_time =
			NormalizeTime(fGameTime - ((rewind_tm / (Current[1]->exec_time - fGameTime)) * current_length - rewind_tm));
		C1->copy(*Current[1]);
		C1->exec_time = NormalizeTime(start_tm);
		for (EnvIt t_it = CurrentWeather->begin() + 2; t_it != CurrentWeather->end() - 1; t_it++)
			(*t_it)->exec_time = NormalizeTime(start_tm + (*t_it)->exec_time_loaded);
		SelectEnv(PrevWeather, WFX_end_desc[0], CE->exec_time);
		SelectEnv(PrevWeather, WFX_end_desc[1], WFX_end_desc[0]->exec_time + 0.5f);
		CT->copy(*WFX_end_desc[0]);
		CT->exec_time = NormalizeTime(CE->exec_time + rewind_tm);
		wfx_time = TimeDiff(fGameTime, CT->exec_time);
		bWFX = true;

		// sort wfx envs
		concurrency::parallel_sort(CurrentWeather->begin(), CurrentWeather->end(), sort_env_pred);

		Current[0] = C0;
		Current[1] = C1;
#ifdef WEATHER_LOGGING
		Msg("Starting WFX: '%s' - %3.2f sec", *name, wfx_time);
#endif
	}
	else
	{
#ifndef _EDITOR
		Msg("! Empty weather effect name");
		return false;
#endif
	}
	return true;
}

bool CEnvironment::StartWeatherFXFromTime(shared_str name, float time)
{
	if (!SetWeatherFX(name))
		return false;

	for (EnvIt it = CurrentWeather->begin(); it != CurrentWeather->end(); it++)
		(*it)->exec_time = NormalizeTime((*it)->exec_time - wfx_time + time);

	wfx_time = time;
	return true;
}

void CEnvironment::StopWFX()
{
	VERIFY(CurrentCycleName.size());
	bWFX = false;
	SetWeather(CurrentCycleName, false);
	Current[0] = WFX_end_desc[0];
	Current[1] = WFX_end_desc[1];
#ifdef WEATHER_LOGGING
	Msg("WFX - end. Weather: '%s' Desc: '%s'/'%s' GameTime: %3.2f", CurrentWeatherName.c_str(),
		Current[0]->m_identifier.c_str(), Current[1]->m_identifier.c_str(), fGameTime);
#endif
}

bool lb_env_pred(const CEnvDescriptor* x, float val)
{
	return x->exec_time < val;
}

void CEnvironment::SelectEnv(EnvVec* envs, CEnvDescriptor*& e, float gt)
{
	EnvIt env = std::lower_bound(envs->begin(), envs->end(), gt, lb_env_pred);
	if (env == envs->end())
	{
		e = envs->front();
	}
	else
	{
		e = *env;
	}
}

void CEnvironment::SelectEnvs(EnvVec* envs, CEnvDescriptor*& e0, CEnvDescriptor*& e1, float gt)
{
	EnvIt env = std::lower_bound(envs->begin(), envs->end(), gt, lb_env_pred);
	if (env == envs->end())
	{
		e0 = *(envs->end() - 1);
		e1 = envs->front();
	}
	else
	{
		e1 = *env;
		if (env == envs->begin())
			e0 = *(envs->end() - 1);
		else
			e0 = *(env - 1);
	}
}

void CEnvironment::SelectEnvs(float gt)
{
	VERIFY(CurrentWeather);
	if ((Current[0] == Current[1]) && (Current[0] == 0))
	{
		VERIFY(!bWFX);
		// first or forced start
		SelectEnvs(CurrentWeather, Current[0], Current[1], gt);
	}
	else
	{
		bool bSelect = false;
		if (Current[0]->exec_time > Current[1]->exec_time)
		{
			// terminator
			bSelect = (gt > Current[1]->exec_time) && (gt < Current[0]->exec_time);
		}
		else
		{
			bSelect = (gt > Current[1]->exec_time);
		}
		if (bSelect)
		{
			Current[0] = Current[1];
			SelectEnv(CurrentWeather, Current[1], gt);
#ifdef WEATHER_LOGGING
			Msg("Weather: '%s' Desc: '%s' Time: %3.2f/%3.2f", CurrentWeatherName.c_str(),
				Current[1]->m_identifier.c_str(), Current[1]->exec_time, fGameTime);
#endif
		}
	}
}

int get_ref_count(IUnknown* ii)
{
	if (ii)
	{
		ii->AddRef();
		return ii->Release();
	}
	else
		return 0;
}

void CEnvironment::lerp(float& current_weight)
{
	if (bWFX && (wfx_time <= 0.f))
		StopWFX();

	SelectEnvs(fGameTime);
	VERIFY(Current[0] && Current[1]);

	current_weight = TimeWeight(fGameTime, Current[0]->exec_time, Current[1]->exec_time);
	// modifiers
	CEnvModifier EM;
	EM.far_plane = 0;
	EM.fog_color.set(0, 0, 0);
	EM.fog_density = 0;
	EM.ambient.set(0, 0, 0);
	EM.sky_color.set(0, 0, 0);
	EM.hemi_color.set(0, 0, 0);
	EM.use_flags.zero();

	fvec3 view = Engine.RenderView.Position;
	float mpower = 0;
	for (xr_vector<CEnvModifier>::iterator mit = Modifiers.begin(); mit != Modifiers.end(); mit++)
		mpower += EM.sum(*mit, view);

	// final lerp
	CurrentEnv->lerp(this, *Current[0], *Current[1], current_weight, EM, mpower);
}

void CEnvironment::OnFrame()
{
#ifdef _EDITOR
	SetGameTime(fGameTime + Engine.TimeManager.GetDeltaTime() * fTimeFactor, fTimeFactor);
	if (fsimilar(ed_to_time, DAY_LENGTH) && fsimilar(ed_from_time, 0.f))
	{
		if (fGameTime > DAY_LENGTH)
			fGameTime -= DAY_LENGTH;
	}
	else
	{
		if (fGameTime > ed_to_time)
		{
			fGameTime = fGameTime - ed_to_time + ed_from_time;
			Current[0] = Current[1] = 0;
		}
		if (fGameTime < ed_from_time)
		{
			fGameTime = ed_from_time;
			Current[0] = Current[1] = 0;
		}
	}
	if (!psDeviceFlags.is(rsEnvironment))
		return;
#else
	if (!g_pGameLevel)
		return;
#endif

	CalcWindValues();

	// if (pInput->iGetAsyncKeyState(DIK_O))        
	//	SetWeatherFX("surge_day");
	float current_weight;
	lerp(current_weight);

	if (Render->is_dynamic_sun_enabled())
		calculate_dynamic_sun_dir();
	else
		set_static_sun_dir();

	// Устанавливаем текстуры скайбокса в render targets
	if (CurrentEnv->sky_texture_0)
	{
		IDirect3DBaseTexture9* e0 = CurrentEnv->sky_texture_0->surface_get();
		tsky0->surface_set(e0);
		_RELEASE(e0);
	}
	else
		tsky0->surface_set(NULL);

	if (CurrentEnv->sky_texture_1)
	{
		IDirect3DBaseTexture9* e1 = CurrentEnv->sky_texture_1->surface_get();
		tsky1->surface_set(e1);
		_RELEASE(e1);
	}
	else
		tsky1->surface_set(NULL);

	// Устанавливаем LUT текстуры
	if (CurrentEnv->lut_texture_0)
	{
		IDirect3DBaseTexture9* lut0 = CurrentEnv->lut_texture_0->surface_get();
		tlut0->surface_set(lut0);
		_RELEASE(lut0);
	}
	else
		tlut0->surface_set(NULL);

	if (CurrentEnv->lut_texture_1)
	{
		IDirect3DBaseTexture9* lut1 = CurrentEnv->lut_texture_1->surface_get();
		tlut1->surface_set(lut1);
		_RELEASE(lut1);
	}
	else
		tlut1->surface_set(NULL);

	PerlinNoise1D->SetFrequency(wind_gust_factor * MAX_NOISE_FREQ);
	wind_strength_factor = clampr(PerlinNoise1D->GetContinious(Engine.TimeManager.GetGlobalTime()) + 0.5f, 0.f, 1.f);

	shared_str l_id = (current_weight < 0.5f) ? Current[0]->lens_flare_id : Current[1]->lens_flare_id;
	eff_LensFlare->OnFrame(l_id);
	shared_str t_id = (current_weight < 0.5f) ? Current[0]->tb_id : Current[1]->tb_id;
	eff_Thunderbolt->OnFrame(t_id, CurrentEnv->bolt_period, CurrentEnv->bolt_duration);
	eff_Rain->OnFrame();
}

void CEnvironment::set_static_sun_dir()
{
	CurrentEnv->sun_dir.setHP(deg2rad(-25.0f), deg2rad(292.0f));
}

void CEnvironment::calculate_dynamic_sun_dir()
{
	float g = (360.0f / 365.25f) * (180.0f + fGameTime / DAY_LENGTH);

	g = deg2rad(g);

	//	Declination
	float D = 0.396372f - 22.91327f * _cos(g) + 4.02543f * _sin(g) - 0.387205f * _cos(2 * g) + 0.051967f * _sin(2 * g) - 0.154527f * _cos(3 * g) + 0.084798f * _sin(3 * g);

	//	Now calculate the time correction for solar angle:
	float TC = 0.004297f + 0.107029f * _cos(g) - 1.837877f * _sin(g) - 0.837378f * _cos(2 * g) - 2.340475f * _sin(2 * g);

	//	IN degrees
	float Longitude = -30.4f;

	float SHA = (fGameTime / (DAY_LENGTH / 24) - 12) * 15 + Longitude + TC;

	//	Need this to correctly determine SHA sign
	if (SHA > 180)
		SHA -= 360;

	if (SHA < -180)
		SHA += 360;

	//	IN degrees
	float const Latitude = 50.27f;
	float const LatitudeR = deg2rad(Latitude);

	//	Now we can calculate the Sun Zenith Angle (SZA):
	float cosSZA = _sin(LatitudeR) * _sin(deg2rad(D)) + _cos(LatitudeR) * _cos(deg2rad(D)) * _cos(deg2rad(SHA));

	clamp(cosSZA, -1.0f, 1.0f);

	float SZA = acosf(cosSZA);
	float SEA = PI / 2 - SZA;

	//	To finish we will calculate the Azimuth Angle (AZ):
	float cosAZ = 0.f;
	float const sin_SZA = _sin(SZA);
	float const cos_Latitude = _cos(LatitudeR);
	float const sin_SZA_X_cos_Latitude = sin_SZA * cos_Latitude;

	if (!fis_zero(sin_SZA_X_cos_Latitude))
		cosAZ = (_sin(deg2rad(D)) - _sin(LatitudeR) * _cos(SZA)) / sin_SZA_X_cos_Latitude;

	clamp(cosAZ, -1.0f, 1.0f);
	float AZ = acosf(cosAZ);

	const fvec2 minAngle = fvec2().set(deg2rad(1.0f), deg2rad(3.0f));

	if (SEA < minAngle.x)
		SEA = minAngle.x;

	float fSunBlend = (SEA - minAngle.x) / (minAngle.y - minAngle.x);
	clamp(fSunBlend, 0.0f, 1.0f);

#ifdef PROCEDURAL_WEATHER
	const fvec2 minAngle1 = fvec2().set(deg2rad(1.0f), deg2rad(20.0f));

	if (SEA < minAngle1.x)
		SEA = minAngle1.x;

	float fSunBlend1 = (SEA - minAngle1.x) / (minAngle1.y - minAngle1.x);
	clamp(fSunBlend1, 0.7f, 1.0f);
#endif

	SEA = -SEA;

	if (SHA < 0)
		AZ = 2 * PI - AZ;

	R_ASSERT(_valid(AZ));
	R_ASSERT(_valid(SEA));

	CurrentEnv->sun_dir.setHP(AZ, SEA);
	R_ASSERT(_valid(CurrentEnv->sun_dir));

#ifdef PROCEDURAL_WEATHER
	CurrentEnv->sun_color.set(1, 1, 1);
	CurrentEnv->sun_color.y *= fSunBlend1;
	CurrentEnv->sun_color.z *= fSunBlend1 * 0.9f;

	float AmbientBrightness = CurrentEnv->ambient_brightness;
	float MinAmb = 0.5f;
	CurrentEnv->ambient_brightness = MinAmb + fSunBlend * (AmbientBrightness - MinAmb);
#endif

	CurrentEnv->sun_color.mul(fSunBlend);
}

void CEnvironment::create_mixer()
{
	VERIFY(!CurrentEnv);
	CurrentEnv = xr_new<CEnvDescriptorMixer>("00:00:00");
}

void CEnvironment::destroy_mixer()
{
	xr_delete(CurrentEnv);
}

void CEnvironment::mods_load()
{
	Modifiers.clear_and_free();
	string_path path;
	if (FS.exist(path, "$level$", "level.env_mod"))
	{
		IReader* fs = FS.r_open(path);
		u32 id = 0;
		while (fs->find_chunk(id))
		{
			CEnvModifier E;
			// Надо обязательно проверять, что файл корректный, даже если чанк был найден. Real Wolf.
			if (!E.load(fs))
				break;
			Modifiers.push_back(E);
			id++;
		}
		FS.r_close(fs);
	}
}

void CEnvironment::load_level_specific_ambients()
{
	const shared_str level_name = g_pGameLevel->name();

	string_path path;
	strconcat(sizeof(path), path, "environment\\ambients\\", level_name.c_str(), ".ltx");

	string_path full_path;
	CInifile* level_ambients = xr_new<CInifile>(FS.update_path(full_path, "$game_config$", path), TRUE, TRUE, FALSE);

	for (EnvAmbVecIt I = Ambients.begin(), E = Ambients.end(); I != E; ++I)
	{
		CEnvAmbient* ambient = *I;

		shared_str section_name = ambient->name();

		// choose a source ini file
		CInifile* source = (level_ambients && level_ambients->section_exist(section_name)) ? level_ambients : m_ambients_config;

		// check and reload if needed
		if (xr_strcmp(ambient->get_ambients_config_filename().c_str(), source->fname()))
		{
			ambient->destroy();
			ambient->load(*source, *m_sound_channels_config, *m_effects_config, section_name);
		}
	}

	xr_delete(level_ambients);
}

void CEnvironment::mods_unload()
{
	if(!Modifiers.empty())
		Modifiers.clear_and_free();
}

void CEnvironment::load_weathers()
{
	if (!WeatherCycles.empty())
		return;

	typedef xr_vector<LPSTR> file_list_type;
	file_list_type* file_list = FS.file_list_open("$game_weathers$", "");
	VERIFY(file_list);

	file_list_type::const_iterator file_list_it = file_list->begin();
	file_list_type::const_iterator file_list_end = file_list->end();
	for (; file_list_it != file_list_end; ++file_list_it)
	{
		u32 length = xr_strlen(*file_list_it);
		VERIFY(length >= 4);
		VERIFY((*file_list_it)[length - 4] == '.');
		VERIFY((*file_list_it)[length - 3] == 'l');
		VERIFY((*file_list_it)[length - 2] == 't');
		VERIFY((*file_list_it)[length - 1] == 'x');
		u32 new_length = length - 4;
		LPSTR identifier = (LPSTR)_alloca((new_length + 1) * sizeof(char));
		std::memcpy(identifier, *file_list_it, new_length * sizeof(char));
		identifier[new_length] = 0;
		EnvVec& env = WeatherCycles[identifier];

		string_path file_name;
		FS.update_path(file_name, "$game_weathers$", identifier);
		strcat(file_name, ".ltx");
		CInifile* config = CInifile::Create(file_name);

		typedef CInifile::Root sections_type;
		sections_type& sections = config->sections();

		env.reserve(sections.size());

		sections_type::const_iterator sections_it = sections.begin();
		sections_type::const_iterator sections_end = sections.end();
		for (; sections_it != sections_end; ++sections_it)
		{
			CEnvDescriptor* object = create_descriptor((*sections_it)->Name, config);
			env.push_back(object);
		}

		CInifile::Destroy(config);
	}

	FS.file_list_close(file_list);

	// sorting weather envs
	EnvsMapIt _I = WeatherCycles.begin();
	EnvsMapIt _E = WeatherCycles.end();
	for (; _I != _E; _I++)
	{
		R_ASSERT3(_I->second.size() > 1, "One weather cycle must have implementations for at least two hours", *_I->first);
		concurrency::parallel_sort(_I->second.begin(), _I->second.end(), sort_env_etl_pred);
	}
	R_ASSERT2(!WeatherCycles.empty(), "Empty weathers.");
	SetWeather((*WeatherCycles.begin()).first.c_str());
}

void CEnvironment::load_weather_effects()
{
	if (!WeatherFXs.empty())
		return;

	typedef xr_vector<LPSTR> file_list_type;
	file_list_type* file_list = FS.file_list_open("$game_weather_effects$", "");
	VERIFY(file_list);

	file_list_type::const_iterator file_list_it = file_list->begin();
	file_list_type::const_iterator file_list_end = file_list->end();
	for (; file_list_it != file_list_end; ++file_list_it)
	{
		u32 length = xr_strlen(*file_list_it);
		VERIFY(length >= 4);
		VERIFY((*file_list_it)[length - 4] == '.');
		VERIFY((*file_list_it)[length - 3] == 'l');
		VERIFY((*file_list_it)[length - 2] == 't');
		VERIFY((*file_list_it)[length - 1] == 'x');
		u32 new_length = length - 4;
		LPSTR identifier = (LPSTR)_alloca((new_length + 1) * sizeof(char));
		std::memcpy(identifier, *file_list_it, new_length * sizeof(char));
		identifier[new_length] = 0;
		EnvVec& env = WeatherFXs[identifier];

		string_path file_name;
		FS.update_path(file_name, "$game_weather_effects$", identifier);
		strcat(file_name, ".ltx");
		CInifile* config = CInifile::Create(file_name);

		typedef CInifile::Root sections_type;
		sections_type& sections = config->sections();

		env.reserve(sections.size() + 2);
		env.push_back(create_descriptor("00:00:00", false));

		sections_type::const_iterator sections_it = sections.begin();
		sections_type::const_iterator sections_end = sections.end();
		for (; sections_it != sections_end; ++sections_it)
		{
			CEnvDescriptor* object = create_descriptor((*sections_it)->Name, config);
			env.push_back(object);
		}

		CInifile::Destroy(config);

		env.push_back(create_descriptor("24:00:00", false));
		env.back()->exec_time_loaded = DAY_LENGTH;
	}

	FS.file_list_close(file_list);

	// sorting weather envs
	EnvsMapIt _I = WeatherFXs.begin();
	EnvsMapIt _E = WeatherFXs.end();
	for (; _I != _E; _I++)
	{
		R_ASSERT3(_I->second.size() > 1, "Environment in weather must >=2", *_I->first);
		concurrency::parallel_sort(_I->second.begin(), _I->second.end(), sort_env_etl_pred);
	}
}

void CEnvironment::load()
{
	if (!CurrentEnv)
		create_mixer();

	if (!eff_Rain)
		eff_Rain = xr_new<CEffect_Rain>();
	if (!eff_LensFlare)
		eff_LensFlare = xr_new<CLensFlare>();
	if (!eff_Thunderbolt)
		eff_Thunderbolt = xr_new<CEffect_Thunderbolt>();

	load_weathers();
	load_weather_effects();
}

void CEnvironment::unload()
{
	EnvsMapIt _I, _E;
	// clear weathers
	_I = WeatherCycles.begin();
	_E = WeatherCycles.end();
	for (; _I != _E; _I++)
	{
		for (EnvIt it = _I->second.begin(); it != _I->second.end(); it++)
			xr_delete(*it);
	}

	WeatherCycles.clear();
	// clear weather effect
	_I = WeatherFXs.begin();
	_E = WeatherFXs.end();
	for (; _I != _E; _I++)
	{
		for (EnvIt it = _I->second.begin(); it != _I->second.end(); it++)
			xr_delete(*it);
	}
	WeatherFXs.clear();
	// clear ambient
	for (EnvAmbVecIt it = Ambients.begin(); it != Ambients.end(); it++)
		xr_delete(*it);
	Ambients.clear();

	for (EnvWindVecIt it = Winds.begin(); it != Winds.end(); it++)
		xr_delete(*it);
	Winds.clear();

	// misc
	xr_delete(eff_Rain);
	xr_delete(eff_LensFlare);
	xr_delete(eff_Thunderbolt);
	CurrentWeather = 0;
	CurrentWeatherName = 0;

	if (CurrentEnv)
		CurrentEnv->clear();

	Invalidate();
}

SThunderboltDesc* CEnvironment::thunderbolt_description(CInifile& config, shared_str const& section)
{
	SThunderboltDesc* result = xr_new<SThunderboltDesc>();
	result->load(config, section);
	return (result);
}

SThunderboltCollection* CEnvironment::thunderbolt_collection(CInifile* pIni, CInifile* thunderbolts, LPCSTR section)
{
	SThunderboltCollection* result = xr_new<SThunderboltCollection>();
	result->load(pIni, thunderbolts, section);
	return (result);
}

SThunderboltCollection* CEnvironment::thunderbolt_collection(xr_vector<SThunderboltCollection*>& collection,
															 shared_str const& id)
{
	typedef xr_vector<SThunderboltCollection*> Container;
	Container::iterator container_it = collection.begin();
	Container::iterator container_end = collection.end();
	for (; container_it != container_end; ++container_it)
		if ((*container_it)->section == id)
			return (*container_it);

	NODEFAULT;
#ifdef DEBUG
	return (0);
#endif // #ifdef DEBUG
}

CLensFlareDescriptor* CEnvironment::add_flare(xr_vector<CLensFlareDescriptor*>& collection, shared_str const& id)
{
	typedef xr_vector<CLensFlareDescriptor*> Flares;

	Flares::const_iterator flares_it = collection.begin();
	Flares::const_iterator flares_end = collection.end();
	for (; flares_it != flares_end; ++flares_it)
	{
		if ((*flares_it)->section == id)
			return (*flares_it);
	}

	CLensFlareDescriptor* result = xr_new<CLensFlareDescriptor>();
	result->load(m_suns_config, id.c_str());
	collection.push_back(result);
	return (result);
}
