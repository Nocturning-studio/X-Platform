#pragma once

enum
{
	AF_GODMODE = (1 << 0),
	AF_INVISIBLE = (1 << 1),
	AF_ALWAYSRUN = (1 << 2),
	AF_UNLIMITEDAMMO = (1 << 3),
	AF_RUN_BACKWARD = (1 << 4),
	AF_AUTOPICKUP = (1 << 5),
	AF_PSP = (1 << 6),
	AF_ZOOM_TIME_SLOW_MO = (1 << 7),
	AF_LAST_CHANCE = (1 << 8),
	AF_NEED_DOF = (1 << 9)
};

extern Flags32 psActorFlags;

extern BOOL GodMode();
extern BOOL LastChanceMode();
