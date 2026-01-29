#include "stdafx.h"
#pragma hdrstop

#include "soundrender_core.h"
#pragma warning(push)
#pragma warning(disable : 4995)
#include <eax/eax.h>
#pragma warning(pop)

BOOL CSoundRender_Core::EAXQuerySupport(BOOL bDeferred, const GUID* guid, u32 prop, void* val, u32 sz)
{
	if (AL_NO_ERROR != eaxGet(guid, prop, 0, val, sz))
		return FALSE;

	if (AL_NO_ERROR != eaxSet(guid, (bDeferred ? DSPROPERTY_EAXLISTENER_DEFERRED : 0) | prop, 0, val, sz))
		return FALSE;

	return TRUE;
}

BOOL CSoundRender_Core::EAXTestSupport(BOOL bDeferred)
{
	EAXLISTENERPROPERTIES ep;

	if (!EAXQuerySupport(bDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOM, &ep.lRoom,
						 sizeof(LONG)))
		return FALSE;
	if (!EAXQuerySupport(bDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOMHF, &ep.lRoomHF,
						 sizeof(LONG)))
		return FALSE;
	if (!EAXQuerySupport(bDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOMROLLOFFFACTOR,
						 &ep.flRoomRolloffFactor, sizeof(float)))
		return FALSE;
	if (!EAXQuerySupport(bDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_DECAYTIME,
						 &ep.flDecayTime, sizeof(float)))
		return FALSE;
	if (!EAXQuerySupport(bDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_DECAYHFRATIO,
						 &ep.flDecayHFRatio, sizeof(float)))
		return FALSE;
	if (!EAXQuerySupport(bDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_REFLECTIONS,
						 &ep.lReflections, sizeof(LONG)))
		return FALSE;
	if (!EAXQuerySupport(bDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_REFLECTIONSDELAY,
						 &ep.flReflectionsDelay, sizeof(float)))
		return FALSE;
	if (!EAXQuerySupport(bDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_REVERB, &ep.lReverb,
						 sizeof(LONG)))
		return FALSE;
	if (!EAXQuerySupport(bDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_REVERBDELAY,
						 &ep.flReverbDelay, sizeof(float)))
		return FALSE;
	if (!EAXQuerySupport(bDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENTDIFFUSION,
						 &ep.flEnvironmentDiffusion, sizeof(float)))
		return FALSE;
	if (!EAXQuerySupport(bDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF,
						 &ep.flAirAbsorptionHF, sizeof(float)))
		return FALSE;
	if (!EAXQuerySupport(bDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_FLAGS, &ep.dwFlags,
						 sizeof(DWORD)))
		return FALSE;

	return TRUE;
}

void CSoundRender_Core::i_eax_set(const GUID* guid, u32 prop, void* val, u32 sz)
{
	if (bEAX)
		eaxSet(guid, prop, 0, val, sz);
}

void CSoundRender_Core::i_eax_get(const GUID* guid, u32 prop, void* val, u32 sz)
{
	if (bEAX)
		eaxGet(guid, prop, 0, val, sz);
}

