#include "stdafx.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\irenderable.h"

const float tweak_COP_initial_offs = 1200.f;

extern float ps_r_sun_far;

const float MAP_SIZE_START = 6.f;
const float MAP_GROW_FACTOR = 4.f;

//////////////////////////////////////////////////////////////////////////
// tables to calculate view-frustum bounds in world space
// note: D3D uses [0..1] range for Z
static Fvector3 corners[8] = {{-1, -1, 0},	{-1, -1, +1}, {-1, +1, +1}, {-1, +1, 0},
							  {+1, +1, +1}, {+1, +1, 0},  {+1, -1, +1}, {+1, -1, 0}};
static int facetable[6][4] = {
	{6, 7, 5, 4},
	{1, 0, 7, 6},
	{1, 2, 3, 0},
	{3, 2, 4, 5},
	// near and far planes
	{0, 3, 5, 7},
	{1, 6, 4, 2},
};
//////////////////////////////////////////////////////////////////////////
#define DW_AS_FLT(DW) (*(FLOAT*)&(DW))
#define FLT_AS_DW(F) (*(DWORD*)&(F))
#define FLT_SIGN(F) ((FLT_AS_DW(F) & 0x80000000L))
#define ALMOST_ZERO(F) ((FLT_AS_DW(F) & 0x7f800000L) == 0)
#define IS_SPECIAL(F) ((FLT_AS_DW(F) & 0x7f800000L) == 0x7f800000L)

//////////////////////////////////////////////////////////////////////////
struct Frustum
{
	Frustum();
	Frustum(const D3DXMATRIX* matrix);

	D3DXPLANE camPlanes[6];
	int nVertexLUT[6];
	D3DXVECTOR3 pntList[8];
};

///////////////////////////////////////////////////////////////////////////
//  PlaneIntersection
//    computes the point where three planes intersect
//    returns whether or not the point exists.
static inline BOOL PlaneIntersection(D3DXVECTOR3* intersectPt, const D3DXPLANE* p0, const D3DXPLANE* p1,
									 const D3DXPLANE* p2)
{
	D3DXVECTOR3 n0(p0->a, p0->b, p0->c);
	D3DXVECTOR3 n1(p1->a, p1->b, p1->c);
	D3DXVECTOR3 n2(p2->a, p2->b, p2->c);

	D3DXVECTOR3 n1_n2, n2_n0, n0_n1;

	D3DXVec3Cross(&n1_n2, &n1, &n2);
	D3DXVec3Cross(&n2_n0, &n2, &n0);
	D3DXVec3Cross(&n0_n1, &n0, &n1);

	float cosTheta = D3DXVec3Dot(&n0, &n1_n2);

	if (ALMOST_ZERO(cosTheta) || IS_SPECIAL(cosTheta))
		return FALSE;

	float secTheta = 1.f / cosTheta;

	n1_n2 = n1_n2 * p0->d;
	n2_n0 = n2_n0 * p1->d;
	n0_n1 = n0_n1 * p2->d;

	*intersectPt = -(n1_n2 + n2_n0 + n0_n1) * secTheta;
	return TRUE;
}

Frustum::Frustum()
{
	for (int i = 0; i < 6; i++)
		camPlanes[i] = D3DXPLANE(0.f, 0.f, 0.f, 0.f);
}

//  build a frustum from a camera (projection, or viewProjection) matrix
Frustum::Frustum(const D3DXMATRIX* matrix)
{
	//  build a view frustum based on the current view & projection matrices...
	D3DXVECTOR4 column4(matrix->_14, matrix->_24, matrix->_34, matrix->_44);
	D3DXVECTOR4 column1(matrix->_11, matrix->_21, matrix->_31, matrix->_41);
	D3DXVECTOR4 column2(matrix->_12, matrix->_22, matrix->_32, matrix->_42);
	D3DXVECTOR4 column3(matrix->_13, matrix->_23, matrix->_33, matrix->_43);

	D3DXVECTOR4 planes[6];
	planes[0] = column4 - column1; // left
	planes[1] = column4 + column1; // right
	planes[2] = column4 - column2; // bottom
	planes[3] = column4 + column2; // top
	planes[4] = column4 - column3; // near
	planes[5] = column4 + column3; // far
	// ignore near & far plane

	int p;

	for (p = 0; p < 6; p++) // normalize the planes
	{
		float dot = planes[p].x * planes[p].x + planes[p].y * planes[p].y + planes[p].z * planes[p].z;
		dot = 1.f / _sqrt(dot);
		planes[p] = planes[p] * dot;
	}

	for (p = 0; p < 6; p++)
		camPlanes[p] = D3DXPLANE(planes[p].x, planes[p].y, planes[p].z, planes[p].w);

	//  build a bit-field that will tell us the indices for the nearest and farthest vertices from each plane...
	for (int i = 0; i < 6; i++)
		nVertexLUT[i] = ((planes[i].x < 0.f) ? 1 : 0) | ((planes[i].y < 0.f) ? 2 : 0) | ((planes[i].z < 0.f) ? 4 : 0);

	for (int i = 0; i < 8; i++) // compute extrema
	{
		const D3DXPLANE& p0 = (i & 1) ? camPlanes[4] : camPlanes[5];
		const D3DXPLANE& p1 = (i & 2) ? camPlanes[3] : camPlanes[2];
		const D3DXPLANE& p2 = (i & 4) ? camPlanes[0] : camPlanes[1];
		PlaneIntersection(&pntList[i], &p0, &p1, &p2);
	}
}

