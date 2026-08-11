///////////////////////////////////////////////////////////////
// ElectricBall.cpp
// ElectricBall - артефакт электрический шар
///////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ElectricBall.h"
#include "PhysicsShell.h"

CElectricBall::CElectricBall(void)
{
}

CElectricBall::~CElectricBall(void)
{
}

void CElectricBall::Load(LPCSTR section)
{
	inherited::Load(section);
}

void CElectricBall::UpdateCLChild()
{
	inherited::UpdateCLChild();

	if (H_Parent())
		Transform().set(H_Parent()->Transform());
};
