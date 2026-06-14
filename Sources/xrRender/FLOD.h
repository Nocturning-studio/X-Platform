#pragma once
#include "../xrEngine/fhierrarhyvisual.h"

class FLOD : public FHierrarhyVisual
{
	typedef FHierrarhyVisual inherited;

  public:
	struct _vertex
	{
		fvec3 v;
		fvec2 t;
		u32 c_rgb_hemi; // rgb,hemi
		u8 c_sun;
	};
	struct _face
	{
		_vertex v[4];
		fvec3 N;
	};
	struct _hw
	{
		fvec3 p0;
		fvec3 p1;
		fvec3 n0;
		fvec3 n1;
		u32 sun_af;
		fvec2 t0;
		fvec2 t1;
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
