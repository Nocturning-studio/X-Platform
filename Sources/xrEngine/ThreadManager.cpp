#include "stdafx.h"
#include "ThreadManager.h"
#include "optick_include.h"
#include <ThreadUtil.h>
#include <algorithm> // для std::sort

CThreadManager::CThreadManager() : m_bMustExit(FALSE), m_bInitialized(false), m_taskCursor(0), m_finishedThreadsCount(0)
{
}

CThreadManager::~CThreadManager()
{
	Destroy();
}

void CThreadManager::Initialize()
{
	if (m_bInitialized)
		return;

	Msg("Initializing Thread Manager...");

	m_bMustExit = FALSE;
	syncFrameDone.Reset();

	// 1. Определяем количество потоков
	// Оставляем 1 ядро для Главного потока и Windows, остальные берем под воркеры.
	// Минимум 1 дополнительный поток (для совместимости с одноядерными CPU, если такие еще живы).
	u32 hwConcurency = std::thread::hardware_concurrency();
	u32 workerCount = (hwConcurency > 1) ? (hwConcurency - 1) : 1;

	// Ограничим разумным числом, чтобы не плодить контексты (например, 7 воркеров для 8-ядерника)
	// Для X-Ray обычно 3-4 потока уже достаточно для насыщения.
	Msg("* Thread Pool: Hardware Threads: %d, Spawning Workers: %d", hwConcurency, workerCount);

	// 2. Создаем воркеры
	m_workers.reserve(workerCount);
	for (u32 i = 0; i < workerCount; ++i)
	{
		WorkerContext* ctx = xr_new<WorkerContext>();
		ctx->Parent = this;
		ctx->ThreadID = i;
		// Событие WakeEvent создается автоматически в конструкторе Event

		m_workers.push_back(ctx);

		string64 threadName;

		sprintf_s(threadName, "X-RAY Worker #%d", i);

		Threading::SpawnThread(ThreadProc, threadName, 0, ctx);
	}

	// Именуем главный поток для Optick
	OPTICK_THREAD("X-RAY Primary thread");

	m_bInitialized = true;
}

void CThreadManager::Destroy()
{
	if (!m_bInitialized)
		return;

	Msg("Destroying Thread Manager...");

	m_bMustExit = TRUE;

	// Будим все потоки, чтобы они увидели флаг выхода
	for (auto ctx : m_workers)
	{
		ctx->WakeEvent.Set();
	}

	// Ждем завершения (тут упрощенно, в идеале нужно ждать хендлы потоков,
	// но SpawnThread в X-Ray обычно detach-ит или закрывает хендлы.
	// Добавим небольшую паузу или механизм подтверждения выхода, если SpawnThread этого не делает).
	Sleep(100);

	// Очистка памяти контекстов
	for (auto ctx : m_workers)
		xr_delete(ctx);
	m_workers.clear();

	m_seqParallel.clear();
	seqFrameMT.R.clear();

	m_bInitialized = false;
}

void CThreadManager::ThreadProc(void* context)
{
	WorkerContext* ctx = static_cast<WorkerContext*>(context);
	CThreadManager* pMgr = ctx->Parent;

	OPTICK_THREAD("X-Ray Worker");

	while (true)
	{
		// 1. Ждем сигнала на старт кадра
		ctx->WakeEvent.Wait();

		if (pMgr->m_bMustExit)
			return;

		// 2. Выполнение параллельных задач (Task Stealing)
		// Все потоки набрасываются на m_seqParallel
		while (true)
		{
			// Атомарно берем индекс следующей задачи
			u32 taskIdx = pMgr->m_taskCursor.fetch_add(1);

			// Если задач больше нет - выходим из цикла
			if (taskIdx >= pMgr->m_seqParallel.size())
				break;

			// Выполняем задачу
			{
				// Копия делегата, чтобы не держать локов (хотя вектор read-only в этот момент)
				// В данном дизайне вектор не меняется во время выполнения, локи не нужны.
				const auto& item = pMgr->m_seqParallel[taskIdx];
				if (item.Delegate)
					item.Delegate();
			}
		}

		// 3. Выполнение Legacy задач (seqFrameMT)
		// Это делает ТОЛЬКО поток #0, чтобы сохранить совместимость со старым кодом (Sound),
		// который может быть не thread-safe при запуске с нескольких потоков.
		if (ctx->ThreadID == 0)
			pMgr->seqFrameMT.Process(rp_Frame);

		// 4. Сигнализируем, что этот поток закончил работу
		// Увеличиваем счетчик завершивших работу потоков
		u32 finished = pMgr->m_finishedThreadsCount.fetch_add(1) + 1;

		// Если это был ПОСЛЕДНИЙ поток, будим главный поток
		if (finished == pMgr->m_workers.size())
			pMgr->syncFrameDone.Set();
	}
}

void CThreadManager::AddParallelTask(const ParallelTask& delegate, TaskPriority priority)
{
	std::lock_guard<std::recursive_mutex> lock(m_csEnter);

	TaskItem item;
	item.Delegate = delegate;
	item.Priority = priority;
	m_seqParallel.push_back(item);
}

void CThreadManager::RemoveParallelTask(const ParallelTask& delegate)
{
	std::lock_guard<std::recursive_mutex> lock(m_csEnter);

	// Удаляем все вхождения этого делегата
	auto it = std::remove_if(m_seqParallel.begin(), m_seqParallel.end(),
							 [&](const TaskItem& item) { return item.Delegate == delegate; });

	if (it != m_seqParallel.end())
		m_seqParallel.erase(it, m_seqParallel.end());
}

bool CThreadManager::HasParallelTask(const ParallelTask& delegate)
{
	std::lock_guard<std::recursive_mutex> lock(m_csEnter);
	for (const auto& item : m_seqParallel)
	{
		if (item.Delegate == delegate)
			return true;
	}
	return false;
}

void CThreadManager::SignalFrameStart()
{
	// 1. Подготовка перед запуском
	m_taskCursor.store(0);
	m_finishedThreadsCount.store(0);
	syncFrameDone.Reset();

	// 2. Сортировка задач по приоритету
	if (!m_seqParallel.empty())
	{
		// Сортируем (наибольший приоритет в начале)
		std::sort(m_seqParallel.begin(), m_seqParallel.end(),
				  [](const TaskItem& a, const TaskItem& b) { return a.Priority > b.Priority; });
	}

	// 3. Будим ВСЕ потоки
	for (auto ctx : m_workers)
	{
		ctx->WakeEvent.Set();
	}
}

void CThreadManager::WaitForFrameEnd()
{
	// Ждем, пока счетчик завершивших потоков не станет равен общему числу.
	// Механизм Event здесь эффективен, чтобы Main Thread спал, а не крутился в SpinWait.
	syncFrameDone.Wait();

	// Очищаем задачи после выполнения (чтобы не выполнять их в след. кадре)
	// Емкость вектора сохраняется, аллокаций не будет.
	// Делаем это здесь, когда гарантированно никто не читает вектор.
	m_seqParallel.clear();
}

void CThreadManager::EnterCritical()
{
	m_csEnter.lock();
}

void CThreadManager::LeaveCritical()
{
	m_csEnter.unlock();
}

bool CThreadManager::TryEnterCritical()
{
	return m_csEnter.try_lock();
}
