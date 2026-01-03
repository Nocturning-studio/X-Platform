#include "stdafx.h"
#pragma hdrstop

#include "soundrender_core.h"
#include "soundrender_source.h"
#include "soundrender_emitter.h"
#pragma warning(push)
#pragma warning(disable : 4995)
#include <eax/eax.h>
#include <xrLevel.h>
#pragma warning(pop)

int psSoundTargets = 64;
Flags32 psSoundFlags = {ss_EAX};
float psSoundOcclusionScale = 0.5f;
float psSoundCull = 0.01f;
float psSoundRolloff = 0.75f;
u32 psSoundModel = 0;
float psSoundVEffects = 1.0f;
float psSoundVFactor = 1.0f;
float psSoundVMaster = 1.0f;

float psSoundVMusic = 0.7f;
float psSoundVWeaponShooting = 0.7f;
float psSoundVAmbient = 1.0f;
int psSoundCacheSizeMB = 64;

float psTimeFactor = 1.0f;

float psDbgEAXRoom = EAXLISTENER_DEFAULTROOM;
float psDbgEAXRoomHF = -2000.0f;
float psDbgEAXRoomRolloff = EAXLISTENER_DEFAULTROOMROLLOFFFACTOR;
float psDbgEAXDecayTime = EAXLISTENER_DEFAULTDECAYTIME;
float psDbgEAXDecayHFRatio = EAXLISTENER_DEFAULTDECAYHFRATIO;
float psDbgEAXReflections = EAXLISTENER_DEFAULTREFLECTIONS;
float psDbgEAXReflectionsDelay = EAXLISTENER_DEFAULTREFLECTIONSDELAY;
float psDbgEAXReverb = EAXLISTENER_DEFAULTREVERB;
float psDbgEAXReverbDelay = EAXLISTENER_DEFAULTREVERBDELAY;
float psDbgEAXEnvironmentSize = EAXLISTENER_DEFAULTENVIRONMENTSIZE;
float psDbgEAXEnvironmentDiffusion = EAXLISTENER_DEFAULTENVIRONMENTDIFFUSION;

CSoundRender_Core* SoundRender = 0;
CSound_manager_interface* Sound = 0;

CSoundRender_Core::CSoundRender_Core()
{
	bPresent = FALSE;
	bEAX = FALSE;
	bDeferredEAX = FALSE;
	bUserEnvironment = FALSE;
	geom_MODEL = NULL;
	geom_SOM = NULL;
	Handler = NULL;
	s_targets_pu = 0;
	s_emitters_u = 0;
	bListenerMoved = FALSE;
	bReady = FALSE;
	bLocked = FALSE;
	fTimer_Value = Timer.GetElapsed_sec();
	fTimer_Delta = 0.0f;
	fTime_Factor = 1.0f;
	m_iPauseCounter = 1;
	bDevicePaused = FALSE;
	fFogDensity = 0.0f;
	fEnvironmentRadius = 0.0f;
	bNeedToUpdateEnvironment = FALSE;
	b_EAXUpdated = false;

	pDevice = 0;
	pDeviceList = 0;
	pContext = 0;
	eaxSet = 0;
	eaxGet = 0;

	m_pOcclusion = nullptr;
}

CSoundRender_Core::~CSoundRender_Core()
{
#ifdef _EDITOR
	ETOOLS::destroy_model(geom_SOM);
#else
	xr_delete(geom_SOM);
#endif
}

