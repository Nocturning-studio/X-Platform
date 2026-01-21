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

	// Приоритет задачи (влияет на порядок выполнения внутри очереди)
	enum class TaskPriority : u32
	{
		Critical = 400, // Критически важные (физика персонажа)
		High = 300,		// Высокий приоритет (AI, анимация)
		Normal = 200,	// Обычный (общая логика)
		Low = 100,		// Низкий (партиклы, декор)
		Background = 0	// Фоновый (распаковка ресурсов)
	};

	// Тип задачи (определяет, какой поток будет её выполнять)
	enum class TaskType : u8
	{
		General, // Выполняется любым свободным потоком (Task Stealing)
		AI // Выполняется СТРОГО последовательно на выделенном потоке
	};

  private:
	// Внутренняя структура задачи
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
		CThreadManager* Manager; // Ссылка на родителя
		u32 ThreadID;			 // ID внутри пула (0, 1, 2...)
		Event WakeEvent;		 // Событие пробуждения
	};

  private:
	// --- Очередь ОБЩИХ задач (General) ---
	xr_vector<TaskItem> m_tasksGeneral;
	std::atomic<u32> m_cursorGeneral;
	std::recursive_mutex m_mutexGeneral;

	// --- Очередь ИИ задач (AI Exclusive) ---
	xr_vector<TaskItem> m_tasksAI;
	std::atomic<u32> m_cursorAI;
	std::recursive_mutex m_mutexAI;

	// --- Управление потоками ---
	xr_vector<WorkerContext*> m_workerThreads;
	std::atomic<u32> m_completedThreadsCount;

	// События
	Event m_eventFrameComplete;

	// Флаги состояния
	volatile BOOL m_shouldExit;
	bool m_isInitialized;

	// Процедура рабочего потока
	static void WorkerThreadProc(void* context);

  public:
	// Регистратор для Legacy-задач (Звук), выполняется только на Потоке #0
	CRegistrator<pureFrame> LegacyFrameMT;

  public:
	CThreadManager();
	~CThreadManager();

	void Initialize();
	void Destroy();

	// Добавление задачи в очередь
	void AddParallelTask(const ParallelTask& delegate, TaskPriority priority = TaskPriority::Normal,
						 TaskType type = TaskType::General);

	// Удаление задачи (из всех очередей)
	void RemoveParallelTask(const ParallelTask& delegate);

	// Проверка наличия задачи
	bool HasParallelTask(const ParallelTask& delegate);

	// Управление циклом
	void SignalFrameStart(); // Запуск всех потоков
	void WaitForFrameEnd();	 // Ожидание завершения

	// API для блокировок (замена старых Device.mt_csEnter)
	void EnterCritical();
	void LeaveCritical();
	bool TryEnterCritical();
};
