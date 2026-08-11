#pragma once

#include "../../entity.h"
#include "../../fastdelegate.h"
#include "../../../xrEngine/SkeletonAnimated.h"

class CParticlesObject;

class CPhantom : public CEntity
{

	typedef CEntity inherited;

  private:
	enum EState
	{
		stInvalid = -2,
		stIdle = -1,
		stBirth = 0,
		stFly = 1,
		stContact = 2,
		stShoot = 3,
		stCount
	};
	EState m_CurState;
	EState m_TgtState;

	void SwitchToState_internal(EState new_state);
	void SwitchToState(EState new_state)
	{
		m_TgtState = new_state;
	}
	void __stdcall OnIdleState();
	void __stdcall OnFlyState();
	void __stdcall OnDeadState();

	void UpdateFlyMedia();

	fastdelegate::FastDelegate0<> UpdateEvent;

  private:
	struct SStateData
	{
		shared_str particles;
		ref_sound sound;
		MotionID motion;
	};
	SStateData m_state_data[stCount];

  private:
	CParticlesObject* m_fly_particles;
	static void animation_end_callback(CBlend* B);

  private:
	CObject* m_enemy;

	float fSpeed;
	float fASpeed;
	fvec2 vHP;

	float fContactHit;

	fmat4x4 Transform_center();

	CParticlesObject* PlayParticles(const shared_str& name, BOOL bAutoRemove, const fmat4x4& transform);
	//	void				PlayMotion					(MotionID);

	void UpdatePosition(const fvec3& tgt_pos);

	void PsyHit(const CObject* object, float value);

  public:
	CPhantom();
	virtual ~CPhantom();

	virtual void Load(LPCSTR section);
	virtual BOOL net_Spawn(CSE_Abstract* DC);
	virtual void net_Destroy();

	virtual void net_Export(NET_Packet& P);
	virtual void net_Import(NET_Packet& P);
	virtual void save(NET_Packet& output_packet);
	virtual void load(IReader& input_packet);

	virtual void shedule_Update(u32 DT);
	virtual void UpdateCL();

	virtual void HitSignal(float HitAmount, fvec3& local_dir, CObject* who, s16 element)
	{
	}
	virtual void HitImpulse(float amount, fvec3& vWorldDir, fvec3& vLocalDir)
	{
	}
	virtual void Hit(SHit* pHDS);

	virtual BOOL IsVisibleForHUD()
	{
		return false;
	}
	virtual bool IsVisibleForZones()
	{
		return false;
	}

	virtual BOOL UsedAI_Locations()
	{
		return false;
	}

	virtual CEntity* cast_entity()
	{
		return this;
	}
};
