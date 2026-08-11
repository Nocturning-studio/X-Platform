#include "stdafx.h"
#include "bloodsucker_vampire_effector.h"

//////////////////////////////////////////////////////////////////////////
// Vampire Postprocess Effector
//////////////////////////////////////////////////////////////////////////

CVampirePPEffector::CVampirePPEffector(const SPPInfo& ppi, float life_time)
	: inherited(EEffectorPPType(eCEHit), life_time)
{
	state = ppi;
	m_total = life_time;
}

#define TIME_ATTACK 0.2f
#define PERIODS 2
#define RAD_TO_PERC(rad) ((rad - PI_DIV_2) / (PERIODS * PI_MUL_2))
#define PERC_TO_RAD(perc) (perc * (PERIODS * PI_MUL_2) + PI_DIV_2)

BOOL CVampirePPEffector::Process(SPPInfo& pp)
{
	inherited::Process(pp);

	// amount of time passed in percents
	float time_past_perc = (m_total - fLifeTime) / m_total;

	float factor;
	if (time_past_perc < TIME_ATTACK)
	{
		factor = 0.75f * time_past_perc / TIME_ATTACK;
	}
	else if (time_past_perc > (1 - TIME_ATTACK))
	{
		factor = 0.75f * (1 - time_past_perc) / TIME_ATTACK;
	}
	else
	{
		float time_past_sine_perc = (time_past_perc - TIME_ATTACK) * (1 / (1 - TIME_ATTACK + TIME_ATTACK));
		factor = 0.5f + 0.25f * std::sin(PERC_TO_RAD(time_past_sine_perc));
	}

	clamp(factor, 0.01f, 1.0f);
	pp.lerp(pp_identity, state, factor);

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
// Vampire Camera Effector
//////////////////////////////////////////////////////////////////////////
#define V_EFF_DELTA_ANGLE_X 10 * PI / 180
#define V_EFF_DELTA_ANGLE_Y V_EFF_DELTA_ANGLE_X
#define V_EFF_DELTA_ANGLE_Z V_EFF_DELTA_ANGLE_X
#define V_EFF_ANGLE_SPEED 0.2f
#define V_EFF_BEST_DISTANCE 0.3f
CVampireCameraEffector::CVampireCameraEffector(float time, const fvec3& src, const fvec3& tgt)
	: inherited(eCEVampire, time)
{
	fLifeTime = time;
	m_time_total = time;

	m_dist = src.distance_to(tgt);

	if (m_dist < V_EFF_BEST_DISTANCE)
	{
		m_direction.sub(src, tgt);
		m_dist = V_EFF_BEST_DISTANCE - m_dist;
	}
	else
	{
		m_direction.sub(tgt, src);
		m_dist = m_dist - V_EFF_BEST_DISTANCE;
	}

	m_direction.normalize();

	dangle_target.set(Random.randFs(V_EFF_DELTA_ANGLE_X), 
					  Random.randFs(V_EFF_DELTA_ANGLE_Y),
					  Random.randFs(V_EFF_DELTA_ANGLE_Z));
	dangle_current.set(0.f, 0.f, 0.f);
}

BOOL CVampireCameraEffector::ProcessCam(SCamEffectorInfo& info)
{
	fLifeTime -= Engine.TimeManager.GetDeltaTime();
	if (fLifeTime < 0)
		return FALSE;

	// процент оставшегося времени
	float time_left_perc = fLifeTime / m_time_total;

	// Инициализация
	fmat4x4 Mdef;
	Mdef.identity();
	Mdef.j.set(info.n);
	Mdef.k.set(info.d);
	Mdef.i.crossproduct(info.n, info.d);
	Mdef.c.set(info.p);

	//////////////////////////////////////////////////////////////////////////
	// using formula: y = k - 2*k*abs(x-1/2)   k - max distance
	// float	cur_dist = m_dist * (1 - 2*_abs((1-time_left_perc) - 0.5f));
	float time_passed = 1 - time_left_perc;
	float cur_dist = m_dist * (std::sqrt(0.5f * 0.5f - (time_passed - 0.5f) * (time_passed - 0.5f)));

	Mdef.c.mad(m_direction, cur_dist);

	// check the time to return
	if (time_left_perc < 0.2f)
	{

		dangle_target.x = 0.f;
		dangle_target.y = 0.f;
		dangle_target.z = 0.f;

		angle_lerp(dangle_current.x, dangle_target.x, _abs(dangle_current.x / fLifeTime + 0.001f), Engine.TimeManager.GetDeltaTime());
		angle_lerp(dangle_current.y, dangle_target.y, _abs(dangle_current.y / fLifeTime + 0.001f), Engine.TimeManager.GetDeltaTime());
		angle_lerp(dangle_current.z, dangle_target.z, _abs(dangle_current.z / fLifeTime + 0.001f), Engine.TimeManager.GetDeltaTime());
	}
	else
	{

		if (angle_lerp(dangle_current.x, dangle_target.x, V_EFF_ANGLE_SPEED, Engine.TimeManager.GetDeltaTime()))
		{
			dangle_target.x = Random.randFs(V_EFF_DELTA_ANGLE_X);
		}

		if (angle_lerp(dangle_current.y, dangle_target.y, V_EFF_ANGLE_SPEED, Engine.TimeManager.GetDeltaTime()))
		{
			dangle_target.y = Random.randFs(V_EFF_DELTA_ANGLE_Y);
		}

		if (angle_lerp(dangle_current.z, dangle_target.z, V_EFF_ANGLE_SPEED, Engine.TimeManager.GetDeltaTime()))
		{
			dangle_target.z = Random.randFs(V_EFF_DELTA_ANGLE_Z);
		}
	}

	//////////////////////////////////////////////////////////////////////////

	// Установить углы смещения
	fmat4x4 R;
	R.setHPB(dangle_current.x, dangle_current.y, dangle_current.z);

	fmat4x4 mR;
	mR.mul(Mdef, R);

	info.d.set(mR.k);
	info.n.set(mR.j);
	info.p.set(mR.c);

	return TRUE;
}
