#ifndef r_backend_transformH
#define r_backend_transformH
#pragma once

class ENGINE_API R_transforms
{
  public:
	fmat4x4 m_World;	// Basic	- world
	fmat4x4 m_InvWorld; // derived	- world2local, cached
	fmat4x4 m_View;	// Basic	- view
	fmat4x4 m_Project;	// Basic	- projection
	fmat4x4 m_WorldView;	// Derived	- world2view
	fmat4x4 m_ViewProject;	// Derived	- view2projection
	fmat4x4 m_WorldViewProject;	// Derived	- world2view2projection

	R_constant* c_World;
	R_constant* c_InvWorld;
	R_constant* c_View;
	R_constant* c_Project;
	R_constant* c_WorldView;
	R_constant* c_ViewProject;
	R_constant* c_WorldViewProject;

  private:
	bool m_bInvWorldMatrixIsValid;

  public:
	R_transforms();
	void unmap();
	void set_World(const fmat4x4& m);
	void set_View(const fmat4x4& m);
	void set_Project(const fmat4x4& m);
	IC const fmat4x4& get_World()
	{
		return m_World;
	}
	IC const fmat4x4& get_View()
	{
		return m_View;
	}
	IC const fmat4x4& get_Project()
	{
		return m_Project;
	}
	IC void set_c_World(R_constant* C);
	IC void set_c_InvWorld(R_constant* C);
	IC void set_c_View(R_constant* C);
	IC void set_c_Project(R_constant* C);
	IC void set_c_WorldView(R_constant* C);
	IC void set_c_ViewProject(R_constant* C);
	IC void set_c_WorldViewProject(R_constant* C);

  private:
	void apply_InvWorld();
};

#endif
