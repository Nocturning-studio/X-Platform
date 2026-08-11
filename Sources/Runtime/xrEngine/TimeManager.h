#pragma once
#include "ftimer.h"

class ENGINE_API CTimeManager
{
  public:
	CTimeManager();
	~CTimeManager();

	void Initialize();
	void Destroy();

	// Вызывается каждый кадр перед основной логикой
	void Update();

	// Управление временем
	void SetTimeFactor(float factor);
	float GetTimeFactor() const;
	void StopTime();

	// Функции таймеров (аналоги старых методов Device)
	u32 TimerAsync();	   // Время с начала запуска Engine (Global)
	u32 TimerAsync_MMT();  // Мультимедийное время (с коррекцией)
	u32 GetFrameElapsed(); // Время, затраченное на кадр

	// Геттеры состояния
	IC u32 GetFrameCount() const
	{
		return m_dwFrame;
	}
	void IncreaseFrameCount()
	{
		++m_dwFrame;
	}
	IC float GetDeltaTime() const
	{
		return m_fTimeDelta;
	}
	IC float GetGlobalTime() const
	{
		return m_fTimeGlobal;
	}
	IC CTimer_paused* GetTimerGlobal()
	{
		return &m_TimerGlobal;
	}
	IC u32 GetDeltaTimeMs() const
	{
		return m_dwTimeDelta;
	}
	IC u32 GetGlobalTimeMs() const
	{
		return m_dwTimeGlobal;
	}
	IC u32 GetContinualTimeMs() const
	{
		return m_dwTimeContinual;
	}

	IC u32 GetGlobalTimeMsFixed() const 
	{ 
		return m_dwTimeGlobalFixed; 
	}
	IC float GetGlobalTimeFixed() const 
	{ 
		return m_fTimeGlobalFixed; 
	}

	// Сброс счетчика кадров (например, при смене уровня)
	void ResetFrameCount()
	{
		m_dwFrame = 0;
	}

	void SetDeltaTimeMs(u32 val)
	{
		m_dwTimeDelta = val;
	}
	void SetGlobalTimeMs(u32 val)
	{
		m_dwTimeGlobal = val;
	}
	void SetDeltaTime(float val)
	{
		m_fTimeDelta = val;
	}
	void SetGlobalTime(float val)
	{
		m_fTimeGlobal = val;
	}

	// Вызывается в самом начале кадра
	void OnFrameStart();

	// Вызывается в конце кадра, возвращает время, которое нужно поспать (в мс)
	u32 CalculateFrameLimitDelay(u32 targetFPS);

	void DoFrameLimit();

  private:
	// Таймеры
	u32 m_Timer_MM_Delta;
	CTimer_paused m_Timer; // Игровой таймер (паузится)
	CTimer_paused m_TimerGlobal; // Глобальный таймер (не паузится обычно, но зависит от time_factor)
	CTimer m_TimerMM;	 // Мультимедийный таймер
	CTimer m_FrameTimer; // Таймер длительности кадра

	// Переменные состояния
	u32 m_dwFrame;

	float m_fTimeDelta;
	float m_fTimeGlobal;

	u32 m_dwTimeDelta;
	u32 m_dwTimeGlobal;
	u32 m_dwTimeContinual;

	// Внутренние настройки
	float m_psTimeFactor;

	// Ограничение количества кадров
	u32 m_FrameStartTime;
	u32 m_FrameEndTime;

	// Время, обновляемое один раз за кадр
	u32 m_dwTimeGlobalFixed;
	float m_fTimeGlobalFixed;
};
