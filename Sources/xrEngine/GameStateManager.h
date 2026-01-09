#pragma once
#include "stdafx.h"
#include "EventAPI.h"
#include "pure.h"
#include "device.h"

class ENGINE_API CGameStateManager : public pureFrame, public IEventReceiver
{
  private:
	// События
	EVENT eQuit;
	EVENT eStart;
	EVENT eStartLoad;
	EVENT eDisconnect;

  public:
	CGameStateManager() = default;
	~CGameStateManager() = default;

	void Initialize();
	void Destroy();

	virtual void OnEvent(EVENT E, u64 P1, u64 P2);
	virtual void OnFrame();
};
