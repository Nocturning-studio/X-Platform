////////////////////////////////////////////////////////////////////////////
//	Module 		: danger_location.cpp
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Danger location
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "danger_location.h"

bool CDangerLocation::useful() const
{
	return (!(Engine.TimeManager.GetGlobalTimeMs() > m_level_time + m_interval));
}
