#pragma once

#include "pure.h"
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
#include <condition_variable>
#include <FastDelegate.h>

class ENGINE_API CThreadManager
{
  public:
	using ParallelTask = fastdelegate::FastDelegate0<>;

	// Приоритет задачи
	enum class TaskPriority : u32
	{
		Critical = 400,
		High = 300,
		Normal = 200,
		Low = 100,
		Background = 0
	};

	// Тип задачи
	enum class TaskType : u8
	{
		General,
		AI
	};

  private:
	// Внутренняя структура задачи
	struct TaskItem
	{
		ParallelTask Delegate;
		TaskPriority Priority;

		bool operator>(const TaskItem& other) const
		{
			return (u32)Priority > (u32)other.Priority;
		}
	};

	// Контекст рабочего потока
	struct WorkerContext
	{
		CThreadManager* Manager;
		u32 ThreadID;
		std::thread Thread;
		std::condition_variable WakeCondition;
		std::mutex WakeMutex;
		bool ShouldWake;

		WorkerContext() : Manager(nullptr), ThreadID(0), ShouldWake(false)
		{
		}

		// Запрещаем копирование
		WorkerContext(const WorkerContext&) = delete;
		WorkerContext& operator=(const WorkerContext&) = delete;
	};

  private:
	// Очереди задач
	xr_vector<TaskItem> m_tasksGeneral;
	xr_vector<TaskItem> m_tasksAI;

	// Атомарные курсоры
	std::atomic<u32> m_cursorGeneral{0};
	std::atomic<u32> m_cursorAI{0};
	std::atomic<u32> m_completedThreadsCount{0};

	// Синхронизация - делаем mutable для использования в const-методах
	mutable std::recursive_mutex m_mutexGeneral;
	mutable std::recursive_mutex m_mutexAI;

	// Синхронизация завершения кадра
	std::condition_variable m_eventFrameComplete;
	std::mutex m_eventFrameCompleteMutex;
	bool m_frameCompleteReady;

	// Потоки - используем вектор указателей
	xr_vector<WorkerContext*> m_workerContexts;

	// Состояние
	std::atomic<bool> m_shouldExit{false};
	std::atomic<bool> m_isInitialized{false};

	// Процедура рабочего потока
	static void WorkerThreadProc(void* context);

  public:
	// Регистратор для Legacy-задач
	CRegistrator<pureFrame> LegacyFrameMT;

  public:
	CThreadManager();
	~CThreadManager();

	// Запрет копирования
	CThreadManager(const CThreadManager&) = delete;
	CThreadManager& operator=(const CThreadManager&) = delete;

	void Initialize();
	void Destroy();

	// Добавление задачи в очередь
	void AddParallelTask(const ParallelTask& delegate, TaskPriority priority = TaskPriority::Normal,
						 TaskType type = TaskType::General);

	// Удаление задачи
	void RemoveParallelTask(const ParallelTask& delegate);
	bool HasParallelTask(const ParallelTask& delegate) const;

	// Управление циклом
	void SignalFrameStart();
	void WaitForFrameEnd();

	// API для блокировок
	void EnterCritical();
	void LeaveCritical();
	bool TryEnterCritical();
};
