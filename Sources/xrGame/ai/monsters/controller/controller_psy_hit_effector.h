#pragma once
#include "../../../../xrEngine/effector.h"
#include "../../../pp_effector_custom.h"

////////////////////////////////////////////////////////////////////////////////////
// CPsyHitEffectorPP
////////////////////////////////////////////////////////////////////////////////////

// class CPsyHitEffectorPP : public CPPEffectorController {
//	typedef CPPEffectorController inherited;
//
//	float			m_attack_perc;
//	float			m_release_perc;
//
// public:
//	virtual void	load					(LPCSTR section);
//	virtual bool	check_completion		();
//	virtual bool	check_start_conditions	();
//	virtual void	update_factor			();
//
//	virtual CPPEffectorControlled *create_effector	();
// };

////////////////////////////////////////////////////////////////////////////////////
// CPsyHitEffectorCam
////////////////////////////////////////////////////////////////////////////////////

// class CPsyHitEffectorCam : public CEffector {
//	typedef CEffector inherited;
//
//	float	m_time_total;
//	float3	dangle_target;
//	float3 dangle_current;
//
// public:
//					CPsyHitEffectorCam	(EEffectorType type);
//	virtual	BOOL	Process				(float3 &p, float3 &d, float3 &n, float& fFov, float& fFar, float& fAspect);
// };
