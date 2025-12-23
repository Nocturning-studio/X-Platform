/*
====================================================================================================
  Presence Audio SDK Integration for X-Ray Engine
  File: Sound_environment.cpp
====================================================================================================
  Author: NSDeathman & Gemini 3
  Description: Реализация менеджера звукового окружения.
  Этот класс управляет жизненным циклом Presence Audio System, связывает его с игровым уровнем
  и транслирует физические расчеты (EAX, Occlusion) в звуковой драйвер (OpenAL).
====================================================================================================

  Copyright (c) 2025 Nocturning Studio, NSDeathman & Gemini 3

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  1. The above copyright notice and this permission notice shall be included in all
	 copies or substantial portions of the Software.

  2. Any project (commercial, free, open-source, or closed-source) using this Software
	 must include attribution to "Presence Audio SDK by Nocturning Studio" in its
	 documentation, credits, or about screen.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

====================================================================================================
  Developed by: NSDeathman (Architecture & Core), Gemini 3 (Optimization & Math)
  Organization: Nocturning Studio
====================================================================================================
*/
///////////////////////////////////////////////////////////////////////////////////
// Includes
///////////////////////////////////////////////////////////////////////////////////
// API библиотеки Presence SDK
#include <PresenceAudioSDK/PresenceAudioAPI.h>

#include "stdafx.h"
#pragma hdrstop

#include "Sound_environment.h"
#include "Sound_environment_geometry_provider.h"
#include "../../xrEngine/IGame_Persistent.h"
#include "../../xrEngine/Environment.h"

// Подключаем определение Core для доступа к методу внедрения зависимости (SetOcclusion)
#include "../../xrSound/SoundRender_Core.h"

// =================================================================================================
// Constructor / Destructor
// =================================================================================================

CSoundEnvironment::CSoundEnvironment()
	: m_pAudioSystem(nullptr), m_pGeometryAdapter(nullptr), m_bLoaded(false), m_bEnabled(false), m_bPaused(false)
{
	Msg("Initializing Presence audio SDK...");

	Msg("Presence audio SDK version: %s", PRESENCE_VERSION_TEXT);

	// ---------------------------------------------------------------------------------------------
	// Динамическая загрузка DLL (Dynamic Loading)
	// ---------------------------------------------------------------------------------------------
	// Мы загружаем библиотеку вручную через LoadLibrary, чтобы иметь полный контроль над процессом.
	// Это позволяет корректно обработать ситуацию отсутствия DLL (вывести ошибку, а не крашнуть старт).
	// ---------------------------------------------------------------------------------------------
	LPCSTR PresenceAudioSDKLibName = "PresenceAudioSDK.dll";
	Log("Loading DLL:", PresenceAudioSDKLibName);
	hPresenceAudioSDKLib = LoadLibrary(PresenceAudioSDKLibName);
	R_ASSERT2(hPresenceAudioSDKLib, "! Can't load PresenceAudioSDK.dll");

	ZeroMemory(&m_CurrentData, sizeof(m_CurrentData));

	// Создаем основные компоненты системы
	// m_pAudioSystem: Ядро расчетов (находится внутри DLL)
	// m_pGeometryAdapter: Мост к геометрии движка (находится в xrGame)
	m_pAudioSystem = new Presence::AudioSystem();
	m_pGeometryAdapter = new XRayGeometryAdapter();

	Msg("- Presence audio SDK initialized successfully!");
}

CSoundEnvironment::~CSoundEnvironment()
{
	// Безопасное завершение работы
	// Сначала выгружаем логику уровня (отключаем коллбеки в xrSound), затем удаляем объекты.
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

	// Выгружаем DLL из памяти процесса
	if (hPresenceAudioSDKLib)
	{
		FreeLibrary(hPresenceAudioSDKLib);
		hPresenceAudioSDKLib = 0;
	}
}

// =================================================================================================
// Level Lifecycle Events
// =================================================================================================

void CSoundEnvironment::OnLevelLoad()
{
	if (!m_pAudioSystem || !m_pGeometryAdapter)
		return;

	Msg("[Presence EAX] Loading Level...");

	// Настройка параметров симуляции
	Presence::Settings s;
	s.maxBounces = 5;
	s.maxRayDistance = 150.0f;
	s.useMultithreading = true;
	s.updateInterval = 0.033f; // Можно задать, если нужно

	m_pGeometryAdapter->BuildMaterialCache(m_pAudioSystem);
	m_pAudioSystem->Initialize(m_pGeometryAdapter, s);
	m_bLoaded = true;

	if (::Sound)
	{
		CSoundRender_Core* pCore = (CSoundRender_Core*)::Sound;
		pCore->SetOcclusion(this);
		Msg("[Presence EAX] Occlusion Calculator registered in xrSound.");
	}

	Msg("[Presence EAX] System initialized and thread started.");
}

