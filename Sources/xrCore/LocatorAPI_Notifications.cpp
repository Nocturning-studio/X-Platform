// LocatorAPI.cpp: implementation of the CLocatorAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "LocatorAPI_Notifications.h"

static std::mutex CS;

CFS_PathNotificator::CFS_PathNotificator() : CThread(0)
{
	FMutex = CreateMutex(NULL, TRUE, NULL);
	if (FMutex)
		WaitForSingleObject(FMutex, INFINITE);
}

CFS_PathNotificator::~CFS_PathNotificator()
{
	for (auto& path : events)
	{
		path.FChangeEvent.clear();
		if (path.FWaitHandle != INVALID_HANDLE_VALUE)
		{
			HANDLE hOld = path.FWaitHandle;
			path.FWaitHandle = INVALID_HANDLE_VALUE;
			FindCloseChangeNotification(hOld);
		}
	}
	events.clear();

	if (FMutex)
	{
		CloseHandle(FMutex);
	}
}

void CFS_PathNotificator::RegisterPath(FS_Path& path)
{
	shared_str dir = path.m_Path;
	for (const auto& event : events)
	{
		if ((event.FDirectory == dir) && (event.bRecurse == path.m_Flags.is(FS_Path::flRecurse)))
			return;
	}

	events.push_back(Path());
	Path& P = events.back();
	P.FDirectory = path.m_Path;
	P.bRecurse = path.m_Flags.is(FS_Path::flRecurse);
	P.FChangeEvent.bind(&path, &FS_Path::rescan_path_cb);
	P.FWaitHandle = INVALID_HANDLE_VALUE;
}

void CFS_PathNotificator::Execute()
{
	OPTICK_THREAD("X-Ray File System notify thread");
	OPTICK_FRAME("X-Ray File System notify thread");

	// Инициализация обработчиков событий
	{
		std::lock_guard<std::mutex> lock(CS);
		for (auto& P : events)
		{
			P.FWaitHandle = FindFirstChangeNotification(P.FDirectory.c_str(), P.bRecurse, FNotifyOptionFlags);

			if (P.FWaitHandle == INVALID_HANDLE_VALUE)
			{
#ifndef __BORLANDC__
				Debug.fatal(DEBUG_INFO, "Can't create notify handle for path: '%s'\nwith error: '%s'",
							P.FDirectory.c_str(), Debug.error2string(GetLastError()));
#else
				Debug.fatal("Can't create notify handle for path: '%s'\nwith error: '%s'", P.FDirectory.c_str(),
							Debug.error2string(GetLastError()));
#endif
			}
		}
	}

	// Основной цикл обработки событий
	while (!Terminated)
	{
		std::vector<HANDLE> hHandles;
		hHandles.push_back(FMutex);

		for (const auto& event : events)
		{
			if (event.FWaitHandle != INVALID_HANDLE_VALUE)
				hHandles.push_back(event.FWaitHandle);
		}

		DWORD result = WaitForMultipleObjects(static_cast<DWORD>(hHandles.size()), hHandles.data(), FALSE, INFINITE);

		if (result == WAIT_OBJECT_0)
		{
			// Мьютекс освобожден - выходим
			ReleaseMutex(FMutex);
			break;
		}
		else if (result > WAIT_OBJECT_0)
		{
			DWORD idx = result - WAIT_OBJECT_0 - 1;
			if (idx < events.size())
			{
				Path& P = events[idx];
				if (!P.FChangeEvent.empty())
				{
					try
					{
						P.FChangeEvent();
					}
					catch (...)
					{
						// Игнорируем исключения в колбэках
					}
				}

				if (P.FWaitHandle != INVALID_HANDLE_VALUE)
					FindNextChangeNotification(P.FWaitHandle);
			}
		}
		else
		{
			// Ошибка ожидания
			break;
		}
	}

	// Очистка ресурсов
	for (auto& P : events)
	{
		if (P.FWaitHandle != INVALID_HANDLE_VALUE)
		{
			FindCloseChangeNotification(P.FWaitHandle);
			P.FWaitHandle = INVALID_HANDLE_VALUE;
		}
	}
}
//---------------------------------------------------------------------------

void CLocatorAPI::SetEventNotification()
{
	FThread = new CFS_PathNotificator();
	FThread->FNotifyOptionFlags =
		FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE;

	for (const auto& pathPair : pathes)
	{
		if (pathPair.second->m_Flags.is(FS_Path::flNotif))
			FThread->RegisterPath(*pathPair.second);
	}

	FThread->Start();
}

void CLocatorAPI::ClearEventNotification()
{
	if (FThread)
	{
		FThread->Terminate();

		// Разблокируем мьютекс, чтобы поток мог завершиться
		if (FThread->FMutex)
		{
			ReleaseMutex(FThread->FMutex);
			// Даем потоку время завершиться
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		xr_delete(FThread);
	}
}
