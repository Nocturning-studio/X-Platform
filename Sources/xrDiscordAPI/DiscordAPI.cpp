/////////////////////////////////////////////////////////////////////
//	Created: 17.11.2024
//	Authors: Maks0, morrazzzz, NS_NSDeathman
/////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "DiscordAPI.h"
/////////////////////////////////////////////////////////////////////
#pragma todo(NSDeathman to NSDeathman : Проверить функциональность)
#pragma todo(NSDeathman to NSDeathman : Добавить скриптовую систему достижений и интеграцию ее в DiscordAPI)
/////////////////////////////////////////////////////////////////////
DISCORDAPI_API CDiscordAPI DiscordAPI;
/////////////////////////////////////////////////////////////////////
namespace Detail
{
	void Log(const char* format, ...)
	{
		va_list mark;
		char buf[1024];
		va_start(mark, format);
		int sz = _vsnprintf(buf, sizeof(buf) - 1, format, mark);
		buf[sizeof(buf) - 1] = 0;
		va_end(mark);
		if (sz)
			printf(buf);
	}

	std::string ANSIToUTF8(const std::string& string)
	{
		wchar_t* wcs{};
		int Lenght_ = MultiByteToWideChar(1251, 0, string.c_str(), (int)string.size(), wcs, 0);
		wcs = new wchar_t[Lenght_ + 1];
		MultiByteToWideChar(1251, 0, string.c_str(), (int)string.size(), wcs, Lenght_);
		wcs[Lenght_] = L'\0';
		char* u8s = nullptr;
		Lenght_ = WideCharToMultiByte(CP_UTF8, 0, wcs, (int)std::wcslen(wcs), u8s, 0, nullptr, nullptr);
		u8s = new char[Lenght_ + 1];
		WideCharToMultiByte(CP_UTF8, 0, wcs, (int)std::wcslen(wcs), u8s, Lenght_, nullptr, nullptr);
		u8s[Lenght_] = '\0';
		std::string result(u8s);
		delete[] wcs;
		delete[] u8s;
		return result;
	}
}

CDiscordAPI::~CDiscordAPI()
{
	delete m_DiscordCore;
	delete m_ActivityDiscord;
}

void CDiscordAPI::Init()
{
	Detail::Log("\nInitializing DiscordAPI...");
	auto ResultSDK_ = discord::Core::Create(m_AppID, DiscordCreateFlags_NoRequireDiscord, &m_DiscordCore);

	if (!m_DiscordCore)
	{
		int ErrorCode = static_cast<int>(ResultSDK_);
		Detail::Log("! DiscordAPI: error while initializing");
		Detail::Log("! Error Code: [%d]", ErrorCode);
		Detail::Log("! Error Assotiation:");
		AssotiateError(ErrorCode);
		return;
	}

	m_ActivityDiscord = new discord::Activity();
	m_ActivityDiscord->GetAssets().SetLargeImage("logo_main");
	m_ActivityDiscord->GetAssets().SetSmallImage("logo_main");

	m_ActivityDiscord->SetInstance(true);
	m_ActivityDiscord->SetType(discord::ActivityType::Playing);
	m_ActivityDiscord->GetTimestamps().SetStart(time(nullptr));

	m_NeedUpdateActivity = true;
}

