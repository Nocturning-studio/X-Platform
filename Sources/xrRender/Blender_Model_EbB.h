///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
#include "shader_configurator.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_Model_EbB : public IBlender
{
  public:
	string64 oT2_Name;	// name of secondary texture
	string64 oT2_transform; // transform for secondary texture
	xrP_BOOL oBlend;

  public:
	virtual LPCSTR getComment()
	{
		return "MODEL: env^base";
	}
	virtual BOOL canBeLMAPped()
	{
		return FALSE;
	}

	CBlender_Model_EbB()
	{
		description.CLS = B_MODEL_EbB;
		description.version = 0x1;
		strcpy(oT2_Name, "$null");
		strcpy(oT2_transform, "$null");
		oBlend.value = FALSE;
	}

	~CBlender_Model_EbB() = default;

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
		IBlender::Compile(C);

		if (oBlend.value)
		{
			// forward
			LPCSTR vsname = 0;
			LPCSTR psname = 0;
			switch (C.iElement)
			{
			case 0:
			case 1:
				vsname = psname = "model_env_lq";
				C.begin_Pass(vsname, psname, "main", "main", TRUE, TRUE, FALSE, TRUE, D3DBLEND_SRCALPHA,
							 D3DBLEND_INVSRCALPHA, TRUE, 0);
				C.set_Sampler("s_base", C.L_textures[0], false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, true);
				C.set_Sampler("s_env", oT2_Name, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
				C.end_Pass();
				break;
			}
		}
		else
		{
			// deferred
			switch (C.iElement)
			{
			case SE_NORMAL_HQ: // deffer
				configure_shader(C, true, "dynamic_mesh", "static_mesh", false);
				break;
			case SE_NORMAL_LQ: // deffer
				configure_shader(C, false, "dynamic_mesh", "static_mesh", false);
				break;
			case SE_SHADOW_DEPTH: // smap
				C.begin_Pass("shadow_depth_stage_dynamic_mesh", "shadow_depth_stage_static_mesh", "main", "main", FALSE,
							 TRUE, TRUE, FALSE);
				C.set_Sampler("s_base", C.L_textures[0]);
				C.end_Pass();
				break;
			case SE_DEPTH_PREPASS:
				C.begin_Pass("depth_prepass_stage_dynamic_mesh", "depth_prepass_stage_static_mesh", "main", "main",
							 FALSE, TRUE, TRUE, FALSE);
				C.set_Sampler("s_base", C.L_textures[0]);
				C.end_Pass();
				break;
			}
		}
	}
};
///////////////////////////////////////////////////////////////////////////////////
