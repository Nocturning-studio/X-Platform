#include "stdafx.h"
#include "ThreadManager.h"
#include "optick_include.h"
#include <algorithm>

CThreadManager::CThreadManager()
{
}

CThreadManager::~CThreadManager()
{
	Destroy();
}

#ifdef WINDOWS
#include <windows.h>
static void SetThreadName(const char* threadName)
{
	const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType;	  // Must be 0x1000
		LPCSTR szName;	  // Pointer to name
		DWORD dwThreadID; // Thread ID (-1 = caller thread)
		DWORD dwFlags;	  // Reserved, must be 0
	} THREADNAME_INFO;
#pragma pack(pop)

	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = -1;
	info.dwFlags = 0;

	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
}
#endif

void CThreadManager::Initialize()
{
	if (m_isInitialized.load())
		return;

	Msg("Initializing Thread Manager...");

	m_shouldExit = false;

	// Расчет количества воркеров
	u32 hardwareConcurrency = std::thread::hardware_concurrency();
	if (hardwareConcurrency == 0)
		hardwareConcurrency = 1;

	u32 workerCount = (hardwareConcurrency > 1) ? (hardwareConcurrency - 1) : 1;
	m_totalWorkers = workerCount;

	Msg("* Thread Pool: Hardware Threads: %d", hardwareConcurrency);
	Msg("* Thread Pool: Spawning %d Worker Threads", workerCount);
	Msg("* Thread Pool: AI Dedicated Thread: %s", (workerCount > 1) ? "Yes (Worker #1)" : "No (Shared on #0)");

	// Создание контекстов
	m_workerContexts.reserve(workerCount);

	for (u32 i = 0; i < workerCount; ++i)
	{
		WorkerContext* ctx = xr_new<WorkerContext>();
		ctx->Manager = this;
		ctx->ThreadID = i;
		ctx->ShouldWake = false;
		ctx->FrameCompleted = false;

		// Создание потока с лямбдой
		ctx->Thread = std::thread([ctx]() { WorkerThreadProc(ctx); });

		m_workerContexts.push_back(ctx);

		// Установка имени потока
#ifdef WINDOWS
		char threadName[64];
		if (i == 0)
			sprintf_s(threadName, "X-RAY Worker #0 (Audio/Gen)");
		else if (i == 1)
			sprintf_s(threadName, "X-RAY Worker #1 (AI/Gen)");
		else
			sprintf_s(threadName, "X-RAY Worker #%d (Gen)", i);

		SetThreadName(threadName);
#endif
	}

	// Ждем, пока все потоки перейдут в состояние ожидания
	m_allThreadsReady = true;

	OPTICK_THREAD("X-RAY Primary Thread");
	m_isInitialized = true;
}

void CThreadManager::Destroy()
{
	if (!m_isInitialized.load())
		return;

	Msg("Destroying Thread Manager...");

	m_shouldExit = true;

	// Будим все потоки для завершения
	for (auto ctx : m_workerContexts)
	{
		if (ctx)
		{
			{
				std::lock_guard<std::mutex> lock(ctx->WakeMutex);
				ctx->ShouldWake = true;
			}
			ctx->WakeCondition.notify_one();
		}
	}

	// Ждем завершения потоков
	for (auto ctx : m_workerContexts)
	{
		if (ctx && ctx->Thread.joinable())
			ctx->Thread.join();
	}

	// Очищаем память
	for (auto ctx : m_workerContexts)
		xr_delete(ctx);

	m_workerContexts.clear();
	m_tasksGeneral.clear();
	m_tasksAI.clear();
	LegacyFrameMT.R.clear();

	m_isInitialized = false;
}

void CThreadManager::WorkerThreadProc(void* context)
{
	WorkerContext* ctx = static_cast<WorkerContext*>(context);
	CThreadManager* self = ctx->Manager;
	const u32 threadID = ctx->ThreadID;
	const u32 totalWorkers = self->m_totalWorkers.load();

	// Установка имени для профилировщика
	char optickThreadName[64];
	if (threadID == 0)
		sprintf_s(optickThreadName, "X-RAY Worker #0 (Audio/Gen)");
	else if (threadID == 1)
		sprintf_s(optickThreadName, "X-RAY Worker #1 (AI/Gen)");
	else
		sprintf_s(optickThreadName, "X-RAY Worker #%d (Gen)", threadID);

	OPTICK_THREAD(optickThreadName);

	while (true)
	{
		// Ожидание сигнала начала кадра
		{
			std::unique_lock<std::mutex> lock(ctx->WakeMutex);
			ctx->WakeCondition.wait(lock, [ctx, self] { return ctx->ShouldWake || self->m_shouldExit.load(); });

			if (self->m_shouldExit.load())
				return;

			ctx->ShouldWake = false;
			ctx->FrameCompleted = false;
		}

		// -------------------------------------------------------------
		// ЛОГИКА РАСПРЕДЕЛЕНИЯ ЗАДАЧ
		// -------------------------------------------------------------

		// A. Обработка AI задач
		bool isAIThread = (threadID == 1) || (totalWorkers == 1);

		if (isAIThread)
		{
			OPTICK_EVENT("Process_AI_Queue");
			while (true)
			{
				u32 taskIndex = self->m_cursorAI.fetch_add(1);
				if (taskIndex >= self->m_tasksAI.size())
					break;

				const auto& item = self->m_tasksAI[taskIndex];
				if (item.Delegate)
					item.Delegate();
			}
		}

		// B. Обработка Общих задач
		{
			OPTICK_EVENT("Process_General_Queue");
			while (true)
			{
				u32 taskIndex = self->m_cursorGeneral.fetch_add(1);
				if (taskIndex >= self->m_tasksGeneral.size())
					break;

				const auto& item = self->m_tasksGeneral[taskIndex];
				if (item.Delegate)
					item.Delegate();
			}
		}

		// C. Обработка Legacy задач (только поток #0)
		if (threadID == 0)
		{
			OPTICK_EVENT("Legacy_FrameMT");
			self->LegacyFrameMT.Process(rp_Frame);
		}

		// -------------------------------------------------------------
		// СИНХРОНИЗАЦИЯ ЗАВЕРШЕНИЯ КАДРА
		// -------------------------------------------------------------

		// Отмечаем, что этот поток завершил работу
		ctx->FrameCompleted = true;

		// Увеличиваем счетчик завершивших потоки
		u32 completedCount = self->m_threadsCompleted.fetch_add(1) + 1;

		// Если это был последний поток, будим главный поток
		if (completedCount == totalWorkers)
		{
			self->m_eventFrameComplete.notify_one();
		}
	}
}

