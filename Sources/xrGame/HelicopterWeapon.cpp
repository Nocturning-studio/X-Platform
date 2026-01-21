#include "stdafx.h"
#include "helicopter.h"
#include "ExplosiveRocket.h"
#include "xrMessages.h"
#include "../xrEngine/../xrNetServer/net_utils.h"
#include "..\xrEngine\skeletoncustom.h"
#include "Level.h"

void CHelicopter::BoneMGunCallbackX(CBoneInstance* B)
{
	CHelicopter* P = static_cast<CHelicopter*>(B->Callback_Param);
	Fmatrix rX;
	rX.rotateX(P->m_cur_rot.x);
	B->mTransform.mulB_43(rX);
}

void CHelicopter::BoneMGunCallbackY(CBoneInstance* B)
{
	CHelicopter* P = static_cast<CHelicopter*>(B->Callback_Param);
	Fmatrix rY;
	rY.rotateY(P->m_cur_rot.y);
	B->mTransform.mulB_43(rY);
}

void CHelicopter::OnEvent(NET_Packet& P, u16 type)
{
	inherited::OnEvent(P, type);
	CExplosive::OnEvent(P, type);
	u16 id;
	switch (type)
	{
	case GE_OWNERSHIP_TAKE: {
		P.r_u16(id);
		CRocketLauncher::AttachRocket(id, this);
	}
	break;
	case GE_OWNERSHIP_REJECT:
	case GE_LAUNCH_ROCKET: {
		bool bLaunch = (type == GE_LAUNCH_ROCKET);
		P.r_u16(id);
		CRocketLauncher::DetachRocket(id, bLaunch);
	}
	break;
	}
}

void CHelicopter::MGunUpdateFire()
{

	fTime -= Engine.TimeManager.GetDeltaTime();
	if (delta_t < 0)
	{
		delta_t = Engine.TimeManager.GetGlobalTime();
		flag_by_fire = 0;
	}
	float time_f = Engine.TimeManager.GetGlobalTime() - delta_t;

	float fire_time;
	if (pSettings->line_exist(*cNameSect(), "fire_time"))
		fire_time = pSettings->r_float(*cNameSect(), "fire_time");
	else
		fire_time = -1;

	float no_fire_time;
	if (pSettings->line_exist(*cNameSect(), "no_fire_time"))
		no_fire_time = pSettings->r_float(*cNameSect(), "no_fire_time");
	else
		no_fire_time = -1;

	CShootingObject::UpdateFlameParticles();
	CShootingObject::UpdateLight();

	if (!IsWorking())
	{
		if (fTime < 0)
			fTime = 0.f;
		return;
	}
	if (no_fire_time > 0 && fire_time > 0)
	{
		if (flag_by_fire == 1 && time_f > fire_time)
		{
			delta_t = Engine.TimeManager.GetGlobalTime();
			time_f = Engine.TimeManager.GetGlobalTime() - delta_t;
			flag_by_fire = 0;
		}
		if (time_f > no_fire_time && flag_by_fire == 0)
		{
			delta_t = Engine.TimeManager.GetGlobalTime();
			time_f = Engine.TimeManager.GetGlobalTime() - delta_t;
			flag_by_fire = 1;
		}
		if (flag_by_fire == 0 && time_f < no_fire_time)
			return;
	}

	if (fTime <= 0)
	{
		OnShot();
		fTime += fTimeToFire;
	}
}
void CHelicopter::OnShot()
{
	Fvector fire_pos, fire_dir;
	fire_pos = get_CurrentFirePoint();
	fire_dir = m_fire_dir;

	float fire_trail_speed = 15.0f;
	clamp(fire_trail_speed, GetCurrVelocity(), 300.0f);
	if (m_enemy.bUseFireTrail)
	{
		Fvector enemy_pos = m_enemy.destEnemyPos;

		float dt = Engine.TimeManager.GetGlobalTime() - m_enemy.fStartFireTime;
		VERIFY(dt >= 0);
		float dist = m_enemy.fire_trail_length_curr - dt * fire_trail_speed;
		if (dist < 0)
		{
			MGunFireEnd();
			return;
		}

		Fvector fp = fire_pos;
		fp.y = enemy_pos.y;
		Fvector fd;
		fd.sub(enemy_pos, fp).normalize_safe();
		if (dist > (m_enemy.fire_trail_length_curr / 2.0f))
		{
			fd.mul(-1.0f);
			dist = dist - (m_enemy.fire_trail_length_curr / 2.0f);
		}
		else
		{
			dist = (m_enemy.fire_trail_length_curr / 2.0f) - dist;
		}

		static float fire_trace_width = pSettings->r_float(*cNameSect(), "fire_trace_width");
		enemy_pos.mad(fd, dist);
		Fvector disp_dir;
		disp_dir.random_point(fire_trace_width);

		enemy_pos.add(disp_dir);
		fire_dir.sub(enemy_pos, fire_pos).normalize_safe();
	};

	FireBullet(fire_pos, fire_dir, fireDispersionBase, m_CurrentAmmo, ID(), ID(), OnServer());

	StartShotParticles();
	if (m_bLightShotEnabled)
		Light_Start();

	StartFlameParticles();
	StartSmokeParticles(fire_pos, zero_vel);
	OnShellDrop(fire_pos, zero_vel);

	HUD_SOUND::PlaySound(m_sndShot, fire_pos, this, false);
}

