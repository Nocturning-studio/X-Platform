////////////////////////////////////////////////////////////////////////////////
// Created: 14.01.2025
// Author: NSDeathman
// Refactored code: Build identification
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
////////////////////////////////////////////////////////////////////////////////
XRCORE_API LPCSTR build_date;
XRCORE_API u32 build_id;
////////////////////////////////////////////////////////////////////////////////
void ComputeBuildIdentificator()
{
	LPSTR month_id[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	int days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	int start_day = 24;
	int start_month = 8;
	int start_year = 2022;

	build_date = __DATE__;

	int days;
	int months = 0;
	int years;
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

	build_id = (years - start_year) * 365 + days - start_day;

	for (int i = 0; i < months; ++i)
		build_id += days_in_month[i];

	for (int i = 0; i < start_month - 1; ++i)
		build_id -= days_in_month[i];
}

void PrintBuildIdentificator()
{
	Msg("X-Ray Engine v1.0 (Edited)");
	Msg("Build ID: %d", build_id);
	Msg("Build date: %s", build_date);

	string256 BuildType;
	strcpy(BuildType, "");
	strconcat(	sizeof(BuildType), 
				BuildType, 
				BuildType,
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
			);

	Msg("Build type: %s\n", BuildType);
}
////////////////////////////////////////////////////////////////////////////////