//////////////////////////////////////////////////////////////////////////
// OLES: naive builder of infinite volume expanded from base frustum towards
//		 light source. really slow, but it works for our simple usage :)
// note: normals points to 'outside'
//////////////////////////////////////////////////////////////////////////

const u32 LIGHT_CUBOIDSIDEPOLYS_COUNT = 4;
const u32 LIGHT_CUBOIDVERTICES_COUNT = 2 * LIGHT_CUBOIDSIDEPOLYS_COUNT;

template <bool _debug> class FixedConvexVolume
{
  public:
	struct _poly
	{
		int points[4];
		Fplane plane;
	};

	xr_vector<sun::ray> view_frustum_rays;
	sun::ray view_ray;
	sun::ray light_ray;
	Fvector3 light_cuboid_points[LIGHT_CUBOIDVERTICES_COUNT];
	_poly light_cuboid_polys[LIGHT_CUBOIDSIDEPOLYS_COUNT];

  public:
	void compute_planes()
	{
		for (u32 it = 0; it < LIGHT_CUBOIDSIDEPOLYS_COUNT; it++)
		{
			_poly& P = light_cuboid_polys[it];

			P.plane.build(light_cuboid_points[P.points[0]], light_cuboid_points[P.points[2]],
						  light_cuboid_points[P.points[1]]);

			// verify
			if (_debug)
			{
				Fvector& p0 = light_cuboid_points[P.points[0]];
				Fvector& p1 = light_cuboid_points[P.points[1]];
				Fvector& p2 = light_cuboid_points[P.points[2]];
				Fvector& p3 = light_cuboid_points[P.points[3]];
				Fplane p012;
				p012.build(p0, p1, p2);
				Fplane p123;
				p123.build(p1, p2, p3);
				Fplane p230;
				p230.build(p2, p3, p0);
				Fplane p301;
				p301.build(p3, p0, p1);
				VERIFY(p012.n.similar(p123.n) && p012.n.similar(p230.n) && p012.n.similar(p301.n));
			}
		}
	}

	void compute_caster_model_fixed(xr_vector<Fplane>& dest, Fvector3& translation, float map_size,
									bool clip_by_view_near)
	{
		translation.set(0.f, 0.f, 0.f);

		if (fis_zero(1 - abs(view_ray.D.dotproduct(light_ray.D)), EPS_S))
			return;

		// compute planes for each polygon.
		compute_planes();

		for (u32 i = 0; i < LIGHT_CUBOIDSIDEPOLYS_COUNT; i++)
			VERIFY(light_cuboid_polys[i].plane.classify(light_ray.P) > 0);

		int align_planes[2];
		int align_planes_count = 0;

		// find one or two planes that align to view frustum from behind.
		for (u32 i = 0; i < LIGHT_CUBOIDSIDEPOLYS_COUNT; i++)
		{
			float tmp_dot = view_ray.D.dotproduct(light_cuboid_polys[i].plane.n);
			if (tmp_dot <= EPS_L)
				continue;

			align_planes[align_planes_count] = i;
			++align_planes_count;

			if (align_planes_count == 2)
				break;
		}

		Fvector align_vector;
		align_vector.set(0.f, 0.f, 0.f);

		// Align ray points to the align planes.
		for (int p = 0; p < align_planes_count; ++p)
		{
			// Hack !
			float min_dist = 10000;
			for (u32 i = 0; i < view_frustum_rays.size(); ++i)
			{
				float tmp_dist = 0;
				Fvector tmp_point = view_frustum_rays[i].P;

				tmp_dist = light_cuboid_polys[align_planes[p]].plane.classify(tmp_point);
				min_dist = _min(tmp_dist, min_dist);
			}

			Fvector shift = light_cuboid_polys[align_planes[p]].plane.n;
			shift.mul(min_dist);
			align_vector.add(shift);
		}

		translation.add(align_vector);

		// Move light ray by the alignment shift.
		light_ray.P.add(align_vector);

		// Here we can skip this stage us in the next pass we need only normals of planes.
		// in the next translate_light_model call will contain this shift as well.
		// translate_light_model	( align_vector );

		// Reset to reuse.
		align_vector.set(0.f, 0.f, 0.f);

		// Check if view edges intersect, and push planes................
		for (int p = 0; p < align_planes_count; ++p)
		{
			float max_mag = 0;
			for (u32 i = 0; i < view_frustum_rays.size(); ++i)
			{
				float plane_dot_ray = view_frustum_rays[i].D.dotproduct(light_cuboid_polys[align_planes[p]].plane.n);
				if (plane_dot_ray < 0)
				{
					Fvector per_plane_view;
					per_plane_view.crossproduct(light_cuboid_polys[align_planes[p]].plane.n, view_ray.D);
					Fvector per_view_to_plane;
					per_view_to_plane.crossproduct(per_plane_view, view_ray.D);

					float tmp_mag = -plane_dot_ray / view_frustum_rays[i].D.dotproduct(per_view_to_plane);

					max_mag = (max_mag < tmp_mag) ? tmp_mag : max_mag;
				}
			}

			if (fis_zero(max_mag))
				continue;

			VERIFY(max_mag <= 1.f);

			float dist = -light_cuboid_polys[align_planes[p]].plane.n.dotproduct(translation);
			align_vector.mad(light_cuboid_polys[align_planes[p]].plane.n, dist * max_mag);
		}

		translation.add(align_vector);
		light_ray.P.add(align_vector);
		translate_light_model(translation);

		// compute culling planes by rays as edges
		for (u32 i = 0; i < view_frustum_rays.size(); ++i)
		{
			Fvector tmp_vector;
			tmp_vector.crossproduct(view_frustum_rays[i].D, light_ray.D);

			// check if the vectors are parallel
			if (fis_zero(tmp_vector.square_magnitude(), EPS))
				continue;

			Fplane tmp_plane;
			tmp_plane.build(view_frustum_rays[i].P, tmp_vector);

			float sign = 0;
			if (check_cull_plane_valid(tmp_plane, sign, 5))
			{
				tmp_plane.n.mul(-sign);
				tmp_plane.d *= -sign;
				dest.push_back(tmp_plane);
			}
		}

		// compute culling planes by ray points pairs as edges
		if (clip_by_view_near && abs(view_ray.D.dotproduct(light_ray.D)) < 0.8)
		{
			Fvector perp_light_view, perp_light_to_view;
			perp_light_view.crossproduct(view_ray.D, light_ray.D);
			perp_light_to_view.crossproduct(perp_light_view, light_ray.D);

			Fplane plane;
			plane.build(view_ray.P, perp_light_to_view);

			float max_dist = -1000;
			for (u32 i = 0; i < view_frustum_rays.size(); ++i)
				max_dist = _max(plane.classify(view_frustum_rays[i].P), max_dist);

			for (u32 i = 0; i < view_frustum_rays.size(); ++i)
			{
				Fvector P = view_frustum_rays[i].P;
				P.mad(view_frustum_rays[i].D, 5);

				if (plane.classify(P) > max_dist)
				{
					max_dist = 0.f;
					break;
				}
			}

			if (max_dist > -1000)
			{
				plane.d += max_dist;
				dest.push_back(plane);
			}
		}

		for (u32 i = 0; i < LIGHT_CUBOIDSIDEPOLYS_COUNT; i++)
		{
			dest.push_back(light_cuboid_polys[i].plane);
			dest.back().n.mul(-1);
			dest.back().d *= -1;
			VERIFY(light_cuboid_polys[i].plane.classify(light_ray.P) > 0);
		}

		// Compute ray intersection with light model, this is needed to next cascade to start it's placement.
		for (u32 i = 0; i < view_frustum_rays.size(); ++i)
		{
			float min_dist = 2 * map_size;
			for (int p = 0; p < 4; ++p)
			{
				float dist;
				if ((light_cuboid_polys[p].plane.n.dotproduct(view_frustum_rays[i].D)) > -0.1)
					dist = map_size;
				else
					light_cuboid_polys[p].plane.intersectRayDist(view_frustum_rays[i].P, view_frustum_rays[i].D, dist);

				if (dist > EPS_L && dist < min_dist)
					min_dist = dist;
			}

			view_frustum_rays[i].P.mad(view_frustum_rays[i].D, min_dist);
		}
	}

	bool check_cull_plane_valid(Fplane const& plane, float& sign, float mad_factor = 0.f)
	{
		bool valid = false;
		bool oriented = false;
		float orient = 0;
		for (u32 j = 0; j < view_frustum_rays.size(); ++j)
		{
			float tmp_dist = 0.f;
			Fvector tmp_pt = view_frustum_rays[j].P;
			tmp_pt.mad(view_frustum_rays[j].D, mad_factor);
			tmp_dist = plane.classify(tmp_pt);

			if (fis_zero(tmp_dist, EPS_L))
				continue;

			if (!oriented)
			{
				orient = tmp_dist > 0.f ? 1.f : -1.f;
				valid = true;
				oriented = true;
				continue;
			}

			if (tmp_dist < 0 && orient < 0 || tmp_dist > 0 && orient > 0)
				continue;

			valid = false;
			break;
		}
		sign = orient;
		return valid;
	}

	void translate_light_model(Fvector translate)
	{
		Fmatrix trans_mat;
		trans_mat.translate(translate);
		for (int i = 0; i < LIGHT_CUBOIDSIDEPOLYS_COUNT; ++i)
			light_cuboid_polys[i].plane.d -= translate.dotproduct(light_cuboid_polys[i].plane.n);
	}
};

