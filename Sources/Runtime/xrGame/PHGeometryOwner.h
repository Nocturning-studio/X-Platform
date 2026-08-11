#ifndef PH_GEOMETRY_OWNER_H
#define PH_GEOMETRY_OWNER_H
#include "Geometry.h"
#include "gamemtllib.h"

DEFINE_VECTOR(CODEGeom*, GEOM_STORAGE, GEOM_I)

struct SBoneShape;
class CKinematics;

class CPHGeometryOwner
{
  protected:
	GEOM_STORAGE m_geoms; // e
	// bl
	bool b_builded;
	dSpaceID m_group;								   // e					//bl
	fvec3 m_mass_center;							   // e ??				//bl
	CPhysicsShellHolder* m_phys_ref_object;			   //->to shell ??		//bl
	float m_volume;									   // e ??				//bl
	u16 ul_material;								   // e ??				//bl
	ContactCallbackFun* contact_callback;			   //->to shell ??		//bt
	ObjectContactCallbackFun* object_contact_callback; //->to shell ??		//st
  public:
	///
	void add_Sphere(const Fsphere& V);								// aux
	void add_Box(const Fobb& V);									// aux
	void add_Cylinder(const Fcylinder& V);							// aux
	void add_Shape(const SBoneShape& shape);						// aux
	void add_Shape(const SBoneShape& shape, const fmat4x4& offset); // aux
	CODEGeom* last_geom()
	{
		if (m_geoms.empty())
			return NULL;
		return m_geoms.back();
	} // aux
	bool has_geoms()
	{
		return !m_geoms.empty();
	}
	void set_ContactCallback(ContactCallbackFun* callback);				   // aux (may not be)
	void set_ObjectContactCallback(ObjectContactCallbackFun* callback);	   // called anywhere ph state influent
	void add_ObjectContactCallback(ObjectContactCallbackFun* callback);	   // called anywhere ph state influent
	void remove_ObjectContactCallback(ObjectContactCallbackFun* callback); // called anywhere ph state influent
	void set_CallbackData(void* cd);
	void* get_CallbackData();
	ObjectContactCallbackFun* get_ObjectContactCallback();
	void set_PhysicsRefObject(CPhysicsShellHolder* ref_object); // aux
	CPhysicsShellHolder* PhysicsRefObject()
	{
		return m_phys_ref_object;
	} // aux
	void SetPhObjectInGeomData(CPHObject* O);

	void SetMaterial(u16 m);
	void SetMaterial(LPCSTR m)
	{
		SetMaterial(GMLib.GetMaterialIdx(m));
	} // aux
	IC CODEGeom* Geom(u16 num)
	{
		R_ASSERT2(num < m_geoms.size(), "out of range");
		return m_geoms[num];
	}
	CODEGeom* GeomByBoneID(u16 bone_id);
	u16 numberOfGeoms();	   // aux
	dGeomID dSpacedGeometry(); // aux
	fvec3 get_mc_data();	   // aux
	fvec3 get_mc_geoms();	   // aux
	void get_mc_kinematics(CKinematics* K, fvec3& mc, float& mass);
	void calc_volume_data(); // aux
	const fvec3& local_mass_Center()
	{
		return m_mass_center;
	} // aux
	float get_volume()
	{
		calc_volume_data();
		return m_volume;
	};																						  // aux
	void get_Extensions(const fvec3& axis, float center_prg, float& lo_ext, float& hi_ext); // aux
	void get_MaxAreaDir(fvec3& dir);
	float getRadius();
	void setStaticForm(const fmat4x4& form);
	void setPosition(const fvec3& pos);
	void clear_cashed_tries();
	void get_mc_vs_transform(fvec3& mc, const fmat4x4& m);

  protected:
	void build();
	void CreateSimulBase();
	void destroy();
	void build_Geom(CODEGeom& V); // aux
	void build_Geom(u16 i);
	void set_body(dBodyID body);

	CPHGeometryOwner();
	virtual ~CPHGeometryOwner();

  private:
};

#endif