void CSoundRender_Core::UpdateEAX()
{
	if (!bEAX)
		return;

	EAXLISTENERPROPERTIES ep;
	ep.lRoom = DSPROPERTY_EAXLISTENER_ROOM;
	ep.lRoomHF = DSPROPERTY_EAXLISTENER_ROOMHF;
	ep.flRoomRolloffFactor = DSPROPERTY_EAXLISTENER_ROOMROLLOFFFACTOR;
	ep.flDecayTime = DSPROPERTY_EAXLISTENER_DECAYTIME;
	ep.flDecayHFRatio = DSPROPERTY_EAXLISTENER_DECAYHFRATIO;
	ep.lReflections = DSPROPERTY_EAXLISTENER_REFLECTIONS;
	ep.flReflectionsDelay = DSPROPERTY_EAXLISTENER_REFLECTIONSDELAY;
	ep.lReverb = DSPROPERTY_EAXLISTENER_REVERB;
	ep.flReverbDelay = DSPROPERTY_EAXLISTENER_REVERBDELAY;
	ep.flEnvironmentDiffusion = DSPROPERTY_EAXLISTENER_ENVIRONMENTDIFFUSION;
	ep.flAirAbsorptionHF = DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF;
	ep.dwFlags = DSPROPERTY_EAXLISTENER_FLAGS;
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOM, &ep.lRoom, sizeof(LONG));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOMHF, &ep.lRoomHF, sizeof(LONG));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOMROLLOFFFACTOR,
				&ep.flRoomRolloffFactor, sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_DECAYTIME, &ep.flDecayTime,
				sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_DECAYHFRATIO, &ep.flDecayHFRatio,
				sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_REFLECTIONS, &ep.lReflections,
				sizeof(LONG));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_REFLECTIONSDELAY, &ep.flReflectionsDelay,
				sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_REVERB, &ep.lReverb, sizeof(LONG));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_REVERBDELAY, &ep.flReverbDelay,
				sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENTDIFFUSION,
				&ep.flEnvironmentDiffusion, sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF, &ep.flAirAbsorptionHF,
				sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_FLAGS, &ep.dwFlags, sizeof(DWORD));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENTSIZE,
			  &ep.flEnvironmentSize, sizeof(float));
}

void CSoundRender_Core::commit_eax(SEAXEnvironmentData* EAXEnvData)
{
	const SEAXEnvironmentData& env = *EAXEnvData;

	EAXLISTENERPROPERTIES ep;
	ZeroMemory(&ep, sizeof(ep));

	ep.lRoom = env.lRoom;
	ep.lRoomHF = env.lRoomHF;
	ep.flRoomRolloffFactor = env.flRoomRolloffFactor;
	ep.flDecayTime = env.flDecayTime;
	ep.flDecayHFRatio = env.flDecayHFRatio;
	ep.lReflections = env.lReflections;
	ep.flReflectionsDelay = env.flReflectionsDelay;
	ep.lReverb = env.lReverb;
	ep.flReverbDelay = env.flReverbDelay;
	ep.flEnvironmentSize = env.flEnvironmentSize;
	ep.flEnvironmentDiffusion = env.flEnvironmentDiffusion;
	ep.flAirAbsorptionHF = env.flAirAbsorptionHF;
	ep.dwFlags = env.dwFlags;

	// Отладочный вывод
	//Msg("[EAX COMMIT] Decay: %.2f, Room: %d, Refl: %d, Rev: %d", ep.flDecayTime, ep.lRoom, ep.lReflections, ep.lReverb);

	u32 deferred = bDeferredEAX ? DSPROPERTY_EAXLISTENER_DEFERRED : 0;

	// Установка параметров Listener
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_ROOM, &ep.lRoom, sizeof(LONG));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_ROOMHF, &ep.lRoomHF, sizeof(LONG));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_DECAYTIME, &ep.flDecayTime,
			  sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_REFLECTIONS, &ep.lReflections,
			  sizeof(LONG));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_REFLECTIONSDELAY,
			  &ep.flReflectionsDelay, sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_REVERB, &ep.lReverb, sizeof(LONG));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_ROOMROLLOFFFACTOR,
			  &ep.flRoomRolloffFactor, sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_DECAYHFRATIO, &ep.flDecayHFRatio,
			  sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_REVERBDELAY, &ep.flReverbDelay,
			  sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_ENVIRONMENTDIFFUSION,
			  &ep.flEnvironmentDiffusion, sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF,
			  &ep.flAirAbsorptionHF, sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_FLAGS, &ep.dwFlags, sizeof(DWORD));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_ENVIRONMENTSIZE, &ep.flEnvironmentSize, sizeof(float));

	if (bDeferredEAX)
	{
		i_eax_set(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_COMMITDEFERREDSETTINGS, NULL, 0);
	}

	if (bEAX && eaxSet)
	{
		LONG lSendLevel = -1000;

		for (u32 tit = 0; tit < s_targets.size(); tit++)
		{
			CSoundRender_Target* T = s_targets[tit];
			if (T->get_emitter())
			{
				eaxSet(&DSPROPSETID_EAX_BufferProperties, DSPROPERTY_EAXBUFFER_ROOM, T->pSource, &lSendLevel, sizeof(LONG));
			}
		}
	}
}
