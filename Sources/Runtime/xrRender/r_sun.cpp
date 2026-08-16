#include "stdafx.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\irenderable.h"

const float tweak_COP_initial_offs = 1200.f;

extern float ps_r_sun_far;
//////////////////////////////////////////////////////////////////////////
// tables to calculate view-frustum bounds in world space
// note: D3D uses [0..1] range for Z
static fvec3 corners[8] = {
	{-1, -1, 0},	
	{-1, -1, +1}, 
	{-1, +1, +1}, 
	{-1, +1, 0},
	{+1, +1, +1}, 
	{+1, +1, 0},  
	{+1, -1, +1}, 
	{+1, -1, 0}
};

static int facetable[6][4] = {
	{6, 7, 5, 4},   // right
	{1, 0, 7, 6},   // bottom
	{1, 2, 3, 0},   // left
	{3, 2, 4, 5},   // top
	{0, 3, 5, 7},   // near
	{1, 2, 4, 6}    // far
};

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

	xr_vector<Sun::Ray> view_frustum_rays;
	Sun::Ray view_ray;
	Sun::Ray light_ray;
	fvec3 light_cuboid_points[LIGHT_CUBOIDVERTICES_COUNT];
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
				fvec3& p0 = light_cuboid_points[P.points[0]];
				fvec3& p1 = light_cuboid_points[P.points[1]];
				fvec3& p2 = light_cuboid_points[P.points[2]];
				fvec3& p3 = light_cuboid_points[P.points[3]];
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

	void compute_caster_model_fixed(xr_vector<Fplane>& dest, 
									fvec3& translation, 
									float map_size,
									bool clip_by_view_near)
	{
		translation.set(0.f, 0.f, 0.f);

		if (fis_zero(1 - abs(view_ray.Direction.dotproduct(light_ray.Direction)), EPS_S))
			return;

		// compute planes for each polygon.
		compute_planes();

		for (u32 i = 0; i < LIGHT_CUBOIDSIDEPOLYS_COUNT; i++)
			VERIFY(light_cuboid_polys[i].plane.classify(light_ray.Position) > 0);

		int align_planes[2];
		int align_planes_count = 0;

		// find one or two planes that align to view frustum from behind.
		for (u32 i = 0; i < LIGHT_CUBOIDSIDEPOLYS_COUNT; i++)
		{
			float tmp_dot = view_ray.Direction.dotproduct(light_cuboid_polys[i].plane.n);
			if (tmp_dot <= EPS_L)
				continue;

			align_planes[align_planes_count] = i;
			++align_planes_count;

			if (align_planes_count == 2)
				break;
		}

		fvec3 align_vector;
		align_vector.set(0.f, 0.f, 0.f);

		// Align ray points to the align planes.
		for (int p = 0; p < align_planes_count; ++p)
		{
			// Hack !
			float min_dist = 10000;
			for (u32 i = 0; i < view_frustum_rays.size(); ++i)
			{
				float tmp_dist = 0;
				fvec3 tmp_point = view_frustum_rays[i].Position;

				tmp_dist = light_cuboid_polys[align_planes[p]].plane.classify(tmp_point);
				min_dist = _min(tmp_dist, min_dist);
			}

			fvec3 shift = light_cuboid_polys[align_planes[p]].plane.n;
			shift.mul(min_dist);
			align_vector.add(shift);
		}

		translation.add(align_vector);

		// Move light ray by the alignment shift.
		light_ray.Position.add(align_vector);

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
				float plane_dot_ray = view_frustum_rays[i].Direction.dotproduct(light_cuboid_polys[align_planes[p]].plane.n);
				if (plane_dot_ray < 0)
				{
					fvec3 per_plane_view;
					per_plane_view.crossproduct(light_cuboid_polys[align_planes[p]].plane.n, view_ray.Direction);
					fvec3 per_view_to_plane;
					per_view_to_plane.crossproduct(per_plane_view, view_ray.Direction);

					float tmp_mag = -plane_dot_ray / view_frustum_rays[i].Direction.dotproduct(per_view_to_plane);

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
		light_ray.Position.add(align_vector);
		translate_light_model(translation);

		// compute culling planes by rays as edges
		for (u32 i = 0; i < view_frustum_rays.size(); ++i)
		{
			fvec3 tmp_vector;
			tmp_vector.crossproduct(view_frustum_rays[i].Direction, light_ray.Direction);

			// check if the vectors are parallel
			if (fis_zero(tmp_vector.square_magnitude(), EPS))
				continue;

			Fplane tmp_plane;
			tmp_plane.build(view_frustum_rays[i].Position, tmp_vector);

			float sign = 0;
			if (check_cull_plane_valid(tmp_plane, sign, 5))
			{
				tmp_plane.n.mul(-sign);
				tmp_plane.d *= -sign;
				dest.push_back(tmp_plane);
			}
		}

		// compute culling planes by ray points pairs as edges
		if (clip_by_view_near && abs(view_ray.Direction.dotproduct(light_ray.Direction)) < 0.8)
		{
			fvec3 perp_light_view, perp_light_to_view;
			perp_light_view.crossproduct(view_ray.Direction, light_ray.Direction);
			perp_light_to_view.crossproduct(perp_light_view, light_ray.Direction);

			Fplane plane;
			plane.build(view_ray.Position, perp_light_to_view);

			float max_dist = -1000;
			for (u32 i = 0; i < view_frustum_rays.size(); ++i)
				max_dist = _max(plane.classify(view_frustum_rays[i].Position), max_dist);

			for (u32 i = 0; i < view_frustum_rays.size(); ++i)
			{
				fvec3 P = view_frustum_rays[i].Position;
				P.mad(view_frustum_rays[i].Direction, 5);

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
			VERIFY(light_cuboid_polys[i].plane.classify(light_ray.Position) > 0);
		}

		// Compute ray intersection with light model, this is needed to next cascade to start it's placement.
		for (u32 i = 0; i < view_frustum_rays.size(); ++i)
		{
			float min_dist = 2 * map_size;
			for (int p = 0; p < 4; ++p)
			{
				float dist;
				if ((light_cuboid_polys[p].plane.n.dotproduct(view_frustum_rays[i].Direction)) > -0.1)
					dist = map_size;
				else
					light_cuboid_polys[p].plane.intersectRayDist(view_frustum_rays[i].Position, view_frustum_rays[i].Direction, dist);

				if (dist > EPS_L && dist < min_dist)
					min_dist = dist;
			}

			view_frustum_rays[i].Position.mad(view_frustum_rays[i].Direction, min_dist);
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
			fvec3 tmp_pt = view_frustum_rays[j].Position;
			tmp_pt.mad(view_frustum_rays[j].Direction, mad_factor);
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

	void translate_light_model(fvec3 translate)
	{
		fmat4x4 trans_mat;
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
		fvec3 planeN;
		float planeD;
		float classify(fvec3& p)
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
	xr_vector<fvec3> points;
	xr_vector<_poly> polys;
	xr_vector<_edge> edges;

  public:
	void compute_planes()
	{
		for (int it = 0; it < int(polys.size()); it++)
		{
			_poly& P = polys[it];
			fvec3 t1, t2;
			t1.sub(points[P.points[0]], points[P.points[1]]);
			t2.sub(points[P.points[0]], points[P.points[2]]);
			P.planeN.crossproduct(t1, t2).normalize();
			P.planeD = -P.planeN.dotproduct(points[P.points[0]]);

			// verify
			if (_debug)
			{
				fvec3& p0 = points[P.points[0]];
				fvec3& p1 = points[P.points[1]];
				fvec3& p2 = points[P.points[2]];
				fvec3& p3 = points[P.points[3]];
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

	void compute_caster_model(xr_vector<Fplane>& dest, fvec3 direction)
	{
		CRenderTarget& T = *RenderImplementation.RenderTarget;

		// COG
		fvec3 cog = {0, 0, 0};
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
			fvec3 point;
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
fvec3 wform(fmat4x4& m, fvec3 const& v)
{
	fvec4 r;
	r.x = v.x * m._11 + v.y * m._21 + v.z * m._31 + m._41;
	r.y = v.x * m._12 + v.y * m._22 + v.z * m._32 + m._42;
	r.z = v.x * m._13 + v.y * m._23 + v.z * m._33 + m._43;
	r.w = v.x * m._14 + v.y * m._24 + v.z * m._34 + m._44;
	// VERIFY		(r.w>0.f);
	float invW = 1.0f / r.w;
	fvec3 r3 = {r.x * invW, r.y * invW, r.z * invW};
	return r3;
}

void CRender::init_cacades()
{
	u32 cascade_count = 3;
	m_sun_cascades.resize(cascade_count);

	m_sun_cascades[SE_SUN_NEAR].reset_chain = true;
	m_sun_cascades[SE_SUN_NEAR].size = ps_r_sun_near;
	m_sun_cascades[SE_SUN_MIDDLE].size = ps_r_sun_near * 3;
	m_sun_cascades[SE_SUN_FAR].size = ps_r_sun_far;

	// Инициализируем ОБА буфера
	m_sun_cascades_buffer[0].Init();
	m_sun_cascades_buffer[1].Init();

	// Сброс индексов
	m_sun_write_ix = 0;
	m_sun_read_ix = 0;
}

void CRender::prepare_sun_cascade(u32 cascade_ind, ShadowCascadeWorkItem& item)
{
    light* sun = (light*)Lights.sun_adapted._get();

    // Calculate view-frustum bounds in world space
    fmat4x4 ex_project, ex_full, ex_full_inverse;
    {
        ex_project = Engine.RenderView.Project;
        ex_full.mul(ex_project, Engine.RenderView.View);
        D3DXMatrixInverse((D3DXMATRIX*)&ex_full_inverse, 0, (D3DXMATRIX*)&ex_full);
    }

    // Local variables for calculation
    CFrustum cull_frustum;
    xr_vector<Fplane> cull_planes;
    fvec3 cull_COP;
    CSector* cull_sector;
    fmat4x4 cull_transform;

    {
        fmat4x4 fulltransform_inv = ex_full_inverse;
#ifdef _DEBUG
        typedef DumbConvexVolume<true> t_volume;
#else
        typedef DumbConvexVolume<false> t_volume;
#endif

        // Search for default sector (largest)
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
        fmat4x4 mdir_View, mdir_Project;
        fvec3 L_dir, L_up, L_right, L_pos;
        L_pos.set(sun->get_position());
        L_dir.set(sun->get_direction()).normalize();
        L_right.set(1, 0, 0);
        if (_abs(L_right.dotproduct(L_dir)) > .99f)
            L_right.set(0, 0, 1);
        L_up.crossproduct(L_dir, L_right).normalize();
        L_right.crossproduct(L_up, L_dir).normalize();
        mdir_View.build_camera_dir(L_pos, L_dir, L_up);

#ifdef _DEBUG
        typedef FixedConvexVolume<true> t_cuboid;
#else
        typedef FixedConvexVolume<false> t_cuboid;
#endif

        t_cuboid light_cuboid;
        {
            // Initialize rays for this cascade
            if (cascade_ind == 0 || m_sun_cascades[cascade_ind].reset_chain)
            {
                fvec3 near_p, edge_vec;
                for (int p = 0; p < 4; p++)
                {
                    near_p = wform(fulltransform_inv, corners[facetable[4][p]]);
                    edge_vec = wform(fulltransform_inv, corners[facetable[5][p]]);
                    edge_vec.sub(near_p);
                    edge_vec.normalize();

                    light_cuboid.view_frustum_rays.push_back(Sun::Ray(near_p, edge_vec));
                }
            }
            else
            {
                light_cuboid.view_frustum_rays = m_sun_cascades[cascade_ind].rays;
            }

            light_cuboid.view_ray.Position = Engine.RenderView.Position;
            light_cuboid.view_ray.Direction = Engine.RenderView.Direction;
            light_cuboid.light_ray.Position = L_pos;
            light_cuboid.light_ray.Direction = L_dir;
        }

        Fplane light_top_plane;
        light_top_plane.build_unit_normal(L_pos, L_dir);
        float dist = light_top_plane.classify(Engine.RenderView.Position);

        float map_size = m_sun_cascades[cascade_ind].size;
        D3DXMatrixOrthoOffCenterLH((D3DXMATRIX*)&mdir_Project,
                                    -map_size * 0.5f, map_size * 0.5f,
                                    -map_size * 0.5f, map_size * 0.5f,
                                    0.1, dist + map_size);

        float view_dim = float(RenderImplementation.o.smapsize);
        fmat4x4 m_viewport = { view_dim / 2.f, 0.0f,          0.0f, 0.0f,
                               0.0f,           -view_dim / 2.f, 0.0f, 0.0f,
                               0.0f,           0.0f,          1.0f, 0.0f,
                               view_dim / 2.f, view_dim / 2.f, 0.0f, 1.0f };
        fmat4x4 m_viewport_inv;
        D3DXMatrixInverse((D3DXMATRIX*)&m_viewport_inv, 0, (D3DXMATRIX*)&m_viewport);

        cull_transform.mul(mdir_Project, mdir_View);
        fmat4x4 cull_transform_inv;
        cull_transform_inv.invert(cull_transform);

        for (int p = 0; p < 8; p++)
        {
            fvec3 xf = wform(cull_transform_inv, corners[p]);
            light_cuboid.light_cuboid_points[p] = xf;
        }

        for (int plane = 0; plane < 4; plane++)
            for (int pt = 0; pt < 4; pt++)
            {
                int asd = facetable[plane][pt];
                light_cuboid.light_cuboid_polys[plane].points[pt] = asd;
            }

        fvec3 lightXZshift;
        light_cuboid.compute_caster_model_fixed(cull_planes,
                                                lightXZshift,
                                                m_sun_cascades[cascade_ind].size,
                                                m_sun_cascades[cascade_ind].reset_chain);

        if (cascade_ind < m_sun_cascades.size() - 1)
            m_sun_cascades[cascade_ind + 1].rays = light_cuboid.view_frustum_rays;

        fvec3 proj_view = Engine.RenderView.Direction;
        proj_view.y = 0;
        proj_view.normalize();

        fvec3 cam_shifted = L_pos;
        cam_shifted.add(lightXZshift);

        mdir_View.identity();
        mdir_View.build_camera_dir(cam_shifted, L_dir, L_up);
        cull_transform.identity();
        cull_transform.mul(mdir_Project, mdir_View);
        cull_transform_inv.invert(cull_transform);

        // Create frustum for query
        cull_frustum._clear();
        for (u32 p = 0; p < cull_planes.size(); p++)
            cull_frustum._add(cull_planes[p]);

        fvec3 cam_proj = Engine.RenderView.Position;
        const float align_aim_step_coef = 4.f;
        cam_proj.set(floorf(cam_proj.x / align_aim_step_coef) + align_aim_step_coef / 2,
                     floorf(cam_proj.y / align_aim_step_coef) + align_aim_step_coef / 2,
                     floorf(cam_proj.z / align_aim_step_coef) + align_aim_step_coef / 2);
        cam_proj.mul(align_aim_step_coef);
        fvec3 cam_pixel = wform(cull_transform, cam_proj);
        cam_pixel = wform(m_viewport, cam_pixel);
        fvec3 shift_proj = lightXZshift;
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
        fvec3 diff = cam_pixel;
        static float sign_test = -1.f;
        diff.mul(sign_test);
        fmat4x4 adjust;
        adjust.translate(diff);
        cull_transform.mulB_44(adjust);
    }

    item.cull_transform = cull_transform;
    item.cull_frustum = cull_frustum;
    item.cull_sector = cull_sector;
    item.cull_COP = cull_COP;

    m_sun_cascades[cascade_ind].transform = cull_transform;
}

void CRender::gather_scene_for_cascade(u32 cascade_ind, ShadowCascadeWorkItem& item)
{
	SceneTraversalContext local_ctx;
	local_ctx.frustum = &item.cull_frustum;
	local_ctx.is_hud_pass = FALSE;
	local_ctx.is_invisible_mode = FALSE;
	local_ctx.current_owner = nullptr;
	local_ctx.current_transform = &Fidentity;
	local_ctx.render_phase = CRender::PHASE_SHADOW_DEPTH;

	CurrentRenderContext::Scope tls_scope(item.packet, local_ctx);

	SceneGraph.BuildScene(item.cull_sector, &item.cull_frustum, item.cull_transform, item.cull_COP, TRUE, FALSE, item.packet);
}

void CRender::draw_sun_cascade(u32 cascade_ind, ShadowCascadeWorkItem& item)
{
	OPTICK_EVENT("Draw Cascade");

	light* sun = (light*)Lights.sun_adapted._get();

	sun->TransformContext.Sun.combine = item.cull_transform;
	sun->TransformContext.Sun.minX = 0;
	sun->TransformContext.Sun.maxX = RenderImplementation.o.smapsize;
	sun->TransformContext.Sun.minY = 0;
	sun->TransformContext.Sun.maxY = RenderImplementation.o.smapsize;

	HOM.Disable();
	set_active_phase(PHASE_SHADOW_DEPTH);

	bool bNormal = item.packet.queue_static[0].size() || item.packet.queue_dynamic[0].size();
	bool bSpecial = item.packet.queue_static[1].size() || item.packet.queue_dynamic[1].size() || item.packet.queue_transparent.size();

	if (bNormal || bSpecial)
	{
		render_shadow_map_sun(sun, cascade_ind);

		RenderBackend.set_transform_world(Fidentity);
		RenderBackend.set_transform_view(Fidentity);
		RenderBackend.set_transform_project(sun->TransformContext.Sun.combine);

		if (m_SunOccluder)
			m_SunOccluder->Render();

		SceneGraph.Render(item.packet, SceneGraphRenderType::Opaque);

		if (g_pGameLevel)
			g_pGameLevel->pHUD->Render_Actor_Shadow();

		if (ps_r_lighting_flags.test(RFLAG_SUN_DETAILS))
			Details->Render(DetailsRenderMode::DepthOnly, &m_sun_cascades[cascade_ind].transform, &item.cull_frustum);

		sun->TransformContext.Sun.transluent = FALSE;
	}

	set_light_accumulator();

	accumulate_sun(	cascade_ind, 
					m_sun_cascades[cascade_ind].transform, 
					m_sun_cascades[cascade_ind].transform );
}

void CRender::render_sun_cascades()
{
	PROFILE_FUNCTION();

	m_sun_write_ix = (m_sun_write_ix + 1) % 2;
	m_sun_read_ix = (m_sun_write_ix + 1) % 2;

	SunCascadeBuffer& writeBuffer = GetSunWriteBuffer();
	SunCascadeBuffer& readBuffer = GetSunReadBuffer();

	writeBuffer.Clear();

	{
		OPTICK_EVENT("Prepare Cascades");
		for (u32 i = 0; i < m_sun_cascades.size(); ++i)
			prepare_sun_cascade(i, *writeBuffer.items[i]);
	}

	{
		OPTICK_EVENT("Gather Cascades");
		HOM.Disable();
		gather_scene_for_cascade(SE_SUN_NEAR,   *writeBuffer.items[SE_SUN_NEAR]);
		gather_scene_for_cascade(SE_SUN_MIDDLE, *writeBuffer.items[SE_SUN_MIDDLE]);
		gather_scene_for_cascade(SE_SUN_FAR,    *writeBuffer.items[SE_SUN_FAR]);;
	}

	{
		OPTICK_EVENT("Draw Cascades Sequence");
		draw_sun_cascade(SE_SUN_NEAR,	*readBuffer.items[SE_SUN_NEAR]);
		draw_sun_cascade(SE_SUN_MIDDLE, *readBuffer.items[SE_SUN_MIDDLE]);
		draw_sun_cascade(SE_SUN_FAR,	*readBuffer.items[SE_SUN_FAR]);
	}

	RenderBackend.set_transform_world(Fidentity);
	RenderBackend.set_transform_view(Engine.RenderView.View);
	RenderBackend.set_transform_project(Engine.RenderView.Project);
}
