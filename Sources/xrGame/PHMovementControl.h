#pragma once
#ifndef CPHMOVEMENT_CONTROL_H
#define CPHMOVEMENT_CONTROL_H

#include "PHCharacter.h"
#include "MathUtils.h"
namespace ALife
{
enum EHitType;
};

namespace DetailPathManager
{
struct STravelPathPoint;
};

class CPHAICharacter;
class CPHSimpleCharacter;
class CPHCapture;
class CPHSynchronize;
class ICollisionDamageInfo;
class CElevatorState;

class CPHMovementControl
{
	collide::rq_results storage;

	static const int path_few_point = 10;

  public:
	CElevatorState* ElevatorState();
	void in_shedule_Update(u32 DT);
	void PHCaptureObject(CPhysicsShellHolder* object);
	void PHCaptureObject(CPhysicsShellHolder* object, u16 element);
	CPHCapture* PHCapture()
	{
		return m_capture;
	}
	CPHCharacter* PHCharacter()
	{
		return m_character;
	}
	void PHReleaseObject();
	fvec3 PHCaptureGetNearestElemPos(const CPhysicsShellHolder* object);
	fmat4x4 PHCaptureGetNearestElemTransform(CPhysicsShellHolder* object);
	void SetMaterial(u16 material);
	void SetAirControlParam(float param)
	{
		fAirControlParam = param;
	}
	void SetActorRestrictorRadius(CPHCharacter::ERestrictionType rt, float r);
	void SetRestrictionType(CPHCharacter::ERestrictionType rt)
	{
		if (m_character)
			m_character->SetRestrictionType(rt);
	}
	void SetActorMovable(bool v)
	{
		if (m_character)
			m_character->SetActorMovable(v);
	}
	void SetForcedPhysicsControl(bool v)
	{
		if (m_character)
			m_character->SetForcedPhysicsControl(v);
	}
	bool ForcedPhysicsControl()
	{
		return m_character && m_character->ForcedPhysicsControl();
	}
	void UpdateObjectBox(CPHCharacter* ach);
	enum JumpType
	{
		jtStrait, // end point before uppermost point
		jtCurved, // end point after uppermost point
		jtHigh	  // end point is uppermost point
	};
	void JumpV(const fvec3& jump_velocity);
	void Jump(const fvec3& start_point, const fvec3& end_point, float time);
	void Jump(const fvec3& end_point, float time);
	float Jump(const fvec3& end_point);
	bool JumpState()
	{
		return (m_character && m_character->b_exist && m_character->IsEnabled() && m_character->JumpState());
	};
	///
	bool PhyssicsOnlyMode()
	{
		return m_character && m_character->b_exist && m_character->IsEnabled() &&
			   (m_character->JumpState() || m_character->ForcedPhysicsControl());
	}
	void GetJumpMinVelParam(fvec3& min_vel, float& time, JumpType& type,
							const fvec3& end_point); // returns vector of velocity of jump with minimal start speed
													   // in min_vel and correspondent jump time in time
	float JumpMinVelTime(const fvec3& end_point); // return time of jump with min start speed
	// input: end_point and time; return velocity and type of jump
	void GetJumpParam(fvec3& velocity, JumpType& type, const fvec3& end_point, float time);
	bool b_exect_position;
	int in_dead_area_count;

  public:
	enum EEnvironment
	{
		peOnGround,
		peAtWall,
		peInAir
	};
	enum CharacterType
	{
		actor,
		ai
	};
	bool isOutBorder()
	{
		return in_dead_area_count > 0;
	}
	void setOutBorder()
	{
		in_dead_area_count = 1;
	}

  private:
	void TraceBorder(const fvec3& previous_position);
	void CheckEnvironment(const fvec3& V);

	CharacterType eCharacterType;
	CPHCharacter* m_character;
	CPHCapture* m_capture;

	float m_fGroundDelayFactor;
	BOOL bIsAffectedByGravity;
	//------------------------------
	CObject* pObject;
	EEnvironment eOldEnvironment;
	EEnvironment eEnvironment;
	Fbox aabb;
	Fbox boxes[4];

	u32 trying_times[4];
	fvec3 trying_poses[4];
	DWORD m_dwCurBox;

	float fMass;
	float fMinCrashSpeed;
	float fMaxCrashSpeed;
	float fCollisionDamageFactor;
	float fAirControlParam;

	fvec3 vVelocity;
	fvec3 vPosition;

	fvec3 vPathPoint;
	fvec3 _vPathDir;
	int m_path_size;
	int m_start_index;

	float m_path_distance;
	u16 m_material;

