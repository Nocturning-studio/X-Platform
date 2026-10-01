///////////////////////////////////////////////////////////////
// DummyArtifact.cpp
// DummyArtefact - артефакт пустышка
///////////////////////////////////////////////////////////////

#include "pch.h"
#include "DummyArtifact.h"
#include "PhysicsShell.h"

CDummyArtefact::CDummyArtefact(void)
{
}

CDummyArtefact::~CDummyArtefact(void)
{
}

void CDummyArtefact::Load(LPCSTR section)
{
	inherited::Load(section);
}
