#ifndef r_backend_transformH
#define r_backend_transformH
#pragma once

class ENGINE_API R_transforms
{
  public:
	Fmatrix m_World;	// Basic	- world
	Fmatrix m_InvWorld; // derived	- world2local, cached
	Fmatrix m_View;	// Basic	- view
	Fmatrix m_Project;	// Basic	- projection
	Fmatrix m_WorldView;	// Derived	- world2view
	Fmatrix m_ViewProject;	// Derived	- view2projection
	Fmatrix m_WorldViewProject;	// Derived	- world2view2projection

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
	void set_World(const Fmatrix& m);
	void set_View(const Fmatrix& m);
	void set_Project(const Fmatrix& m);
	IC const Fmatrix& get_World()
	{
		return m_World;
	}
	IC const Fmatrix& get_View()
	{
		return m_View;
	}
	IC const Fmatrix& get_Project()
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