void CHelicopter::MGunFireStart()
{
	if (!m_use_mgun_on_attack)
		return;

	if (FALSE == IsWorking() && m_enemy.bUseFireTrail)
	{
		// start calc fire trail
		m_enemy.fStartFireTime = Engine.TimeManager.GetGlobalTime();
		Fvector fp = get_CurrentFirePoint();
		Fvector ep = m_enemy.destEnemyPos;

		// calc min firetrail length
		float h = fp.y - ep.y;
		if (h > 0.0f)
		{
			float dl = h * tanf(m_lim_x_rot.y);
			float ds = fp.distance_to_xz(ep);
			if (ds > dl)
			{
				float half_trail = ds - dl;
				m_enemy.fire_trail_length_curr = half_trail * 2.0f;
				clamp(m_enemy.fire_trail_length_curr, 0.0f, m_enemy.fire_trail_length_des);
				//				Msg("Start fire. Desired length=%f,
				//cur_length=%f",m_enemy.fire_trail_length_des,m_enemy.fire_trail_length_curr);
			}
			else
				m_enemy.fire_trail_length_curr = m_enemy.fire_trail_length_des;
		}
		else
			m_enemy.fire_trail_length_curr = m_enemy.fire_trail_length_des;
	}

	CShootingObject::FireStart();
}

void CHelicopter::MGunFireEnd()
{
	CShootingObject::FireEnd();
	StopFlameParticles();
	m_enemy.fStartFireTime = -1.0f;
}

bool between(const float& src, const float& min, const float& max)
{
	return ((src > min) && (src < max));
}

void CHelicopter::UpdateWeapons()
{
	if (isOnAttack())
	{
		UpdateMGunDir();
	}
	else
	{
		m_tgt_rot.set(0.0f, 0.0f);
	};

	// lerp angle
	angle_lerp(m_cur_rot.x, m_tgt_rot.x, PI, Engine.TimeManager.GetDeltaTime());
	angle_lerp(m_cur_rot.y, m_tgt_rot.y, PI, Engine.TimeManager.GetDeltaTime());

	if (isOnAttack())
	{

		if (m_allow_fire)
		{

			float d = Transform().c.distance_to_xz(m_enemy.destEnemyPos);

			if (between(d, m_min_mgun_dist, m_max_mgun_dist))
				MGunFireStart();

			if (between(d, m_min_rocket_dist, m_max_rocket_dist) &&
				(Engine.TimeManager.GetGlobalTimeMs() - m_last_rocket_attack > m_time_between_rocket_attack))
			{
				if (m_syncronize_rocket)
				{
					startRocket(1);
					startRocket(2);
				}
				else
				{
					if (m_last_launched_rocket == 1)
						startRocket(2);
					else
						startRocket(1);
				}

				m_last_rocket_attack = Engine.TimeManager.GetGlobalTimeMs();
			}
		}
		else
		{
			MGunFireEnd();
		}
	}
	else
		MGunFireEnd();

	MGunUpdateFire();
}

