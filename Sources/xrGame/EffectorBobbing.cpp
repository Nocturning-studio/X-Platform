////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Last modified: 24.08.2022
// Modifier: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <math.h>
#include "stdafx.h"
#include "EffectorBobbing.h"

#include "actor.h"
#include "actor_defs.h"
#include "../xrEngine/xr_ioc_cmd.h"

#define BOBBING_SECT "bobbing_effector"

#define GLOBAL_VIEW_BOBBING_FACTOR 0.75f
#define SPRINT_FACTOR 1.1f
#define STRAFE_FACTOR 0.75f
#define CROUCH_FACTOR 0.5f
#define ZOOM_FACTOR 0.5f

#define GLOBAL_VIEW_BOBBING_INTENCITY_FACTOR 1.0f
#define SPRINT_BOBBING_INTENCITY_FACTOR 1.1f
#define STRAFE_BOBBING_INTENCITY_FACTOR 0.9f
#define CROUCH_BOBBING_INTENCITY_FACTOR 0.5f
#define ZOOM_BOBBING_INTENCITY_FACTOR 0.5f

#define SPRINT_FOV_MODIFIER_FACTOR 1.0015f
#define WALK_FOV_MODIFIER_FACTOR 1.0001f
#define BACKWARD_WALK_FOV_MODIFIER_FACTOR 0.999f
#define CROUCH_WALK_FOV_MODIFIER_FACTOR 0.999f

#define SPEED_REMINDER 5.f
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEffectorBobbing::CEffectorBobbing() : CEffectorCam(eCEBobbing, 10000.f)
{
	fTime = 0;
	fReminderFactor = 0;
	is_limping = false;

	m_fAmplitudeRun = pSettings->r_float(BOBBING_SECT, "run_amplitude");
	m_fAmplitudeWalk = pSettings->r_float(BOBBING_SECT, "walk_amplitude");
	m_fAmplitudeLimp = pSettings->r_float(BOBBING_SECT, "limp_amplitude");

	m_fSpeedRun = pSettings->r_float(BOBBING_SECT, "run_speed");
	m_fSpeedWalk = pSettings->r_float(BOBBING_SECT, "walk_speed");
	m_fSpeedLimp = pSettings->r_float(BOBBING_SECT, "limp_speed");
}

CEffectorBobbing::~CEffectorBobbing()
{
}

void CEffectorBobbing::SetState(u32 mstate, bool limping, bool ZoomMode)
{
	dwMState = mstate;
	is_limping = limping;
	m_bZoomMode = ZoomMode;
}

BOOL CEffectorBobbing::ProcessCam(SCamEffectorInfo& info)
{
	fTime += Engine.TimeManager.GetDeltaTime();
	if (dwMState & ACTOR_DEFS::mcAnyMove)
	{
		if (fReminderFactor < 1.f)
			fReminderFactor += SPEED_REMINDER * Engine.TimeManager.GetDeltaTime();
		else
			fReminderFactor = 1.f;
	}
	else
	{
		if (fReminderFactor > 0.f)
			fReminderFactor -= SPEED_REMINDER * Engine.TimeManager.GetDeltaTime();
		else
			fReminderFactor = 0.f;
	}

	if (!fsimilar(fReminderFactor, 0))
	{
		float4x4 M;
		M.identity();
		M.j.set(info.n);
		M.k.set(info.d);
		M.i.crossproduct(info.n, info.d);
		M.c.set(info.p);

		// apply footstep bobbing effect
		float k = GLOBAL_VIEW_BOBBING_FACTOR;

		if (dwMState & ACTOR_DEFS::mcCrouch)
			k *= CROUCH_FACTOR;
		if ((dwMState & ACTOR_DEFS::mcLStrafe) || (dwMState & ACTOR_DEFS::mcRStrafe))
			k *= STRAFE_FACTOR;
		if (dwMState & ACTOR_DEFS::mcSprint)
			k *= SPRINT_FACTOR;
		if (m_bZoomMode)
			k *= ZOOM_FACTOR;

		float Intencity = GLOBAL_VIEW_BOBBING_INTENCITY_FACTOR;

		if (dwMState & ACTOR_DEFS::mcCrouch)
			Intencity *= CROUCH_BOBBING_INTENCITY_FACTOR;
		if ((dwMState & ACTOR_DEFS::mcLStrafe) || (dwMState & ACTOR_DEFS::mcRStrafe))
			Intencity *= STRAFE_BOBBING_INTENCITY_FACTOR;
		if (dwMState & ACTOR_DEFS::mcSprint)
			Intencity *= SPRINT_BOBBING_INTENCITY_FACTOR;
		if (m_bZoomMode)
			Intencity *= ZOOM_BOBBING_INTENCITY_FACTOR;

		float A, ST;

		if (isActorAccelerated(dwMState, m_bZoomMode))
		{
			A = m_fAmplitudeRun * k;
			ST = m_fSpeedRun * fTime * k;
		}
		else if (is_limping)
		{
			A = m_fAmplitudeLimp * k;
			ST = m_fSpeedLimp * fTime * k;
		}
		else
		{
			A = m_fAmplitudeWalk * k;
			ST = m_fSpeedWalk * fTime * k;
		}

		if (ps_effectors_ls_flags.test(DYNAMIC_FOV_ENABLED))
		{
			float fov_modifier = 1.0f;

			if (dwMState & ACTOR_DEFS::mcSprint)
				fov_modifier = SPRINT_FOV_MODIFIER_FACTOR;
			if (dwMState & ACTOR_DEFS::mcFwd)
				fov_modifier = WALK_FOV_MODIFIER_FACTOR;
			if (dwMState & ACTOR_DEFS::mcBack)
				fov_modifier = BACKWARD_WALK_FOV_MODIFIER_FACTOR;
			if (dwMState & ACTOR_DEFS::mcCrouch)
				fov_modifier = CROUCH_WALK_FOV_MODIFIER_FACTOR;

			info.fFov *= fov_modifier + Engine.TimeManager.GetDeltaTime();
		}

		float _sinA = _abs(_sin(ST * Intencity) * A) * fReminderFactor;
		float _cosA = _cos(ST * Intencity) * A * fReminderFactor;

		float3 dangle;
		dangle.x = _cosA;
		dangle.y = _sinA;
		dangle.z = _cosA;

		float4x4 R;
		R.setHPB(dangle.x, dangle.y, dangle.z);

		float4x4 mR;
		mR.mul(M, R);

		info.d.set(mR.k);
		info.n.set(mR.j);
	}

	return TRUE;
}
