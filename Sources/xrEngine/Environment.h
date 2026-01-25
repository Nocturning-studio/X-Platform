#pragma once

// refs
class ENGINE_API IRender_Visual;
class ENGINE_API CInifile;
class ENGINE_API CEnvironment;
class ENGINE_API CLensFlare;
class ENGINE_API CEffect_Rain;
class ENGINE_API CEffect_Thunderbolt;
class ENGINE_API CPerlinNoise1D;
class ENGINE_API CEnvWind;

struct SThunderboltDesc;
struct SThunderboltCollection;
class CLensFlareDescriptor;

ENGINE_API extern Flags32 psEnvFlags;

#include "Environment_render.h"
#include "Environment_ambient.h"
#include "Environment_modifiers.h"
#include "Environment_descriptor.h"
#include "Environment_descriptor_mixer.h"
#include "Environment_wind.h"							

class ENGINE_API CEnvironment
{
	struct str_pred
	{
		IC bool operator()(const shared_str& x, const shared_str& y) const
		{
			return xr_strcmp(x, y) < 0;
		}
	};

  public:
	DEFINE_VECTOR(CEnvAmbient*, EnvAmbVec, EnvAmbVecIt);
	DEFINE_VECTOR(CEnvDescriptor*, EnvVec, EnvIt);
	DEFINE_MAP_PRED(shared_str, EnvVec, EnvsMap, EnvsMapIt, str_pred);
	DEFINE_VECTOR(CEnvWind*, EnvWindVec, EnvWindVecIt);

  private:
	// clouds
	FvectorVec CloudsVerts;
	U16Vec CloudsIndices;

  private:
	float NormalizeTime(float tm);
	float TimeDiff(float prev, float cur);
	float TimeWeight(float val, float min_t, float max_t);
	void SelectEnvs(EnvVec* envs, CEnvDescriptor*& e0, CEnvDescriptor*& e1, float tm);
	void SelectEnv(EnvVec* envs, CEnvDescriptor*& e, float tm);

  public:
	static bool sort_env_pred(const CEnvDescriptor* x, const CEnvDescriptor* y)
	{
		return x->exec_time < y->exec_time;
	}
	static bool sort_env_etl_pred(const CEnvDescriptor* x, const CEnvDescriptor* y)
	{
		return x->exec_time_loaded < y->exec_time_loaded;
	}

  protected:
	CBlender_skybox m_b_skybox;
	CPerlinNoise1D* PerlinNoise1D;

	float fGameTime;

  public:
	BOOL bNeed_re_create_env;

	float wind_strength_factor;
	float wind_gust_factor;

	// wind blast params
	float wind_blast_strength;
	Fvector wind_blast_direction;
	Fquaternion wind_blast_start_time;
	Fquaternion wind_blast_stop_time;
	float wind_blast_strength_start_value;
	float wind_blast_strength_stop_value;
	Fquaternion wind_blast_current;
	// Environments
	CEnvDescriptorMixer* CurrentEnv;
	CEnvDescriptor* Current[2];

	bool bWFX;
	float wfx_time;
	CEnvDescriptor* WFX_end_desc[2];

	EnvVec* CurrentWeather;
	shared_str CurrentWeatherName;
	shared_str CurrentCycleName;

	EnvsMap WeatherCycles;
	EnvsMap WeatherFXs;
	xr_vector<CEnvModifier> Modifiers;
	EnvAmbVec Ambients;

	CInifile* m_winds_config;
	EnvWindVec Winds;

	ref_shader sh_2sky;
	ref_geom sh_2geom;

	ref_shader clouds_sh;
	ref_geom clouds_geom;

	CEffect_Rain* eff_Rain;
	CLensFlare* eff_LensFlare;
	CEffect_Thunderbolt* eff_Thunderbolt;

	float fTimeFactor;

	ref_texture tsky0, tsky1;
	ref_texture tlut0, tlut1;

	void SelectEnvs(float gt);

	void UpdateAmbient();
	CEnvAmbient* AppendEnvAmb(const shared_str& sect);

	CEnvWind* AppendEnvWind(const shared_str& sect);

	void Invalidate();

  public:
	CEnvironment();
	~CEnvironment();

	void load();
	void unload();

	void mods_load();
	void mods_unload();

	void load_level_specific_ambients();

	void CalcWindValues();

	void OnFrame();
	void set_static_sun_dir();
	void calculate_dynamic_sun_dir();
	void lerp(float& current_weight);

	void RenderSky();
	void RenderClouds();
	void RenderFlares();
	void RenderLast();

	void RenderThunderbolt();

	void RenderRain();

	float GetFlaresBlendFactor();

	bool SetWeatherFX(shared_str name);
	bool StartWeatherFXFromTime(shared_str name, float time);
	bool IsWFXPlaying()
	{
		return bWFX;
	}
	void StopWFX();
	void SetWeather(shared_str name, bool forced = false);
	shared_str GetWeather()
	{
		return CurrentWeatherName;
	}

	// Kondr48: функция перемотки времени
	void ChangeGameTime(float game_time);

	float GetGameTime();
	void SetGameTime(float game_time, float time_factor);
	void OnDeviceCreate();
	void OnDeviceDestroy();

	// editor-related
#ifdef _EDITOR
  public:
	float ed_from_time;
	float ed_to_time;

  public:
	void ED_Reload();
	float GetGameTime()
	{
		return fGameTime;
	}
#endif
	CInifile* m_ambients_config;
	CInifile* m_sound_channels_config;
	CInifile* m_effects_config;
	CInifile* m_suns_config;
	CInifile* m_thunderbolt_collections_config;
	CInifile* m_thunderbolts_config;

  protected:
	CEnvDescriptor* create_descriptor(shared_str const& identifier, CInifile* config);
	void load_weathers();
	void load_weather_effects();
	void create_mixer();
	void destroy_mixer();

  public:
	SThunderboltDesc* thunderbolt_description(CInifile& config, shared_str const& section);
	SThunderboltCollection* thunderbolt_collection(CInifile* pIni, CInifile* thunderbolts, LPCSTR section);
	SThunderboltCollection* thunderbolt_collection(xr_vector<SThunderboltCollection*>& collection,
												   shared_str const& id);
	CLensFlareDescriptor* add_flare(xr_vector<CLensFlareDescriptor*>& collection, shared_str const& id);

  public:
	float p_var_alt;
	float p_var_long;
	float p_min_dist;
	float p_tilt;
	float p_second_prop;
	float p_sky_color;
	float p_sun_color;
	float p_fog_color;
};
