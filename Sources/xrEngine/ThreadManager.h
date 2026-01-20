#pragma once

#include "pure.h"
#include "../xrCore/Event.hpp"
#include <mutex>
#include <FastDelegate.h>

class ENGINE_API CThreadManager
{
  public:
	using ParallelTask = fastdelegate::FastDelegate0<>;

  private:
	// Очередь задач для параллельного выполнения
	xr_vector<ParallelTask> m_seqParallel;

	// События синхронизации
	Event syncProcessFrame; // Сигнал потоку: начинай работу
	Event syncFrameDone;	// Сигнал главному потоку: работа завершена
	Event syncThreadExit;	// Сигнал: поток успешно завершился

	volatile BOOL m_bMustExit;
	bool m_bInitialized;

	// Мьютексы для критических секций (замена Device.mt_csEnter/Leave)
	std::recursive_mutex m_csEnter;
	std::recursive_mutex m_csLeave;

	// Процедура вторичного потока
	static void SecondaryThreadProc(void* context);

  public:
	// Регистратор для покадровых задач во вторичном потоке (Sound и т.д.)
	// Ранее это было Device.seqFrameMT
	CRegistrator<pureFrame> seqFrameMT;

  public:
	CThreadManager();
	~CThreadManager();

	void Initialize();
	void Destroy();

	// Добавление задачи в параллельное выполнение
	void AddParallelTask(const ParallelTask& delegate);
	void RemoveParallelTask(const ParallelTask& delegate);

	// Управление циклом (вызывается из главного потока)
	void SignalFrameStart();
	void WaitForFrameEnd();

	// Блокировки (Multi-threading protection)
	void EnterCritical();
	void LeaveCritical();
	bool TryEnterCritical(); // Полезно иметь try_lock

	// Проверка наличия параллельной задачи
	bool HasParallelTask(const ParallelTask& delegate);
};
