#pragma once
#include "stdafx.h"
#include "EventAPI.h"
#include "pure.h"
#include "device.h"

class ENGINE_API CApplication : public pureFrame, public IEventReceiver
{
  private:
	// События
	EVENT eQuit;
	EVENT eStart;
	EVENT eStartLoad;
	EVENT eDisconnect;

  public:

	virtual void OnEvent(EVENT E, u64 P1, u64 P2);

	CApplication();
	~CApplication();

	virtual void OnFrame();
};

extern ENGINE_API CApplication* pApp;
