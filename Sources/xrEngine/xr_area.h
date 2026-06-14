#ifndef __XR_AREA_H__
#define __XR_AREA_H__

#include "xr_collide_form.h"
#include "xr_collide_defs.h"

// refs
class ENGINE_API ISpatial;
class ENGINE_API ICollisionForm;
class ENGINE_API CObject;

//-----------------------------------------------------------------------------------------------------------
// Space Area
//-----------------------------------------------------------------------------------------------------------
class ENGINE_API CObjectSpace
{
  private:
	CDB::MODEL Static;
	Fbox m_BoundingVolume;

  public:
#ifdef DEBUG
	ref_shader sh_debug;
	clQueryCollision q_debug;				  // MT: dangerous
	xr_vector<std::pair<Fsphere, u32>> dbg_S; // MT: dangerous
#endif

  private:
	struct RayQueryThreadData
	{
		xrXRC xrc;
		collide::rq_results r_temp;
		xr_vector<ISpatial*> r_spatial;

		// Kонструктор для резерва памяти, чтобы избежать аллокаций на первом кадре
		RayQueryThreadData()
		{
			r_spatial.reserve(128);
		}
	};

	RayQueryThreadData& GetRayThreadData()
	{
		// thread_local гарантирует, что у каждого потока будет своя копия этой переменной.
		// Она инициализируется один раз при первом обращении в потоке.
		static thread_local RayQueryThreadData data;
		return data;
	}

  private:

	BOOL _RayTest(const fvec3& start, const fvec3& dir, float range, collide::rq_target tgt,
				  collide::ray_cache* cache, CObject* ignore_object);
	BOOL _RayPick(const fvec3& start, const fvec3& dir, float range, collide::rq_target tgt, collide::rq_result& R,
				  CObject* ignore_object);
	BOOL _RayQuery(collide::rq_results& dest, const collide::ray_defs& rq, collide::rq_callback* cb, LPVOID user_data,
				   collide::test_callback* tb, CObject* ignore_object);
	BOOL _RayQuery2(collide::rq_results& dest, const collide::ray_defs& rq, collide::rq_callback* cb, LPVOID user_data,
					collide::test_callback* tb, CObject* ignore_object);
	BOOL _RayQuery3(collide::rq_results& dest, const collide::ray_defs& rq, collide::rq_callback* cb, LPVOID user_data,
					collide::test_callback* tb, CObject* ignore_object);

  public:
	CObjectSpace();
	~CObjectSpace();

	void Load();

	// Occluded/No
	BOOL RayTest(const fvec3& start, const fvec3& dir, float range, collide::rq_target tgt,
				 collide::ray_cache* cache, CObject* ignore_object);

	// Game raypick (nearest) - returns object and addititional params
	BOOL RayPick(const fvec3& start, const fvec3& dir, float range, collide::rq_target tgt, collide::rq_result& R,
				 CObject* ignore_object);

	// General collision query
	BOOL RayQuery(collide::rq_results& dest, const collide::ray_defs& rq, collide::rq_callback* cb, LPVOID user_data,
				  collide::test_callback* tb, CObject* ignore_object);
	BOOL RayQuery(collide::rq_results& dest, ICollisionForm* target, const collide::ray_defs& rq);
	// void								BoxQuery			( collide::rq_results& dest, const Fbox& B, const fmat4x4&
	// M, u32 flags=clGET_TRIS|clGET_BOXES|clQUERY_STATIC|clQUERY_DYNAMIC);

	int GetNearest(xr_vector<CObject*>& q_nearest, ICollisionForm* obj, float range);
	int GetNearest(xr_vector<CObject*>& q_nearest, const fvec3& point, float range, CObject* ignore_object);

	CDB::TRI* GetStaticTris()
	{
		return Static.get_tris();
	}
	fvec3* GetStaticVerts()
	{
		return Static.get_verts();
	}
	CDB::MODEL* GetStaticModel()
	{
		return &Static;
	}

	const Fbox& GetBoundingVolume()
	{
		return m_BoundingVolume;
	}

	// Debugging
#ifdef DEBUG
	void dbgRender();
	ref_shader dbgGetShader()
	{
		return sh_debug;
	}
#endif
};

#endif //__XR_AREA_H__
