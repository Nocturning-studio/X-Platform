#include "stdafx.h"
#pragma hdrstop

#include "soundrender_target.h"
#include "soundrender_core.h"
#include "soundrender_emitter.h"
#include "soundrender_source.h"

xr_vector<u8> g_target_temp_data;

// OggVorbis callbacks
extern int ov_seek_func(void* datasource, s64 offset, int whence);
extern size_t ov_read_func(void* ptr, size_t size, size_t nmemb, void* datasource);
extern int ov_close_func(void* datasource);
extern long ov_tell_func(void* datasource);

CSoundRender_Target::CSoundRender_Target(void)
{
	pEmitter = 0;
	rendering = FALSE;
	wave = 0;

	// OpenAL specific initialization
	cache_gain = 0.f;
	cache_pitch = 1.f;
	pSource = 0;
	buf_block = 0;

	// Initialize buffers array
	for (u32 i = 0; i < sdef_target_count; i++)
		pBuffers[i] = 0;
}

CSoundRender_Target::~CSoundRender_Target(void)
{
	VERIFY(wave == 0);
}

BOOL CSoundRender_Target::_initialize()
{
	// Initialize OpenAL buffers and source
	A_CHK(alGenBuffers(sdef_target_count, pBuffers));
	alGenSources(1, &pSource);
	ALenum al_error = alGetError();

	if (AL_NO_ERROR == al_error)
	{
		A_CHK(alSourcei(pSource, AL_LOOPING, AL_FALSE));
		A_CHK(alSourcef(pSource, AL_MIN_GAIN, 0.f));
		A_CHK(alSourcef(pSource, AL_MAX_GAIN, 1.f));
		A_CHK(alSourcef(pSource, AL_GAIN, cache_gain));
		A_CHK(alSourcef(pSource, AL_PITCH, cache_pitch));
		return TRUE;
	}
	else
	{
		Msg("! OpenAL: Can't create source. Error: %s.", (LPCSTR)alGetString(al_error));
		return FALSE;
	}
}

void CSoundRender_Target::_destroy()
{
	// Clean up OpenAL resources
	if (alIsSource(pSource))
		alDeleteSources(1, &pSource);
	A_CHK(alDeleteBuffers(sdef_target_count, pBuffers));
}

void CSoundRender_Target::_restart()
{
	_destroy();
	_initialize();
}

void CSoundRender_Target::start(CSoundRender_Emitter* E)
{
	R_ASSERT(E);

	// Initial buffer startup
	pEmitter = E;
	rendering = FALSE;

	// Calc storage for OpenAL
	buf_block = sdef_target_block * E->source()->m_wformat.nAvgBytesPerSec / 1000;
	g_target_temp_data.resize(buf_block);
}

void CSoundRender_Target::render()
{
	// Fill and queue all buffers
	for (u32 buf_idx = 0; buf_idx < sdef_target_count; buf_idx++)
		fill_block(pBuffers[buf_idx]);

	A_CHK(alSourceQueueBuffers(pSource, sdef_target_count, pBuffers));
	A_CHK(alSourcePlay(pSource));

	rendering = TRUE;
}

void CSoundRender_Target::stop()
{
	if (rendering)
	{
		A_CHK(alSourceStop(pSource));
		A_CHK(alSourcei(pSource, AL_BUFFER, NULL));
		A_CHK(alSourcei(pSource, AL_SOURCE_RELATIVE, TRUE));
	}

	dettach();
	pEmitter = NULL;
	rendering = FALSE;
}

void CSoundRender_Target::rewind()
{
	R_ASSERT(rendering);

	A_CHK(alSourceStop(pSource));
	A_CHK(alSourcei(pSource, AL_BUFFER, NULL));

	for (u32 buf_idx = 0; buf_idx < sdef_target_count; buf_idx++)
		fill_block(pBuffers[buf_idx]);

	A_CHK(alSourceQueueBuffers(pSource, sdef_target_count, pBuffers));
	A_CHK(alSourcePlay(pSource));
}

