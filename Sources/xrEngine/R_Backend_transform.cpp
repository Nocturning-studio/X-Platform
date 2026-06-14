#include "stdafx.h"
#pragma hdrstop

#include "r_backend_transform.h"

void R_transforms::set_World(const fmat4x4& m)
{
	m_World.set(m);
	m_WorldView.mul_43(m_View, m_World);
	m_WorldViewProject.mul(m_Project, m_WorldView);
	if (c_World)
		RenderBackendLegacy.set_Constant(c_World, m_World);
	if (c_WorldView)
		RenderBackendLegacy.set_Constant(c_WorldView, m_WorldView);
	if (c_WorldViewProject)
		RenderBackendLegacy.set_Constant(c_WorldViewProject, m_WorldViewProject);
	m_bInvWorldMatrixIsValid = false;
	if (c_InvWorld)
		apply_InvWorld();
}
void R_transforms::set_View(const fmat4x4& m)
{
	m_View.set(m);
	m_WorldView.mul_43(m_View, m_World);
	m_ViewProject.mul(m_Project, m_View);
	m_WorldViewProject.mul(m_Project, m_WorldView);
	if (c_View)
		RenderBackendLegacy.set_Constant(c_View, m_View);
	if (c_ViewProject)
		RenderBackendLegacy.set_Constant(c_ViewProject, m_ViewProject);
	if (c_WorldView)
		RenderBackendLegacy.set_Constant(c_WorldView, m_WorldView);
	if (c_WorldViewProject)
		RenderBackendLegacy.set_Constant(c_WorldViewProject, m_WorldViewProject);
}
void R_transforms::set_Project(const fmat4x4& m)
{
	m_Project.set(m);
	m_ViewProject.mul(m_Project, m_View);
	m_WorldViewProject.mul(m_Project, m_WorldView);
	if (c_Project)
		RenderBackendLegacy.set_Constant(c_Project, m_Project);
	if (c_ViewProject)
		RenderBackendLegacy.set_Constant(c_ViewProject, m_ViewProject);
	if (c_WorldViewProject)
		RenderBackendLegacy.set_Constant(c_WorldViewProject, m_WorldViewProject);
}

void R_transforms::apply_InvWorld()
{
	VERIFY(c_InvWorld);

	if (!m_bInvWorldMatrixIsValid)
	{
		m_InvWorld.invert_b(m_World);
		m_bInvWorldMatrixIsValid = true;
	}

	RenderBackendLegacy.set_Constant(c_InvWorld, m_InvWorld);
}

void R_transforms::unmap()
{
	c_World = NULL;
	c_View = NULL;
	c_Project = NULL;
	c_WorldView = NULL;
	c_ViewProject = NULL;
	c_WorldViewProject = NULL;
}
R_transforms::R_transforms()
{
	unmap();
	m_World.identity();
	m_InvWorld.identity();
	m_View.identity();
	m_Project.identity();
	m_WorldView.identity();
	m_ViewProject.identity();
	m_WorldViewProject.identity();
	m_bInvWorldMatrixIsValid = true;
}