void CDiscordAPI::AssotiateError(int ErrorCode)
{
	switch (ErrorCode)
	{
	case 0:
		Detail::Log("- DiscordResult: Ok");
		break;
	case 1:
		Detail::Log("! DiscordResult: Service Unavailable");
		break;
	case 2:
		Detail::Log("! DiscordResult: Invalid Version");
		break;
	case 3:
		Detail::Log("! DiscordResult: Lock Failed");
		break;
	case 4:
		Detail::Log("! DiscordResult: Internal Error");
		break;
	case 5:
		Detail::Log("! DiscordResult: Invalid Payload");
		break;
	case 6:
		Detail::Log("! DiscordResult: Invalid Command");
		break;
	case 7:
		Detail::Log("! DiscordResult: Invalid Permissions");
		break;
	case 8:
		Detail::Log("! DiscordResult: Not Fetched");
		break;
	case 9:
		Detail::Log("! DiscordResult: Not Found");
		break;
	case 10:
		Detail::Log("! DiscordResult: Conflict");
		break;
	case 11:
		Detail::Log("! DiscordResult: Invalid Secret");
		break;
	case 12:
		Detail::Log("! DiscordResult: Invalid Join Secret");
		break;
	case 13:
		Detail::Log("! DiscordResult: No Eligible Activity");
		break;
	case 14:
		Detail::Log("! DiscordResult: Invalid Invite");
		break;
	case 15:
		Detail::Log("! DiscordResult: Not Authenticated");
		break;
	case 16:
		Detail::Log("! DiscordResult: Invalid Access Token");
		break;
	case 17:
		Detail::Log("! DiscordResult: Application Mismatch");
		break;
	case 18:
		Detail::Log("! DiscordResult: Invalid Data Url");
		break;
	case 19:
		Detail::Log("! DiscordResult: Invalid Base 64");
		break;
	case 20:
		Detail::Log("! DiscordResult: Not Filtered");
		break;
	case 21:
		Detail::Log("! DiscordResult: Lobby Full");
		break;
	case 22:
		Detail::Log("! DiscordResult: Invalid Lobby Secret");
		break;
	case 23:
		Detail::Log("! DiscordResult: Invalid Filename");
		break;
	case 24:
		Detail::Log("! DiscordResult: Invalid File Size");
		break;
	case 25:
		Detail::Log("! DiscordResult: Invalid Entitlement");
		break;
	case 26:
		Detail::Log("! DiscordResult: Not Installed");
		break;
	case 27:
		Detail::Log("! DiscordResult: Not Running");
		break;
	case 28:
		Detail::Log("! DiscordResult: Insufficient Buffer");
		break;
	case 29:
		Detail::Log("! DiscordResult: Purchase Canceled");
		break;
	case 30:
		Detail::Log("! DiscordResult: Invalid Guild");
		break;
	case 31:
		Detail::Log("! DiscordResult: Invalid Event");
		break;
	case 32:
		Detail::Log("! DiscordResult: Invalid Channel");
		break;
	case 33:
		Detail::Log("! DiscordResult: Invalid Origin");
		break;
	case 34:
		Detail::Log("! DiscordResult: Rate Limited");
		break;
	case 35:
		Detail::Log("! DiscordResult: OAuth2 Error");
		break;
	case 36:
		Detail::Log("! DiscordResult: Select Channel Timeout");
		break;
	case 37:
		Detail::Log("! DiscordResult: Get Guild Timeout");
		break;
	case 38:
		Detail::Log("! DiscordResult: Select Voice Force Required");
		break;
	case 39:
		Detail::Log("! DiscordResult: Capture Shortcut Already Listening");
		break;
	case 40:
		Detail::Log("! DiscordResult: Unauthorized For Achievement");
		break;
	case 41:
		Detail::Log("! DiscordResult: Invalid Gift Codet");
		break;
	case 42:
		Detail::Log("! DiscordResult: Purchase Error");
		break;
	case 43:
		Detail::Log("! DiscordResult: Transaction Aborted");
		break;
	case 44:
		Detail::Log("! DiscordResult: Drawing Init Failed");
		break;
    }
}

void CDiscordAPI::Update()
{
	if (!m_DiscordCore)
		return;

	if (m_NeedUpdateActivity)
		UpdateActivity();

	m_DiscordCore->RunCallbacks();
}

void CDiscordAPI::UpdateActivity()
{
	m_ActivityDiscord->SetState(Detail::ANSIToUTF8(m_PhaseDiscord).c_str());
	m_ActivityDiscord->SetDetails(Detail::ANSIToUTF8(m_StatusDiscord).c_str());

	m_DiscordCore->ActivityManager().UpdateActivity(*m_ActivityDiscord, [](discord::Result result)
	{
		if (result != discord::Result::Ok)
			Detail::Log("! DiscordAPI: Invalid UpdateActivity");
	});

	m_NeedUpdateActivity = false;
}

void CDiscordAPI::SetPhase(const LPCSTR phase)
{
	m_PhaseDiscord = phase;
	m_NeedUpdateActivity = true;
}

void CDiscordAPI::SetStatus(const LPCSTR status)
{
	m_StatusDiscord = status;
	m_NeedUpdateActivity = true;
}
/////////////////////////////////////////////////////////////////////
