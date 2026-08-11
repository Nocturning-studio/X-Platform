////////////////////////////////////////////////////////////////////////////
//	Module 		: danger_cover_location.cpp
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Danger cover location
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "danger_cover_location.h"
#include "cover_point.h"

const fvec3& CDangerCoverLocation::position() const
{
	return (m_cover->position());
}
