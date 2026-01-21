#include "stdafx.h"
#pragma hdrstop

#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\igame_level.h"
#include "..\xrEngine\environment.h"
#include "..\xrEngine\fmesh.h"

#include "ftreevisual.h"

shared_str m_transform;
shared_str m_transform_v;
shared_str c_consts;
shared_str c_wave;
shared_str c_wind;
shared_str c_c_bias;
shared_str c_c_scale;
shared_str c_c_sun;

FTreeVisual::FTreeVisual(void)
{
}

FTreeVisual::~FTreeVisual(void)
{
}

void FTreeVisual::Release()
{
	IRender_Visual::Release();
}

void FTreeVisual::Load(const char* N, IReader* data, u32 dwFlags)
{
	IRender_Visual::Load(N, data, dwFlags);

	D3DVERTEXELEMENT9* vFormat = NULL;

	// read vertices
	R_ASSERT(data->find_chunk(OGF_GCONTAINER));
	{
		// verts
		u32 ID = data->r_u32();
		vBase = data->r_u32();
		vCount = data->r_u32();
		vFormat = RenderImplementation.getVB_Format(ID);

		VERIFY(NULL == p_rm_Vertices);

		p_rm_Vertices = RenderImplementation.getVB(ID);
		p_rm_Vertices->AddRef();

		// indices
		dwPrimitives = 0;
		ID = data->r_u32();
		iBase = data->r_u32();
		iCount = data->r_u32();
		dwPrimitives = iCount / 3;

		VERIFY(NULL == p_rm_Indices);
		p_rm_Indices = RenderImplementation.getIB(ID);
		p_rm_Indices->AddRef();
	}

	// load tree-def
	R_ASSERT(data->find_chunk(OGF_TREEDEF2));
	{
		data->r(&transform, sizeof(transform));
		data->r(&c_scale, sizeof(c_scale));
		c_scale.rgb.mul(.5f);
		c_scale.hemi *= .5f;
		c_scale.sun *= .5f;
		data->r(&c_bias, sizeof(c_bias));
		c_bias.rgb.mul(.5f);
		c_bias.hemi *= .5f;
		c_bias.sun *= .5f;
		// Msg				("hemi[%f / %f], sun[%f / %f]",c_scale.hemi,c_bias.hemi,c_scale.sun,c_bias.sun);
	}

	// Geom
	rm_geom.create(vFormat, p_rm_Vertices, p_rm_Indices);

	// Get constants
	m_transform = "m_transform";
	m_transform_v = "m_transform_v";
	c_consts = "consts";
	c_wave = "wave";
	c_wind = "wind";
	c_c_bias = "c_bias";
	c_c_scale = "c_scale";
	c_c_sun = "c_sun";
}

struct FTreeVisual_setup
{
	u32 dwFrame;
	float scale;
	Fvector4 wave;
	Fvector4 wind;

	FTreeVisual_setup()
	{
		dwFrame = 0;
	}

	void calculate()
	{
		dwFrame = Engine.TimeManager.GetFrameCount();

		CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;

		// Calc wind-vector3, scale
		float tm_rot = PI_MUL_2 * Engine.TimeManager.GetGlobalTime() / desc->m_fTreeRotation;
		wind.set(_sin(tm_rot), 0, _cos(tm_rot), 0);
		wind.normalize();
		wind.mul(desc->m_fTreeAmplitude); // dir1*amplitude
		scale = 1.f / float(FTreeVisual_quant);

		// setup constants
		wave.set(desc->m_fTreeWave.x, desc->m_fTreeWave.y, desc->m_fTreeWave.z,
				 Engine.TimeManager.GetGlobalTime() * desc->m_fTreeSpeed); // wave
		wave.div(PI_MUL_2);
	}
};

