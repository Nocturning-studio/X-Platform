#pragma once
#include "physicsshellholder.h"
class CPHLeaderGeomShell;
class CPHCharacter;
struct dContact;
struct SGameMtl;
class CClimableObject : public CPhysicsShellHolder
#ifdef DEBUG
	,
						public pureRender
#endif
{
	typedef CPhysicsShellHolder inherited;
	CPHLeaderGeomShell* m_pStaticShell;
	Fobb m_box;
	float3 m_axis;
	float3 m_side;
	float3 m_norm;
	float m_radius;

  public:
	CClimableObject();
	~CClimableObject();
	virtual void Load(LPCSTR section);
	virtual BOOL net_Spawn(CSE_Abstract* DC);
	virtual void net_Destroy();
	virtual void shedule_Update(u32 dt); // Called by sheduler
	virtual void UpdateCL();			 // Called each frame, so no need for dt
	virtual void Center(float3& C) const;
	virtual float Radius() const;
#ifdef DEBUG
	virtual void OnRender();
#endif
  protected:
	virtual BOOL UsedAI_Locations();

  public:
	const float3& Axis() const
	{
		return m_axis;
	}
	float DDAxis(float3& dir) const;

	const float3& Side() const
	{
		return m_side;
	}
	float DDSide(float3& dir) const;

	const float3& Norm() const
	{
		return m_norm;
	}
	float DDNorm(float3& dir) const;
	bool BeforeLadder(CPHCharacter* actor, float tolerance = 0.f) const;
	float DDLowerP(CPHCharacter* actor, float3& out_dir) const; // returns distance and dir to lover point
	float DDUpperP(CPHCharacter* actor, float3& out_dir) const; // returns distance and dir to upper point

	void DToAxis(CPHCharacter* actor, float3& dir) const;
	float DDToAxis(CPHCharacter* actor, float3& out_dir) const; // returns distance and dir to ladder axis
	void POnAxis(CPHCharacter* actor, float3& P) const;

	float AxDistToUpperP(CPHCharacter* actor) const;
	float AxDistToLowerP(CPHCharacter* actor) const;

	void DSideToAxis(CPHCharacter* actor, float3& dir) const;
	float DDSideToAxis(CPHCharacter* actor, float3& dir) const;

	void DToPlain(CPHCharacter* actor, float3& dist) const;
	float DDToPlain(CPHCharacter* actor, float3& dir) const;
	bool InRange(CPHCharacter* actor) const;
	bool InTouch(CPHCharacter* actor) const;

	void LowerPoint(float3& P) const;
	void UpperPoint(float3& P) const;
	void DefineClimbState(CPHCharacter* actor) const;
	static void ObjectContactCallback(bool& /**do_colide/**/, bool bo1, dContact& c, SGameMtl* /*material_1*/,
									  SGameMtl* /*material_2*/);

  public:
	virtual bool register_schedule() const
	{
		return false;
	}
};
