/*
====================================================================================================
  Presence Audio SDK Integration for X-Ray Engine
  File: Sound_environment.cpp
====================================================================================================
*/

#ifdef GetVersionString
#undef GetVersionString
#endif

#include <PresenceAudioSDK/PresenceAudioAPI.h>
#include "stdafx.h"
#pragma hdrstop

#include "Sound_environment.h"
#include "Sound_environment_geometry_provider.h"
#include "../../xrEngine/IGame_Persistent.h"
#include "../../xrEngine/Environment.h"
#include "../../xrEngine/xr_ioconsole.h" // Для регистрации команд
#include "../../xrSound/SoundRender_Core.h"

// -------------------------------------------------------------------------------------------------
// Console Variables (Default Values)
// -------------------------------------------------------------------------------------------------
float g_fPresenceRayDist = 150.0f;
int g_iPresenceBounces = 5;
bool g_bPresenceThreads = true;
float g_fPresenceUpdateRate = 0.033f;

// =================================================================================================
// Constructor / Destructor
// =================================================================================================

CSoundEnvironment::CSoundEnvironment()
	: m_pAudioSystem(nullptr), m_pGeometryAdapter(nullptr), m_bLoaded(false), m_bEnabled(false), m_bPaused(false)
{
	Msg("[Presence Audio] Initializing SDK wrapper...");

	// Загрузка DLL
	LPCSTR LibName = "PresenceAudioSDK.dll";
	hPresenceAudioSDKLib = LoadLibrary(LibName);
	if (!hPresenceAudioSDKLib)
	{
		Msg("! [Presence Audio] Error: Can't load %s", LibName);
		return;
	}

	ZeroMemory(&m_CurrentData, sizeof(m_CurrentData));

	m_pAudioSystem = new Presence::AudioSystem();
	m_pGeometryAdapter = new XRayGeometryAdapter();

	Msg("- Presence Audio SDK loaded.");
}

CSoundEnvironment::~CSoundEnvironment()
{
	if (m_bLoaded)
		OnLevelUnload();

	if (m_pAudioSystem)
	{
		m_pAudioSystem->Shutdown();
		delete m_pAudioSystem;
		m_pAudioSystem = nullptr;
	}
	if (m_pGeometryAdapter)
	{
		delete m_pGeometryAdapter;
		m_pGeometryAdapter = nullptr;
	}
	if (hPresenceAudioSDKLib)
	{
		FreeLibrary(hPresenceAudioSDKLib);
		hPresenceAudioSDKLib = 0;
	}
}

// =================================================================================================
// Lifecycle
// =================================================================================================

void CSoundEnvironment::OnLevelLoad()
{
	if (!m_pAudioSystem || !m_pGeometryAdapter)
		return;

	Msg("[Presence Audio] Starting simulation...");

	// Применяем настройки из консоли
	Presence::Settings s;
	s.maxBounces = g_iPresenceBounces;
	s.maxRayDistance = g_fPresenceRayDist;
	s.useMultithreading = g_bPresenceThreads;
	s.updateInterval = g_fPresenceUpdateRate;

	// Строим кэш материалов
	m_pGeometryAdapter->BuildMaterialCache(m_pAudioSystem);

	// Инициализация
	m_pAudioSystem->Initialize(m_pGeometryAdapter, s);
	Msg("[Presence Audio] Core Version: %s", m_pAudioSystem->GetVersionString());

	m_bLoaded = true;
	m_bEnabled = true;

	// Внедрение в движок звука
	if (::Sound)
	{
		CSoundRender_Core* pCore = (CSoundRender_Core*)::Sound;
		pCore->SetOcclusion(this);
		Msg("[Presence Audio] Hooked into xrSound.");
	}
}

