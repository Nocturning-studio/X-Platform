///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
#include "shader_configurator.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_multiple_usage : public IBlender
{
  public:
	xrP_BOOL oBlend;
	xrP_BOOL oNotAnTree;

  public:
	virtual LPCSTR getComment()
	{
		return "LEVEL: trees/bushes";
	}
	virtual BOOL canBeLMAPped()
	{
		return FALSE;
	}
	virtual BOOL canBeDetailed()
	{
		return TRUE;
	}

	CBlender_multiple_usage()
	{
		description.CLS = B_MULTIPLE_USAGE;
		description.version = 1;
		oBlend.value = FALSE;
		oNotAnTree.value = FALSE;
	}

	~CBlender_multiple_usage()
	{
	}

	void Save(IWriter& fs)
	{
		IBlender::Save(fs);
		xrPWRITE_PROP(fs, "Alpha-blend", xrPID_BOOL, oBlend);
		xrPWRITE_PROP(fs, "Object LOD", xrPID_BOOL, oNotAnTree);
	}

	void Load(IReader& fs, u16 version)
	{
		IBlender::Load(fs, version);
		xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
		if (version >= 1)
		{
			xrPREAD_PROP(fs, xrPID_BOOL, oNotAnTree);
		}
	}

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		//*************** codepath is the same, only shaders differ
		LPCSTR tvs = "multiple_usage_object_animated";
		LPCSTR tvs_s = "shadow_depth_stage_multiple_usage_object_animated";

		if (oNotAnTree.value)
		{
			tvs = "multiple_usage_object";
			tvs_s = "shadow_depth_stage_multiple_usage_object";
		}

		string_path AlbedoTexture;
		strcpy_s(AlbedoTexture, sizeof(AlbedoTexture), *C.L_textures[0]);
		string_path Dummy = {0};

		bool bUseCustomWeight = false;
		string_path CustomWeightTexture;

		#pragma todo("NSDeathman to NSDeathman : Rewrite")
		bool bUseConfigurator = false;
		CInifile* MaterialConfiguration;
		string_path MaterialConfiguratorSearchPath;
		string_path MaterialConfiguratorRealPath;
		strcpy_s(MaterialConfiguratorSearchPath, sizeof(MaterialConfiguratorSearchPath), AlbedoTexture);
		strconcat(sizeof(MaterialConfiguratorSearchPath), MaterialConfiguratorSearchPath, MaterialConfiguratorSearchPath, "_material_configuration.ltx");
		FS.update_path(MaterialConfiguratorRealPath, "$game_textures$", MaterialConfiguratorSearchPath);

		if (FS.exist(MaterialConfiguratorRealPath))
		{
			MaterialConfiguration = CInifile::Create(MaterialConfiguratorRealPath);
			bUseConfigurator = true;
		}

		bool bNeedHashedAlphaTest = true;
		bool bUseAlphaTest = true;

		// Wind params
		bool bUseWind = false;
		bool bUseXAxisAsWeight = false;
		bool bUseYAxisAsWeight = false;
		bool bUseBothAxisAsWeight = false;
		bool bInvertWeightAxis = false;
		int WindTypeNum = 0;

		if (bUseConfigurator)
		{
			LPCSTR AlphaTestType = GetStringValueIfExist("material_configuration", "shadows_alpha_test_type",
														 "alpha_hashed", MaterialConfiguration);

			if (StringsIsSimilar(AlphaTestType, "alpha_clip"))
				bNeedHashedAlphaTest = false;

			if (StringsIsSimilar(AlphaTestType, "alpha_hashed"))
				bNeedHashedAlphaTest = true;

			if (StringsIsSimilar(AlphaTestType, "none"))
			{
				bNeedHashedAlphaTest = false;
				bUseAlphaTest = false;
			}

			bUseAlphaTest =
				GetBoolValueIfExist("material_configuration", "use_alpha_test", bUseAlphaTest, MaterialConfiguration);

			// Wind configuration
			bUseWind = GetBoolValueIfExist("wind_configuration", "use_wind", bUseWind, MaterialConfiguration);
			LPCSTR WindType = GetStringValueIfExist("wind_configuration", "wind_type", "none", MaterialConfiguration);

			if (StringsIsSimilar(WindType, "legacy"))
				WindTypeNum = 0;
			else if (StringsIsSimilar(WindType, "trunk"))
				WindTypeNum = 1;
			else if (StringsIsSimilar(WindType, "branchcard"))
				WindTypeNum = 2;
			else if (StringsIsSimilar(WindType, "leafcard"))
				WindTypeNum = 3;

			bUseXAxisAsWeight = GetBoolValueIfExist("wind_configuration", "use_x_axis_as_weight", bUseXAxisAsWeight, MaterialConfiguration);
			bUseYAxisAsWeight = GetBoolValueIfExist("wind_configuration", "use_y_axis_as_weight", bUseYAxisAsWeight, MaterialConfiguration);
			bUseBothAxisAsWeight = GetBoolValueIfExist("wind_configuration", "use_both_axis_as_weight", bUseBothAxisAsWeight, MaterialConfiguration);
			bInvertWeightAxis = GetBoolValueIfExist("wind_configuration", "invert_weight_axis", bInvertWeightAxis, MaterialConfiguration);
		}

		// Get weight texture
		strcpy_s(CustomWeightTexture, sizeof(CustomWeightTexture), AlbedoTexture);
		strconcat(sizeof(CustomWeightTexture), CustomWeightTexture, CustomWeightTexture, "_weight");
		if (FS.exist(Dummy, "$game_textures$", CustomWeightTexture, ".dds"))
			bUseCustomWeight = true;

		// Get opacity texture
		bool bUseCustomOpacity = false;
		string_path CustomOpacityTexture;
		strcpy_s(CustomOpacityTexture, sizeof(CustomOpacityTexture), AlbedoTexture);
		strconcat(sizeof(CustomOpacityTexture), CustomOpacityTexture, CustomOpacityTexture, "_opacity");
		if (FS.exist(Dummy, "$game_textures$", CustomOpacityTexture, ".dds"))
			bUseCustomOpacity = true;

		switch (C.iElement)
		{
		case SE_NORMAL_HQ: // deffer
			configure_shader(C, true, tvs, "static_mesh", oBlend.value);
			break;
		case SE_NORMAL_LQ: // deffer
			configure_shader(C, false, tvs, "static_mesh", oBlend.value);
			break;
		case SE_SHADOW_DEPTH: // smap-spot
		case SE_DEPTH_PREPASS:
			// C.set_Define(bUseCustomWeight, "USE_WEIGHT_MAP", "1");
			// C.set_Define(bUseCustomOpacity, "USE_CUSTOM_OPACITY", "1");

			// C.set_Define(bNeedHashedAlphaTest, "USE_HASHED_ALPHA_TEST", "1");

			// C.set_Define(bUseWind, "USE_WIND", "1");
			// C.set_Define(WindTypeNum == 0, "USE_LEGACY_WIND", "1");
			// C.set_Define(WindTypeNum == 1, "USE_TRUNK_WIND", "1");
			// C.set_Define(WindTypeNum == 2, "USE_BRANCHCARD_WIND", "1");
			// C.set_Define(WindTypeNum == 3, "USE_LEAFCARD_WIND", "1");
			// C.set_Define(bUseXAxisAsWeight, "USE_X_AXIS_AS_WEIGHT", "1");
			// C.set_Define(bUseYAxisAsWeight, "USE_Y_AXIS_AS_WEIGHT", "1");
			// C.set_Define(bUseBothAxisAsWeight, "USE_BOTH_AXIS_AS_WEIGHT", "1");
			// C.set_Define(bInvertWeightAxis, "INVERT_WEIGHT_AXIS", "1");

			// if (oBlend.value || bUseCustomOpacity || bUseAlphaTest)
			//{
			//	C.begin_Pass(tvs_s, "shadow_depth_stage_static_mesh_alphatest");

			//	C.set_Sampler("s_custom_opacity", CustomOpacityTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
			//D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
			//}
			// else
			//{
			//	C.begin_Pass(tvs_s, "shadow_depth_stage_static_mesh");
			//}
			// C.set_Sampler("s_base", C.L_textures[0]);

			// if (bUseCustomWeight)
			//	C.set_Sampler("s_custom_weight", CustomWeightTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
			//D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

			// jitter(C);
			configure_shader(C, true, tvs, "static_mesh", oBlend.value, true);

			C.end_Pass();
			break;
		}
	}
};
///////////////////////////////////////////////////////////////////////////////////
