#include "stdafx.h"
#include "..\xrEngine\cl_intersect.h"

extern Fvector du_cone_vertices[DU_CONE_NUMVERTEX];

BOOL tri_vs_sphere_intersect(Fvector& SC, float R, Fvector& v0, Fvector& v1, Fvector& v2)
{
	Fvector e0, e1;
	return CDB::TestSphereTri(SC, R, v0, e0.sub(v1, v0), e1.sub(v2, v0));
}

BOOL CRender::enable_scissor(light* L) // true if intersects near plane
{
	// Near plane intersection
	BOOL near_intersect = FALSE;
	{
		Fmatrix& M = Engine.RenderView.ViewProjection;
		Fvector4 plane;
		plane.x = -(M._14 + M._13);
		plane.y = -(M._24 + M._23);
		plane.z = -(M._34 + M._33);
		plane.w = -(M._44 + M._43);
		float denom = -1.0f / _sqrt(_sqr(plane.x) + _sqr(plane.y) + _sqr(plane.z));
		plane.mul(denom);
		Fplane P;
		P.n.set(plane.x, plane.y, plane.z);
		P.d = plane.w;
		float p_dist = P.classify(L->spatial.sphere.P) - L->spatial.sphere.R;
		near_intersect = (p_dist <= 0);
	}
//#ifdef DEBUG
//	if (1)
//	{
//		Fsphere S;
//		S.set(L->spatial.sphere.P, L->spatial.sphere.R);
//		dbg_spheres.push_back(mk_pair(S, L->get_color()));
//	}
//#endif

	// Scissor
	//. disable scissor because some bugs prevent it to work through multi-portals
	//. if (!HW.Caps.bScissor)	return		near_intersect;
	return near_intersect;
}
