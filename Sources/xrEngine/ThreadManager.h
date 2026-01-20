#pragma once

#include "pure.h"
#include "../xrCore/Event.hpp"
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
#include <FastDelegate.h>

class ENGINE_API CThreadManager
{
  public:
	using ParallelTask = fastdelegate::FastDelegate0<>;

	enum class TaskPriority : u32
	{
		Critical = 400, // Физика, важная логика
		High = 300,		// Скелетная анимация, AI
		Normal = 200,	// Обычное обновление (по умолчанию)
		Low = 100,		// Партиклы, дальние объекты
		Background = 0	// Стриминг текстур, распаковка
	};

  private:
	// Структура задачи с приоритетом
	struct TaskItem
	{
		ParallelTask Delegate;
		TaskPriority Priority;

		// Для сортировки: высокий приоритет идет первым
		bool operator>(const TaskItem& other) const
		{
			return (u32)Priority > (u32)other.Priority;
		}
	};

	// Контекст рабочего потока
	struct WorkerContext
	{
		CThreadManager* Parent;
		u32 ThreadID;
		Event WakeEvent; // Событие для пробуждения конкретного потока
	};

  private:
	// Очередь задач
	xr_vector<TaskItem> m_seqParallel;
	std::atomic<u32> m_taskCursor; // Атомарный курсор текущей задачи

	// Управление потоками
	xr_vector<WorkerContext*> m_workers;
	std::atomic<u32> m_finishedThreadsCount; // Сколько потоков завершили работу

	// События
	Event syncFrameDone; // Сигнал главному потоку: ВСЕ завершили работу

	volatile BOOL m_bMustExit;
	bool m_bInitialized;

	// Мьютексы
	std::recursive_mutex m_csEnter;
	std::recursive_mutex m_csLeave;

	// Процедура потока
	static void ThreadProc(void* context);

  public:
	// Регистратор для legacy-задач (Sound), выполняется только на Потоке #0
	CRegistrator<pureFrame> seqFrameMT;

  public:
	CThreadManager();
	~CThreadManager();

	void Initialize();
	void Destroy();

	// Добавление задачи. priority: больше = важнее.
	void AddParallelTask(const ParallelTask& delegate, TaskPriority priority = TaskPriority::Normal);
	void RemoveParallelTask(const ParallelTask& delegate);
	bool HasParallelTask(const ParallelTask& delegate);

	// Управление циклом
	void SignalFrameStart();
	void WaitForFrameEnd();

	// Блокировки
	void EnterCritical();
	void LeaveCritical();
	bool TryEnterCritical();
};
