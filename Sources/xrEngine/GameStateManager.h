#pragma once
#include "stdafx.h"
#include "EventAPI.h"
#include "pure.h"

class ENGINE_API CGameStateManager : public IEventReceiver, public pureFrame
{
  private:
	// События движка
	EVENT eQuit;
	EVENT eStart;
	EVENT eStartLoad;
	EVENT eDisconnect;

  public:
	CGameStateManager() = default;
	~CGameStateManager() = default;

	void Initialize();
	void Destroy();

	// Обработка событий (Start, Stop, Quit)
	virtual void OnEvent(EVENT E, u64 P1, u64 P2);

	// Обновление каждый кадр (Discord, Spatial, и т.д.)
	virtual void OnFrame();
};
