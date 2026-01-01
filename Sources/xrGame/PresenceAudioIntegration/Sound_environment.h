/*
====================================================================================================
  Presence Audio SDK Integration for X-Ray Engine
  File: Sound_environment.h
====================================================================================================
*/
#pragma once

#include <PresenceAudioSDK/PresenceAudioAPI.h>
#include "Sound_environment_common.h"

class XRayGeometryAdapter;

// Глобальные переменные для консольных команд
extern float g_fPresenceRayDist;
extern int g_iPresenceBounces;
extern bool g_bPresenceThreads;
extern float g_fPresenceUpdateRate;

class CSoundEnvironment : public Presence::ISoundOcclusionCalculator
{
  private:
	HMODULE hPresenceAudioSDKLib;
	SEAXEnvironmentData m_CurrentData;

	Presence::AudioSystem* m_pAudioSystem;
	XRayGeometryAdapter* m_pGeometryAdapter;

	bool m_bLoaded;
	bool m_bEnabled;
	bool m_bPaused;

  public:
	CSoundEnvironment();
	~CSoundEnvironment();

	// Основной цикл
	void Update();
	void OnLevelLoad();
	void OnLevelUnload();

	// Управление
	void Pause()
	{
		m_bPaused = true;
	}
	void Play()
	{
		m_bPaused = false;
	}

	// Перезагрузка конфигов материалов на лету (для консольной команды)
	void ReloadMaterials();

	// Передача данных в OpenAL
	void ApplyToSoundDriver(const Presence::EAXResult& res);

	// Интерфейс расчета окклюзии
	virtual float CalculateOcclusion(const Presence::float3& listenerPos, const Presence::float3& sourcePos) override;

	// Геттеры для отладки
	bool IsReady() const
	{
		return m_bLoaded && m_pAudioSystem;
	}
};
