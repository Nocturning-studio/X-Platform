#include "stdafx.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
#include "..\xrEngine\fvf.h"

CPortalTraverser::CPortalTraverser()
{
	i_marker = 0xffffffff;
}

#ifdef DEBUG
xr_vector<IRender_Sector*> dbg_sectors;
#endif

void CPortalTraverser::traverse(IRender_Sector* start, CFrustum& F, Fvector& vBase, Fmatrix& mTransform, u32 options)
{
	////OPTICK_EVENT("CPortalTraverser::traverse");

	Fmatrix m_viewport_01 = {1.f / 2.f, 0.0f, 0.0f, 0.0f, 0.0f, -1.f / 2.f,		   0.0f,
							 0.0f,		0.0f, 0.0f, 1.0f, 0.0f, 1.f / 2.f + 0 + 0, 1.f / 2.f + 0 + 0,
							 0.0f,		1.0f};

	if (options & VQ_FADE)
	{
		f_portals.clear();
		f_portals.reserve(16);
	}

	VERIFY(start);
	i_marker++;
	i_options = options;
	i_vBase = vBase;
	i_mTransform = mTransform;
	i_mTransform_01.mul(m_viewport_01, mTransform);
	i_start = (CSector*)start;
	r_sectors.clear();
	_scissor scissor;
	scissor.set(0, 0, 1, 1);
	scissor.depth = 0;
	i_start->traverse(F, scissor, *this);

	if (options & VQ_SCISSOR)
	{
		// dbg_sectors					= r_sectors;
		// merge scissor info
		for (u32 s = 0; s < r_sectors.size(); s++)
		{
			CSector* S = (CSector*)r_sectors[s];
			S->r_scissor_merged.invalidate();
			S->r_scissor_merged.depth = flt_max;
			for (u32 it = 0; it < S->r_scissors.size(); it++)
			{
				S->r_scissor_merged.merge(S->r_scissors[it]);
				if (S->r_scissors[it].depth < S->r_scissor_merged.depth)
					S->r_scissor_merged.depth = S->r_scissors[it].depth;
			}
		}
	}
}

void CPortalTraverser::fade_portal(CPortal* _p, float ScreenSpaceArea)
{
	////OPTICK_EVENT("CPortalTraverser::fade_portal");

	f_portals.push_back(mk_pair(_p, ScreenSpaceArea));
}

void CPortalTraverser::initialize()
{
	////OPTICK_EVENT("CPortalTraverser::initialize");

	f_shader.create("portal");
	f_geom.create(FVF::F_L, RenderBackend.Vertex.Buffer(), 0);
}

void CPortalTraverser::destroy()
{
	////OPTICK_EVENT("CPortalTraverser::destroy");

	f_geom.destroy();
	f_shader.destroy();
}

extern float r_ssaDISCARD;
extern float r_ssaLOD_A, r_ssaLOD_B;

void CPortalTraverser::fade_render()
{
	if (f_portals.empty())
		return;

	// Локальная копия позиции камеры для лямбды
	Fvector camera_pos = i_vBase;

	// Лямбда-предикат
	auto pred = [camera_pos](const std::pair<CPortal*, float>& _1, const std::pair<CPortal*, float>& _2) {
		float d1 = camera_pos.distance_to_sqr(_1.first->S.P);
		float d2 = camera_pos.distance_to_sqr(_2.first->S.P);
		return d2 > d1; // descending, back to front
	};

	// re-sort, back to front
	concurrency::parallel_sort(f_portals.begin(), f_portals.end(), pred);

	// calc poly-count
	u32 _pcount = 0;
	for (u32 _it = 0; _it < f_portals.size(); _it++)
		_pcount += f_portals[_it].first->getPoly().size() - 2;

	// fill buffers
	u32 _offset = 0;
	FVF::L* _v = (FVF::L*)RenderBackend.Vertex.Lock(_pcount * 3, f_geom.stride(), _offset);
	float ssaRange = r_ssaLOD_A - r_ssaLOD_B;
	Fvector _ambient_f = g_pGamePersistent->Environment().CurrentEnv->ambient;
	u32 _ambient = color_rgba_f(_ambient_f.x, _ambient_f.y, _ambient_f.z, 0);
	for (u32 _it = 0; _it < f_portals.size(); _it++)
	{
		std::pair<CPortal*, float>& fp = f_portals[_it];
		CPortal* _P = fp.first;
		float _ssa = fp.second;
		float ssaDiff = _ssa - r_ssaLOD_B;
		float ssaScale = ssaDiff / ssaRange;
		int iA = iFloor((1 - ssaScale) * 255.5f);
		clamp(iA, 0, 255);
		u32 _clr = subst_alpha(_ambient, u32(iA));

		// fill polys
		u32 _polys = _P->getPoly().size() - 2;
		for (u32 _pit = 0; _pit < _polys; _pit++)
		{
			_v->set(_P->getPoly()[0], _clr);
			_v++;
			_v->set(_P->getPoly()[_pit + 1], _clr);
			_v++;
			_v->set(_P->getPoly()[_pit + 2], _clr);
			_v++;
		}
	}
	RenderBackend.Vertex.Unlock(_pcount * 3, f_geom.stride());

	// render
	RenderBackend.set_transform_world(Fidentity);
	RenderBackend.set_Shader(f_shader);
	RenderBackend.set_Geometry(f_geom);
	RenderBackend.set_CullMode(CULL_DISABLE);
	RenderBackend.Render(D3DPT_TRIANGLELIST, _offset, _pcount);
	RenderBackend.set_CullMode(CULL_BACKFACE);

	// cleanup
	f_portals.clear();
}

#ifdef DEBUG
void CPortalTraverser::dbg_draw()
{
	////OPTICK_EVENT("CPortalTraverser::dbg_draw");

	RenderBackend.OnFrameEnd();
	RenderBackend.set_transform_world(Fidentity);
	RenderBackend.set_transform_view(Fidentity);
	RenderBackend.set_transform_project(Fidentity);
	for (u32 s = 0; s < dbg_sectors.size(); s++)
	{
		CSector* S = (CSector*)dbg_sectors[s];
		FVF::L verts[5];
		Fbox2 bb = S->r_scissor_merged;
		bb.min.x = bb.min.x * 2 - 1;
		bb.max.x = bb.max.x * 2 - 1;
		bb.min.y = (1 - bb.min.y) * 2 - 1;
		bb.max.y = (1 - bb.max.y) * 2 - 1;

		verts[0].set(bb.min.x, bb.min.y, EPS, 0xffffffff);
		verts[1].set(bb.max.x, bb.min.y, EPS, 0xffffffff);
		verts[2].set(bb.max.x, bb.max.y, EPS, 0xffffffff);
		verts[3].set(bb.min.x, bb.max.y, EPS, 0xffffffff);
		verts[4].set(bb.min.x, bb.min.y, EPS, 0xffffffff);
		RenderBackend.dbg_Draw(D3DPT_LINESTRIP, verts, 4);
	}
}
#endif
