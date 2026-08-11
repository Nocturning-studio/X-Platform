////////////////////////////////////////////////////////////////////////////
//	Module 		: particle_params_inline.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Particle parameters class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC CParticleParams::CParticleParams(const fvec3& tPositionOffset, const fvec3& tAnglesOffset,
									const fvec3& tVelocity)
{
	m_tParticlePosition = tPositionOffset;
	m_tParticleAngles = tAnglesOffset;
	m_tParticleVelocity = tVelocity;
}

IC void CParticleParams::initialize()
{
}
