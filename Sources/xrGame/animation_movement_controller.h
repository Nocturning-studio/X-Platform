#pragma once
#include <boost/noncopyable.hpp>
class CBlend;
class animation_movement_controller : private boost::noncopyable
{
	Fmatrix& m_pObjTransform;
	Fmatrix m_startObjTransform;
	Fmatrix m_startRootTransform;
	CKinematics* m_pKinematicsC;
	CBlend* m_control_blend;
	static void RootBoneCallback(CBoneInstance* B);
	void deinitialize();

  public:
	animation_movement_controller(Fmatrix* _pObjTransform, CKinematics* _pKinematicsC, CBlend* b);
	~animation_movement_controller();
	void ObjStartTransform(Fmatrix& m) const
	{
		m.set(m_startObjTransform);
	}
	CBlend* ControlBlend() const
	{
		return m_control_blend;
	}
	bool isActive() const;
	void OnFrame();
};
