#include "stdafx.h"
#include "../xrEngine/fmesh.h"
#include "flod.h"
#pragma warning(push)
#pragma warning(disable : 4995)
#include <ppl.h>
#pragma warning(pop)

struct _hw
{
	float3 p0;
	float3 p1;
	float3 n0;
	float3 n1;
	u32 sun_af;
	float2 t0;
	float2 t1;
	u32 rgbh0;
	u32 rgbh1;
};
static D3DVERTEXELEMENT9 dwDecl_FLOD[] = {
	{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},	// pos-0
	{0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 1},	// pos-1
	{0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},		// nrm-0
	{0, 36, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 1},		// nrm-1
	{0, 48, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},	// factors
	{0, 52, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},	// uv
	{0, 60, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},	// uv
	{0, 68, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2}, // rgbh-0
	{0, 72, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3}, // rgbh-1
	D3DDECL_END()};

void FLOD::Load(LPCSTR N, IReader* data, u32 dwFlags)
{
	inherited::Load(N, data, dwFlags);

	// LOD-def
	R_ASSERT(data->find_chunk(OGF_LODDEF2));
	for (int f = 0; f < 8; f++)
	{
		data->r(facets[f].v, sizeof(facets[f].v));
		_vertex* v = facets[f].v;

		float3 Normal, T;
		Normal.set(0, 0, 0);
		T.mknormal(v[0].v, v[1].v, v[2].v);
		Normal.add(T);
		T.mknormal(v[1].v, v[2].v, v[3].v);
		Normal.add(T);
		T.mknormal(v[2].v, v[3].v, v[0].v);
		Normal.add(T);
		T.mknormal(v[3].v, v[0].v, v[1].v);
		Normal.add(T);
		Normal.div(4.f);
		facets[f].N.normalize(Normal);
		facets[f].N.invert();
	}

	// VS
	geom.create(dwDecl_FLOD, RenderBackend.Vertex.Buffer(), RenderBackend.QuadIB);

	// lod correction
	float3 S;
	vis.box.getradius(S);
	float r = vis.sphere.R;
	concurrency::parallel_sort(&S.x, &S.x + 3);
	float a = S.y;
	float Sf = 4.f * (0.5f * (r * r * asinf(a / r) + a * _sqrt(r * r - a * a)));
	float Ss = M_PI * r * r;
	lod_factor = Sf / Ss;
}
void FLOD::Copy(IRender_Visual* pFrom)
{
	inherited::Copy(pFrom);

	FLOD* F = (FLOD*)pFrom;
	geom = F->geom;
	lod_factor = F->lod_factor;
	CopyMemory(facets, F->facets, sizeof(facets));
}
void FLOD::Render(float LOD)
{
	/*
	float3				Ldir;
	Ldir.sub			(vis.sphere.P,Engine.RenderView.Position);
	Ldir.normalize		();

	int					best_id		= 0;
	float				best_dot	= Ldir.dotproduct(facets[0].N);
	float				dot;

	dot	= Ldir.dotproduct	(facets[1].N); if (dot>best_dot) { best_id=1; best_dot=dot; }
	dot	= Ldir.dotproduct	(facets[2].N); if (dot>best_dot) { best_id=2; best_dot=dot; }
	dot	= Ldir.dotproduct	(facets[3].N); if (dot>best_dot) { best_id=3; best_dot=dot; }
	dot	= Ldir.dotproduct	(facets[4].N); if (dot>best_dot) { best_id=4; best_dot=dot; }
	dot	= Ldir.dotproduct	(facets[5].N); if (dot>best_dot) { best_id=5; best_dot=dot; }
	dot	= Ldir.dotproduct	(facets[6].N); if (dot>best_dot) { best_id=6; best_dot=dot; }
	dot	= Ldir.dotproduct	(facets[7].N); if (dot>best_dot) { best_id=7; best_dot=dot; }

#pragma todo("Smooth transitions")
#pragma todo("5-coloring")

	// Fill VB
	_face&		F					= facets[best_id];
	u32			vOffset				= 0;
	_hw*		V					= (_hw*) RenderBackend.Vertex.Lock(4,geom->vb_stride,vOffset);
	V[0].set	(F.v[0].v,F.N,F.v[0].c_rgb_hemi,F.v[0].t.x,F.v[0].t.y);
	V[1].set	(F.v[1].v,F.N,F.v[1].c_rgb_hemi,F.v[1].t.x,F.v[1].t.y);
	V[2].set	(F.v[2].v,F.N,F.v[2].c_rgb_hemi,F.v[2].t.x,F.v[2].t.y);
	V[3].set	(F.v[3].v,F.N,F.v[3].c_rgb_hemi,F.v[3].t.x,F.v[3].t.y);
	RenderBackend.Vertex.Unlock			(4,geom->vb_stride);

	// Draw IT
	RenderBackend.set_Geometry		(geom);
	RenderBackend.Render			(D3DPT_TRIANGLEFAN,vOffset,2);
	*/
}
