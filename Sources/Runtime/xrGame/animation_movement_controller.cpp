#include "StdAfx.h"
#include "animation_movement_controller.h"
#include "../xrEngine/SkeletonAnimated.h"
#include "game_object_space.h"

animation_movement_controller::animation_movement_controller(fmat4x4* _pObjTransform, CKinematics* _pKinematicsC, CBlend* b)
	: m_startObjTransform(*_pObjTransform), m_pObjTransform(*_pObjTransform), m_pKinematicsC(_pKinematicsC), m_control_blend(b)
{
	VERIFY(_pKinematicsC);
	VERIFY(_pObjTransform);
	VERIFY(b);
	CBoneInstance& B = m_pKinematicsC->LL_GetBoneInstance(m_pKinematicsC->LL_GetBoneRoot());
	VERIFY(!B.Callback && !B.Callback_Param);
	B.set_callback(bctCustom, RootBoneCallback, this);
	m_startRootTransform.set(B.mTransform);
}

animation_movement_controller::~animation_movement_controller()
{
	if (isActive())
		deinitialize();
}
void animation_movement_controller::deinitialize()
{
	CBoneInstance& B = m_pKinematicsC->LL_GetBoneInstance(m_pKinematicsC->LL_GetBoneRoot());
	VERIFY(B.Callback == RootBoneCallback);
	VERIFY(B.Callback_Param == (void*)this);
	B.reset_callback();
	m_control_blend = 0;
}
void animation_movement_controller::OnFrame()
{
	m_pKinematicsC->CalculateBones();

	if (CBlend::eFREE_SLOT == m_control_blend->blend)
	{
		deinitialize();
		return;
	}
	if (m_control_blend->blend == CBlend::eAccrue && m_control_blend->blendPower - EPS_L > m_control_blend->blendAmount)
		m_control_blend->timeCurrent = 0;
}

void animation_movement_controller::RootBoneCallback(CBoneInstance* B)
{
	VERIFY(B);
	VERIFY(B->Callback_Param);

	animation_movement_controller* O = (animation_movement_controller*)(B->Callback_Param);

	if (O->m_control_blend->playing)
	{
		fmat4x4 m;
		m.mul_43(B->mTransform, fmat4x4().invert(O->m_startRootTransform));
		O->m_pObjTransform.mul_43(O->m_startObjTransform, m);
	}
	B->mTransform.set(O->m_startRootTransform);
}

bool animation_movement_controller::isActive() const
{
	return !!m_control_blend;
}
