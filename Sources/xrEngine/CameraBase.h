// CameraBase.h: interface for the CCameraBase class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CAMERABASE_H__B11F8AE1_1213_11D4_B4E3_4854E82A090D__INCLUDED_)
#define AFX_CAMERABASE_H__B11F8AE1_1213_11D4_B4E3_4854E82A090D__INCLUDED_
#pragma once

#include "CameraDefs.h"

// refs
class CObject;

class ENGINE_API CCameraBase
{
  protected:
	CObject* parent;

  public:
	BOOL bClampYaw, bClampPitch, bClampRoll;
	float yaw, pitch, roll;

	enum
	{
		flRelativeLink = (1 << 0),
		flPositionRigid = (1 << 1),
		flDirectionRigid = (1 << 2),
	};
	Flags32 m_Flags;

	ECameraStyle style;
	float2 lim_yaw, lim_pitch, lim_roll;
	float3 rot_speed;

	float3 vPosition;
	float3 vDirection;
	float3 vNormal;
	float f_fov;
	float f_aspect;

	int tag;

  public:
	CCameraBase(CObject* p, u32 flags);
	virtual ~CCameraBase();
	virtual void Load(LPCSTR section);
	CObject* GetParent() const
	{
		return parent;
	}
	void SetParent(CObject* p)
	{
		parent = p;
		VERIFY(p);
	}
	virtual void OnActivate(CCameraBase* old_cam)
	{
		;
	}
	virtual void OnDeactivate()
	{
		;
	}
	virtual void Move(int cmd, float val = 0, float factor = 1.0f)
	{
		;
	}
	virtual void Update(float3& point, float3& noise_angle)
	{
		;
	}
	virtual void Get(float3& P, float3& D, float3& N)
	{
		P.set(vPosition);
		D.set(vDirection);
		N.set(vNormal);
	}
	virtual void Set(const float3& P, const float3& D, const float3& N)
	{
		vPosition.set(P);
		vDirection.set(D);
		vNormal.set(N);
	}
	virtual void Set(float Y, float P, float R)
	{
		yaw = Y;
		pitch = P;
		roll = R;
	}

	virtual float GetWorldYaw()
	{
		return 0;
	};
	virtual float GetWorldPitch()
	{
		return 0;
	};

	virtual float CheckLimYaw();
	virtual float CheckLimPitch();
	virtual float CheckLimRoll();
};

#endif // !defined(AFX_CAMERABASE_H__B11F8AE1_1213_11D4_B4E3_4854E82A090D__INCLUDED_)
