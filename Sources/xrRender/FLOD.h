#pragma once
#include "../xrEngine/fhierrarhyvisual.h"

class FLOD : public FHierrarhyVisual
{
	typedef FHierrarhyVisual inherited;

  public:
	struct _vertex
	{
		float3 v;
		float2 t;
		u32 c_rgb_hemi; // rgb,hemi
		u8 c_sun;
	};
	struct _face
	{
		_vertex v[4];
		float3 N;
	};
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

	ref_geom geom;
	_face facets[8];
	float lod_factor;

  public:
	virtual void Render(float LOD); // LOD - Level Of Detail  [0.0f - min, 1.0f - max], Ignored
	virtual void Load(LPCSTR N, IReader* data, u32 dwFlags);
	virtual void Copy(IRender_Visual* pFrom);
};