void CSoundRender_Core::_initialize(u64 window)
{
	Msg("\nStarting Sound Engine...");

	bPresent = FALSE;

	Msg("Initializing OpenAL...");
	pDeviceList = xr_new<ALDeviceList>();

	if (0 == pDeviceList->GetNumDevices())
	{
		Log("* OpenAL: Can't create sound device.");
		xr_delete(pDeviceList);
		return;
	}

	pDeviceList->SelectBestDevice();

	R_ASSERT(snd_device_id >= 0 && snd_device_id < pDeviceList->GetNumDevices());
	const ALDeviceDesc& deviceDesc = pDeviceList->GetDeviceDesc(snd_device_id);

	pDevice = alcOpenDevice(deviceDesc.name.c_str());

	if (!pDevice)
	{
		Log(" OpenAL: Failed to create device.");
		bPresent = FALSE;
		return;
	}

	const ALCchar* deviceSpecifier;
	deviceSpecifier = alcGetString(pDevice, ALC_DEVICE_SPECIFIER);
	Msg("* OpenAL: Required device: %s", deviceDesc.name.c_str());
	Msg("* OpenAL: Created device: %s", deviceSpecifier);

	pContext = alcCreateContext(pDevice, NULL);

	if (!pContext)
	{
		Log(" OpenAL: Failed to create context.");
		bPresent = FALSE;
		alcCloseDevice(pDevice);
		pDevice = 0;
		return;
	}

	alGetError();
	alcGetError(pDevice);

	AC_CHK(alcMakeContextCurrent(pContext));

	A_CHK(alListener3f(AL_POSITION, 0.f, 0.f, 0.f));
	A_CHK(alListener3f(AL_VELOCITY, 0.f, 0.f, 0.f));
	Fvector orient[2] = {{0.f, 0.f, 1.f}, {0.f, 1.f, 0.f}};
	A_CHK(alListenerfv(AL_ORIENTATION, &orient[0].x));
	A_CHK(alListenerf(AL_GAIN, 0.5f));

	bEAX = true;
	eaxSet = (EAXSet)alGetProcAddress((const ALchar*)"EAXSet");
	if (eaxSet == NULL)
		bEAX = false;
	eaxGet = (EAXGet)alGetProcAddress((const ALchar*)"EAXGet");
	if (eaxGet == NULL)
		bEAX = false;

	if (bEAX)
	{
		bDeferredEAX = EAXTestSupport(TRUE);
		bEAX = EAXTestSupport(FALSE);

		if (bEAX)
			Msg("-  OpenAL: EAX Supported");
		else
			Msg("!  OpenAL: EAX Unsupported");
		if (bDeferredEAX)
			Msg("!  OpenAL: EAX Deffered");
	}

	Log("* Sound: EAX 2.0 extension:", bEAX ? "present" : "absent");
	Log("* Sound: EAX 2.0 deferred:", bDeferredEAX ? "present" : "absent");
	Timer.Start();

	bPresent = TRUE;

	cache_bytes_per_line = (sdef_target_block / 8) * 276400 / 1000;
	cache.initialize(psSoundCacheSizeMB * 1024, cache_bytes_per_line);

	bReady = TRUE;

	CSoundRender_Target* T = nullptr;
	for (u32 tit = 0; tit < u32(psSoundTargets); tit++)
	{
		T = xr_new<CSoundRender_Target>();
		if (T->_initialize())
		{
			s_targets.push_back(T);
		}
		else
		{
			Log("!  OpenAL: Max targets - ", tit);
			T->_destroy();
			xr_delete(T);
			break;
		}
	}
}

extern xr_vector<u8> g_target_temp_data;

void CSoundRender_Core::_clear()
{
	bReady = FALSE;
	cache.destroy();

	for (u32 sit = 0; sit < s_sources.size(); sit++)
		xr_delete(s_sources[sit]);
	s_sources.clear();

	for (u32 eit = 0; eit < s_emitters.size(); eit++)
		xr_delete(s_emitters[eit]);
	s_emitters.clear();

	g_target_temp_data.clear();

	CSoundRender_Target* T = 0;
	for (u32 tit = 0; tit < s_targets.size(); tit++)
	{
		T = s_targets[tit];
		T->_destroy();
		xr_delete(T);
	}

	alcMakeContextCurrent(NULL);
	alcDestroyContext(pContext);
	pContext = 0;
	alcCloseDevice(pDevice);
	pDevice = 0;
	xr_delete(pDeviceList);
}

void CSoundRender_Core::_restart()
{
	cache.destroy();
	cache.initialize(psSoundCacheSizeMB * 1024, cache_bytes_per_line);
}

void CSoundRender_Core::set_master_volume(float f)
{
	if (bPresent)
	{
		A_CHK(alListenerf(AL_GAIN, f));
	}
}

void CSoundRender_Core::update_listener(const Fvector& P, const Fvector& D, const Fvector& N, float dt)
{
	if (!Listener.position.similar(P))
	{
		Listener.position.set(P);
		bListenerMoved = TRUE;
	}
	else
	{
		bListenerMoved = FALSE;
	}

	Listener.orientation[0].set(D.x, D.y, -D.z);
	Listener.orientation[1].set(N.x, N.y, -N.z);

	A_CHK(alListener3f(AL_POSITION, Listener.position.x, Listener.position.y, -Listener.position.z));
	A_CHK(alListener3f(AL_VELOCITY, 0.f, 0.f, 0.f));
	A_CHK(alListenerfv(AL_ORIENTATION, &Listener.orientation[0].x));
}

void CSoundRender_Core::stop_emitters()
{
	for (u32 eit = 0; eit < s_emitters.size(); eit++)
		s_emitters[eit]->stop(FALSE);
}

