#ifndef r_backend_transformH
#define r_backend_transformH
#pragma once

class ENGINE_API R_transforms
{
  public:
	float4x4 m_World;	// Basic	- world
	float4x4 m_InvWorld; // derived	- world2local, cached
	float4x4 m_View;	// Basic	- view
	float4x4 m_Project;	// Basic	- projection
	float4x4 m_WorldView;	// Derived	- world2view
	float4x4 m_ViewProject;	// Derived	- view2projection
	float4x4 m_WorldViewProject;	// Derived	- world2view2projection

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
	void set_World(const float4x4& m);
	void set_View(const float4x4& m);
	void set_Project(const float4x4& m);
	IC const float4x4& get_World()
	{
		return m_World;
	}
	IC const float4x4& get_View()
	{
		return m_View;
	}
	IC const float4x4& get_Project()
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