	float fLastMotionMag;

	float fActualVelocity;
	float fContactSpeed;
	float fLastUpdateTime;

  public:
	fvec3 vExternalImpulse;
	bool bExernalImpulse;
	BOOL bSleep;

	BOOL gcontact_Was;		   // Приземление
	float gcontact_Power;	   // Насколько сильно ударились
	float gcontact_HealthLost; // Скоко здоровья потеряли

  public:
	void AllocateCharacterObject(CharacterType type);
	void DeleteCharacterObject();
	void CreateCharacter();
	void DestroyCharacter();
	void Load(LPCSTR section);
#ifdef DEBUG
	void dbg_Draw()
	{
		if (m_character)
			m_character->OnRender();
	};
#endif

	void SetPLastMaterialIDX(u16* p)
	{
		m_character->SetPLastMaterialIDX(p);
	}
	dBodyID GetBody()
	{
		if (m_character)
			return m_character->get_body();
		else
			return NULL;
	}
	const fvec3& GetVelocity()
	{
		return vVelocity;
	}
	const fvec3& GetPathDir()
	{
		return _vPathDir;
	}
	void SetPathDir(const fvec3& v);

	void GetCharacterVelocity(fvec3& velocity)
	{
		if (m_character)
			m_character->GetVelocity(velocity);
		else
			velocity.set(0.f, 0.f, 0.f);
	}
	float GetVelocityMagnitude()
	{
		return vVelocity.magnitude();
	}
	float GetVelocityActual()
	{
		return fActualVelocity;
	}
	float GetXZVelocityActual()
	{
		return dXZMag(vVelocity);
	}
	float GetActVelProj(const fvec3& dir)
	{
		return vVelocity.dotproduct(dir);
	}
	float GetActVelInGoingDir()
	{
		float r = GetActVelProj(GetPathDir());
		return r > 0.f ? r : 0.f;
	}
	float GetXZActVelInGoingDir()
	{
		float r = dXZDot(GetPathDir(), vVelocity);
		return r > EPS_L ? r : 0.f;
	}
	void GetSmoothedVelocity(fvec3& v)
	{
		if (m_character)
			m_character->GetSmothedVelocity(v);
		else
			v.set(0, 0, 0);
	}
	float GetContactSpeed()
	{
		return fContactSpeed;
	}
	void GroundNormal(fvec3& norm);
	CPHSynchronize* GetSyncItem();
	void Freeze();
	void UnFreeze();
	void SetVelocity(float x, float y, float z)
	{
		SetVelocity(fvec3().set(x, y, z));
	}
	void SetVelocity(const fvec3& v)
	{
		vVelocity.set(v);
		SetCharacterVelocity(v);
	}
	void SetCharacterVelocity(const fvec3& v)
	{
		if (m_character)
			m_character->SetVelocity(v);
	}
	void SetPhysicsRefObject(CPhysicsShellHolder* ref_object)
	{
		m_character->SetPhysicsRefObject(ref_object);
	};

	void CalcMaximumVelocity(fvec3& /**dest/**/, fvec3& /**accel/**/, float /**friction/**/){};
	void CalcMaximumVelocity(float& /**dest/**/, float /**accel/**/, float /**friction/**/){};
	void ActivateBox(DWORD id, BOOL Check = false);
	bool ActivateBoxDynamic(DWORD id, int num_it = 9, int num_steps = 5, float resolve_depth = 0.01f);
	void InterpolateBox(DWORD id, float k);
	EEnvironment Environment()
	{
		return eEnvironment;
	}
	EEnvironment OldEnvironment()
	{
		return eOldEnvironment;
	}
	const Fbox& Box()
	{
		return aabb;
	}
	DWORD BoxID() const
	{
		return m_dwCurBox;
	}
	const Fbox* Boxes()
	{
		return boxes;
	}
	float FootRadius()
	{
		if (m_character)
			return m_character->FootRadius();
		else
			return 0.f;
	};
	void CollisionEnable(BOOL enable)
	{
		if (m_character)
			if (enable)
				m_character->collision_enable();
			else
				m_character->collision_disable();
	}
	void SetBox(DWORD id, const Fbox& BB)
	{
		boxes[id].set(BB);
		aabb.set(BB);
	}
	void SetMass(float M)
	{
		fMass = M;
		if (m_character)
			m_character->SetMas(fMass);
	}
	float GetMass()
	{
		return fMass;
	}
	void SetCrashSpeeds(float min, float max)
	{
		fMinCrashSpeed = min;
		fMaxCrashSpeed = max;
	}

