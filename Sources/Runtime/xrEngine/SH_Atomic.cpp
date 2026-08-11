#include "stdafx.h"
#pragma hdrstop

#include "sh_atomic.h"
#include "ResourceManager.h"

// Atomic
SVS::~SVS()
{
	_RELEASE(sh);
	Engine.ResourceManager->DestroyShader<SVS>(this);
}
SPS::~SPS()
{
	_RELEASE(sh);
	Engine.ResourceManager->DestroyShader<SPS>(this);
}
SState::~SState()
{
	_RELEASE(state);
	Engine.ResourceManager->_DeleteState(this);
}
SDeclaration::~SDeclaration()
{
	_RELEASE(dcl);
	Engine.ResourceManager->_DeleteDecl(this);
}
