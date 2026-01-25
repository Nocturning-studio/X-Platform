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

	enum class TaskPriority : u32
	{
		Critical = 400,
		High = 300,
		Normal = 200,
		Low = 100,
		Background = 0
	};

	enum class TaskType : u8
	{
		General,
		AI
	};

  private:
	struct TaskItem
	{
		ParallelTask Delegate;
		TaskPriority Priority;

		bool operator>(const TaskItem& other) const
		{
			return (u32)Priority > (u32)other.Priority;
		}
	};

	struct WorkerContext
	{
		CThreadManager* Manager;
		u32 ThreadID;
		std::condition_variable WakeCondition;
		std::mutex WakeMutex;
		bool ShouldWake;
		std::thread Thread;

		// Флаг, что поток завершил работу над текущим кадром
		bool FrameCompleted;

		WorkerContext() : Manager(nullptr), ThreadID(0), ShouldWake(false), FrameCompleted(false)
		{
		}
	};

  private:
	// Очереди задач
	xr_vector<TaskItem> m_tasksGeneral;
	xr_vector<TaskItem> m_tasksAI;

	// Атомарные курсоры
	std::atomic<u32> m_cursorGeneral{0};
	std::atomic<u32> m_cursorAI{0};

	// Синхронизация
	mutable std::recursive_mutex m_mutexGeneral;
	mutable std::recursive_mutex m_mutexAI;

	// Синхронизация завершения кадра
	std::condition_variable m_eventFrameComplete;
	std::mutex m_eventFrameCompleteMutex;

	// Счетчик завершивших потоки для текущего кадра
	std::atomic<u32> m_threadsCompleted{0};

	// Общее количество воркеров
	std::atomic<u32> m_totalWorkers{0};

	// Потоки
	xr_vector<WorkerContext*> m_workerContexts;

	// Состояние
	std::atomic<bool> m_shouldExit{false};
	std::atomic<bool> m_isInitialized{false};

	// Флаг, что все потоки готовы к новому кадру
	std::atomic<bool> m_allThreadsReady{false};

	static void WorkerThreadProc(void* context);

  public:
	CRegistrator<pureFrame> LegacyFrameMT;

  public:
	CThreadManager();
	~CThreadManager();

	CThreadManager(const CThreadManager&) = delete;
	CThreadManager& operator=(const CThreadManager&) = delete;

	void Initialize();
	void Destroy();

	void AddParallelTask(const ParallelTask& delegate, TaskPriority priority = TaskPriority::Normal,
						 TaskType type = TaskType::General);

	void RemoveParallelTask(const ParallelTask& delegate);
	bool HasParallelTask(const ParallelTask& delegate) const;

	void SignalFrameStart();
	void WaitForFrameEnd();

	void EnterCritical();
	void LeaveCritical();
	bool TryEnterCritical();
};
