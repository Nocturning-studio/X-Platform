/////////////////////////////////////////////////////////////////////
//	Created: 17.11.2024
//	Authors: Maks0, morrazzzz, NS_NSDeathman
/////////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////////
#include "stdafx.h"
/////////////////////////////////////////////////////////////////////
namespace discord
{
	class Activity;
	class Core;
}
/////////////////////////////////////////////////////////////////////
class DISCORDAPI_API CDiscordAPI
{
  private:
	LPCSTR m_StatusDiscord = "";
	LPCSTR m_PhaseDiscord = "";

	std::atomic_bool m_NeedUpdateActivity;

	int64_t m_AppID = 1307777829374656562;

	discord::Activity* m_ActivityDiscord = nullptr;
	discord::Core* m_DiscordCore = nullptr;

  public:
	void Init();
	void AssotiateError(int ErrorCode);

	void Update();
	void UpdateActivity();
	void SetPhase(const LPCSTR phase);
	void SetStatus(const LPCSTR status);
	void SetAppID(int64_t appId)
	{
		m_AppID = appId;
	}

	CDiscordAPI() = default;
	~CDiscordAPI();
};
/////////////////////////////////////////////////////////////////////
extern DISCORDAPI_API CDiscordAPI DiscordAPI;
/////////////////////////////////////////////////////////////////////
