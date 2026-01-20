#include "stdafx.h"
#include "ThreadManager.h"
#include "optick_include.h"
#include <ThreadUtil.h>

CThreadManager::CThreadManager() : m_bMustExit(FALSE), m_bInitialized(false)
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

	LPCSTR MainThreadName = "X-RAY Primary thread";
	Msg("Setting main thread name: %s", MainThreadName);
	OPTICK_THREAD(MainThreadName);

	m_bMustExit = FALSE;

	// Сброс событий в исходное состояние
	syncProcessFrame.Reset();
	syncFrameDone.Reset();
	syncThreadExit.Reset();

	// Запуск вторичного потока
	Threading::SpawnThread(SecondaryThreadProc, "X-RAY Secondary thread", 0, this);

	m_bInitialized = true;
}

void CThreadManager::Destroy()
{
	if (!m_bInitialized)
		return;

	Msg("Destroying Thread Manager...");

	// Сигнал на выход
	m_bMustExit = TRUE;
	syncProcessFrame.Set(); // Будим поток, чтобы он проверил флаг выхода

	// Ждем завершения потока
	syncThreadExit.Wait();

	// Очистка очередей
	m_seqParallel.clear();
	seqFrameMT.R.clear();

	m_bInitialized = false;
}

void CThreadManager::SecondaryThreadProc(void* context)
{
	// Настройка профайлера для потока
	OPTICK_THREAD("X-Ray Secondary Thread");
	OPTICK_EVENT("X-Ray Secondary Thread");

	CThreadManager* pMgr = static_cast<CThreadManager*>(context);
	R_ASSERT(pMgr);

	while (true)
	{
		// Ждем сигнала от главного потока на начало кадра
		pMgr->syncProcessFrame.Wait();

		if (pMgr->m_bMustExit)
		{
			pMgr->m_bMustExit = FALSE;
			pMgr->syncThreadExit.Set(); // Сообщаем, что мы вышли
			return;
		}

		// 1. Выполняем разовые параллельные задачи (загрузка текстур, геометрии и т.д.)
		for (u32 pit = 0; pit < pMgr->m_seqParallel.size(); pit++)
		{
			pMgr->m_seqParallel[pit]();
		}
		pMgr->m_seqParallel.clear(); // Очищаем список (capacity сохраняется для оптимизации)

		// 2. Выполняем постоянные задачи (Sound Update и т.д.)
		// rp_Frame - это маркер этапа, определен в pure.h/device.h
		pMgr->seqFrameMT.Process(rp_Frame);

		// Сигнализируем главному потоку, что мы закончили
		pMgr->syncFrameDone.Set();
	}
}

void CThreadManager::AddParallelTask(const ParallelTask& delegate)
{
	std::lock_guard<std::recursive_mutex> lock(m_csEnter); // Защита добавления на всякий случай
	m_seqParallel.push_back(delegate);
}

void CThreadManager::RemoveParallelTask(const ParallelTask& delegate)
{
	std::lock_guard<std::recursive_mutex> lock(m_csEnter);

	auto it = std::find(m_seqParallel.begin(), m_seqParallel.end(), delegate);
	if (it != m_seqParallel.end())
	{
		m_seqParallel.erase(it);
	}
}

void CThreadManager::SignalFrameStart()
{
	// Сбрасываем флаг завершения перед запуском
	syncFrameDone.Reset();
	// Запускаем вторичный поток
	syncProcessFrame.Set();
}

void CThreadManager::WaitForFrameEnd()
{
	// Ждем, пока вторичный поток закончит работу
	syncFrameDone.Wait();
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

bool CThreadManager::HasParallelTask(const ParallelTask& delegate)
{
	std::lock_guard<std::recursive_mutex> lock(m_csEnter);

	// Ищем делегат в списке
	auto it = std::find(m_seqParallel.begin(), m_seqParallel.end(), delegate);
	return it != m_seqParallel.end();
}
