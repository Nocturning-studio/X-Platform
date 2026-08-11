#include "stdafx.h"
#pragma hdrstop

#include "SoundRender_Core.h"

XRSOUND_API xr_token* snd_devices_token = NULL;
XRSOUND_API u32 snd_device_id = u32(-1);

void CSound_manager_interface::_create(u64 window)
{
	SoundRender = xr_new<CSoundRender_Core>();
	Sound = SoundRender;

	if (strstr(Core.Params, "-nosound"))
	{
		psSoundVEffects = 0.0f;
		psSoundVFactor = 0.0f;
		psSoundVMaster = 0.0f;
		psSoundVMusic = 0.0f;
		// SoundRender->bPresent = FALSE;
		// return;
	}
	Sound->_initialize(window);
}

void CSound_manager_interface::_destroy()
{
	Msg("Destroying sound manager interface");
	Sound->_clear();
	xr_delete(SoundRender);
	Sound = 0;
}
