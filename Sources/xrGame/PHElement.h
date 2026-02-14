/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///////////////////////////Implemetation//for//CPhysicsElement//////////////////
////////////////////////////////////////////////////////////////////////////////
#include "Geometry.h"
#include "phdefs.h"
#include "PhysicsCommon.h"
#include "PHSynchronize.h"
#include "PHDisabling.h"
#include "PHGeometryOwner.h"
#include "PHInterpolation.h"
#ifndef PH_ELEMENT
#define PH_ELEMENT
class CPHElement;
class CPHShell;
class CPHFracture;
struct SPHImpact;
class CPHFracturesHolder;
class CPHElement : public CPhysicsElement, public CPHSynchronize, public CPHDisablingFull, public CPHGeometryOwner
{
	friend class CPHFracturesHolder;

	// float						m_start_time;				//uu ->to shell ??	//aux
	dMass m_mass;						   // e ??				//bl
	dBodyID m_body;						   // e					//st
	dReal m_l_scale;					   // ->to shell ??	//bl
	dReal m_w_scale;					   // ->to shell ??	//bl
	CPHElement* m_parent_element;		   // bool !			//bl
	CPHShell* m_shell;					   // e					//bl
	CPHInterpolation m_body_interpolation; // e					//bl
	CPHFracturesHolder* m_fratures_holder; // e					//bl

	dReal m_w_limit; //->to shell ??		//bl
	dReal m_l_limit; //->to shell ??		//bl
					 //	dVector3					m_safe_position;			//e					//st
					 //	dQuaternion					m_safe_quaternion;
					 //	dVector3					m_safe_velocity;			//e					//st
					 //	float4x4						m_inverse_local_transform;	//e				//bt
	dReal k_w;		 //->to shell ??		//st
	dReal k_l;		 //->to shell ??		//st
	// ObjectContactCallbackFun*	temp_for_push_out;			//->to shell ??		//aux
	// u32							push_untill;				//->to shell ??		//st
	Flags8 m_flags; //
	enum
	{
		flActive = 1 << 0,
		flActivating = 1 << 1,
		flUpdate = 1 << 2,
		flWasEnabledBeforeFreeze = 1 << 3,
		flEnabledOnStep = 1 << 4,
		flFixed = 1 << 5
	};
	//	bool						was_enabled_before_freeze;
	//	bool						bUpdate;					//->to shell ??		//st
	//	bool						b_enabled_onstep;
  private:
	////////////////////////////////////////////Interpolation/////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void FillInterpolation() // interpolation called anywhere visual influent
	{
		m_body_interpolation.ResetPositions();
		m_body_interpolation.ResetRotations();
		// bUpdate=true;
		m_flags.set(flUpdate, TRUE);
	}
	IC void UpdateInterpolation() // interpolation called from ph update visual influent
	{
		/// VERIFY(dBodyStateValide(m_body));
		m_body_interpolation.UpdatePositions();
		m_body_interpolation.UpdateRotations();
		// bUpdate=true;
		m_flags.set(flUpdate, TRUE);
	}

