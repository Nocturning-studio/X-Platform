////////////////////////////////////////////////////////////////////////////////
// Created: 17.02.2026
// Author: NSDeathman
// Build identification
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "build_identificator.h"
////////////////////////////////////////////////////////////////////////////////
XRCORE_API xrBuildInfo GlobalBuildInfo;
////////////////////////////////////////////////////////////////////////////////
xrBuildInfo ComputeBuildIdentificator()
{
	xrBuildInfo BuildID = {0};

	LPSTR month_id[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	int days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	int start_day = 24;
	int start_month = 8;
	int start_year = 2022;

	BuildID.Date = __DATE__;

	int days, months, years = 0;
	string16 month;
	string256 buffer;
	strcpy_s(buffer, __DATE__);
	(void)sscanf(buffer, "%s %d %d", month, &days, &years);

	for (int i = 0; i < 12; i++)
	{
		if (_stricmp(month_id[i], month))
			continue;

		months = i;
		break;
	}

	BuildID.ID = (years - start_year) * 365 + days - start_day;

	for (int i = 0; i < months; ++i)
		BuildID.ID += days_in_month[i];

	for (int i = 0; i < start_month - 1; ++i)
		BuildID.ID -= days_in_month[i];

	BuildID.Type =
#if DEBUG
		"Debug"
#elif DEMO_BUILD
		"Demo"
#elif MASTER_GOLD
		"Gold master"
#elif NDEBUG
		"Release"
#else
		"Release"
#endif
		;

	return BuildID;
}

void PrintBuildIdentificator(xrBuildInfo BuildID)
{
	Msg("X-Ray Engine v1.0 (Edited)");
	Msg("Build ID: %d", BuildID.ID);
	Msg("Build date: %s", BuildID.Date);
	Msg("Build type: %s\n", BuildID.Type);
}

void InitializeGlobalBuildID()
{
	GlobalBuildInfo = ComputeBuildIdentificator();
	PrintBuildIdentificator(GlobalBuildInfo);
}
////////////////////////////////////////////////////////////////////////////////
