#ifndef LocatorAPI_NotificationsH
#define LocatorAPI_NotificationsH
#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <windows.h>

class CThread
{
  protected:
	volatile u32 thID;
	std::atomic<bool> Terminated;
	std::thread threadHandle;

  public:
	CThread(u32 _ID) : thID(_ID), Terminated(false)
	{
	}
	virtual ~CThread()
	{
		Terminate();
		if (threadHandle.joinable())
			threadHandle.join();
	}

	void Start()
	{
		threadHandle = std::thread([this]() { this->Execute(); });
	}

	virtual void Execute() = 0;

	void Terminate()
	{
		Terminated = true;
	}
};

class CFS_PathNotificator : public CThread
{
  private:
	struct Path
	{
		shared_str FDirectory;
		HANDLE FWaitHandle;
		fastdelegate::FastDelegate0<> FChangeEvent;
		BOOL bRecurse;
	};

	xr_vector<Path> events;
	HANDLE FMutex;
	unsigned FNotifyOptionFlags;

  protected:
	virtual void Execute();

  public:
	CFS_PathNotificator();
	virtual ~CFS_PathNotificator();
	void RegisterPath(FS_Path& path);
};

#endif // LocatorAPI_NotificationsH
