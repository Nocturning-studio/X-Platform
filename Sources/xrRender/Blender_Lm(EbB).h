///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
#include "shader_configurator.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_LmEbB : public IBlender
{
  public:
	string64 oT2_Name;	// name of secondary texture
	string64 oT2_transform; // transform for secondary texture
	xrP_BOOL oBlend;

  public:
	virtual LPCSTR getComment()
	{
		return "LEVEL: lmap*(env^base)";
	}
	virtual BOOL canBeLMAPped()
	{
		return TRUE;
	}

	CBlender_LmEbB()
	{
		description.CLS = B_LmEbB;
		description.version = 0x1;
		strcpy(oT2_Name, "$null");
		strcpy(oT2_transform, "$null");
		oBlend.value = FALSE;
	}

	~CBlender_LmEbB()
	{
	}

	void Save(IWriter& fs)
	{
		description.version = 0x1;
		IBlender::Save(fs);
		xrPWRITE_MARKER(fs, "Environment map");
		xrPWRITE_PROP(fs, "Name", xrPID_TEXTURE, oT2_Name);
		xrPWRITE_PROP(fs, "Transform", xrPID_MATRIX, oT2_transform);
		xrPWRITE_PROP(fs, "Alpha-Blend", xrPID_BOOL, oBlend);
	}

	void Load(IReader& fs, u16 version)
	{
		IBlender::Load(fs, version);
		xrPREAD_MARKER(fs);
		xrPREAD_PROP(fs, xrPID_TEXTURE, oT2_Name);
		xrPREAD_PROP(fs, xrPID_MATRIX, oT2_transform);
		if (version >= 0x1)
		{
			xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
		}
	}
	
	void Compile(CBlender_Compile& C)
	{
		if (oBlend.value)
			C.begin_Pass("lmapE", "lmapE", "main", "main", TRUE, TRUE, FALSE, TRUE, D3DBLEND_SRCALPHA,
						 D3DBLEND_INVSRCALPHA, TRUE, 0);
		else
			C.begin_Pass("lmapE", "lmapE", "main", "main", TRUE, TRUE, TRUE);

		C.set_Sampler("s_base", C.L_textures[0], false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,D3DTEXF_ANISOTROPIC, true);
		C.set_Sampler("s_lmap", C.L_textures[1]);
		C.set_Sampler_linear("s_hemi", *C.L_textures[2]);
		C.set_Sampler("s_env", oT2_Name, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
		C.end_Pass();
	}
};
///////////////////////////////////////////////////////////////////////////////////