int CSoundRender_Core::pause_emitters(bool val)
{
	m_iPauseCounter += val ? +1 : -1;
	VERIFY(m_iPauseCounter >= 0);

	for (u32 it = 0; it < s_emitters.size(); it++)
		((CSoundRender_Emitter*)s_emitters[it])->pause(val, val ? m_iPauseCounter : m_iPauseCounter + 1);

	return m_iPauseCounter;
}

void CSoundRender_Core::set_handler(sound_event* E)
{
	Handler = E;
}

void CSoundRender_Core::set_geometry_occ(CDB::MODEL* M)
{
	geom_MODEL = M;
}

void CSoundRender_Core::set_geometry_som(IReader* I)
{
#ifdef _EDITOR
	ETOOLS::destroy_model(geom_SOM);
#else
	xr_delete(geom_SOM);
#endif

	if (0 == I)
		return;

	// check version
	R_ASSERT(I->find_chunk(0));
	u32 version = I->r_u32();
	VERIFY2(version == 0, "Invalid SOM version");

	// load geometry
	IReader* geom = I->open_chunk(1);
	VERIFY2(geom, "Corrupted SOM file");

	// Load tris and merge them
	struct SOM_poly
	{
		Fvector3 v1;
		Fvector3 v2;
		Fvector3 v3;
		u32 b2sided;
		float occ;
	};

	// Create AABB-tree
#ifdef _EDITOR
	CDB::Collector* CL = ETOOLS::create_collector();
	while (!geom->eof())
	{
		SOM_poly P;
		geom->r(&P, sizeof(P));
		ETOOLS::collector_add_face_pd(CL, P.v1, P.v2, P.v3, *(u32*)&P.occ, 0.01f);
		if (P.b2sided)
			ETOOLS::collector_add_face_pd(CL, P.v3, P.v2, P.v1, *(u32*)&P.occ, 0.01f);
	}
	geom_SOM = ETOOLS::create_model_cl(CL);
	ETOOLS::destroy_collector(CL);
#else
	CDB::Collector CL;
	while (!geom->eof())
	{
		SOM_poly P;
		geom->r(&P, sizeof(P));
		CL.add_face_packed_D(P.v1, P.v2, P.v3, *(u32*)&P.occ, 0.01f);
		if (P.b2sided)
			CL.add_face_packed_D(P.v3, P.v2, P.v1, *(u32*)&P.occ, 0.01f);
	}
	geom_SOM = xr_new<CDB::MODEL>();
	geom_SOM->build(CL.getV(), int(CL.getVS()), CL.getT(), int(CL.getTS()));
#endif
}

void CSoundRender_Core::create(ref_sound& S, const char* fName, esound_type sound_type, int game_type)
{
	if (!bPresent)
		return;

	S._p = xr_new<ref_sound_data>(fName, sound_type, game_type);
}

void CSoundRender_Core::attach_tail(ref_sound& S, const char* fName)
{
	if (!bPresent)
		return;

	string_path fn;
	strcpy_s(fn, fName);

	if (strext(fn))
		*strext(fn) = 0;

	if (S._p->fn_attached[0].size() && S._p->fn_attached[1].size())
	{
#ifdef DEBUG
		Msg("! 2 file already in queue [%s][%s]", S._p->fn_attached[0].c_str(), S._p->fn_attached[1].c_str());
#endif // #ifdef DEBUG
		return;
	}

	u32 idx = S._p->fn_attached[0].size() ? 1 : 0;

	S._p->fn_attached[idx] = fn;

	CSoundRender_Source* s = SoundRender->i_create_source(fn);
	S._p->dwBytesTotal += s->bytes_total();
	S._p->fTimeTotal += s->length_sec();

	if (S._feedback())
		((CSoundRender_Emitter*)S._feedback())->fTimeToStop += s->length_sec();

	SoundRender->i_destroy_source(s);
}

void CSoundRender_Core::clone(ref_sound& S, const ref_sound& from, esound_type sound_type, int game_type)
{
	if (!bPresent)
		return;

	S._p = xr_new<ref_sound_data>();
	S._p->handle = from._p->handle;
	S._p->dwBytesTotal = from._p->dwBytesTotal;
	S._p->fTimeTotal = from._p->fTimeTotal;
	S._p->fn_attached[0] = from._p->fn_attached[0];
	S._p->fn_attached[1] = from._p->fn_attached[1];
	S._p->g_type = (game_type == sg_SourceType) ? S._p->handle->game_type() : game_type;
	S._p->s_type = sound_type;
}