  public:
	////////////////////////////////////////////////Geometry/////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void add_Sphere(const Fsphere& V);								// aux
	virtual void add_Box(const Fobb& V);									// aux
	virtual void add_Cylinder(const Fcylinder& V);							// aux
	virtual void add_Shape(const SBoneShape& shape);						// aux
	virtual void add_Shape(const SBoneShape& shape, const float4x4& offset); // aux
	virtual CODEGeom* last_geom()
	{
		return CPHGeometryOwner::last_geom();
	} // aux
	virtual bool has_geoms()
	{
		return CPHGeometryOwner::has_geoms();
	}
	virtual void set_ContactCallback(ContactCallbackFun* callback);				// aux (may not be)
	virtual void set_ObjectContactCallback(ObjectContactCallbackFun* callback); // called anywhere ph state influent
	virtual void add_ObjectContactCallback(ObjectContactCallbackFun* callback); // called anywhere ph state influent
	virtual void remove_ObjectContactCallback(ObjectContactCallbackFun* callback);
	virtual void set_CallbackData(void* cd);
	virtual void* get_CallbackData();
	virtual ObjectContactCallbackFun* get_ObjectContactCallback();
	virtual void set_PhysicsRefObject(CPhysicsShellHolder* ref_object); // aux
	virtual CPhysicsShellHolder* PhysicsRefObject()
	{
		return m_phys_ref_object;
	}								 // aux
	virtual void SetMaterial(u16 m); // aux
	virtual void SetMaterial(LPCSTR m)
	{
		CPHGeometryOwner::SetMaterial(m);
	}							 // aux
	virtual u16 numberOfGeoms(); // aux
	virtual const float3& local_mass_Center()
	{
		return CPHGeometryOwner::local_mass_Center();
	} // aux
	virtual float getVolume()
	{
		return CPHGeometryOwner::get_volume();
	}																								  // aux
	virtual void get_Extensions(const float3& axis, float center_prg, float& lo_ext, float& hi_ext); // aux
	virtual void get_MaxAreaDir(float3& dir)
	{
		CPHGeometryOwner::get_MaxAreaDir(dir);
	}
	virtual float getRadius();
	////////////////////////////////////////////////////Mass/////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  private:
	void calculate_it_data(const float3& mc, float mass);				  // aux
	void calculate_it_data_use_density(const float3& mc, float density); // aux
	void calc_it_fract_data_use_density(const float3& mc,
										float density); // sets element mass and fractures parts mass	//aux
	dMass recursive_mass_summ(u16 start_geom, FRACTURE_I cur_fracture); // aux
  public:																//
	virtual const float3& mass_Center();								// aux
	virtual void setDensity(float M);									// aux
	virtual float getDensity()
	{
		return m_mass.mass / m_volume;
	}																// aux
	virtual void setMassMC(float M, const float3& mass_center);	// aux
	virtual void setDensityMC(float M, const float3& mass_center); // aux
	virtual void setInertia(const dMass& M);						// aux
	virtual void addInertia(const dMass& M);
	virtual void add_Mass(const SBoneShape& shape, const float4x4& offset, const float3& mass_center, float mass,
						  CPHFracture* fracture = NULL);   // aux
	virtual void set_BoxMass(const Fobb& box, float mass); // aux
	virtual void setMass(float M);						   // aux
	virtual float getMass()
	{
		return m_mass.mass;
	}																	   // aux
	virtual dMass* getMassTensor();										   // aux
	void ReAdjustMassPositions(const float4x4& shift_pivot, float density); // aux
	void ResetMass(float density);										   // aux
	void CutVelocity(float l_limit, float a_limit);
	///////////////////////////////////////////////////PushOut///////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  private: //
  public:  //
	//////////////////////////////////////////////Disable/////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void Disable();	 //
	virtual void ReEnable(); //
	void Enable();			 // aux
	virtual bool isEnabled()
	{
		return isActive() && dBodyIsEnabled(m_body);
	}
	virtual bool isFullActive()
	{
		return isActive() && !m_flags.test(flActivating);
	}
	virtual bool isActive()
	{
		return !!m_flags.test(flActive);
	}
	virtual void Freeze();	 //
	virtual void UnFreeze(); //
	virtual bool EnabledStateOnStep()
	{
		return dBodyIsEnabled(m_body) || m_flags.test(flEnabledOnStep);
	} //
	////////////////////////////////////////////////Updates///////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool AnimToVel(float dt, float l_limit, float a_limit);
	void BoneGlPos(float4x4& m, CBoneInstance* B);
	void ToBonePos(CBoneInstance* B);

	void SetBoneCallbackOverwrite(bool v);
	void BonesCallBack(CBoneInstance* B); // called from updateCL visual influent
	void StataticRootBonesCallBack(CBoneInstance* B);
	void PhDataUpdate(dReal step); // ph update
	void PhTune(dReal step);	   // ph update
	virtual void Update();		   // called update CL visual influence
	//////////////////////////////////////////////////Dynamics////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void SetAirResistance(dReal linear = default_k_l, dReal angular = default_k_w) // aux (may not be)
	{																					   //
		k_w = angular;																	   //
		k_l = linear;																	   //
	}																					   //
	virtual void GetAirResistance(float& linear, float& angular)						   //
	{																					   //
		linear = k_l;																	   //
		angular = k_w;																	   //
	}
	virtual void applyImpact(const SPHImpact& impact); //
	virtual void applyImpulseTrace(const float3& pos, const float3& dir, float val,
								   const u16 id);				 // called anywhere ph state influent
	virtual void set_DisableParams(const SAllDDOParams& params); //
	virtual void set_DynamicLimits(float l_limit = default_l_limit, float w_limit = default_w_limit); // aux (may not
																									  // be)
	virtual void set_DynamicScales(float l_scale = default_l_scale, float w_scale = default_w_scale); // aux (may not
																									  // be)
	virtual void Fix();
	virtual void ReleaseFixed();
	virtual bool isFixed()
	{
		return !!(m_flags.test(flFixed));
	}
	virtual void applyForce(const float3& dir, float val);	  // aux
	virtual void applyForce(float x, float y, float z);		  // called anywhere ph state influent
	virtual void applyImpulse(const float3& dir, float val); // aux
	virtual void applyImpulseVsMC(const float3& pos, const float3& dir, float val); //
	virtual void applyImpulseVsGF(const float3& pos, const float3& dir, float val); //
	virtual void applyGravityAccel(const float3& accel);
	virtual void getForce(float3& force);
	virtual void getTorque(float3& torque);
	virtual void get_LinearVel(float3& velocity);		  // aux
	virtual void get_AngularVel(float3& velocity);		  // aux
	virtual void set_LinearVel(const float3& velocity);  // called anywhere ph state influent
	virtual void set_AngularVel(const float3& velocity); // called anywhere ph state influent
	virtual void setForce(const float3& force);		  //
	virtual void setTorque(const float3& torque);		  //
	virtual void set_ApplyByGravity(bool flag);			  //
	virtual bool get_ApplyByGravity();					  //
	///////////////////////////////////////////////////Net////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void get_State(SPHNetState& state);		  //
	virtual void set_State(const SPHNetState& state); //
	virtual void net_Import(NET_Packet& P);
	virtual void net_Export(NET_Packet& P);
	///////////////////////////////////////////////////Position///////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void SetTransform(const float4x4& m0); //
	virtual void TransformPosition(const float4x4& form);
	virtual void getQuaternion(Fquaternion& quaternion);								  //
	virtual void setQuaternion(const Fquaternion& quaternion);							  //
	virtual void SetGlobalPositionDynamic(const float3& position);						  //
	virtual void GetGlobalPositionDynamic(float3* v);									  //
	virtual void cv2obj_Xfrom(const Fquaternion& q, const float3& pos, float4x4& transform);  //
	virtual void cv2bone_Xfrom(const Fquaternion& q, const float3& pos, float4x4& transform); //
	virtual void InterpolateGlobalTransform(float4x4* m); // called UpdateCL vis influent
	virtual void InterpolateGlobalPosition(float3* v);	 // aux
	virtual void GetGlobalTransformDynamic(float4x4* m);	 // aux
	IC void InverceLocalForm(float4x4&);
	IC void MulB43InverceLocalForm(float4x4&);

