#ifndef SoundRender_TargetH
#define SoundRender_TargetH
#pragma once

#include "soundrender.h"
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <libvorbis/include/vorbis/vorbisfile.h>

class CSoundRender_Target
{
  protected:
	CSoundRender_Emitter* pEmitter;
	BOOL rendering;

  public:
	// OpenAL specific
	ALuint pSource;
	ALuint pBuffers[sdef_target_count];
	float cache_gain;
	float cache_pitch;
	ALuint buf_block;

	float priority;

  protected:
	OggVorbis_File ovf;
	IReader* wave;

  public:
	void attach();
	void dettach();
	void fill_block(ALuint BufferID);
	void source_changed();

	OggVorbis_File* get_data()
	{
		if (!wave)
			attach();
		return &ovf;
	}

  public:
	CSoundRender_Target(void);
	virtual ~CSoundRender_Target(void);

	CSoundRender_Emitter* get_emitter()
	{
		return pEmitter;
	}
	BOOL get_Rendering()
	{
		return rendering;
	}

	BOOL _initialize();
	void _destroy();
	void _restart();

	void start(CSoundRender_Emitter* E);
	void render();
	void rewind();
	void stop();
	void update();
	void fill_parameters();
};
#endif