void CSoundRender_Core::play(ref_sound& S, CObject* O, u32 flags, float delay)
{
	if (!bPresent || 0 == S._handle())
		return;

	S._p->g_object = O;

	if (S._feedback())
		((CSoundRender_Emitter*)S._feedback())->rewind();
	else
		i_play(&S, flags & sm_Looped, delay);

	if (flags & sm_NoPitch)
		S._feedback()->set_pitch_using(false);

	if (flags & sm_2D || S._handle()->channels_num() == 2)
		S._feedback()->switch_to_2D();
}

void CSoundRender_Core::play_no_feedback(ref_sound& S, CObject* O, u32 flags, float delay, Fvector* pos, float* vol,
										 float* freq, Fvector2* range)
{
	if (!bPresent || 0 == S._handle())
		return;

	ref_sound_data_ptr orig = S._p;
	S._p = xr_new<ref_sound_data>();
	S._p->handle = orig->handle;
	S._p->g_type = orig->g_type;
	S._p->g_object = O;
	S._p->dwBytesTotal = orig->dwBytesTotal;
	S._p->fTimeTotal = orig->fTimeTotal;
	S._p->fn_attached[0] = orig->fn_attached[0];
	S._p->fn_attached[1] = orig->fn_attached[1];

	i_play(&S, flags & sm_Looped, delay);

	if (flags & sm_2D || S._handle()->channels_num() == 2)
		S._feedback()->switch_to_2D();

	if (flags & sm_NoPitch)
		S._feedback()->set_pitch_using(false);

	if (pos)
		S._feedback()->set_position(*pos);

	if (freq)
		S._feedback()->set_frequency(*freq);

	if (range)
		S._feedback()->set_range((*range)[0], (*range)[1]);

	if (vol)
		S._feedback()->set_volume(*vol);

	S._p = orig;
}

void CSoundRender_Core::play_at_pos(ref_sound& S, CObject* O, const Fvector& pos, u32 flags, float delay)
{
	if (!bPresent || 0 == S._handle())
		return;

	S._p->g_object = O;

	if (S._feedback())
		((CSoundRender_Emitter*)S._feedback())->rewind();
	else
		i_play(&S, flags & sm_Looped, delay);

	S._feedback()->set_position(pos);

	if (flags & sm_NoPitch)
		S._feedback()->set_pitch_using(false);

	if (flags & sm_2D || S._handle()->channels_num() == 2)
		S._feedback()->switch_to_2D();
}

void CSoundRender_Core::destroy(ref_sound& S)
{
	if (S._feedback())
	{
		CSoundRender_Emitter* E = (CSoundRender_Emitter*)S._feedback();
		E->stop(FALSE);
	}

	S._p = 0;
}

void CSoundRender_Core::_create_data(ref_sound_data& S, LPCSTR fName, esound_type sound_type, int game_type)
{
	string_path fn;
	strcpy(fn, fName);

	if (strext(fn))
		*strext(fn) = 0;

	S.handle = (CSound_source*)SoundRender->i_create_source(fn);
	S.g_type = (game_type == sg_SourceType) ? S.handle->game_type() : game_type;
	S.s_type = sound_type;
	S.feedback = nullptr;
	S.g_object = nullptr;
	S.g_userdata = nullptr;
	S.dwBytesTotal = S.handle->bytes_total();
	S.fTimeTotal = S.handle->length_sec();
}

void CSoundRender_Core::_destroy_data(ref_sound_data& S)
{
	if (S.feedback)
	{
		CSoundRender_Emitter* E = (CSoundRender_Emitter*)S.feedback;
		E->stop(FALSE);
	}

	R_ASSERT(0 == S.feedback);
	SoundRender->i_destroy_source((CSoundRender_Source*)S.handle);
	S.handle = NULL;
}

void CSoundRender_Core::object_relcase(CObject* obj)
{
	if (!obj)
		return;

	for (u32 eit = 0; eit < s_emitters.size(); eit++)
	{
		if (s_emitters[eit])
			if (s_emitters[eit]->owner_data)
				if (obj == s_emitters[eit]->owner_data->g_object)
					s_emitters[eit]->owner_data->g_object = 0;
	}
}

void CSoundRender_Core::refresh_sources()
{
	for (u32 eit = 0; eit < s_emitters.size(); eit++)
		s_emitters[eit]->stop(FALSE);

	for (u32 sit = 0; sit < s_sources.size(); sit++)
	{
		CSoundRender_Source* s = s_sources[sit];
		s->unload();
		s->load(*s->fname);
	}
}
