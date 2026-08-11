#ifndef SoundRender_CoreH
#define SoundRender_CoreH
#pragma once

#include "SoundRender.h"
#include "SoundRender_Cache.h"
#include "soundrender_target.h"

#pragma warning(push)
#pragma warning(disable : 4995)
#include <PresenceAudioSDK/Include/PresenceAudioAPI.h>
#include "../xrGame/PresenceAudioIntegration/Sound_environment_common.h"
#pragma warning(pop)

#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <eax/eax.h>
#include "OpenALDeviceList.h"

#ifdef DEBUG
#define A_CHK(expr)                                                                                                    \
	{                                                                                                                  \
		alGetError();                                                                                                  \
		expr;                                                                                                          \
		ALenum error = alGetError();                                                                                   \
		VERIFY2(error == AL_NO_ERROR, (LPCSTR)alGetString(error));                                                     \
	}
#define AC_CHK(expr)                                                                                                   \
	{                                                                                                                  \
		alcGetError(pDevice);                                                                                          \
		expr;                                                                                                          \
		ALCenum error = alcGetError(pDevice);                                                                          \
		VERIFY2(error == ALC_NO_ERROR, (LPCSTR)alcGetString(pDevice, error));                                          \
	}
#else
#define A_CHK(expr)                                                                                                    \
	{                                                                                                                  \
		expr;                                                                                                          \
	}
#define AC_CHK(expr)                                                                                                   \
	{                                                                                                                  \
		expr;                                                                                                          \
	}
#endif

class CSoundRender_Core : public CSound_manager_interface
{
	volatile BOOL bLocked;

  protected:
	virtual void _create_data(ref_sound_data& S, LPCSTR fName, esound_type sound_type, int game_type);
	virtual void _destroy_data(ref_sound_data& S);

  protected:
	BOOL bListenerMoved;

	// OpenAL specific
	EAXSet eaxSet;
	EAXGet eaxGet;
	ALCdevice* pDevice;
	ALCcontext* pContext;
	ALDeviceList* pDeviceList;

	struct SListener
	{
		fvec3 position;
		fvec3 orientation[2];
	};
	SListener Listener;

	BOOL EAXQuerySupport(BOOL bDeferred, const GUID* guid, u32 prop, void* val, u32 sz);
	BOOL EAXTestSupport(BOOL bDeferred);

  public:
	typedef std::pair<ref_sound_data_ptr, float> event;
	xr_vector<event> s_events;

	// ”казатель на интерфейс из SDK
	Presence::ISoundOcclusionCalculator* m_pOcclusion;

	void SetOcclusion(Presence::ISoundOcclusionCalculator* pOcc)
	{
		m_pOcclusion = pOcc;
	}

  public:
	BOOL bPresent;
	BOOL bUserEnvironment;
	BOOL bEAX;
	BOOL bDeferredEAX;
	BOOL bReady;
	BOOL bDevicePaused;

	CTimer Timer;
	float fTimer_Value;
	float fTimer_Delta;
	float fTime_Factor;
	sound_event* Handler;

	bool bNeedToUpdateEnvironment;
	float fEnvironmentRadius;
	float fFogDensity;

  protected:
	CDB::COLLIDER geom_DB;
	CDB::MODEL* geom_SOM;
	CDB::MODEL* geom_MODEL;

	xr_vector<CSoundRender_Source*> s_sources;
	xr_vector<CSoundRender_Emitter*> s_emitters;
	u32 s_emitters_u;
	xr_vector<CSoundRender_Target*> s_targets;
	xr_vector<CSoundRender_Target*> s_targets_defer;
	u32 s_targets_pu;

	int m_iPauseCounter;

  public:
	CSoundRender_Cache cache;
	u32 cache_bytes_per_line;

  protected:
	virtual void i_eax_set(const GUID* guid, u32 prop, void* val, u32 sz);
	virtual void i_eax_get(const GUID* guid, u32 prop, void* val, u32 sz);

  public:
	CSoundRender_Core();
	virtual ~CSoundRender_Core();

	virtual void _initialize(u64 window);
	virtual void _clear();
	virtual void _restart();

	virtual void create(ref_sound& S, LPCSTR fName, esound_type sound_type, int game_type);
	virtual void attach_tail(ref_sound& S, LPCSTR fName);
	virtual void clone(ref_sound& S, const ref_sound& from, esound_type sound_type, int game_type);
	virtual void destroy(ref_sound& S);
	virtual void stop_emitters();
	virtual int pause_emitters(bool val);

	virtual void play(ref_sound& S, CObject* O, u32 flags = 0, float delay = 0.f);
	virtual void play_at_pos(ref_sound& S, CObject* O, const fvec3& pos, u32 flags = 0, float delay = 0.f);
	virtual void play_no_feedback(ref_sound& S, CObject* O, u32 flags = 0, float delay = 0.f, fvec3* pos = 0,
								  float* vol = 0, float* freq = 0, fvec2* range = 0);
	virtual void set_master_volume(float f);
	virtual void set_geometry_som(IReader* I);
	virtual void set_geometry_occ(CDB::MODEL* M);
	virtual void set_handler(sound_event* E);

	virtual void update(const fvec3& P, const fvec3& D, const fvec3& N);
	virtual void update_events();
	virtual void statistic(CSound_stats* dest, CSound_stats_ext* ext);
	virtual void update_listener(const fvec3& P, const fvec3& D, const fvec3& N, float dt);

	void refresh_sources();
	void UpdateEAX();
	void commit_eax(SEAXEnvironmentData* EAXEnvData);

	const fvec3& listener_position()
	{
		return Listener.position;
	}

  public:
	CSoundRender_Source* i_create_source(LPCSTR name);
	void i_destroy_source(CSoundRender_Source* S);
	CSoundRender_Emitter* i_play(ref_sound* S, BOOL _loop, float delay);
	void i_start(CSoundRender_Emitter* E);
	void i_stop(CSoundRender_Emitter* E);
	void i_rewind(CSoundRender_Emitter* E);
	BOOL i_allow_play(CSoundRender_Emitter* E);
	virtual BOOL i_locked()
	{
		return bLocked;
	}

	virtual void object_relcase(CObject* obj);
	virtual float get_occlusion_to(const fvec3& hear_pt, const fvec3& snd_pt, float dispersion = 0.2f);
	float get_occlusion(fvec3& P, float R, fvec3* occ);

	SEAXEnvironmentData m_EAXEnvData;
	bool b_EAXUpdated;

	void set_device_pause_state(bool paused)
	{
		bDevicePaused = paused;
	};
};
extern CSoundRender_Core* SoundRender;
#endif