	////////////////////////////////////////////////////Structure/////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual CPhysicsShell* PhysicsShell(); // aux
	CPHShell* PHShell();
	virtual void set_ParentElement(CPhysicsElement* p)
	{
		m_parent_element = (CPHElement*)p;
	}							// aux
	void SetShell(CPHShell* p); // aux
	virtual dBodyID get_body()
	{
		return m_body;
	}; // aux
	virtual const dBodyID get_bodyConst() const
	{
		return m_body;
	}; // aux
	//////////////////////////////////////////////////////Breakable//////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	IC CPHFracturesHolder* FracturesHolder()
	{
		return m_fratures_holder;
	} // aux
	IC const CPHFracturesHolder* constFracturesHolder() const
	{
		return m_fratures_holder;
	}													   // aux
	void DeleteFracturesHolder();						   //
	virtual bool isBreakable();							   // aux
	virtual u16 setGeomFracturable(CPHFracture& fracture); // aux
	virtual CPHFracture& Fracture(u16 num);				   // aux
	void SplitProcess(ELEMENT_PAIR_VECTOR& new_elements);  // aux
	void PassEndGeoms(u16 from, u16 to, CPHElement* dest); // aux
	////////////////////////////////////////////////////Build/Activate////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void Activate(const float4x4& m0, float dt01, const float4x4& m2,
						  bool disable = false); // some isues not to be aux
	virtual void Activate(const float4x4& transform, const float3& lin_vel, const float3& ang_vel,
						  bool disable = false);							// some isues not to be aux
	virtual void Activate(bool disable = false);							// some isues not to be aux
	virtual void Activate(const float4x4& start_from, bool disable = false); // some isues not to be aux
	virtual void Deactivate();												// aux
							   // //aux
	void CreateSimulBase();											// create body & cpace																//aux
	void ReInitDynamics(const float4x4& shift_pivot, float density); // set body & geom positions
	void PresetActive();											//
	void build();													// aux
	void build(bool disable);										// aux
	void destroy();													// called anywhere ph state influent
	void Start();													// aux
	void RunSimulation();											// called anywhere ph state influent
	void RunSimulation(const float4x4& start_from);					//
	void ClearDestroyInfo();
	void GetAnimBonePos(float4x4& bp);
	//		bool						CheckBreakConsistent					()
	CPHElement();		   // aux
	virtual ~CPHElement(); // aux
};

IC CPHElement* cast_PHElement(CPhysicsElement* e)
{
	return static_cast<CPHElement*>(static_cast<CPhysicsElement*>(e));
}
IC CPHElement* cast_PHElement(void* e)
{
	return static_cast<CPHElement*>(static_cast<CPhysicsElement*>(e));
}
IC CPhysicsElement* cast_PhysicsElement(CPHElement* e)
{
	return static_cast<CPhysicsElement*>(static_cast<CPHElement*>(e));
}
#endif
