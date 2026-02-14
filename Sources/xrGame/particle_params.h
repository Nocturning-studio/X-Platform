////////////////////////////////////////////////////////////////////////////
//	Module 		: particle_params.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Particle parameters class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_export_space.h"

class CParticleParams
{
  public:
	float3 m_tParticlePosition;
	float3 m_tParticleAngles;
	float3 m_tParticleVelocity;

  public:
	IC CParticleParams(const float3& tPositionOffset = float3().set(0, 0, 0),
					   const float3& tAnglesOffset = float3().set(0, 0, 0),
					   const float3& tVelocity = float3().set(0, 0, 0));
	virtual ~CParticleParams();
	IC void initialize();

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CParticleParams)
#undef script_type_list
#define script_type_list save_type_list(CParticleParams)

#include "particle_params_inline.h"