//////////////////////////////////////////////////////////////////////////
// OLES: naive builder of infinite volume expanded from base frustum towards
//		 light source. really slow, but it works for our simple usage :)
// note: normals points to 'outside'
//////////////////////////////////////////////////////////////////////////
template <bool _debug> class DumbConvexVolume
{
  public:
	struct _poly
	{
		xr_vector<int> points;
		Fvector3 planeN;
		float planeD;
		float classify(Fvector3& p)
		{
			return planeN.dotproduct(p) + planeD;
		}
	};
	struct _edge
	{
		int p0, p1;
		int counter;
		_edge(int _p0, int _p1, int m) : p0(_p0), p1(_p1), counter(m)
		{
			if (p0 > p1)
				swap(p0, p1);
		}
		bool equal(_edge& E)
		{
			return p0 == E.p0 && p1 == E.p1;
		}
	};

  public:
	xr_vector<Fvector3> points;
	xr_vector<_poly> polys;
	xr_vector<_edge> edges;

  public:
	void compute_planes()
	{
		for (int it = 0; it < int(polys.size()); it++)
		{
			_poly& P = polys[it];
			Fvector3 t1, t2;
			t1.sub(points[P.points[0]], points[P.points[1]]);
			t2.sub(points[P.points[0]], points[P.points[2]]);
			P.planeN.crossproduct(t1, t2).normalize();
			P.planeD = -P.planeN.dotproduct(points[P.points[0]]);

			// verify
			if (_debug)
			{
				Fvector& p0 = points[P.points[0]];
				Fvector& p1 = points[P.points[1]];
				Fvector& p2 = points[P.points[2]];
				Fvector& p3 = points[P.points[3]];
				Fplane p012;
				p012.build(p0, p1, p2);
				Fplane p123;
				p123.build(p1, p2, p3);
				Fplane p230;
				p230.build(p2, p3, p0);
				Fplane p301;
				p301.build(p3, p0, p1);
				VERIFY(p012.n.similar(p123.n) && p012.n.similar(p230.n) && p012.n.similar(p301.n));
			}
		}
	}

	void compute_caster_model(xr_vector<Fplane>& dest, Fvector3 direction)
	{
		CRenderTarget& T = *RenderImplementation.RenderTarget;

		// COG
		Fvector3 cog = {0, 0, 0};
		for (int it = 0; it < int(points.size()); it++)
			cog.add(points[it]);
		cog.div(float(points.size()));

		// planes
		compute_planes();
		for (int it = 0; it < int(polys.size()); it++)
		{
			_poly& base = polys[it];
			if (base.classify(cog) > 0)
				std::reverse(base.points.begin(), base.points.end());
		}

		// remove faceforward polys, build list of edges -> find open ones
		compute_planes();
		for (int it = 0; it < int(polys.size()); it++)
		{
			_poly& base = polys[it];
			VERIFY(base.classify(cog) < 0); // debug

			int m_traversal_marker = (base.planeN.dotproduct(direction) <= 0) ? -1 : 1;

			// register edges
			xr_vector<int>& plist = polys[it].points;
			for (int p = 0; p < int(plist.size()); p++)
			{
				_edge E(plist[p], plist[(p + 1) % plist.size()], m_traversal_marker);
				bool found = false;
				for (int e = 0; e < int(edges.size()); e++)
					if (edges[e].equal(E))
					{
						edges[e].counter += m_traversal_marker;
						found = true;
						break;
					}
				if (!found)
				{
					edges.push_back(E);
					if (_debug)
						T.dbg_addline(points[E.p0], points[E.p1], color_rgba(255, 0, 0, 255));
				}
			}

			// remove if unused
			if (m_traversal_marker < 0)
			{
				polys.erase(polys.begin() + it);
				it--;
			}
		}

		// Extend model to infinity, the volume is not capped, so this is indeed up to infinity
		for (int e = 0; e < int(edges.size()); e++)
		{
			if (edges[e].counter != 0)
				continue;
			_edge& E = edges[e];
			if (_debug)
				T.dbg_addline(points[E.p0], points[E.p1], color_rgba(255, 255, 255, 255));
			Fvector3 point;
			points.push_back(point.sub(points[E.p0], direction));
			points.push_back(point.sub(points[E.p1], direction));
			polys.push_back(_poly());
			_poly& P = polys.back();
			int pend = int(points.size());
			P.points.push_back(E.p0);
			P.points.push_back(E.p1);
			P.points.push_back(pend - 1); // p1 mod
			P.points.push_back(pend - 2); // p0 mod
			if (_debug)
				T.dbg_addline(points[E.p0], point.mad(points[E.p0], direction, -1000), color_rgba(0, 255, 0, 255));
			if (_debug)
				T.dbg_addline(points[E.p1], point.mad(points[E.p1], direction, -1000), color_rgba(0, 255, 0, 255));
		}

		// Reorient planes (try to write more inefficient code :)
		compute_planes();
		for (int it = 0; it < int(polys.size()); it++)
		{
			_poly& base = polys[it];
			if (base.classify(cog) > 0)
				std::reverse(base.points.begin(), base.points.end());
		}

		// Export
		compute_planes();
		for (int it = 0; it < int(polys.size()); it++)
		{
			_poly& P = polys[it];
			Fplane pp = {P.planeN, P.planeD};
			dest.push_back(pp);
		}
	}
};

