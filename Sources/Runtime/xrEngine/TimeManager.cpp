#include "stdafx.h"
#include "TimeManager.h"
#include "IGame_Persistent.h" // Для g_pauseMngr, если нужно, или передавать через Engine
#include <timeapi.h>

// Необходимо для доступа к флагам (rsConstantFPS)
// В X-Ray это обычно глобальная структура, либо через Device.
// Если psDeviceFlags глобальна:
extern ENGINE_API Flags32 psDeviceFlags;

CTimeManager::CTimeManager()
{
	m_Timer_MM_Delta = 0;
	m_dwFrame = 0;
	m_fTimeDelta = 0.0f;
	m_fTimeGlobal = 0.0f;
	m_dwTimeDelta = 0;
	m_dwTimeGlobal = 0;
	m_dwTimeContinual = 0;
	m_psTimeFactor = 1.0f;
	m_dwTimeGlobalFixed = 0;
	m_fTimeGlobalFixed = 0.0f;
}

CTimeManager::~CTimeManager()
{
}

void CTimeManager::Initialize()
{
	Msg("Initializing Time Manager...");
	m_TimerGlobal.Start();
	m_TimerMM.Start();

	// Расчет дельты мультимедийного таймера (код из старого Device::PrepareEventLoop)
	m_dwTimeGlobal = 0;
	m_Timer_MM_Delta = 0;
	{
		u32 time_mm = timeGetTime();
		while (timeGetTime() == time_mm)
			; // wait for next tick
		u32 time_system = timeGetTime();
		u32 time_local = TimerAsync();
		m_Timer_MM_Delta = time_system - time_local;
	}
}

void CTimeManager::Destroy()
{
	// Очистка, если требуется
}

void CTimeManager::SetTimeFactor(float factor)
{
	m_Timer.time_factor(factor);
	m_TimerGlobal.time_factor(factor);
	m_psTimeFactor = factor;
}

float CTimeManager::GetTimeFactor() const
{
	return m_Timer.time_factor();
}

void CTimeManager::StopTime()
{
	const float kTimeFactorMin = 0.0001f;

	SetTimeFactor(kTimeFactorMin);
}

u32 CTimeManager::TimerAsync()
{
	return m_TimerGlobal.GetElapsed_ms();
}

u32 CTimeManager::TimerAsync_MMT()
{
	return m_TimerMM.GetElapsed_ms() + m_Timer_MM_Delta;
}

u32 CTimeManager::GetFrameElapsed()
{
	return m_FrameTimer.GetElapsed_ms();
}

void CTimeManager::Update()
{
	m_dwFrame++;

	m_dwTimeContinual = m_TimerMM.GetElapsed_ms();

	// Логика rsConstantFPS (обычно используется для записи демок или бенчмарков)
	if (psDeviceFlags.test(rsConstantFPS))
	{
		// 20ms = 50fps fix
		m_fTimeDelta = 0.020f;
		m_fTimeGlobal += 0.020f;
		m_dwTimeDelta = 20;
		m_dwTimeGlobal += 20;
	}
	else
	{
		// Обычный расчет времени
		float fPreviousFrameTime = m_Timer.GetElapsed_sec();
		m_Timer.Start(); // начало нового кадра для локального таймера

		m_fTimeDelta = 0.1f * m_fTimeDelta + 0.9f * fPreviousFrameTime; // сглаживание

		if (m_fTimeDelta > 0.1f)
			m_fTimeDelta = 0.1f; // лимит минимум 10fps (защита от гигантских лагов)

		// Проверка на паузу (g_pauseMngr обычно управляет тем, идет ли время в игре)
		// В оригинале: if (Paused()) fTimeDelta = 0.0f;
		// Здесь нам нужно получить состояние паузы.
		// Если g_pauseMngr глобален:
		if (g_pauseMngr.Paused())
			m_fTimeDelta = 0.0f;

		m_fTimeGlobal = m_TimerGlobal.GetElapsed_sec();
		u32 _old_global = m_dwTimeGlobal;
		m_dwTimeGlobal = m_TimerGlobal.GetElapsed_ms();
		m_dwTimeDelta = m_dwTimeGlobal - _old_global;
	}
}

void CTimeManager::OnFrameStart()
{
	// Запоминаем время начала кадра для лимитера
	m_FrameStartTime = GetGlobalTimeMs();

	// Обновляем специфичные тайм значения
	m_dwTimeGlobalFixed = m_TimerGlobal.GetElapsed_ms();
	m_fTimeGlobalFixed = m_TimerGlobal.GetElapsed_sec();
}

u32 CTimeManager::CalculateFrameLimitDelay(u32 targetFPS)
{
	if (targetFPS == 0)
		return 0; // Без лимита

	m_FrameEndTime = GetGlobalTimeMs();
	u32 frameDuration = m_FrameEndTime - m_FrameStartTime;
	u32 targetDuration = 1000 / targetFPS;

	if (frameDuration < targetDuration)
	{
		return targetDuration - frameDuration;
	}
	return 0;
}

extern int g_frametime;
void CTimeManager::DoFrameLimit()
{
	PROFILE_FUNCTION();

	u32 targetFPS = g_frametime > 0 ? g_frametime : 0;
	u32 sleepTime = Engine.TimeManager.CalculateFrameLimitDelay(targetFPS);

	if (sleepTime > 0)
	{
		Sleep(sleepTime);
	}
}
