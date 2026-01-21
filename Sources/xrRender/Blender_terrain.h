///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
#include "shader_configurator.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_terrain : public IBlender
{
  public:
	string64 oT2_Name;	// name of secondary texture
	string64 oT2_transform; // transform for secondary texture
	string64 oR_Name;
	string64 oG_Name;
	string64 oB_Name;
	string64 oA_Name;
	string64 oRb_Name;
	string64 oGb_Name;
	string64 oBb_Name;
	string64 oAb_Name;
	string64 oRbx_Name;
	string64 oGbx_Name;
	string64 oBbx_Name;
	string64 oAbx_Name;

  public:
	virtual LPCSTR getComment()
	{
		return "LEVEL: Implicit**detail";
	}
	virtual BOOL canBeDetailed()
	{
		return TRUE;
	}
	virtual BOOL canBeLMAPped()
	{
		return TRUE;
	}

	void Save(IWriter& fs)
	{
		IBlender::Save(fs);
		xrPWRITE_MARKER(fs, "Detail map");
		xrPWRITE_PROP(fs, "Name", xrPID_TEXTURE, oT2_Name);
		xrPWRITE_PROP(fs, "Transform", xrPID_MATRIX, oT2_transform);
		xrPWRITE_PROP(fs, "R2-R", xrPID_TEXTURE, oR_Name);
		xrPWRITE_PROP(fs, "R2-G", xrPID_TEXTURE, oG_Name);
		xrPWRITE_PROP(fs, "R2-B", xrPID_TEXTURE, oB_Name);
		xrPWRITE_PROP(fs, "R2-A", xrPID_TEXTURE, oA_Name);
	}

	void Load(IReader& fs, u16 version)
	{
		IBlender::Load(fs, version);
		if (version < 3)
		{
			xrPREAD_MARKER(fs);
			xrPREAD_PROP(fs, xrPID_TEXTURE, oT2_Name);
			xrPREAD_PROP(fs, xrPID_MATRIX, oT2_transform);
		}
		else
		{
			xrPREAD_MARKER(fs);
			xrPREAD_PROP(fs, xrPID_TEXTURE, oT2_Name);
			xrPREAD_PROP(fs, xrPID_MATRIX, oT2_transform);
			xrPREAD_PROP(fs, xrPID_TEXTURE, oR_Name);
			xrPREAD_PROP(fs, xrPID_TEXTURE, oG_Name);
			xrPREAD_PROP(fs, xrPID_TEXTURE, oB_Name);
			xrPREAD_PROP(fs, xrPID_TEXTURE, oA_Name);
		}
	}

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		string256 mask;
		int BumpType = 0;
		extern u32 ps_r_material_quality;

		strconcat(sizeof(mask), mask, C.L_textures[0].c_str(), "_mask");

		switch (C.iElement)
		{
		case SE_NORMAL_HQ: // deffer
			if (ps_r_material_quality == 1)
				BumpType = 1; // normal
			else if (ps_r_material_quality == 2 || ps_r_material_quality == 3)
				BumpType = 2; // parallax
			else if (ps_r_material_quality == 4)
				BumpType = 3; // steep parallax

			C.set_Define("MATERIAL_QUALITY", (int)ps_r_material_quality, CBlender_Compile::ShaderScope::Pixel);
			C.set_Define(BumpType == 1, "USE_NORMAL_MAPPING", "1", CBlender_Compile::ShaderScope::Pixel);
			C.set_Define(BumpType == 2, "USE_PARALLAX_MAPPING", "1", CBlender_Compile::ShaderScope::Pixel);
			C.set_Define(BumpType == 3, "USE_PARALLAX_OCCLUSION_MAPPING", "1", CBlender_Compile::ShaderScope::Pixel);
			C.set_Define(true, "USE_SRGB_COLOR_CONVERTING", "1", CBlender_Compile::ShaderScope::Pixel);
			C.set_Define(true, "USE_DETAIL_SRGB_COLOR_CONVERTING", "1", CBlender_Compile::ShaderScope::Pixel);

			C.begin_Pass("gbuffer_stage_terrain", "gbuffer_stage_terrain", "main", "main", TRUE, TRUE, TRUE);

			C.set_Sampler("s_mask", mask);
			C.set_Sampler("s_base", C.L_textures[0], false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, true);
			//C.set_Sampler("s_cell_bomber", "vfx\\vfx_cell_bomber", false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, true);
			C.set_Sampler("s_dt_r", oR_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, true);
			C.set_Sampler("s_dt_g", oG_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, true);
			C.set_Sampler("s_dt_b", oB_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, true);
			C.set_Sampler("s_dt_a", oA_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, true);

			if (ps_r_material_quality > 0)
			{
				C.set_Sampler("s_dn_r", strconcat(sizeof(oRb_Name), oRb_Name, oR_Name, "_bump"), false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, false);
				C.set_Sampler("s_dn_g", strconcat(sizeof(oGb_Name), oGb_Name, oG_Name, "_bump"), false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, false);
				C.set_Sampler("s_dn_b", strconcat(sizeof(oBb_Name), oBb_Name, oB_Name, "_bump"), false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, false);
				C.set_Sampler("s_dn_a", strconcat(sizeof(oAb_Name), oAb_Name, oA_Name, "_bump"), false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, false);

				C.set_Sampler("s_dn_rX", strconcat(sizeof(oRbx_Name), oRbx_Name, oR_Name, "_bump#"), false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, false);
				C.set_Sampler("s_dn_gX", strconcat(sizeof(oGbx_Name), oGbx_Name, oG_Name, "_bump#"), false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, false);
				C.set_Sampler("s_dn_bX", strconcat(sizeof(oBbx_Name), oBbx_Name, oB_Name, "_bump#"), false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, false);
				C.set_Sampler("s_dn_aX", strconcat(sizeof(oAbx_Name), oAbx_Name, oA_Name, "_bump#"), false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, false);
			}
			C.end_Pass();
			break;
		case SE_NORMAL_LQ: // deffer
			C.set_Define(true, "USE_SRGB_COLOR_CONVERTING", "1");
			C.set_Define(true, "USE_DETAIL_SRGB_COLOR_CONVERTING", "1");
			C.begin_Pass("gbuffer_stage_terrain_lq", "gbuffer_stage_terrain_lq", "main", "main", TRUE, TRUE, TRUE);

			C.set_Sampler("s_lmap", C.L_textures[1], false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
			C.set_Sampler("s_base", C.L_textures[0], false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, true);
			C.set_Sampler("s_detail", oT2_Name);

			C.end_Pass();
			break;
		case SE_SHADOW_DEPTH: // smap
			C.begin_Pass("shadow_depth_stage_static_mesh", "shadow_depth_stage_static_mesh", "main", "main", FALSE,
						 TRUE, TRUE, FALSE);

			C.set_Sampler("s_base", C.L_textures[0], false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, true);

			C.end_Pass();
			break;
		case SE_DEPTH_PREPASS:
			C.begin_Pass("depth_prepass_stage_static_mesh", "depth_prepass_stage_static_mesh", "main", "main", FALSE,
						 TRUE, TRUE, FALSE);

			C.set_Sampler("s_base", C.L_textures[0], false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, true);

			C.end_Pass();
			break;
		}
	}

	CBlender_terrain()
	{
		description.CLS = B_TERRAIN;
		strcpy(oT2_Name, "$null");
		strcpy(oT2_transform, "$null");
		description.version = 3;
		strcpy(oR_Name, "detail\\detail_grnd_grass");	//"$null");
		strcpy(oG_Name, "detail\\detail_grnd_asphalt"); //"$null");
		strcpy(oB_Name, "detail\\detail_grnd_earth");	//"$null");
		strcpy(oA_Name, "detail\\detail_grnd_yantar");	//"$null");
	}

	virtual ~CBlender_terrain() = default;
};
///////////////////////////////////////////////////////////////////////////////////