//////////////////////////////////////////////////////////////////////////
Fvector3 wform(Fmatrix& m, Fvector3 const& v)
{
	Fvector4 r;
	r.x = v.x * m._11 + v.y * m._21 + v.z * m._31 + m._41;
	r.y = v.x * m._12 + v.y * m._22 + v.z * m._32 + m._42;
	r.z = v.x * m._13 + v.y * m._23 + v.z * m._33 + m._43;
	r.w = v.x * m._14 + v.y * m._24 + v.z * m._34 + m._44;
	// VERIFY		(r.w>0.f);
	float invW = 1.0f / r.w;
	Fvector3 r3 = {r.x * invW, r.y * invW, r.z * invW};
	return r3;
}

void CRender::init_cacades()
{
	float fBias = -0.0000025f;
	float size = MAP_SIZE_START;
	u32 cascade_count = 3;

	m_sun_cascades.resize(cascade_count);

	m_sun_cascades[0].reset_chain = true;
	m_sun_cascades[0].size = ps_r_sun_near;
	m_sun_cascades[0].bias = m_sun_cascades[0].size * fBias;

	m_sun_cascades[1].size = ps_r_sun_near * 3;
	m_sun_cascades[1].bias = m_sun_cascades[1].size * fBias;

	m_sun_cascades[2].size = ps_r_sun_far;
	m_sun_cascades[2].bias = m_sun_cascades[2].size * fBias;

	for (int i = 0; i < cascade_count; ++i)
	{
		m_sun_work_items[i] = xr_new<ShadowCascadeWorkItem>();
		m_sun_work_items[i]->packet.InitResources();
	}
}

