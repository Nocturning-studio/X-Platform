////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "pure.h"
#include <mutex>
#include <atomic>
#include <queue>
#include <future>
#include <thread>
#include <array>
#include <functional>
#include <condition_variable>
#include "xr_engine_common.h"
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API CThreadManager
{
  public:
	using ParallelTask = std::function<void()>;

	using TaskID = u64;

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
		TaskID Id;
		ParallelTask Delegate;
		TaskPriority Priority;
		std::shared_ptr<std::packaged_task<void()>> PackagedTask;

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
		bool FrameCompleted;
		std::thread Thread;

		WorkerContext() : Manager(nullptr), ThreadID(0), ShouldWake(false), FrameCompleted(false) {}

		WorkerContext(const WorkerContext&) = delete;
		WorkerContext& operator=(const WorkerContext&) = delete;

		WorkerContext(WorkerContext&& other) noexcept : Manager(other.Manager), ThreadID(other.ThreadID), ShouldWake(other.ShouldWake), FrameCompleted(other.FrameCompleted), Thread(std::move(other.Thread))
		{
			other.Manager = nullptr;
		}
	};

  private:
	struct BackgroundItem
	{
		TaskID Id;
		ParallelTask Delegate;
		TaskPriority Priority;

		bool operator<(const BackgroundItem& other) const
		{
			return (u32)Priority < (u32)other.Priority;
		}
	};

	xr_vector<TaskItem> m_tasksGeneral;
	xr_vector<TaskItem> m_tasksAI;

	std::atomic<u32> m_cursorGeneral{0};
	std::atomic<u32> m_cursorAI{0};

	mutable std::recursive_mutex m_mutexGeneral;
	mutable std::recursive_mutex m_mutexAI;

	std::condition_variable m_eventFrameComplete;
	std::mutex m_eventFrameCompleteMutex;
	std::atomic<u32> m_threadsCompleted{0};

	std::atomic<TaskID> m_nextTaskId{1};

	static constexpr u32 MAX_WORKERS = 2;
	WorkerContext m_workers[MAX_WORKERS];
	u32 m_workerCount;

	std::atomic<bool> m_shouldExit{false};
	std::atomic<bool> m_isInitialized{false};

	std::array<std::thread::id, MAX_WORKERS> m_workerThreadIds;

	std::priority_queue<BackgroundItem> m_backgroundQueue;
	std::mutex                          m_backgroundMutex;
	std::condition_variable             m_backgroundCV;
	std::thread                         m_backgroundThread;

	void BackgroundThreadProc();

	bool IsWorkerThread() const;
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

	TaskID AddParallelTask(const ParallelTask& delegate, TaskPriority priority = TaskPriority::Normal, TaskType type = TaskType::General);
	std::future<void> AddParallelTaskWithFuture(const ParallelTask& delegate, TaskPriority priority = TaskPriority::Normal, TaskType type = TaskType::General);
	TaskID AddBackgroundTask(const ParallelTask& delegate, TaskPriority priority = TaskPriority::Background);

	void RemoveParallelTask(TaskID id);
	bool HasParallelTask(TaskID id) const;

	void SignalFrameStart();
	void WaitForFrameEnd();

	u32 GetWorkerCount() const { return m_workerCount; }
};
////////////////////////////////////////////////////////////////////////////////