void CHelicopter::UpdateMGunDir()
{
	CKinematics* K = smart_cast<CKinematics*>(Visual());
	m_fire_bone_transform = K->LL_GetTransform(m_fire_bone);

	m_fire_bone_transform.mulA_43(Transform());
	m_fire_pos.set(0, 0, 0);
	m_fire_bone_transform.transform_tiny(m_fire_pos);
	m_fire_dir.set(0, 0, 1);
	m_fire_bone_transform.transform_dir(m_fire_dir);

	m_fire_dir.sub(m_enemy.destEnemyPos, m_fire_pos).normalize_safe();

	m_left_rocket_bone_transform = K->LL_GetTransform(m_left_rocket_bone);
	m_left_rocket_bone_transform.mulA_43(Transform());
	m_left_rocket_bone_transform.c.y += 1.0f;
	//.fake
	m_right_rocket_bone_transform = K->LL_GetTransform(m_right_rocket_bone);
	m_right_rocket_bone_transform.mulA_43(Transform());
	m_right_rocket_bone_transform.c.y += 1.0f;
	//.fake

	m_allow_fire = TRUE;
	Fmatrix XFi;
	XFi.invert(Transform());
	Fvector dep;
	XFi.transform_tiny(dep, m_enemy.destEnemyPos);
	{ // x angle
		Fvector A_;
		A_.sub(dep, m_bind_x);
		m_i_bind_x_transform.transform_dir(A_);
		A_.normalize();
		m_tgt_rot.x = angle_normalize_signed(m_bind_rot.x - A_.getP());
		float sv_x = m_tgt_rot.x;
		clamp(m_tgt_rot.x, -m_lim_x_rot.y, -m_lim_x_rot.x);
		if (!fsimilar(sv_x, m_tgt_rot.x, EPS_L))
			m_allow_fire = FALSE;
	}
	{ // y angle
		Fvector A_;
		A_.sub(dep, m_bind_y);
		m_i_bind_y_transform.transform_dir(A_);
		A_.normalize();
		m_tgt_rot.y = angle_normalize_signed(m_bind_rot.y - A_.getH());
		float sv_y = m_tgt_rot.y;
		clamp(m_tgt_rot.y, -m_lim_y_rot.y, -m_lim_y_rot.x);
		if (!fsimilar(sv_y, m_tgt_rot.y, EPS_L))
			m_allow_fire = FALSE;
	}

	if ((angle_difference(m_cur_rot.x, m_tgt_rot.x) > deg2rad(m_barrel_dir_tolerance)) ||
		(angle_difference(m_cur_rot.y, m_tgt_rot.y) > deg2rad(m_barrel_dir_tolerance)))
		m_allow_fire = FALSE;
}

void CHelicopter::startRocket(u16 idx)
{
	if ((getRocketCount() >= 1) && m_use_rocket_on_attack)
	{
		CExplosiveRocket* pGrenade = smart_cast<CExplosiveRocket*>(getCurrentRocket());
		VERIFY(pGrenade);
		pGrenade->SetInitiator(this->ID());

		Fmatrix rocketTransform;
		(idx == 1) ? rocketTransform = m_left_rocket_bone_transform : rocketTransform = m_right_rocket_bone_transform;

		Fvector vel;
		Fvector dir;
		dir.sub(m_enemy.destEnemyPos, rocketTransform.c).normalize_safe();
		vel.mul(dir, CRocketLauncher::m_fLaunchSpeed);

		Fmatrix transform;
		transform.identity();
		transform.k.set(dir);
		Fvector::generate_orthonormal_basis(transform.k, transform.j, transform.i);
		transform.c = rocketTransform.c;
		VERIFY2(_valid(transform), "CHelicopter::startRocket. Invalid transform");
		LaunchRocket(transform, vel, zero_vel);

		NET_Packet P;
		u_EventGen(P, GE_LAUNCH_ROCKET, ID());
		P.w_u16(u16(getCurrentRocket()->ID()));
		u_EventSend(P);

		dropCurrentRocket();

		m_last_launched_rocket = idx;
		HUD_SOUND::PlaySound(m_sndShotRocket, transform.c, this, false);
	}
}

const Fmatrix& CHelicopter::get_ParticlesTransform()

{
	return m_fire_bone_transform;
}

const Fvector& CHelicopter::get_CurrentFirePoint()
{
	return m_fire_pos;
}
