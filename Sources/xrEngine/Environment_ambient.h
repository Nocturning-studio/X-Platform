#pragma once

class ENGINE_API CEnvAmbient
{
  public:
	struct SEffect
	{
		u32 life_time;
		ref_sound sound;
		shared_str particles;
		float3 offset;
		float wind_gust_factor;
		float wind_blast_in_time;
		float wind_blast_out_time;
		float wind_blast_strength;
		float3 wind_blast_direction;

		~SEffect()
		{
		}
	};
	DEFINE_VECTOR(SEffect*, EffectVec, EffectVecIt);
	struct SSndChannel
	{
		shared_str m_load_section;
		float2 m_sound_dist;
		int4 m_sound_period;

		typedef xr_vector<ref_sound> sounds_type;

		void load(CInifile& config, LPCSTR sect);
		ref_sound& get_rnd_sound()
		{
			return sounds()[Random.randI(sounds().size())];
		}
		u32 get_rnd_sound_time()
		{
			return (m_sound_period.z < m_sound_period.w) ? Random.randI(m_sound_period.z, m_sound_period.w) : 0;
		}
		u32 get_rnd_sound_first_time()
		{
			return (m_sound_period.x < m_sound_period.y) ? Random.randI(m_sound_period.x, m_sound_period.y) : 0;
		}
		float get_rnd_sound_dist()
		{
			return (m_sound_dist.x < m_sound_dist.y) ? Random.randF(m_sound_dist.x, m_sound_dist.y) : 0;
		}
		~SSndChannel()
		{
		}
		IC sounds_type& sounds()
		{
			return m_sounds;
		}

	  protected:
		xr_vector<ref_sound> m_sounds;
	};
	DEFINE_VECTOR(SSndChannel*, SSndChannelVec, SSndChannelVecIt);

  protected:
	shared_str m_load_section;

	EffectVec m_effects;
	int2 m_effect_period;

	SSndChannelVec m_sound_channels;
	shared_str m_ambients_config_filename;

  public:
	IC const shared_str& name()
	{
		return m_load_section;
	}
	IC const shared_str& get_ambients_config_filename()
	{
		return m_ambients_config_filename;
	}

	void load(CInifile& ambients_config, CInifile& sound_channels_config, CInifile& effects_config,
			  const shared_str& section);
	IC SEffect* get_rnd_effect()
	{
		return effects().empty() ? 0 : effects()[Random.randI(effects().size())];
	}
	IC u32 get_rnd_effect_time()
	{
		return Random.randI(m_effect_period.x, m_effect_period.y);
	}

	SEffect* create_effect(CInifile& config, LPCSTR id);
	SSndChannel* create_sound_channel(CInifile& config, LPCSTR id);
	~CEnvAmbient();
	void destroy();
	IC EffectVec& effects()
	{
		return m_effects;
	}
	IC SSndChannelVec& get_snd_channels()
	{
		return m_sound_channels;
	}
};