void CSoundRender_Target::update()
{
	R_ASSERT(pEmitter);

	ALint processed, state;

	// Get status
	A_CHK(alGetSourcei(pSource, AL_SOURCE_STATE, &state));
	A_CHK(alGetSourcei(pSource, AL_BUFFERS_PROCESSED, &processed));

	if (alGetError() != AL_NO_ERROR)
	{
		Msg("!![%s] Source state error", __FUNCTION__);
		return;
	}

	// Process processed buffers
	while (processed)
	{
		ALuint BufferID;
		A_CHK(alSourceUnqueueBuffers(pSource, 1, &BufferID));
		fill_block(BufferID);
		A_CHK(alSourceQueueBuffers(pSource, 1, &BufferID));
		processed--;

		if (alGetError() != AL_NO_ERROR)
		{
			Msg("!![%s] Buffer queue error", __FUNCTION__);
			return;
		}
	}

	// Check for underruns and restart if needed
	if (state != AL_PLAYING && state != AL_PAUSED)
	{
		ALint queued;
		alGetSourcei(pSource, AL_BUFFERS_QUEUED, &queued);

		if (queued)
		{
			alSourcePlay(pSource);
			if (alGetError() != AL_NO_ERROR)
			{
				Msg("!![%s] Playback restart error", __FUNCTION__);
				return;
			}
		}
	}
}

void CSoundRender_Target::fill_parameters()
{
	VERIFY(pEmitter);
	CSoundRender_Emitter* SE = pEmitter;

	// 3D parameters
	VERIFY2(pEmitter, SE->source()->file_name());
	A_CHK(alSourcef(pSource, AL_REFERENCE_DISTANCE, pEmitter->p_source.min_distance));

	VERIFY2(pEmitter, SE->source()->file_name());
	A_CHK(alSourcef(pSource, AL_MAX_DISTANCE, pEmitter->p_source.max_distance));

	VERIFY2(pEmitter, SE->source()->file_name());
	A_CHK(alSource3f(pSource, AL_POSITION, pEmitter->p_source.position.x, pEmitter->p_source.position.y,
					 -pEmitter->p_source.position.z));

	VERIFY2(pEmitter, SE->source()->file_name());
	A_CHK(alSourcei(pSource, AL_SOURCE_RELATIVE, pEmitter->b2D));

	A_CHK(alSourcef(pSource, AL_ROLLOFF_FACTOR, psSoundRolloff));

	// Gain (volume)
	VERIFY2(pEmitter, SE->source()->file_name());
	float _gain = pEmitter->smooth_volume;
	clamp(_gain, EPS_S, 1.f);
	if (!fsimilar(_gain, cache_gain))
	{
		cache_gain = _gain;
		A_CHK(alSourcef(pSource, AL_GAIN, _gain));
	}

	// Pitch (frequency)
	VERIFY2(pEmitter, SE->source()->file_name());
	float _pitch = pEmitter->p_source.freq;

	if (pEmitter->p_source.use_pitch)
		_pitch *= psTimeFactor;

	clamp(_pitch, EPS_L, 2.f);
	if (!fsimilar(_pitch, cache_pitch))
	{
		cache_pitch = _pitch;
		A_CHK(alSourcef(pSource, AL_PITCH, _pitch));
	}
}

void CSoundRender_Target::fill_block(ALuint BufferID)
{
	R_ASSERT(pEmitter);
	pEmitter->fill_block(&g_target_temp_data.front(), buf_block);

	ALuint format = (pEmitter->source()->m_wformat.nChannels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	A_CHK(alBufferData(BufferID, format, &g_target_temp_data.front(), buf_block,
					   pEmitter->source()->m_wformat.nSamplesPerSec));
}

void CSoundRender_Target::attach()
{
	VERIFY(0 == wave);
	VERIFY(pEmitter);

	ov_callbacks ovc = {ov_read_func, ov_seek_func, ov_close_func, ov_tell_func};
	wave = FS.r_open(pEmitter->source()->pname.c_str());
	R_ASSERT3(wave && wave->length(), "Can't open wave file:", pEmitter->source()->pname.c_str());
	ov_open_callbacks(wave, &ovf, NULL, 0, ovc);
}

void CSoundRender_Target::dettach()
{
	if (wave)
	{
		ov_clear(&ovf);
		FS.r_close(wave);
		wave = 0;
	}
}

void CSoundRender_Target::source_changed()
{
	dettach();
	attach();
}