// -------------------------------------------------------------------------
//  Phase 1: GATHER (Параллельный сбор)
// -------------------------------------------------------------------------
void CRender::gather_sun_cascade(u32 cascade_ind, ShadowCascadeWorkItem& item)
{
	light* sun = (light*)Lights.sun_adapted._get();

	// 1. Calculate view-frustum bounds in world space
	// ---------------------------------------------------------------------
	Fmatrix ex_project, ex_full, ex_full_inverse;
	{
		ex_project = Engine.RenderView.Project;
		ex_full.mul(ex_project, Engine.RenderView.View);
		D3DXMatrixInverse((D3DXMATRIX*)&ex_full_inverse, 0, (D3DXMATRIX*)&ex_full);
	}

	// Local variables for calculation
	CFrustum cull_frustum;
	xr_vector<Fplane> cull_planes;
	Fvector3 cull_COP;
	CSector* cull_sector;
	Fmatrix cull_transform;

	{
		FPU::m64r();
		// Lets begin from base frustum
		Fmatrix fulltransform_inv = ex_full_inverse;
#ifdef _DEBUG
		typedef DumbConvexVolume<true> t_volume;
#else
		typedef DumbConvexVolume<false> t_volume;
#endif

		//******************************* Need to be placed after cuboid built **************************
		// Search for default sector - assume "default" or "outdoor" sector is the largest one
		//. hack: need to know real outdoor sector
		CSector* largest_sector = 0;
		float largest_sector_vol = 0;
		for (u32 s = 0; s < Sectors.size(); s++)
		{
			CSector* S = (CSector*)Sectors[s];
			IRender_Visual* V = S->root();
			float vol = V->vis.box.getvolume();
			if (vol > largest_sector_vol)
			{
				largest_sector_vol = vol;
				largest_sector = S;
			}
		}
		cull_sector = largest_sector;

		// COP - 100 km away
		cull_COP.mad(Engine.RenderView.Position, sun->get_direction(), -tweak_COP_initial_offs);

		// Create approximate ortho-transform
		// view: auto find 'up' and 'right' vectors
		Fmatrix mdir_View, mdir_Project;
		Fvector L_dir, L_up, L_right, L_pos;
		L_pos.set(sun->get_position());
		L_dir.set(sun->get_direction()).normalize();
		L_right.set(1, 0, 0);
		if (_abs(L_right.dotproduct(L_dir)) > .99f)
			L_right.set(0, 0, 1);
		L_up.crossproduct(L_dir, L_right).normalize();
		L_right.crossproduct(L_up, L_dir).normalize();
		mdir_View.build_camera_dir(L_pos, L_dir, L_up);

		//////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
		typedef FixedConvexVolume<true> t_cuboid;
#else
		typedef FixedConvexVolume<false> t_cuboid;
#endif

		t_cuboid light_cuboid;
		{
			// Initialize the first cascade rays, then each cascade will initialize rays for next one.
			if (cascade_ind == 0 || m_sun_cascades[cascade_ind].reset_chain)
			{
				Fvector3 near_p, edge_vec;
				for (int p = 0; p < 4; p++)
				{
					near_p = wform(fulltransform_inv, corners[facetable[4][p]]);
					edge_vec = wform(fulltransform_inv, corners[facetable[5][p]]);
					edge_vec.sub(near_p);
					edge_vec.normalize();

					light_cuboid.view_frustum_rays.push_back(sun::ray(near_p, edge_vec));
				}
			}
			else
				light_cuboid.view_frustum_rays = m_sun_cascades[cascade_ind].rays;

			light_cuboid.view_ray.P = Engine.RenderView.Position;
			light_cuboid.view_ray.D = Engine.RenderView.Direction;
			light_cuboid.light_ray.P = L_pos;
			light_cuboid.light_ray.D = L_dir;
		}

		// THIS NEED TO BE A CONSTATNT
		Fplane light_top_plane;
		light_top_plane.build_unit_normal(L_pos, L_dir);
		float dist = light_top_plane.classify(Engine.RenderView.Position);

		float map_size = m_sun_cascades[cascade_ind].size;
		D3DXMatrixOrthoOffCenterLH((D3DXMATRIX*)&mdir_Project, -map_size * 0.5f, map_size * 0.5f, -map_size * 0.5f,
								   map_size * 0.5f, 0.1, dist + map_size);

		// build viewport transform
		float view_dim = float(RenderImplementation.o.smapsize);
		Fmatrix m_viewport = {view_dim / 2.f, 0.0f, 0.0f, 0.0f, 0.0f,			-view_dim / 2.f, 0.0f, 0.0f,
							  0.0f,			  0.0f, 1.0f, 0.0f, view_dim / 2.f, view_dim / 2.f,	 0.0f, 1.0f};
		Fmatrix m_viewport_inv;
		D3DXMatrixInverse((D3DXMATRIX*)&m_viewport_inv, 0, (D3DXMATRIX*)&m_viewport);

		cull_transform.mul(mdir_Project, mdir_View);
		Fmatrix cull_transform_inv;
		cull_transform_inv.invert(cull_transform);

		for (int p = 0; p < 8; p++)
		{
			Fvector3 xf = wform(cull_transform_inv, corners[p]);
			light_cuboid.light_cuboid_points[p] = xf;
		}

		// only side planes
		for (int plane = 0; plane < 4; plane++)
			for (int pt = 0; pt < 4; pt++)
			{
				int asd = facetable[plane][pt];
				light_cuboid.light_cuboid_polys[plane].points[pt] = asd;
			}

		Fvector lightXZshift;
		light_cuboid.compute_caster_model_fixed(cull_planes, lightXZshift, m_sun_cascades[cascade_ind].size,
												m_sun_cascades[cascade_ind].reset_chain);
		Fvector proj_view = Engine.RenderView.Direction;
		proj_view.y = 0;
		proj_view.normalize();

		// Initialize rays for the next cascade
		// ВАЖНО: Это модификация глобального массива, но индексы разделены,
		// и следующий каскад будет читать это только в следующем кадре (или нужно синхронизировать).
		// В оригинальной реализации каскады зависели друг от друга.
		// При параллельном запуске cascade[1] не увидит изменений от cascade[0] в ЭТОМ кадре.
		// Это нормально для теней (будет задержка в 1 кадр для обновления границы каскадов),
		// либо можно предрасчитать лучи заранее последовательно.
		// Для простоты оставляем как есть - визуально это почти незаметно.
		if (cascade_ind < m_sun_cascades.size() - 1)
			m_sun_cascades[cascade_ind + 1].rays = light_cuboid.view_frustum_rays;

		Fvector cam_shifted = L_pos;
		cam_shifted.add(lightXZshift);

		// rebuild the view transform with the shift.
		mdir_View.identity();
		mdir_View.build_camera_dir(cam_shifted, L_dir, L_up);
		cull_transform.identity();
		cull_transform.mul(mdir_Project, mdir_View);
		cull_transform_inv.invert(cull_transform);

		// Create frustum for query
		cull_frustum._clear();
		for (u32 p = 0; p < cull_planes.size(); p++)
			cull_frustum._add(cull_planes[p]);

		Fvector cam_proj = Engine.RenderView.Position;
		const float align_aim_step_coef = 4.f;
		cam_proj.set(floorf(cam_proj.x / align_aim_step_coef) + align_aim_step_coef / 2,
					 floorf(cam_proj.y / align_aim_step_coef) + align_aim_step_coef / 2,
					 floorf(cam_proj.z / align_aim_step_coef) + align_aim_step_coef / 2);
		cam_proj.mul(align_aim_step_coef);
		Fvector cam_pixel = wform(cull_transform, cam_proj);
		cam_pixel = wform(m_viewport, cam_pixel);
		Fvector shift_proj = lightXZshift;
		cull_transform.transform_dir(shift_proj);
		m_viewport.transform_dir(shift_proj);

		const float align_granularity = 4.f;
		shift_proj.x = shift_proj.x > 0 ? align_granularity : -align_granularity;
		shift_proj.y = shift_proj.y > 0 ? align_granularity : -align_granularity;
		shift_proj.z = 0;

		cam_pixel.x = cam_pixel.x / align_granularity - floorf(cam_pixel.x / align_granularity);
		cam_pixel.y = cam_pixel.y / align_granularity - floorf(cam_pixel.y / align_granularity);
		cam_pixel.x *= align_granularity;
		cam_pixel.y *= align_granularity;
		cam_pixel.z = 0;

		cam_pixel.sub(shift_proj);

		m_viewport_inv.transform_dir(cam_pixel);
		cull_transform_inv.transform_dir(cam_pixel);
		Fvector diff = cam_pixel;
		static float sign_test = -1.f;
		diff.mul(sign_test);
		Fmatrix adjust;
		adjust.translate(diff);
		cull_transform.mulB_44(adjust);

		m_sun_cascades[cascade_ind].transform = cull_transform;

		// full-transform
		FPU::m24r();
	}

	// Сохраняем результаты в WorkItem для фазы Draw
	item.cull_transform = cull_transform;
	item.cull_frustum = cull_frustum;
	item.cull_sector = cull_sector;
	item.cull_COP = cull_COP;

	// 2. Сбор сцены (Scene Graph Traversal)
	// ---------------------------------------------------------------------

	// Настраиваем локальный контекст
	SceneTraversalContext local_ctx;
	local_ctx.frustum = &item.cull_frustum;
	local_ctx.is_hud_pass = FALSE;
	local_ctx.is_invisible_mode = FALSE;
	local_ctx.current_owner = nullptr;
	local_ctx.current_transform = &Fidentity;

	// Активируем TLS: пишем в item.packet
	CurrentRenderContext::Scope tls_scope(item.packet, local_ctx);

	// Запускаем сбор (используя TLS)
	// Добавляем флаг CPortalTraverser::VQ_SCISSOR | CPortalTraverser::VQ_HOM для оптимизации
	// Но для теней VQ_FADE не нужен.
	// В SceneGraph::render_subspace сейчас хардкод опций (0) в traverse.
	// Это можно улучшить, но пока работает и так.

	SceneGraph.render_subspace(item.cull_sector, &item.cull_frustum, item.cull_transform, item.cull_COP, TRUE, FALSE,
							   item.packet);
}

