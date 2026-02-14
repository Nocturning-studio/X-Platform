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
	float3 dangle_target;
	float3 dangle_current;

	float m_dist;
	float3 m_direction;

  public:
	CVampireCameraEffector(float time, const float3& src, const float3& tgt);
	virtual BOOL ProcessCam(SCamEffectorInfo& info);
};