void CThreadManager::SignalFrameStart()
{
	// Ждем, пока все потоки не будут готовы к новому кадру
	// Это гарантирует, что все потоки завершили предыдущий кадр
	// и находятся в состоянии ожидания
	if (!m_allThreadsReady.load())
	{
		// Даем небольшую паузу для стабилизации
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}

	// Сбрасываем атомарные счетчики
	m_cursorGeneral.store(0);
	m_cursorAI.store(0);
	m_threadsCompleted.store(0);

	// Сортировка задач по приоритету
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutexGeneral);
		if (!m_tasksGeneral.empty())
		{
			std::sort(m_tasksGeneral.begin(), m_tasksGeneral.end(),
					  [](const TaskItem& a, const TaskItem& b) { return a.Priority > b.Priority; });
		}
	}

	{
		std::lock_guard<std::recursive_mutex> lock(m_mutexAI);
		if (!m_tasksAI.empty())
		{
			std::sort(m_tasksAI.begin(), m_tasksAI.end(),
					  [](const TaskItem& a, const TaskItem& b) { return a.Priority > b.Priority; });
		}
	}

	// Пробуждение всех воркеров
	for (auto ctx : m_workerContexts)
	{
		if (ctx)
		{
			{
				std::lock_guard<std::mutex> lock(ctx->WakeMutex);
				ctx->ShouldWake = true;
			}
			ctx->WakeCondition.notify_one();
		}
	}
}

void CThreadManager::WaitForFrameEnd()
{
	// Ждем сигнала о завершении кадра
	{
		std::unique_lock<std::mutex> lock(m_eventFrameCompleteMutex);

		// Ждем, пока все потоки не завершат работу
		m_eventFrameComplete.wait(lock, [this] { return m_threadsCompleted.load() >= m_totalWorkers.load(); });
	}

	// Дополнительная проверка: убеждаемся, что все потоки действительно завершили
	bool allCompleted = true;
	for (auto ctx : m_workerContexts)
	{
		if (ctx && !ctx->FrameCompleted)
		{
			allCompleted = false;
			break;
		}
	}

	if (!allCompleted)
	{
		// Даем еще немного времени на завершение
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	// Очищаем списки задач для следующего кадра
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutexGeneral);
		m_tasksGeneral.clear();
	}
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutexAI);
		m_tasksAI.clear();
	}

	// Сбрасываем флаг готовности потоков
	m_allThreadsReady = false;

	// Даем время потокам вернуться в состояние ожидания
	std::this_thread::sleep_for(std::chrono::microseconds(50));

	// Устанавливаем флаг готовности
	m_allThreadsReady = true;
}

// Остальные методы без изменений
void CThreadManager::AddParallelTask(const ParallelTask& delegate, TaskPriority priority, TaskType type)
{
	TaskItem item;
	item.Delegate = delegate;
	item.Priority = priority;

	if (type == TaskType::AI)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutexAI);
		m_tasksAI.push_back(item);
	}
	else
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutexGeneral);
		m_tasksGeneral.push_back(item);
	}
}

void CThreadManager::RemoveParallelTask(const ParallelTask& delegate)
{
	// Удаляем из General
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutexGeneral);
		auto it = std::remove_if(m_tasksGeneral.begin(), m_tasksGeneral.end(),
								 [&](const TaskItem& item) { return item.Delegate == delegate; });

		if (it != m_tasksGeneral.end())
			m_tasksGeneral.erase(it, m_tasksGeneral.end());
	}

	// Удаляем из AI
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutexAI);
		auto it = std::remove_if(m_tasksAI.begin(), m_tasksAI.end(),
								 [&](const TaskItem& item) { return item.Delegate == delegate; });

		if (it != m_tasksAI.end())
			m_tasksAI.erase(it, m_tasksAI.end());
	}
}

bool CThreadManager::HasParallelTask(const ParallelTask& delegate) const
{
	// Проверка General
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutexGeneral);
		for (const auto& item : m_tasksGeneral)
		{
			if (item.Delegate == delegate)
				return true;
		}
	}

	// Проверка AI
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutexAI);
		for (const auto& item : m_tasksAI)
		{
			if (item.Delegate == delegate)
				return true;
		}
	}

	return false;
}

void CThreadManager::EnterCritical()
{
	m_mutexGeneral.lock();
}

void CThreadManager::LeaveCritical()
{
	m_mutexGeneral.unlock();
}

bool CThreadManager::TryEnterCritical()
{
	return m_mutexGeneral.try_lock();
}