void FTreeVisual::Render(float LOD)
{
	static FTreeVisual_setup tvs;
	if (tvs.dwFrame != Engine.TimeManager.GetFrameCount())
		tvs.calculate();
	// setup constants
	Fmatrix transform_v;
	transform_v.mul_43(RenderBackend.get_transform_view(), transform);
	RenderBackend.set_Constant(m_transform_v, transform_v); // matrix
	float s = ps_r_Tree_SBC;
	RenderBackend.set_Constant(m_transform, transform);						// matrix
	RenderBackend.set_Constant(c_consts, tvs.scale, tvs.scale, 0, 0); // consts/scale
	RenderBackend.set_Constant(c_wave, tvs.wave);						// wave
	RenderBackend.set_Constant(c_wind, tvs.wind);						// wind
	s *= 1.3333f;
	RenderBackend.set_Constant(c_c_scale, s * c_scale.rgb.x, s * c_scale.rgb.y, s * c_scale.rgb.z, s * c_scale.hemi); // scale
	RenderBackend.set_Constant(c_c_bias, s * c_bias.rgb.x, s * c_bias.rgb.y, s * c_bias.rgb.z, s * c_bias.hemi);		// bias
	RenderBackend.set_Constant(c_c_sun, s * c_scale.sun, s * c_bias.sun, 0, 0); // sun
}

#define PCOPY(a) a = pFrom->a
void FTreeVisual::Copy(IRender_Visual* pSrc)
{
	IRender_Visual::Copy(pSrc);

	FTreeVisual* pFrom = dynamic_cast<FTreeVisual*>(pSrc);

	PCOPY(rm_geom);

	PCOPY(p_rm_Vertices);
	if (p_rm_Vertices)
		p_rm_Vertices->AddRef();

	PCOPY(vBase);
	PCOPY(vCount);

	PCOPY(p_rm_Indices);
	if (p_rm_Indices)
		p_rm_Indices->AddRef();

	PCOPY(iBase);
	PCOPY(iCount);
	PCOPY(dwPrimitives);

	PCOPY(transform);
	PCOPY(c_scale);
	PCOPY(c_bias);
}

//-----------------------------------------------------------------------------------
// Stripified Tree
//-----------------------------------------------------------------------------------
FTreeVisual_ST::FTreeVisual_ST(void)
{
}
FTreeVisual_ST::~FTreeVisual_ST(void)
{
}
void FTreeVisual_ST::Release()
{
	inherited::Release();
}
void FTreeVisual_ST::Load(const char* N, IReader* data, u32 dwFlags)
{
	inherited::Load(N, data, dwFlags);
}
void FTreeVisual_ST::Render(float LOD)
{
	inherited::Render(LOD);
	RenderBackend.set_Geometry(rm_geom);
	RenderBackend.Render(D3DPT_TRIANGLELIST, vBase, 0, vCount, iBase, dwPrimitives);
	RenderBackend.stat.r.s_flora.add(vCount);
}
void FTreeVisual_ST::Copy(IRender_Visual* pSrc)
{
	inherited::Copy(pSrc);
}

//-----------------------------------------------------------------------------------
// Progressive Tree
//-----------------------------------------------------------------------------------
FTreeVisual_PM::FTreeVisual_PM(void)
{
	pSWI = 0;
	last_lod = 0;
}
FTreeVisual_PM::~FTreeVisual_PM(void)
{
}
void FTreeVisual_PM::Release()
{
	inherited::Release();
}
void FTreeVisual_PM::Load(const char* N, IReader* data, u32 dwFlags)
{
	inherited::Load(N, data, dwFlags);
	R_ASSERT(data->find_chunk(OGF_SWICONTAINER));
	{
		u32 ID = data->r_u32();
		pSWI = RenderImplementation.getSWI(ID);
	}
}
void FTreeVisual_PM::Render(float LOD)
{
	inherited::Render(LOD);
	int lod_id = last_lod;
	if (LOD >= 0.f)
	{
		lod_id = iFloor((1.f - LOD) * float(pSWI->count - 1) + 0.5f);
		last_lod = lod_id;
	}
	VERIFY(lod_id >= 0 && lod_id < int(pSWI->count));
	FSlideWindow& SW = pSWI->sw[lod_id];
	RenderBackend.set_Geometry(rm_geom);
	RenderBackend.Render(D3DPT_TRIANGLELIST, vBase, 0, SW.num_verts, iBase + SW.offset, SW.num_tris);
	RenderBackend.stat.r.s_flora.add(SW.num_verts);
}
void FTreeVisual_PM::Copy(IRender_Visual* pSrc)
{
	inherited::Copy(pSrc);
	FTreeVisual_PM* pFrom = dynamic_cast<FTreeVisual_PM*>(pSrc);
	PCOPY(pSWI);
}