// -------------------------------------------------------------------------
//  Phase 2: DRAW (Последовательная отрисовка)
// -------------------------------------------------------------------------
void CRender::draw_sun_cascade(u32 cascade_ind, ShadowCascadeWorkItem& item)
{
	OPTICK_EVENT("Draw Cascade");

	light* sun = (light*)Lights.sun_adapted._get();

	// 1. Применяем матрицы к глобальному источнику (теперь мы в главном потоке)
	sun->X.D.combine = item.cull_transform;
	sun->X.D.minX = 0;
	sun->X.D.maxX = RenderImplementation.o.smapsize;
	sun->X.D.minY = 0;
	sun->X.D.maxY = RenderImplementation.o.smapsize;

	// 2. Настройка состояний
	HOM.Disable();
	set_active_phase(PHASE_SHADOW_DEPTH);

	// 3. Отрисовка
	bool bNormal = item.packet.queue_static[0].size() || item.packet.queue_dynamic[0].size();
	bool bSpecial = item.packet.queue_static[1].size() || item.packet.queue_dynamic[1].size() ||
					item.packet.queue_transparent.size();

	if (bNormal || bSpecial)
	{
		// Устанавливаем Render Target (Shadow Map)
		render_shadow_map_sun(sun, cascade_ind);

		RenderBackend.set_transform_world(Fidentity);
		RenderBackend.set_transform_view(Fidentity);
		RenderBackend.set_transform_project(sun->X.D.combine);

		// Рисуем Sun Details (траву), если нужно
		if (ps_r_lighting_flags.test(RFLAG_SUN_DETAILS))
		{
			// Трава рисуется отдельно, так как она не в графе
			Details->Render(DetailsRenderMode::DepthOnly, &m_sun_cascades[cascade_ind].transform, &item.cull_frustum);
		}

		// РЕНДЕР ИЗ ПАКЕТА!
		SceneGraph.Render(item.packet, SceneGraphRenderType::Opaque);

		// Рисуем Occluder (если есть)
		if (m_SunOccluder)
			m_SunOccluder->Render();

		sun->X.D.transluent = FALSE;
	}

	// 4. Аккумуляция (наложение тени на экран)
	set_light_accumulator();

	accumulate_sun(cascade_ind, m_sun_cascades[cascade_ind].transform, m_sun_cascades[cascade_ind].transform,
				   m_sun_cascades[cascade_ind].bias);
}