void CSoundEnvironment::OnLevelUnload()
{
	if (::Sound)
	{
		CSoundRender_Core* pCore = (CSoundRender_Core*)::Sound;
		pCore->SetOcclusion(nullptr); // Важно! Убираем хук
	}

	if (m_pAudioSystem)
	{
		m_pAudioSystem->Shutdown();
	}

	m_bLoaded = false;
	Msg("[Presence Audio] Simulation stopped.");
}

void CSoundEnvironment::ReloadMaterials()
{
	if (!m_bLoaded || !m_pAudioSystem || !m_pGeometryAdapter)
		return;

	Msg("[Presence Audio] Hot-reloading materials...");
	// Форсируем перестройку кэша
	m_pGeometryAdapter->m_bCacheBuilt = false;
	m_pGeometryAdapter->BuildMaterialCache(m_pAudioSystem);
}

// =================================================================================================
// Occlusion Logic
// =================================================================================================

float CSoundEnvironment::CalculateOcclusion(const Presence::float3& listenerPos, const Presence::float3& sourcePos)
{
	// Быстрая проверка
	if (!m_bLoaded || !m_pAudioSystem || m_bPaused)
		return 1.0f;

	return m_pAudioSystem->CalculateOcclusion(listenerPos, sourcePos);
}

// =================================================================================================
// Update Loop
// =================================================================================================

void CSoundEnvironment::Update()
{
	PROFILE_FUNCTION();

	// 1. Проверка состояния
	if (!psSoundFlags.test(ss_EAX) || !m_bLoaded || !m_pAudioSystem || m_bPaused)
	{
		if (m_bEnabled)
		{
			// Если выключили на лету - сбрасываем эффекты в ноль
			Presence::EAXResult emptyRes;
			ApplyToSoundDriver(emptyRes);
			m_bEnabled = false;
		}
		return;
	}

	if (!g_pGameLevel)
		return;
	if (!m_bEnabled)
		m_bEnabled = true;

	// 2. Сбор данных окружения
	fvec3 pos = Engine.RenderView.Position;

	// Конвертация координат: X-Ray (Y-up) -> Presence (Y-up).
	// Обычно оси совпадают, но если SDK ожидает Z-up, тут нужно менять.
	Presence::float3 camPos(pos.x, pos.y, pos.z);

	float dt = Engine.TimeManager.GetDeltaTime();

	// Получаем погоду (туман влияет на поглощение ВЧ)
	float fog_density = 0.0f;
	if (g_pGamePersistent && g_pGamePersistent->Environment().CurrentEnv)
		fog_density = g_pGamePersistent->Environment().CurrentEnv->fog_density;

	// 3. Отправка в SDK
	m_pAudioSystem->Update(camPos, dt, fog_density);

	// 4. Получение и применение EAX
	Presence::EAXResult res = m_pAudioSystem->GetEAXResult();
	if (res.isValid)
	{
		ApplyToSoundDriver(res);
	}
}

void CSoundEnvironment::ApplyToSoundDriver(const Presence::EAXResult& res)
{
	m_CurrentData.lRoom = res.lRoom;
	m_CurrentData.lRoomHF = res.lRoomHF;
	m_CurrentData.flRoomRolloffFactor = res.flRoomRolloffFactor;
	m_CurrentData.flDecayTime = res.flDecayTime;
	m_CurrentData.flDecayHFRatio = res.flDecayHFRatio;
	m_CurrentData.lReflections = res.lReflections;
	m_CurrentData.flReflectionsDelay = res.flReflectionsDelay;
	m_CurrentData.lReverb = res.lReverb;
	m_CurrentData.flReverbDelay = res.flReverbDelay;
	m_CurrentData.flEnvironmentSize = res.flEnvironmentSize;
	m_CurrentData.flEnvironmentDiffusion = res.flEnvironmentDiffusion;
	m_CurrentData.flAirAbsorptionHF = res.flAirAbsorptionHF;
	m_CurrentData.dwFlags = 0x3F; // EAX_ALL

	if (::Sound)
		::Sound->commit_eax(&m_CurrentData);
}
