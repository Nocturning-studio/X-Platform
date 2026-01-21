#include "stdafx.h"
#include "ThreadManager.h"
#include "optick_include.h"
#include <ThreadUtil.h>
#include <algorithm>

CThreadManager::CThreadManager()
	: m_shouldExit(FALSE), m_isInitialized(false), m_cursorGeneral(0), m_cursorAI(0), m_completedThreadsCount(0)
{
}

CThreadManager::~CThreadManager()
{
	Destroy();
}

void CThreadManager::Initialize()
{
	if (m_isInitialized)
		return;

	Msg("Initializing Thread Manager...");

	m_shouldExit = FALSE;
	m_eventFrameComplete.Reset();

	// 1. Расчет количества воркеров
	// Оставляем 1 ядро для Main Thread, остальные занимаем воркерами.
	u32 hardwareConcurrency = std::thread::hardware_concurrency();
	u32 workerCount = (hardwareConcurrency > 1) ? (hardwareConcurrency - 1) : 1;

	// Логируем конфигурацию
	Msg("* Thread Pool: Hardware Threads: %d", hardwareConcurrency);
	Msg("* Thread Pool: Spawning %d Worker Threads", workerCount);
	Msg("* Thread Pool: AI Dedicated Thread: %s", (workerCount > 1) ? "Yes (Worker #1)" : "No (Shared on #0)");

	// 2. Создание потоков
	m_workerThreads.reserve(workerCount);
	for (u32 i = 0; i < workerCount; ++i)
	{
		WorkerContext* ctx = xr_new<WorkerContext>();
		ctx->Manager = this;
		ctx->ThreadID = i;
		m_workerThreads.push_back(ctx);

		string64 threadName;
		if (i == 0)
			sprintf_s(threadName, "X-RAY Worker #0 (Audio/Gen)");
		else if (i == 1)
			sprintf_s(threadName, "X-RAY Worker #1 (AI/Gen)");
		else
			sprintf_s(threadName, "X-RAY Worker #%d (Gen)", i);

		Threading::SpawnThread(WorkerThreadProc, threadName, 0, ctx);
	}

	OPTICK_THREAD("X-RAY Primary Thread");
	m_isInitialized = true;
}

void CThreadManager::Destroy()
{
	if (!m_isInitialized)
		return;

	Msg("Destroying Thread Manager...");

	m_shouldExit = TRUE;

	// Будим все потоки, чтобы они вышли из цикла
	for (auto ctx : m_workerThreads)
		ctx->WakeEvent.Set();

	Sleep(100); // Даем время на корректное завершение

	// Очистка памяти
	for (auto ctx : m_workerThreads)
		xr_delete(ctx);
	m_workerThreads.clear();

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
	const u32 totalWorkers = self->m_workerThreads.size();

	OPTICK_THREAD("Worker Thread");

	while (true)
	{
		// 1. Ожидание сигнала начала кадра
		ctx->WakeEvent.Wait();

		if (self->m_shouldExit)
			return;

		// -------------------------------------------------------------
		// ЛОГИКА РАСПРЕДЕЛЕНИЯ ЗАДАЧ
		// -------------------------------------------------------------

		// A. Обработка AI задач (TaskType::AI)
		// Условие:
		// 1. Если потоков много (>1), AI берет только Поток #1.
		// 2. Если поток всего один (single core), он вынужден брать всё.
		bool isAIThread = (threadID == 1) || (totalWorkers == 1);

		if (isAIThread)
		{
			OPTICK_EVENT("Process_AI_Queue");
			while (true)
			{
				// Атомарно получаем индекс задачи
				u32 taskIndex = self->m_cursorAI.fetch_add(1);

				// Если вышли за пределы вектора - задач больше нет
				if (taskIndex >= self->m_tasksAI.size())
					break;

				// Выполнение
				const auto& item = self->m_tasksAI[taskIndex];
				if (item.Delegate)
					item.Delegate();
			}
		}

		// B. Обработка Общих задач (TaskType::General)
		// Все потоки могут помогать с общими задачами.
		// Поток #1 помогает только после того, как закончит с AI.
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

		// C. Обработка Legacy задач (Sound и прочее старье)
		// Строго только на Потоке #0 для совместимости
		if (threadID == 0)
		{
			OPTICK_EVENT("Legacy_FrameMT");
			self->LegacyFrameMT.Process(rp_Frame);
		}

		// -------------------------------------------------------------
		// СИНХРОНИЗАЦИЯ
		// -------------------------------------------------------------

		// Увеличиваем счетчик завершивших работу
		u32 finishedCount = self->m_completedThreadsCount.fetch_add(1) + 1;

		// Если это был последний поток, будим Main Thread
		if (finishedCount == totalWorkers)
		{
			self->m_eventFrameComplete.Set();
		}
	}
}

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

bool CThreadManager::HasParallelTask(const ParallelTask& delegate)
{
	// Проверка General
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutexGeneral);
		for (const auto& item : m_tasksGeneral)
			if (item.Delegate == delegate)
				return true;
	}
	// Проверка AI
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutexAI);
		for (const auto& item : m_tasksAI)
			if (item.Delegate == delegate)
				return true;
	}
	return false;
}

void CThreadManager::SignalFrameStart()
{
	// Сброс атомарных счетчиков
	m_cursorGeneral.store(0);
	m_cursorAI.store(0);
	m_completedThreadsCount.store(0);
	m_eventFrameComplete.Reset();

	// Сортировка General задач
	if (!m_tasksGeneral.empty())
	{
		std::sort(m_tasksGeneral.begin(), m_tasksGeneral.end(),
				  [](const TaskItem& a, const TaskItem& b) { return a.Priority > b.Priority; });
	}

	// Сортировка AI задач
	if (!m_tasksAI.empty())
	{
		std::sort(m_tasksAI.begin(), m_tasksAI.end(),
				  [](const TaskItem& a, const TaskItem& b) { return a.Priority > b.Priority; });
	}

	// Пробуждение всех воркеров
	for (auto ctx : m_workerThreads)
		ctx->WakeEvent.Set();
}

void CThreadManager::WaitForFrameEnd()
{
	// Ждем, пока последний поток не подаст сигнал
	m_eventFrameComplete.Wait();

	// Очищаем списки задач для следующего кадра
	m_tasksGeneral.clear();
	m_tasksAI.clear();
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
