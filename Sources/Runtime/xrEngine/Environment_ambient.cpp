//-----------------------------------------------------------------------------
// Environment ambient
//-----------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "Environment.h"
#include "../xrGame/object_broker.h"

void CEnvAmbient::SSndChannel::load(CInifile& config, LPCSTR sect)
{
	m_load_section = sect;

	m_sound_dist.x = config.r_float(m_load_section, "min_distance");
	m_sound_dist.y = config.r_float(m_load_section, "max_distance");
	m_sound_period.x = config.r_s32(m_load_section, "period0");
	m_sound_period.y = config.r_s32(m_load_section, "period1");
	m_sound_period.z = config.r_s32(m_load_section, "period2");
	m_sound_period.w = config.r_s32(m_load_section, "period3");

	if (!(m_sound_period.x <= m_sound_period.y))
		Msg("! Environment Ambient: Sound Channels - period0 should be less than period1, error in section with name %s", sect);
		
	if (!(m_sound_period.z <= m_sound_period.w))
		Msg("! Environment Ambient: Sound Channels - period2 should be less than period3, error in section with name %s", sect);

	if(!(m_sound_dist.y > m_sound_dist.x))
		Msg("! Environment Ambient: Sound Channels - min_distance should be less than max_distance, error in section with name %s", sect);

	LPCSTR snds = config.r_string(sect, "sounds");
	u32 cnt = _GetItemCount(snds);
	string_path tmp;

	if (cnt == 0)
	{
		Msg("! Environment Ambient: Sound Channels - sounds array empty, error in section with name %s", sect);
		snds = "$no_sound";
		cnt = 1;
	}

	m_sounds.resize(cnt);

	for (u32 k = 0; k < cnt; ++k)
	{
		_GetItem(snds, k, tmp);
		m_sounds[k].create(tmp, st_Effect, sg_SourceType);
	}
}

CEnvAmbient::SEffect* CEnvAmbient::create_effect(CInifile& config, LPCSTR id)
{
	SEffect* result = xr_new<SEffect>();
	result->life_time = iFloor(config.r_float(id, "life_time") * 1000.f);
	result->particles = config.r_string(id, "particles");
	VERIFY(result->particles.size());
	result->offset = config.r_fvector3(id, "offset");
	result->wind_gust_factor = config.r_float(id, "wind_gust_factor");

	if (config.line_exist(id, "sound"))
		result->sound.create(config.r_string(id, "sound"), st_Effect, sg_SourceType);

	if (config.line_exist(id, "wind_blast_strength"))
	{
		result->wind_blast_strength = config.r_float(id, "wind_blast_strength");
		result->wind_blast_direction.setHP(deg2rad(config.r_float(id, "wind_blast_longitude")), 0.f);
		result->wind_blast_in_time = config.r_float(id, "wind_blast_in_time");
		result->wind_blast_out_time = config.r_float(id, "wind_blast_out_time");
		return (result);
	}

	result->wind_blast_strength = 0.f;
	result->wind_blast_direction.set(0.f, 0.f, 1.f);
	result->wind_blast_in_time = 0.f;
	result->wind_blast_out_time = 0.f;

	return (result);
}

CEnvAmbient::SSndChannel* CEnvAmbient::create_sound_channel(CInifile& config, LPCSTR id)
{
	SSndChannel* result = xr_new<SSndChannel>();
	result->load(config, id);
	return (result);
}

CEnvAmbient::~CEnvAmbient()
{
	destroy();
}

void CEnvAmbient::destroy()
{
	delete_data(m_effects);
	delete_data(m_sound_channels);
}

void CEnvAmbient::load(CInifile& ambients_config, CInifile& sound_channels_config, CInifile& effects_config,
					   const shared_str& sect)
{
	m_ambients_config_filename = ambients_config.fname();
	m_load_section = sect;
	string_path tmp;

	// sounds
	LPCSTR channels = ambients_config.r_string(sect, "sound_channels");
	u32 cnt = _GetItemCount(channels);
	m_sound_channels.resize(cnt);

	for (u32 i = 0; i < cnt; ++i)
		m_sound_channels[i] = create_sound_channel(sound_channels_config, _GetItem(channels, i, tmp));

	// effects
	m_effect_period.set(iFloor(ambients_config.r_float(sect, "min_effect_period") * 1000.f),
						iFloor(ambients_config.r_float(sect, "max_effect_period") * 1000.f));
	LPCSTR effs = ambients_config.r_string(sect, "effects");
	cnt = _GetItemCount(effs);

	m_effects.resize(cnt);
	for (u32 k = 0; k < cnt; ++k)
		m_effects[k] = create_effect(effects_config, _GetItem(effs, k, tmp));

	R_ASSERT(!m_sound_channels.empty() || !m_effects.empty());
}

CEnvAmbient* CEnvironment::AppendEnvAmb(const shared_str& sect)
{
	for (EnvAmbVecIt it = Ambients.begin(); it != Ambients.end(); it++)
		if ((*it)->name().equal(sect))
			return (*it);

	Ambients.push_back(xr_new<CEnvAmbient>());
	Ambients.back()->load(*m_ambients_config, *m_sound_channels_config, *m_effects_config, sect);
	return (Ambients.back());
}