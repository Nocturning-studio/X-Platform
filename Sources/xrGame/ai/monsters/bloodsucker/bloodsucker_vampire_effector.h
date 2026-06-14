#pragma once

#include "../../../../xrEngine/effectorPP.h"
#include "../../../CameraEffector.h"
#include "../../../../xrEngine/cameramanager.h"

//////////////////////////////////////////////////////////////////////////
// Vampire Postprocess Effector
//////////////////////////////////////////////////////////////////////////
class CVampirePPEffector : public CEffectorPP
{
	typedef CEffectorPP inherited;

	SPPInfo state; // current state
	float m_total; // total PP time

  public:
	CVampirePPEffector(const SPPInfo& ppi, float life_time);
	virtual BOOL Process(SPPInfo& pp);
};

//////////////////////////////////////////////////////////////////////////
// Vampire Camera Effector
//////////////////////////////////////////////////////////////////////////
class CVampireCameraEffector : public CEffectorCam
{
	typedef CEffectorCam inherited;

	float m_time_total;
	fvec3 dangle_target;
	fvec3 dangle_current;

	float m_dist;
	fvec3 m_direction;

  public:
	CVampireCameraEffector(float time, const fvec3& src, const fvec3& tgt);
	virtual BOOL ProcessCam(SCamEffectorInfo& info);
};
