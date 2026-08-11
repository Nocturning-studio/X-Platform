///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
#include "shader_configurator.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_skinned_mesh : public IBlender
{
  public:
	xrP_Integer oAREF;
	xrP_BOOL oBlend;

  public:
	virtual LPCSTR getComment()
	{
		return "LEVEL: deffer-model-flat";
	}

	virtual BOOL canBeDetailed()
	{
		return TRUE;
	}

	virtual BOOL canBeLMAPped()
	{
		return FALSE;
	}

	CBlender_skinned_mesh()
	{
		description.CLS = B_SKINNED_MESH;
		description.version = 1;
		oAREF.value = 32;
		oAREF.min = 0;
		oAREF.max = 255;
		oBlend.value = FALSE;
	}

	~CBlender_skinned_mesh() = default;

	void Save(IWriter& fs)
	{
		IBlender::Save(fs);
		xrPWRITE_PROP(fs, "Use alpha-channel", xrPID_BOOL, oBlend);
		xrPWRITE_PROP(fs, "Alpha ref", xrPID_INTEGER, oAREF);
	}
	void Load(IReader& fs, u16 version)
	{
		IBlender::Load(fs, version);

		switch (version)
		{
		case 0:
			oAREF.value = 32;
			oAREF.min = 0;
			oAREF.max = 255;
			oBlend.value = FALSE;
			break;
		case 1:
		default:
			xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
			xrPREAD_PROP(fs, xrPID_INTEGER, oAREF);
			break;
		}
	}

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		BOOL bForward = FALSE;
		if (oBlend.value && oAREF.value < 16)
			bForward = TRUE;
		if (oStrictSorting.value)
			bForward = TRUE;

		if (bForward)
		{
			// forward rendering
			LPCSTR vsname, psname;
			switch (C.iElement)
			{
			case 0:
			case 1:
				vsname = psname = "model_def_lq";
				C.begin_Pass(vsname, psname, "main", "main", TRUE, TRUE, FALSE, TRUE, D3DBLEND_SRCALPHA,D3DBLEND_INVSRCALPHA);
				C.set_Sampler("s_base", C.L_textures[0], false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, true);
				C.end_Pass();
				break;
			default:
				break;
			}
		}
		else
		{
			BOOL bAref = oBlend.value;
			// deferred rendering
			// codepath is the same, only the shaders differ
			switch (C.iElement)
			{
			case SE_NORMAL_HQ: // deffer
				configure_shader(C, true, "dynamic_mesh", "static_mesh", bAref);
				break;
			case SE_NORMAL_LQ: // deffer
				configure_shader(C, false, "dynamic_mesh", "static_mesh", bAref);
				break;
			case SE_SHADOW_DEPTH: // smap
				if (bAref)
				{
					C.begin_Pass("shadow_depth_stage_dynamic_mesh_alphatest", "shadow_depth_stage_static_mesh_alphatest", FALSE);
					C.set_Sampler("s_base", C.L_textures[0]);
					jitter(C);
					C.end_Pass();
					break;
				}
				else
				{
					C.begin_Pass("shadow_depth_stage_dynamic_mesh", "shadow_depth_stage_static_mesh", "main", "main",
								 FALSE, TRUE, TRUE, FALSE);
					C.set_Sampler("s_base", C.L_textures[0]);
					C.end_Pass();
					break;
				}
			case SE_DEPTH_PREPASS:
				if (bAref)
				{
					C.begin_Pass("depth_prepass_stage_dynamic_mesh_alphatest", "depth_prepass_stage_static_mesh_alphatest", "main", "main", FALSE, TRUE, TRUE, FALSE);
					C.set_Sampler("s_base", C.L_textures[0]);
					jitter(C);
					C.end_Pass();
					break;
				}
				else
				{
					C.begin_Pass("depth_prepass_stage_dynamic_mesh", "depth_prepass_stage_static_mesh", "main", "main", FALSE, TRUE, TRUE, FALSE);
					C.set_Sampler("s_base", C.L_textures[0]);
					C.end_Pass();
					break;
				}
			}
		}
	}
};
///////////////////////////////////////////////////////////////////////////////////