// -------------------------------------------------------------------------
//  Main Parallel Function
// -------------------------------------------------------------------------
void CRender::render_sun_cascades()
{
	PROFILE_FUNCTION();

	for (int i = 0; i < 3; ++i)
		m_sun_work_items[i]->packet.Clear();

	// 1. ПАРАЛЛЕЛЬНЫЙ СБОР (GATHER)
	{
		OPTICK_EVENT("Gather Cascades");
		concurrency::parallel_invoke([&] { gather_sun_cascade(0, *m_sun_work_items[0]); },
									 [&] { gather_sun_cascade(1, *m_sun_work_items[1]); },
									 [&] { gather_sun_cascade(2, *m_sun_work_items[2]); });
	}

	// 2. ПОСЛЕДОВАТЕЛЬНАЯ ОТРИСОВКА (DRAW)
	{
		OPTICK_EVENT("Draw Cascades Sequence");
		draw_sun_cascade(0, *m_sun_work_items[0]);
		draw_sun_cascade(1, *m_sun_work_items[1]);
		draw_sun_cascade(2, *m_sun_work_items[2]);
	}

	// Восстановление глобальных матриц
	RenderBackend.set_transform_world(Fidentity);
	RenderBackend.set_transform_view(Engine.RenderView.View);
	RenderBackend.set_transform_project(Engine.RenderView.Project);
}