	void SetPosition(const fvec3& P);
	void GetPosition(fvec3& P);
	void GetCharacterPosition(fvec3& P)
	{
		m_character->GetPosition(P);
	}
	void InterpolatePosition(fvec3& P)
	{
		VERIFY(m_character && m_character->b_exist);
		m_character->IPosition(P);
	}
	bool TryPosition(fvec3& pos);
	bool IsCharacterEnabled()
	{
		return m_character->IsEnabled() || bExernalImpulse;
	}
	void DisableCharacter()
	{
		m_character->Disable();
	}
	void Calculate(fvec3& vAccel, const fvec3& camDir, float ang_speed, float jump, float dt, bool bLight);
	void Calculate(const xr_vector<DetailPathManager::STravelPathPoint>& path, // in path
				   float speed,												   // in speed
				   u32& travel_point, // in- travel start, out - current trev point
				   float& precesition // in- tolerance, out - precesition
	);
	void AddControlVel(const fvec3& vel);
	void SetVelocityLimit(float val);
	float VelocityLimit();
	void PathNearestPoint(const xr_vector<DetailPathManager::STravelPathPoint>& path,		// in path
						  const fvec3& new_position,										// in position
						  int& index,														// out nearest
						  bool& type														// out type
	);																						// return nearest point
	void PathNearestPointFindUp(const xr_vector<DetailPathManager::STravelPathPoint>& path, // in path
								const fvec3& new_position,								// in position
								int& index,													// out nearest
								float radius,												// in exit radius
								bool& near_line												// out type
	);
	void PathNearestPointFindDown(const xr_vector<DetailPathManager::STravelPathPoint>& path, // in path
								  const fvec3& new_position,								  // in position
								  int& index,												  // out nearest
								  float radius,												  // in exit radius
								  bool& near_line											  // out type
	);

	void PathDIrPoint(const xr_vector<DetailPathManager::STravelPathPoint>& path, // in path
					  int index,												  // in index
					  float distance,											  // in distance
					  float precesition,										  // in precesition
					  fvec3& dir												  // out dir
	);
	void PathDIrLine(const xr_vector<DetailPathManager::STravelPathPoint>& path, // in path
					 int index,													 // in point
					 float distance,											 // in distance
					 float precesition,											 // in precesition
					 fvec3& dir												 // out dir
	);
	void CorrectPathDir(const fvec3& real_path_dir, const xr_vector<DetailPathManager::STravelPathPoint>& path,
						int index, fvec3& corrected_path_dir);

	//	void				Move					(fvec3& Dest, fvec3& Motion, BOOL bDynamic=FALSE){};
	void SetApplyGravity(BOOL flag)
	{
		bIsAffectedByGravity = flag;
		if (m_character && m_character->b_exist)
			m_character->SetApplyGravity(flag);
	}
	void GetDeathPosition(fvec3& pos)
	{
		m_character->DeathPosition(pos);
	}
	void SetEnvironment(int enviroment, int old_enviroment);
	void SetFrictionFactor(float f);
	float GetFrictionFactor();
	void MulFrictionFactor(float f);
	void ApplyImpulse(const fvec3& dir, const dReal P);
	void ApplyHit(const fvec3& dir, const dReal P, ALife::EHitType hit_type);
	void SetJumpUpVelocity(float velocity)
	{
		m_character->SetJupmUpVelocity(velocity);
	}
	void EnableCharacter()
	{
		if (m_character && m_character->b_exist)
			m_character->Enable();
	}
	void SetOjectContactCallback(ObjectContactCallbackFun* callback)
	{
		if (m_character)
			m_character->SetObjectContactCallback(callback);
	}
	void SetFootCallBack(ObjectContactCallbackFun* callback)
	{
		VERIFY(m_character);
		m_character->SetWheelContactCallback(callback);
	}
	static BOOL CPHMovementControl::BorderTraceCallback(collide::rq_result& result, LPVOID params);
	ObjectContactCallbackFun* ObjectContactCallback()
	{
		if (m_character)
			return m_character->ObjectContactCallBack();
		else
			return NULL;
	}
	u16 ContactBone()
	{
		return m_character->ContactBone();
	}
	const ICollisionDamageInfo* CollisionDamageInfo() const
	{
		VERIFY(m_character);
		return m_character->CollisionDamageInfo();
	}
	void GetDesiredPos(fvec3& dpos)
	{
		m_character->GetDesiredPosition(dpos);
	}
	bool CharacterExist()
	{
		return (m_character && m_character->b_exist);
	}
	CPHMovementControl(CObject* parent);
	~CPHMovementControl(void);
};
#endif