void CSoundEnvironment::OnLevelUnload()
{
	// ---------------------------------------------------------------------------------------------
	// Отключение от xrSound (CRITICAL SAFETY STEP)
	// ---------------------------------------------------------------------------------------------
	// Перед уничтожением объекта мы ОБЯЗАНЫ убрать свой указатель из звукового движка.
	// Если этого не сделать, xrSound попытается вызвать метод у уничтоженного объекта -> Crash.
	// ---------------------------------------------------------------------------------------------
	if (::Sound)
	{
		CSoundRender_Core* pCore = (CSoundRender_Core*)::Sound;
		pCore->SetOcclusion(nullptr);
		Msg("[Presence EAX] Occlusion Calculator unregistered.");
	}

	if (m_pAudioSystem)
	{
		m_pAudioSystem->Shutdown();
	}
	m_bLoaded = false;
	Msg("[Presence EAX] System shutdown.");
}

// =================================================================================================
// ISoundOcclusionCalculator Implementation
// =================================================================================================

// Этот метод вызывается из xrSound для каждого источника звука (эмиттера).
// Он служит прокси-методом, перенаправляющим запрос в библиотеку Presence SDK.
float CSoundEnvironment::CalculateOcclusion(const Presence::float3& listenerPos, const Presence::float3& sourcePos)
{
	// Если система не готова (уровень загружается или выгружен), звук проходит свободно (1.0).
	if (!m_bLoaded || !m_pAudioSystem)
		return 1.0f;

	// Вызов "тяжелой" математики внутри SDK
	return m_pAudioSystem->CalculateOcclusion(listenerPos, sourcePos);
}

// =================================================================================================
// Main Update Loop
// =================================================================================================

void CSoundEnvironment::Update()
{
	// Проверка флага консоли (ss_EAX) — позволяет отключить систему на лету
	if (!psSoundFlags.test(ss_EAX) || !m_bLoaded || !m_pAudioSystem || m_bPaused)
	{
		if (m_bEnabled)
		{
			Presence::EAXResult res = Presence::EAXResult();
			ApplyToSoundDriver(res);
			m_bEnabled = false;
		}
		return;
	}

	if (!g_pGameLevel || !g_pGamePersistent)
		return;

	if (!m_bEnabled)
		m_bEnabled = true;

	// Сбор данных
	Fvector pos = Device.vCameraPosition;
	Presence::float3 camPos(pos.x, pos.y, pos.z); // Использует новый конструктор float3

	float dt = Device.fTimeDelta;

	float fog_density = 0.0f;
	if (g_pGamePersistent->Environment().CurrentEnv)
		fog_density = g_pGamePersistent->Environment().CurrentEnv->fog_density;

	// Обновление SDK
	// SDK v0.2 ожидает dt и envFogDensity
	m_pAudioSystem->Update(camPos, dt, fog_density);

	// Получение результата
	Presence::EAXResult res = m_pAudioSystem->GetEAXResult();

	if (res.isValid)
	{
		ApplyToSoundDriver(res);
	}
}

// =================================================================================================
// Data Marshalling (SDK -> Engine)
// =================================================================================================

void CSoundEnvironment::ApplyToSoundDriver(const Presence::EAXResult& res)
{
	// Конвертация (Marshalling) данных из универсального формата Presence::EAXResult
	// в специфичную для X-Ray структуру SEAXEnvironmentData.

	// Основные параметры реверберации
	m_CurrentData.lRoom = res.lRoom;
	m_CurrentData.lRoomHF = res.lRoomHF;
	m_CurrentData.flRoomRolloffFactor = res.flRoomRolloffFactor;

	m_CurrentData.flDecayTime = res.flDecayTime;
	m_CurrentData.flDecayHFRatio = res.flDecayHFRatio;

	// Ранние отражения
	m_CurrentData.lReflections = res.lReflections;
	m_CurrentData.flReflectionsDelay = res.flReflectionsDelay;

	// Поздняя реверберация
	m_CurrentData.lReverb = res.lReverb;
	m_CurrentData.flReverbDelay = res.flReverbDelay;

	// Параметры среды
	m_CurrentData.flEnvironmentSize = res.flEnvironmentSize;
	m_CurrentData.flEnvironmentDiffusion = res.flEnvironmentDiffusion;
	m_CurrentData.flAirAbsorptionHF = res.flAirAbsorptionHF;

	// Флаги EAX (0x3F включает все основные эффекты: Scale, Decay, Reflections, Reverb...)
	m_CurrentData.dwFlags = 0x3F;

	// Финальная отправка в звуковой движок.
	// ::Sound — глобальный интерфейс звука в X-Ray.
	if (::Sound)
		::Sound->commit_eax(&m_CurrentData);
}
