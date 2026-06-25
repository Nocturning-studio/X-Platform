////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Created: 14.10.2023
// Author: NSDeathman
// Nocturning studio for NS Platform X
// Updated: 25.11.2025 (ARMD, Packed Normal, Alpha Test Fixes)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "shader_configurator.h"
#include "../xrEngine/ResourceManager.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern u32 ps_r_material_quality;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Безопасные функции для работы со строками
namespace safe_string
{
inline void copy(char* dest, size_t dest_size, const char* src)
{
	if (dest_size > 0)
	{
		size_t len = strnlen(src, dest_size - 1);
		memcpy(dest, src, len);
		dest[len] = '\0';
	}
}

inline void concat(char* dest, size_t dest_size, const char* src1, const char* src2)
{
	if (dest_size == 0)
		return;

	size_t len1 = strnlen(src1, dest_size - 1);
	size_t copy_len = std::min(len1, dest_size - 1);
	memcpy(dest, src1, copy_len);
	dest[copy_len] = '\0';

	if (copy_len < dest_size - 1)
	{
		size_t remaining = dest_size - copy_len - 1;
		size_t len2 = strnlen(src2, remaining);
		memcpy(dest + copy_len, src2, len2);
		dest[copy_len + len2] = '\0';
	}
}

inline void concat3(char* dest, size_t dest_size, const char* src1, const char* src2, const char* src3)
{
	if (dest_size == 0)
		return;

	// Первая часть
	size_t len1 = strnlen(src1, dest_size - 1);
	size_t copy_len = std::min(len1, dest_size - 1);
	memcpy(dest, src1, copy_len);
	dest[copy_len] = '\0';

	// Вторая часть
	if (copy_len < dest_size - 1)
	{
		size_t remaining = dest_size - copy_len - 1;
		size_t len2 = strnlen(src2, remaining);
		memcpy(dest + copy_len, src2, len2);
		copy_len += len2;
		dest[copy_len] = '\0';
	}

	// Третья часть
	if (copy_len < dest_size - 1)
	{
		size_t remaining = dest_size - copy_len - 1;
		size_t len3 = strnlen(src3, remaining);
		memcpy(dest + copy_len, src3, len3);
		dest[copy_len + len3] = '\0';
	}
}
} // namespace safe_string

bool ConcatAndFindTexture(string_path& ResultPath, string_path AlbedoPath, LPCSTR TexturePrefix)
{
	string_path DummyPath = {0};
	bool SearchResult = false;

	safe_string::copy(ResultPath, sizeof(ResultPath), AlbedoPath);
	safe_string::concat(ResultPath, sizeof(ResultPath), ResultPath, TexturePrefix);

	if (FS.exist(DummyPath, "$game_textures$", ResultPath, ".dds"))
		SearchResult = true;

	return SearchResult;
}

bool ConcatAndFindLevelTexture(string_path& ResultPath, string_path AlbedoPath, LPCSTR TexturePrefix)
{
	string_path DummyPath = {0};
	bool SearchResult = false;

	safe_string::copy(ResultPath, sizeof(ResultPath), AlbedoPath);
	safe_string::concat(ResultPath, sizeof(ResultPath), ResultPath, TexturePrefix);

	if (FS.exist(DummyPath, "$level$", ResultPath, ".dds"))
		SearchResult = true;

	return SearchResult;
}

bool FindTexture(string_path& ResultPath, LPCSTR TexturePath)
{
	string_path DummyPath = {0};
	bool SearchResult = false;

	safe_string::copy(ResultPath, sizeof(ResultPath), TexturePath);

	if (FS.exist(DummyPath, "$game_textures$", ResultPath, ".dds"))
		SearchResult = true;

	return SearchResult;
}

float GetFloatValueIfExist(LPCSTR section_name, LPCSTR line_name, float default_value, CInifile* config)
{
	if (config->line_exist(section_name, line_name))
		return config->r_float(section_name, line_name);
	else
		return default_value;
}

fvec3 GetRGBColorValueIfExist(LPCSTR section_name, LPCSTR line_name, fvec3 default_value, CInifile* config)
{
	if (config->line_exist(section_name, line_name))
		return config->r_fvector3(section_name, line_name);
	else
		return default_value;
}

fvec4 GetRGBAColorValueIfExist(LPCSTR section_name, LPCSTR line_name, fvec4 default_value, CInifile* config)
{
	if (config->line_exist(section_name, line_name))
		return config->r_fvector4(section_name, line_name);
	else
		return default_value;
}

LPCSTR GetStringValueIfExist(LPCSTR section_name, LPCSTR line_name, LPCSTR default_value, CInifile* config)
{
	if (config->line_exist(section_name, line_name))
		return config->r_string(section_name, line_name);
	else
		return default_value;
}

bool GetBoolValueIfExist(LPCSTR section_name, LPCSTR line_name, bool default_state, CInifile* config)
{
	if (config->line_exist(section_name, line_name))
		return config->r_bool(section_name, line_name);
	else
		return default_state;
}

bool LineIsExist(LPCSTR section_name, LPCSTR line_name, CInifile* config)
{
	return config->line_exist(section_name, line_name);
}

bool StringsIsSimilar(LPCSTR x, LPCSTR y)
{
	bool result = false;
	if (0 == xr_strcmp(x, y))
		result = true;
	return result;
}

bool CheckAndApplyManualTexturePath(LPCSTR section_name, LPCSTR line_name, string_path& ResultPath, CInifile* config,
									string_path AlbedoBaseName)
{
	// Проверяем наличие строки в конфиге
	if (!LineIsExist(section_name, line_name, config))
		return false;

	// Получаем значение пути
	LPCSTR Path = GetStringValueIfExist(section_name, line_name, "path_is_empty", config);

	const char* Token = "$albedo_path$";

	// Проверяем, начинается ли путь с макроса
	if (strstr(Path, Token) == Path)
	{
		// Вычисляем суффикс (всё, что идет после токена)
		LPCSTR Suffix = Path + strlen(Token);

		// Используем базовое имя альбедо + суффикс
		return ConcatAndFindTexture(ResultPath, AlbedoBaseName, Suffix);
	}
	else
	{
		// Стандартное поведение (полный путь)
		return FindTexture(ResultPath, Path);
	}
}

// Вспомогательная функция для установки дефайнов каналов
void DefineCustomChannel(CBlender_Compile& C, LPCSTR component_name, LPCSTR channel_suffix)
{
	string_path define_name;

	if (StringsIsSimilar(component_name, "ao"))
		sprintf(define_name, "C_MAT_%s_IS_AO", channel_suffix);
	else if (StringsIsSimilar(component_name, "roughness"))
		sprintf(define_name, "C_MAT_%s_IS_ROUGHNESS", channel_suffix);
	else if (StringsIsSimilar(component_name, "gloss") || StringsIsSimilar(component_name, "glossiness"))
		sprintf(define_name, "C_MAT_%s_IS_GLOSS", channel_suffix);
	else if (StringsIsSimilar(component_name, "metallic"))
		sprintf(define_name, "C_MAT_%s_IS_METALLIC", channel_suffix);
	else if (StringsIsSimilar(component_name, "emission"))
		sprintf(define_name, "C_MAT_%s_IS_EMISSION", channel_suffix);
	else if (StringsIsSimilar(component_name, "height") || StringsIsSimilar(component_name, "displacement"))
		sprintf(define_name, "C_MAT_%s_IS_HEIGHT", channel_suffix);
	else if (StringsIsSimilar(component_name, "cavity"))
		sprintf(define_name, "C_MAT_%s_IS_CAVITY", channel_suffix);
	else if (StringsIsSimilar(component_name, "subsurface"))
		sprintf(define_name, "C_MAT_%s_IS_SUBSURFACE", channel_suffix);
	else if (StringsIsSimilar(component_name, "specular_tint"))
		sprintf(define_name, "C_MAT_%s_IS_SPECULAR_TINT", channel_suffix);
	else if (StringsIsSimilar(component_name, "opacity"))
		sprintf(define_name, "C_MAT_%s_IS_OPACITY", channel_suffix);
	else
		return;

	C.set_Define(define_name, "1", CBlender_Compile::ShaderScope::Pixel);
}

bool CheckAndApplyCustomMaterialPath(CInifile* config, LPCSTR section, LPCSTR line, string_path& ResultPath,
									 LPCSTR AlbedoBaseName)
{
	if (!config->line_exist(section, line))
		return false;

	LPCSTR ConfigValue = config->r_string(section, line);
	const char* Token = "$albedo_path$";

	if (strstr(ConfigValue, Token) == ConfigValue)
	{
		LPCSTR Suffix = ConfigValue + strlen(Token);
		string_path CleanAlbedo;
		safe_string::copy(CleanAlbedo, sizeof(CleanAlbedo), AlbedoBaseName);
		return ConcatAndFindTexture(ResultPath, CleanAlbedo, Suffix);
	}
	else
	{
		return FindTexture(ResultPath, ConfigValue);
	}
}

void SetupCustomMaterialNormal(CBlender_Compile& C, LPCSTR channel_setup)
{
	size_t len = strlen(channel_setup);
	if (len < 2)
		return;

	C.set_Define("C_MAT_HAS_CUSTOM_NORMAL", "1", CBlender_Compile::ShaderScope::Pixel);

	auto set_component_source = [&](char channel_char, const char* component_name) {
		char def_name[64];
		char ch = tolower(channel_char);
		const char* source_suffix = "R";

		if (ch == 'r')
			source_suffix = "R";
		else if (ch == 'g')
			source_suffix = "G";
		else if (ch == 'b')
			source_suffix = "B";
		else if (ch == 'a')
			source_suffix = "A";

		sprintf(def_name, "C_MAT_NORM_%s_IS_%s", component_name, source_suffix);
		C.set_Define(def_name, "1");
	};

	set_component_source(channel_setup[0], "X");
	set_component_source(channel_setup[1], "Y");

	if (len >= 3)
		set_component_source(channel_setup[2], "Z");
	else
		C.set_Define("C_MAT_NORM_RECONSTRUCT_Z", "1", CBlender_Compile::ShaderScope::Pixel);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void configure_shader(CBlender_Compile& C, bool bIsHightQualityGeometry, LPCSTR VertexShaderName,
					  LPCSTR PixelShaderName, BOOL bUseAlpha, BOOL bUseDepthOnly)
{
	C.set_Define("MATERIAL_QUALITY", (int)ps_r_material_quality, CBlender_Compile::ShaderScope::Pixel);

	// Output shader names
	string_path NewPixelShaderName = { 0 };
	string_path NewVertexShaderName = {0};

	// Base part of material
	string_path AlbedoTexture = {0};
	string_path BumpTexture = {0};
	string_path BumpCorrectionTexture = {0};

	// Detail part of material
	string_path DetailAlbedoTexture = {0};
	string_path DetailBumpTexture = {0};
	string_path DetailBumpCorrectionTexture = {0};

	// Other textures
	string_path HemisphereLightMapTexture = {0};
	string_path LightMapTexture = {0};

	string_path Dummy = {0};

	safe_string::copy(AlbedoTexture, sizeof(AlbedoTexture), *C.L_textures[0]);

	// Add extension to texture and check for null
	Engine.ResourceManager->fix_texture_name(AlbedoTexture);

	bool DisablePBR = ps_r_shading_mode == SHADING_MODE_LEASHED;
	bool UseAlbedoOnly = bUseDepthOnly;
	bool bUseLightMap = false;

	// Check detail existing and allowing
	bool bDetailTextureExist = C.detail_texture;
	bool bDetailTextureAllowed = C.bDetail;
	bool bUseDetail = bDetailTextureExist && bDetailTextureAllowed;
	bool bUseDetailBump = false;

	bool bUseConfigurator = false;
	CInifile* MaterialConfiguration;
	string_path MaterialConfiguratorSearchPath = {0};
	string_path MaterialConfiguratorRealPath = {0};
	safe_string::copy(MaterialConfiguratorSearchPath, sizeof(MaterialConfiguratorSearchPath), AlbedoTexture);
	safe_string::concat(MaterialConfiguratorSearchPath, sizeof(MaterialConfiguratorSearchPath),
						MaterialConfiguratorSearchPath, ".ltx");
	FS.update_path(MaterialConfiguratorRealPath, "$game_textures$", MaterialConfiguratorSearchPath);

	if (FS.exist(MaterialConfiguratorRealPath))
	{
		MaterialConfiguration = CInifile::Create(MaterialConfiguratorRealPath);
		bUseConfigurator = true;
	}

	// Alpha test params
	bool bNeedHashedAlphaTest = true;
	bool bUseAlphaTest = bUseAlpha;

	// [ИСПРАВЛЕНИЕ] Читаем настройки альфа-теста ДО установки define
	if (bUseConfigurator)
	{
		// Сначала читаем сам флаг использования
		bUseAlphaTest =
			GetBoolValueIfExist("material_configuration", "use_alpha_test", bUseAlpha, MaterialConfiguration);

		LPCSTR AlphaTestType =
			GetStringValueIfExist("material_configuration", "alpha_test_type", "alpha_hashed", MaterialConfiguration);

		if (StringsIsSimilar(AlphaTestType, "alpha_clip"))
			bNeedHashedAlphaTest = false;
		else if (StringsIsSimilar(AlphaTestType, "none"))
			bUseAlphaTest = false;
	}

	// Get opacity texture
	bool bUseOpacity = false;
	string_path OpacityTexture = {0};
	bUseOpacity = ConcatAndFindTexture(OpacityTexture, AlbedoTexture, "_opacity");
	C.set_Define(bUseOpacity, "USE_CUSTOM_OPACITY", "1");

	// Create shader with alpha testing if need
	C.set_Define(bUseAlphaTest || bUseOpacity, "USE_ALPHA_TEST", "1", CBlender_Compile::ShaderScope::Pixel);
	C.set_Define(bNeedHashedAlphaTest, "USE_HASHED_ALPHA_TEST", "1", CBlender_Compile::ShaderScope::Pixel);

	bool bUseBump = false;
	bool bBumpPresent = false;
	bool bUseBumpDecompression = false;

	if (!UseAlbedoOnly)
	{
		// Color space params
		bool bIsSrgbAlbedo = true;
		if (bUseConfigurator)
		{
			LPCSTR AlbedoColorSpace =
				GetStringValueIfExist("albedo_configuration", "color_space", "srgb", MaterialConfiguration);

			if (StringsIsSimilar(AlbedoColorSpace, "linear"))
				bIsSrgbAlbedo = false;
		}

		C.set_Define(bIsSrgbAlbedo, "USE_SRGB_COLOR_CONVERTING", "1", CBlender_Compile::ShaderScope::Pixel);

		// Normal map params
		bool bIsOpenGLNormal = false;

		// Check bump existing
		if (bUseConfigurator)
		{
			bUseBump = GetBoolValueIfExist("material_configuration", "use_bump", bUseBump, MaterialConfiguration);

			if (bUseBump)
			{
				bBumpPresent = CheckAndApplyManualTexturePath("material_configuration", "bump_path", BumpTexture,
															  MaterialConfiguration, AlbedoTexture);
				bUseBumpDecompression =
					CheckAndApplyManualTexturePath("material_configuration", "bumpX_path", BumpCorrectionTexture,
												   MaterialConfiguration, AlbedoTexture);
				if (bUseBumpDecompression)
					C.set_Define("USE_BUMP_DECOMPRESSION", "1", CBlender_Compile::ShaderScope::Pixel);
			}
		}

		ref_texture refAlbedoTexture;
		refAlbedoTexture.create(AlbedoTexture);
		if (!bUseBump)
			bUseBump = refAlbedoTexture.bump_exist();

		if (bUseConfigurator)
		{
			// Check for details using
			bUseDetail =
				GetBoolValueIfExist("material_configuration", "use_detail_map", bUseDetail, MaterialConfiguration);
			bIsOpenGLNormal =
				GetBoolValueIfExist("normal_configuration", "is_opengl_type", bIsOpenGLNormal, MaterialConfiguration);
		}

		// Get bump map texture
		if (!bBumpPresent)
		{
			if (bUseBump)
			{
				safe_string::copy(BumpTexture, sizeof(BumpTexture), refAlbedoTexture.bump_get().c_str());
			}
			else
			{
				bUseBump = ConcatAndFindTexture(BumpTexture, AlbedoTexture, "_bump");
			}
		}

		// Get bump decompression map
		if (bUseBump && !bUseBumpDecompression)
		{
			safe_string::copy(BumpCorrectionTexture, sizeof(BumpCorrectionTexture), BumpTexture);
			safe_string::concat(BumpCorrectionTexture, sizeof(BumpCorrectionTexture), BumpCorrectionTexture, "#");

			if (FS.exist(Dummy, "$game_textures$", BumpCorrectionTexture, ".dds") && (ps_r_material_quality > 1))
				C.set_Define("USE_BUMP_DECOMPRESSION", "1", CBlender_Compile::ShaderScope::Pixel);
		}

		C.set_Define(bUseBump, "USE_BUMP", "1", CBlender_Compile::ShaderScope::Pixel);
		C.set_Define(bIsOpenGLNormal, "IS_OPENGL_NORMAL", "1", CBlender_Compile::ShaderScope::Pixel);
	}

	// Wind params
	bool bUseWind = false;
	bool bUseXAxisAsWeight = false;
	bool bUseYAxisAsWeight = false;
	bool bUseBothAxisAsWeight = false;
	bool bInvertWeightAxis = false;
	int WindTypeNum = 1;
	if (bUseConfigurator)
	{
		// Wind configuration
		bUseWind = GetBoolValueIfExist("wind_configuration", "use_wind", bUseWind, MaterialConfiguration);
		LPCSTR WindType = GetStringValueIfExist("wind_configuration", "wind_type", "trunk", MaterialConfiguration);

		if (StringsIsSimilar(WindType, "legacy"))
			WindTypeNum = 0;
		else if (StringsIsSimilar(WindType, "trunk"))
			WindTypeNum = 1;
		else if (StringsIsSimilar(WindType, "branchcard"))
			WindTypeNum = 2;
		else if (StringsIsSimilar(WindType, "leafcard"))
			WindTypeNum = 3;

		bUseXAxisAsWeight =
			GetBoolValueIfExist("wind_configuration", "use_x_axis_as_weight", bUseXAxisAsWeight, MaterialConfiguration);
		bUseYAxisAsWeight =
			GetBoolValueIfExist("wind_configuration", "use_y_axis_as_weight", bUseYAxisAsWeight, MaterialConfiguration);
		bUseBothAxisAsWeight = GetBoolValueIfExist("wind_configuration", "use_both_axis_as_weight",
												   bUseBothAxisAsWeight, MaterialConfiguration);
		bInvertWeightAxis =
			GetBoolValueIfExist("wind_configuration", "invert_weight_axis", bInvertWeightAxis, MaterialConfiguration);
	}
	C.set_Define(bUseWind, "USE_WIND", "1", CBlender_Compile::ShaderScope::Vertex);
	C.set_Define(WindTypeNum == 0, "USE_LEGACY_WIND", "1", CBlender_Compile::ShaderScope::Vertex);
	C.set_Define(WindTypeNum == 1, "USE_TRUNK_WIND", "1", CBlender_Compile::ShaderScope::Vertex);
	C.set_Define(WindTypeNum == 2, "USE_BRANCHCARD_WIND", "1", CBlender_Compile::ShaderScope::Vertex);
	C.set_Define(WindTypeNum == 3, "USE_LEAFCARD_WIND", "1", CBlender_Compile::ShaderScope::Vertex);
	C.set_Define(bUseXAxisAsWeight, "USE_X_AXIS_AS_WEIGHT", "1", CBlender_Compile::ShaderScope::Vertex);
	C.set_Define(bUseYAxisAsWeight, "USE_Y_AXIS_AS_WEIGHT", "1", CBlender_Compile::ShaderScope::Vertex);
	C.set_Define(bUseBothAxisAsWeight, "USE_BOTH_AXIS_AS_WEIGHT", "1", CBlender_Compile::ShaderScope::Vertex);
	C.set_Define(bInvertWeightAxis, "INVERT_WEIGHT_AXIS", "1", CBlender_Compile::ShaderScope::Vertex);

	safe_string::copy(AlbedoTexture, sizeof(AlbedoTexture), *C.L_textures[0]);

	// Get weight texture
	bool bUseCustomWeight = false;
	string_path CustomWeightTexture = {0};

	if (bUseConfigurator)
	{
		bUseCustomWeight = CheckAndApplyManualTexturePath("wind_configuration", "weight_path", CustomWeightTexture,
														  MaterialConfiguration, AlbedoTexture);
	}

	if (!bUseCustomWeight)
		bUseCustomWeight = ConcatAndFindTexture(CustomWeightTexture, AlbedoTexture, "_weight");

	C.set_Define(bUseCustomWeight, "USE_WEIGHT_MAP", "1", CBlender_Compile::ShaderScope::Vertex);

	// Get AO-Roughness-Metallic texture
	bool bUseARMMap = false;
	string_path ARMTexture = {0};

	// Get ARMD (AO-Roughness-Metallic-Displacement)
	bool bUseARMDMap = false;
	string_path ARMDTexture = {0};

	// Get Empty-Roughness-Metallic texture
	bool bUseERMMap = false;
	string_path ERMTexture = {0};

	// Get normal texture
	bool bUseCustomNormal = false;
	string_path CustomNormalTexture = {0};

	// Get packed normal texture
	bool bUsePackedNormal = false;
	string_path PackedNormalTexture = {0};

	// Get subsurface_power power texture
	bool bUseCustomSubsurfacePower = false;
	string_path CustomSubsurfacePowerTexture = {0};

	// Get BakedAO texture
	bool bUseBakedAO = false;
	string_path BakedAOTexture = {0};

	// Get emission power texture
	bool bUseCustomEmission = false;
	string_path CustomEmissionTexture = {0};

	// Get roughness texture
	bool bUseCustomRoughness = false;
	string_path CustomRoughnessTexture = {0};

	// Get gloss texture
	bool bUseCustomGloss = false;
	string_path CustomGlossTexture = {0};

	// Get metallic texture
	bool bUseCustomMetallic = false;
	string_path CustomMetallicTexture = {0};

	// Get cavity texture
	bool bUseCustomCavity = false;
	string_path CustomCavityTexture = {0};

	// Get Specular Tint texture
	bool bUseCustomSpecularTint = false;
	string_path CustomSpecularTintTexture = {0};

	// Get displacement texture
	bool bUseCustomDisplacement = false;
	string_path CustomDisplacementTexture = {0};

	// Get sheen intensity texture
	bool bUseCustomSheenIntensity = false;
	string_path CustomSheenIntensityTexture = {0};

	// Get sheen roughness texture
	bool bUseCustomSheenRoughness = false;
	string_path CustomSheenRoughnessTexture = {0};

	// Get coat intensity texture
	bool bUseCustomCoatIntensity = false;
	string_path CustomCoatIntensityTexture = {0};

	// Get coat roughness texture
	bool bUseCustomCoatRoughness = false;
	string_path CustomCoatRoughnessTexture = {0};

	// Custom Packed Material Logic
	bool bUseCustomMaterial = false;
	string_path CustomMaterialTexture = {0};

	if (!UseAlbedoOnly)
	{
		// Get detail texture
		if (bUseDetail)
		{
			// Check do we need to use custom shader
			bool bIsSrgbDetail = true;
			if (bUseConfigurator)
			{
				LPCSTR AlbedoColorSpace =
					GetStringValueIfExist("detail_albedo_configuration", "color_space", "srgb", MaterialConfiguration);

				if (StringsIsSimilar(AlbedoColorSpace, "linear"))
					bIsSrgbDetail = false;
			}

			C.set_Define(bIsSrgbDetail, "USE_DETAIL_SRGB_COLOR_CONVERTING", "1", CBlender_Compile::ShaderScope::Pixel);

			safe_string::copy(DetailAlbedoTexture, sizeof(DetailAlbedoTexture), C.detail_texture);

			// Get bump for detail texture
			bUseDetailBump = ConcatAndFindTexture(DetailBumpTexture, DetailAlbedoTexture, "_bump");

			if (bUseDetailBump)
			{
				// Get bump decompression map for detail texture
				safe_string::copy(DetailBumpCorrectionTexture, sizeof(DetailBumpCorrectionTexture), DetailBumpTexture);
				safe_string::concat(DetailBumpCorrectionTexture, sizeof(DetailBumpCorrectionTexture),
									DetailBumpCorrectionTexture, "#");
			}
		}

		// Check lightmap existing
		if (C.L_textures.size() >= 3)
		{
			pcstr HemisphereLightMapTextureName = C.L_textures[2].c_str();
			pcstr LightMapTextureName = C.L_textures[1].c_str();

			if ((HemisphereLightMapTextureName[0] == 'l' && HemisphereLightMapTextureName[1] == 'm' &&
				 HemisphereLightMapTextureName[2] == 'a' && HemisphereLightMapTextureName[3] == 'p') &&
				(LightMapTextureName[0] == 'l' && LightMapTextureName[1] == 'm' && LightMapTextureName[2] == 'a' &&
				 LightMapTextureName[3] == 'p'))
			{
				bUseLightMap = true;

				// Get LightMap texture
				safe_string::copy(HemisphereLightMapTexture, sizeof(HemisphereLightMapTexture), *C.L_textures[2]);
				safe_string::copy(LightMapTexture, sizeof(LightMapTexture), *C.L_textures[1]);
			}
			else
			{
				bUseLightMap = false;
			}
		}

		// Create lightmapped shader if need
		C.set_Define(bUseLightMap, "USE_LIGHTMAP", "1");

		// [НОВОЕ] Логика поиска ARMD и ARM
		if (!DisablePBR)
		{
			// 1. Проверяем наличие ARMD (AO, Roughness, Metallic, Displacement)
			// Сначала ищем путь в конфигураторе (armd_path)
			if (bUseConfigurator)
			{
				bUseARMDMap = CheckAndApplyManualTexturePath("material_configuration", "armd_path", ARMDTexture,
															 MaterialConfiguration, AlbedoTexture);
			}

			// Если в конфигураторе нет, ищем по стандартному суффиксу _armd
			if (!bUseARMDMap)
			{
				bUseARMDMap = ConcatAndFindTexture(ARMDTexture, AlbedoTexture, "_armd");
			}

			// 2. Если ARMD не найдена, ищем обычную ARM (AO, Roughness, Metallic)
			if (!bUseARMDMap)
			{
				// Сначала ищем путь в конфигураторе (arm_path)
				if (bUseConfigurator)
				{
					bUseARMMap = CheckAndApplyManualTexturePath("material_configuration", "arm_path", ARMTexture,
																MaterialConfiguration, AlbedoTexture);
				}

				// Если в конфигураторе нет, ищем по стандартному суффиксу _arm
				if (!bUseARMMap)
				{
					bUseARMMap = ConcatAndFindTexture(ARMTexture, AlbedoTexture, "_arm");
				}
			}
		}

		C.set_Define(bUseARMDMap, "USE_ARMD_MAP", "1", CBlender_Compile::ShaderScope::Pixel);
		C.set_Define(bUseARMMap, "USE_ARM_MAP", "1", CBlender_Compile::ShaderScope::Pixel);

		// Get Empty-Roughness-Metallic texture if needed
		bUseERMMap = ConcatAndFindTexture(ERMTexture, AlbedoTexture, "_erm") && !DisablePBR;
		C.set_Define(bUseERMMap, "USE_ERM_MAP", "1", CBlender_Compile::ShaderScope::Pixel);

		// Custom Material Logic
		if (bUseConfigurator)
		{
			bUseCustomMaterial =
				CheckAndApplyManualTexturePath("material_configuration", "custom_material_path", CustomMaterialTexture,
											   MaterialConfiguration, AlbedoTexture);
		}

		if (bUseConfigurator)
		{
			bUseCustomMaterial =
				CheckAndApplyCustomMaterialPath(MaterialConfiguration, "material_configuration", "custom_material_path",
												CustomMaterialTexture, *C.L_textures[0]);
		}

		// Fallback for custom material
		if (!bUseCustomMaterial)
			bUseCustomMaterial = ConcatAndFindTexture(CustomMaterialTexture, AlbedoTexture, "_custom");

		// Setup channels
		if (bUseCustomMaterial)
		{
			C.set_Define("USE_CUSTOM_MATERIAL", "1", CBlender_Compile::ShaderScope::Pixel);

			LPCSTR red_type = "ao";
			LPCSTR green_type = "roughness";
			LPCSTR blue_type = "metallic";
			LPCSTR alpha_type = "height";

			if (bUseConfigurator && MaterialConfiguration->section_exist("custom_material_setup"))
			{
				red_type = GetStringValueIfExist("custom_material_setup", "red", red_type, MaterialConfiguration);
				green_type = GetStringValueIfExist("custom_material_setup", "green", green_type, MaterialConfiguration);
				blue_type = GetStringValueIfExist("custom_material_setup", "blue", blue_type, MaterialConfiguration);
				alpha_type = GetStringValueIfExist("custom_material_setup", "alpha", alpha_type, MaterialConfiguration);

				if (MaterialConfiguration->line_exist("custom_material_setup", "normal"))
				{
					LPCSTR normal_setup = MaterialConfiguration->r_string("custom_material_setup", "normal");
					SetupCustomMaterialNormal(C, normal_setup);
				}
			}

			DefineCustomChannel(C, red_type, "R");
			DefineCustomChannel(C, green_type, "G");
			DefineCustomChannel(C, blue_type, "B");
			DefineCustomChannel(C, alpha_type, "A");
		}

		if (!DisablePBR)
		{
			// [ИСПРАВЛЕНИЕ] Если используются ARMD или ARM, не ищем отдельные текстуры
			if (!bUseARMDMap && !bUseARMMap && !bUseERMMap)
			{
				if (bUseConfigurator)
				{
					bUseBakedAO = CheckAndApplyManualTexturePath("material_configuration", "ao_path", BakedAOTexture,
																 MaterialConfiguration, AlbedoTexture);

					if (LineIsExist("material_configuration", "gloss_path", MaterialConfiguration))
						bUseCustomGloss =
							CheckAndApplyManualTexturePath("material_configuration", "gloss_path", CustomGlossTexture,
														   MaterialConfiguration, AlbedoTexture);
					else
						bUseCustomRoughness = CheckAndApplyManualTexturePath("material_configuration", "roughness_path",
																			 CustomRoughnessTexture,
																			 MaterialConfiguration, AlbedoTexture);

					bUseCustomMetallic =
						CheckAndApplyManualTexturePath("material_configuration", "metallic_path", CustomMetallicTexture,
													   MaterialConfiguration, AlbedoTexture);
				}

				if (!bUseBakedAO)
					bUseBakedAO = ConcatAndFindTexture(BakedAOTexture, AlbedoTexture, "_ao");

				if (!bUseCustomGloss)
					bUseCustomGloss = ConcatAndFindTexture(CustomGlossTexture, AlbedoTexture, "_gloss");

				if (!bUseCustomRoughness && !bUseCustomGloss)
					bUseCustomRoughness = ConcatAndFindTexture(CustomRoughnessTexture, AlbedoTexture, "_roughness");

				if (!bUseCustomMetallic)
					bUseCustomMetallic = ConcatAndFindTexture(CustomMetallicTexture, AlbedoTexture, "_metallic");

				C.set_Define(bUseBakedAO, "USE_BAKED_AO", "1", CBlender_Compile::ShaderScope::Pixel);
				C.set_Define(bUseCustomRoughness, "USE_CUSTOM_ROUGHNESS", "1", CBlender_Compile::ShaderScope::Pixel);
				C.set_Define(bUseCustomGloss, "USE_CUSTOM_GLOSS", "1", CBlender_Compile::ShaderScope::Pixel);
				C.set_Define(bUseCustomMetallic, "USE_CUSTOM_METALLIC", "1", CBlender_Compile::ShaderScope::Pixel);
			}

			if (bUseConfigurator)
			{
				bUseCustomNormal = CheckAndApplyManualTexturePath(
					"material_configuration", "normal_path", CustomNormalTexture, MaterialConfiguration, AlbedoTexture);
				bUseCustomSubsurfacePower =
					CheckAndApplyManualTexturePath("material_configuration", "subsurface_power_path",
												   CustomSubsurfacePowerTexture, MaterialConfiguration, AlbedoTexture);
				bUseCustomCavity = CheckAndApplyManualTexturePath(
					"material_configuration", "cavity_path", CustomCavityTexture, MaterialConfiguration, AlbedoTexture);
				bUseCustomSpecularTint =
					CheckAndApplyManualTexturePath("material_configuration", "specular_tint_path",
												   CustomSpecularTintTexture, MaterialConfiguration, AlbedoTexture);
				bUseCustomSheenIntensity =
					CheckAndApplyManualTexturePath("material_configuration", "sheen_intensity_path",
												   CustomSheenIntensityTexture, MaterialConfiguration, AlbedoTexture);
				bUseCustomSheenRoughness =
					CheckAndApplyManualTexturePath("material_configuration", "sheen_roughness_path",
												   CustomSheenRoughnessTexture, MaterialConfiguration, AlbedoTexture);
				bUseCustomCoatIntensity =
					CheckAndApplyManualTexturePath("material_configuration", "coat_intensity_path",
												   CustomCoatIntensityTexture, MaterialConfiguration, AlbedoTexture);
				bUseCustomCoatRoughness =
					CheckAndApplyManualTexturePath("material_configuration", "coat_roughness_path",
												   CustomCoatRoughnessTexture, MaterialConfiguration, AlbedoTexture);
			}

			if (!bUseCustomNormal)
				bUseCustomNormal = ConcatAndFindTexture(CustomNormalTexture, AlbedoTexture, "_normal");

			// [НОВОЕ] Если обычной нормали нет, ищем упакованную
			if (!bUseCustomNormal)
			{
				if (bUseConfigurator)
				{
					bUsePackedNormal =
						CheckAndApplyManualTexturePath("material_configuration", "packed_normal_path",
													   PackedNormalTexture, MaterialConfiguration, AlbedoTexture);
				}

				if (!bUsePackedNormal)
				{
					bUsePackedNormal = ConcatAndFindTexture(PackedNormalTexture, AlbedoTexture, "_packed_normal");
				}
			}

			C.set_Define(bUseCustomNormal, "USE_CUSTOM_NORMAL", "1", CBlender_Compile::ShaderScope::Pixel);
			C.set_Define(bUsePackedNormal, "USE_PACKED_NORMAL", "1", CBlender_Compile::ShaderScope::Pixel);

			if (!bUseCustomSubsurfacePower)
				bUseCustomSubsurfacePower =
					ConcatAndFindTexture(CustomSubsurfacePowerTexture, AlbedoTexture, "_subsurface_power");

			C.set_Define(bUseCustomSubsurfacePower, "USE_CUSTOM_SUBSURFACE_POWER", "1",
						 CBlender_Compile::ShaderScope::Pixel);

			if (!bUseCustomCavity)
				bUseCustomCavity = ConcatAndFindTexture(CustomCavityTexture, AlbedoTexture, "_cavity");

			C.set_Define(bUseCustomCavity, "USE_CUSTOM_CAVITY", "1", CBlender_Compile::ShaderScope::Pixel);

			if (!bUseCustomSpecularTint)
				bUseCustomSpecularTint =
					ConcatAndFindTexture(CustomSpecularTintTexture, AlbedoTexture, "_specular_tint");

			C.set_Define(bUseCustomSpecularTint, "USE_CUSTOM_SPECULAR_TINT", "1", CBlender_Compile::ShaderScope::Pixel);

			if (!bUseCustomSheenIntensity)
				bUseCustomSheenIntensity =
					ConcatAndFindTexture(CustomSheenIntensityTexture, AlbedoTexture, "_sheen_intensity");

			C.set_Define(bUseCustomSheenIntensity, "USE_CUSTOM_SHEEN_INTENSITY", "1",
						 CBlender_Compile::ShaderScope::Pixel);

			if (!bUseCustomSheenRoughness)
				bUseCustomSheenRoughness =
					ConcatAndFindTexture(CustomSheenRoughnessTexture, AlbedoTexture, "_sheen_roughness");

			C.set_Define(bUseCustomSheenRoughness, "USE_CUSTOM_SHEEN_ROUGHNESS", "1",
						 CBlender_Compile::ShaderScope::Pixel);

			if (!bUseCustomCoatIntensity)
				bUseCustomCoatIntensity =
					ConcatAndFindTexture(CustomCoatIntensityTexture, AlbedoTexture, "_coat_intensity");

			C.set_Define(bUseCustomCoatIntensity, "USE_CUSTOM_COAT_INTENSITY", "1",
						 CBlender_Compile::ShaderScope::Pixel);

			if (!bUseCustomCoatRoughness)
				bUseCustomCoatRoughness =
					ConcatAndFindTexture(CustomCoatRoughnessTexture, AlbedoTexture, "_coat_roughness");

			C.set_Define(bUseCustomCoatRoughness, "USE_CUSTOM_COAT_ROUGHNESS", "1",
						 CBlender_Compile::ShaderScope::Pixel);
		}

		if (bUseConfigurator)
		{
			bUseCustomEmission = CheckAndApplyManualTexturePath(
				"material_configuration", "emission_path", CustomEmissionTexture, MaterialConfiguration, AlbedoTexture);
		}

		if (!bUseCustomEmission)
			bUseCustomEmission = ConcatAndFindTexture(CustomEmissionTexture, AlbedoTexture, "_emission");

		C.set_Define(bUseCustomEmission, "USE_CUSTOM_EMISSION", "1");

		// Create shader with normal mapping or displacement if need
		int DisplacementType = 1;
		bool bUseDisplacement = true;

		// Configuration file have priority above original parameters
		if (bUseConfigurator && bIsHightQualityGeometry)
		{
			// Check do we realy need for displacement or not
			bUseDisplacement =
				GetBoolValueIfExist("material_configuration", "use_displacement", false, MaterialConfiguration);

			// Check what displacement type we need to set
			if (bUseDisplacement)
			{
				LPCSTR displacement_type = GetStringValueIfExist("material_configuration", "displacement_type",
																 "parallax_occlusion_mapping", MaterialConfiguration);

				if (StringsIsSimilar(displacement_type, "normal_mapping"))
					DisplacementType = 1;
				else if (StringsIsSimilar(displacement_type, "parallax_mapping"))
					DisplacementType = 2;
				else if (StringsIsSimilar(displacement_type, "parallax_occlusion_mapping"))
					DisplacementType = 3;
			}
		}
		else
		{
			if (ps_r_material_quality == 1 || !bUseBump)
				DisplacementType = 1; // normal
			else if (ps_r_material_quality == 2 || !C.bSteepParallax)
				DisplacementType = 2; // parallax
			else if ((ps_r_material_quality == 3) && C.bSteepParallax)
				DisplacementType = 3; // steep parallax
		}

		C.set_Define(DisplacementType == 1, "USE_NORMAL_MAPPING", "1", CBlender_Compile::ShaderScope::Pixel);
		C.set_Define(DisplacementType == 2, "USE_PARALLAX_MAPPING", "1", CBlender_Compile::ShaderScope::Pixel);
		C.set_Define(DisplacementType == 3, "USE_PARALLAX_OCCLUSION_MAPPING", "1",
					 CBlender_Compile::ShaderScope::Pixel);

		// Get displacement texture
		if (bUseConfigurator)
			bUseCustomDisplacement =
				CheckAndApplyManualTexturePath("material_configuration", "displacement_path", CustomDisplacementTexture,
											   MaterialConfiguration, AlbedoTexture);

		if (!bUseCustomDisplacement)
			bUseCustomDisplacement = ConcatAndFindTexture(CustomDisplacementTexture, AlbedoTexture, "_displacement");

		C.set_Define(bUseCustomDisplacement, "USE_CUSTOM_DISPLACEMENT", "1", CBlender_Compile::ShaderScope::Pixel);

		// Create shader with deatil texture if need
		C.set_Define(bUseDetail, "USE_TDETAIL", "1");

		C.set_Define(bUseDetailBump, "USE_DETAIL_BUMP", "1");

		// Check do we need to use custom shader
		if (bUseConfigurator)
		{
			LPCSTR CustomPixelShader =
				GetStringValueIfExist("material_configuration", "pixel_shader", "default", MaterialConfiguration);
			LPCSTR CustomVertexShader =
				GetStringValueIfExist("material_configuration", "vertex_shader", "default", MaterialConfiguration);

			if (!StringsIsSimilar(CustomPixelShader, "default"))
				PixelShaderName = CustomPixelShader;

			if (!StringsIsSimilar(CustomVertexShader, "default"))
				PixelShaderName = CustomPixelShader;
		}
	}
	// Create shader pass
	safe_string::concat3(NewPixelShaderName, sizeof(NewPixelShaderName), "gbuffer_stage_", PixelShaderName, "");
	safe_string::concat3(NewVertexShaderName, sizeof(NewVertexShaderName), "gbuffer_stage_", VertexShaderName, "");
	C.begin_Pass(NewVertexShaderName, NewPixelShaderName, "main", "main", FALSE, TRUE, TRUE);

	C.set_Sampler("s_base", AlbedoTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
				  D3DTEXF_ANISOTROPIC, true);

	if (bUseOpacity)
		C.set_Sampler("s_custom_opacity", OpacityTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
					  D3DTEXF_ANISOTROPIC);

	// [НОВОЕ] Сэмплеры для ARMD и ARM
	if (bUseARMDMap)
	{
		C.set_Sampler("s_armd_map", ARMDTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
					  D3DTEXF_ANISOTROPIC);
	}
	else if (bUseARMMap)
	{
		C.set_Sampler("s_arm_map", ARMTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
					  D3DTEXF_ANISOTROPIC);
	}
	else
	{
		if (bUseERMMap)
		{
			C.set_Sampler("s_erm_map", ERMTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
						  D3DTEXF_ANISOTROPIC);
		}

		if (bUseBakedAO)
			C.set_Sampler("s_baked_ao", BakedAOTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
						  D3DTEXF_ANISOTROPIC);

		if (bUseCustomRoughness)
			C.set_Sampler("s_custom_roughness", CustomRoughnessTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
						  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

		if (bUseCustomGloss)
			C.set_Sampler("s_custom_gloss", CustomGlossTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
						  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

		if (bUseCustomMetallic)
			C.set_Sampler("s_custom_metallic", CustomMetallicTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
						  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
	}

	if (bUseCustomMaterial)
		C.set_Sampler("s_custom_material", CustomMaterialTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomNormal)
		C.set_Sampler("s_custom_normal", CustomNormalTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	// [НОВОЕ] Сэмплер для упакованной нормали
	if (bUsePackedNormal)
		C.set_Sampler("s_packed_normal", PackedNormalTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomSubsurfacePower)
		C.set_Sampler("s_custom_subsurface_power", CustomSubsurfacePowerTexture, false, D3DTADDRESS_WRAP,
					  D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomEmission)
		C.set_Sampler("s_custom_emission", CustomEmissionTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomDisplacement)
		C.set_Sampler("s_custom_displacement", CustomDisplacementTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomCavity)
		C.set_Sampler("s_custom_cavity", CustomCavityTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomSpecularTint)
		C.set_Sampler("s_custom_specular_tint", CustomSpecularTintTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomSheenIntensity)
		C.set_Sampler("s_custom_sheen_intensity", CustomSheenIntensityTexture, false, D3DTADDRESS_WRAP,
					  D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomSheenRoughness)
		C.set_Sampler("s_custom_sheen_roughness", CustomSheenRoughnessTexture, false, D3DTADDRESS_WRAP,
					  D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomCoatIntensity)
		C.set_Sampler("s_custom_coat_intensity", CustomCoatIntensityTexture, false, D3DTADDRESS_WRAP,
					  D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomCoatRoughness)
		C.set_Sampler("s_custom_coat_roughness", CustomCoatRoughnessTexture, false, D3DTADDRESS_WRAP,
					  D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomWeight)
		C.set_Sampler("s_custom_weight", CustomWeightTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseBump)
	{
		C.set_Sampler("s_bumpX", BumpCorrectionTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
					  D3DTEXF_ANISOTROPIC);

		C.set_Sampler("s_bump", BumpTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
					  D3DTEXF_ANISOTROPIC);
	}

	if (bUseDetail)
	{
		C.set_Sampler("s_detail", DetailAlbedoTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
					  D3DTEXF_ANISOTROPIC, true);

		if (bUseDetailBump)
		{
			C.set_Sampler("s_detailBump", DetailBumpTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
						  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

			C.set_Sampler("s_detailBumpX", DetailBumpCorrectionTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
						  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
		}
	}

	if (bUseLightMap)
	{
		C.set_Sampler("s_hemi", HemisphereLightMapTexture, false, D3DTADDRESS_CLAMP, D3DTEXF_ANISOTROPIC,
					  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
		C.set_Sampler("s_lmap", LightMapTexture, false, D3DTADDRESS_CLAMP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
					  D3DTEXF_ANISOTROPIC);
	}

	jitter(C);

	C.end_Pass();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void configure_shader_detail_object(CBlender_Compile& C, bool bIsHightQualityGeometry, LPCSTR VertexShaderName,
									LPCSTR PixelShaderName, BOOL bUseAlpha)
{
	C.set_Define("MATERIAL_QUALITY", (int)ps_r_material_quality, CBlender_Compile::ShaderScope::Pixel);

	// Output shader names
	string_path NewPixelShaderName = {0};
	string_path NewVertexShaderName = {0};

	// Base part of material
	string_path AlbedoTexture = {0};
	safe_string::copy(AlbedoTexture, sizeof(AlbedoTexture), *C.L_textures[0]);

	// Add extension to texture  and chek for null
	Engine.ResourceManager->fix_texture_name(AlbedoTexture);

	// Get  for base texture for material
	safe_string::copy(AlbedoTexture, sizeof(AlbedoTexture), *C.L_textures[0]);

	bool DisablePBR = ps_r_shading_mode == SHADING_MODE_LEASHED;

	bool bUseConfigurator = false;
	CInifile* MaterialConfiguration;
	string_path MaterialConfiguratorSearchPath = {0};
	string_path MaterialConfiguratorRealPath = {0};
	safe_string::copy(MaterialConfiguratorSearchPath, sizeof(MaterialConfiguratorSearchPath), AlbedoTexture);
	safe_string::concat3(MaterialConfiguratorSearchPath, sizeof(MaterialConfiguratorSearchPath),
						 MaterialConfiguratorSearchPath, "_material_configuration", ".ltx");
	FS.update_path(MaterialConfiguratorRealPath, "$game_textures$", MaterialConfiguratorSearchPath);

	if (FS.exist(MaterialConfiguratorRealPath))
	{
		MaterialConfiguration = CInifile::Create(MaterialConfiguratorRealPath);
		bUseConfigurator = true;
	}

	// Check do we need to use custom shader
	bool bIsSrgbAlbedo = true;
	if (bUseConfigurator)
	{
		LPCSTR AlbedoColorSpace =
			GetStringValueIfExist("albedo_configuration", "color_space", "srgb", MaterialConfiguration);

		if (StringsIsSimilar(AlbedoColorSpace, "linear"))
			bIsSrgbAlbedo = false;
	}

	C.set_Define(bIsSrgbAlbedo, "USE_SRGB_COLOR_CONVERTING", "1", CBlender_Compile::ShaderScope::Pixel);

	C.set_Define(true, "USE_NORMAL_MAPPING", "1", CBlender_Compile::ShaderScope::Pixel);

	// Get opacity texture
	bool bUseOpacity = false;
	string_path OpacityTexture = {0};
	bUseOpacity = ConcatAndFindLevelTexture(OpacityTexture, AlbedoTexture, "_opacity");
	C.set_Define(bUseOpacity, "USE_CUSTOM_OPACITY", "1", CBlender_Compile::ShaderScope::Pixel);

	// Create shader with alpha testing if need
	C.set_Define(bUseAlpha || bUseOpacity, "USE_ALPHA_TEST", "1", CBlender_Compile::ShaderScope::Pixel);

	// Get BakedAO texture
	bool bUseBakedAO = false;
	string_path BakedAOTexture = {0};
	bUseBakedAO = ConcatAndFindLevelTexture(BakedAOTexture, AlbedoTexture, "_ao") && !DisablePBR;
	C.set_Define(bUseBakedAO, "USE_BAKED_AO", "1", CBlender_Compile::ShaderScope::Pixel);

	// Get normal texture
	bool bUseCustomNormal = false;
	string_path CustomNormalTexture = {0};
	bUseCustomNormal = ConcatAndFindLevelTexture(CustomNormalTexture, AlbedoTexture, "_normal") && !DisablePBR;
	C.set_Define(bUseCustomNormal, "USE_CUSTOM_NORMAL", "1", CBlender_Compile::ShaderScope::Pixel);

	// Get roughness texture
	bool bUseCustomRoughness = false;
	string_path CustomRoughnessTexture = {0};
	bUseCustomRoughness = ConcatAndFindLevelTexture(CustomRoughnessTexture, AlbedoTexture, "_roughness") && !DisablePBR;
	C.set_Define(bUseCustomRoughness, "USE_CUSTOM_ROUGHNESS", "1", CBlender_Compile::ShaderScope::Pixel);

	// Get metallic texture
	bool bUseCustomMetallic = false;
	string_path CustomMetallicTexture = {0};
	bUseCustomMetallic = ConcatAndFindLevelTexture(CustomMetallicTexture, AlbedoTexture, "_metallic") && !DisablePBR;
	C.set_Define(bUseCustomMetallic, "USE_CUSTOM_METALLIC", "1", CBlender_Compile::ShaderScope::Pixel);

	// Get subsurface_power power texture
	bool bUseCustomSubsurfacePower = false;
	string_path CustomSubsurfacePowerTexture = {0};
	bUseCustomSubsurfacePower =
		ConcatAndFindLevelTexture(CustomSubsurfacePowerTexture, AlbedoTexture, "_subsurface_power") && !DisablePBR;
	C.set_Define(bUseCustomSubsurfacePower, "USE_CUSTOM_SUBSURFACE_POWER", "1", CBlender_Compile::ShaderScope::Pixel);

	// Get emission power texture
	bool bUseCustomEmission = false;
	string_path CustomEmissionTexture = {0};
	bUseCustomEmission = ConcatAndFindLevelTexture(CustomEmissionTexture, AlbedoTexture, "_emission");
	C.set_Define(bUseCustomEmission, "USE_CUSTOM_EMISSION", "1", CBlender_Compile::ShaderScope::Pixel);

	// Get displacement texture
	bool bUseCustomDisplacement = false;
	string_path CustomDisplacementTexture = {0};
	bUseCustomDisplacement =
		ConcatAndFindLevelTexture(CustomDisplacementTexture, AlbedoTexture, "_displacement") && !DisablePBR;
	C.set_Define(bUseCustomDisplacement, "USE_CUSTOM_DISPLACEMENT", "1", CBlender_Compile::ShaderScope::Pixel);

	// Get cavity texture
	bool bUseCustomCavity = false;
	string_path CustomCavityTexture = {0};
	bUseCustomCavity = ConcatAndFindLevelTexture(CustomCavityTexture, AlbedoTexture, "_cavity") && !DisablePBR;
	C.set_Define(bUseCustomCavity, "USE_CUSTOM_CAVITY", "1", CBlender_Compile::ShaderScope::Pixel);

	// Get weight texture
	bool bUseCustomWeight = false;
	string_path CustomWeightTexture = {0};
	bUseCustomWeight = ConcatAndFindLevelTexture(CustomWeightTexture, AlbedoTexture, "_weight");
	C.set_Define(bUseCustomWeight, "USE_WEIGHT_MAP", "1", CBlender_Compile::ShaderScope::Vertex);

	// Create shader pass
	safe_string::concat3(NewPixelShaderName, sizeof(NewPixelShaderName), "gbuffer_stage_", PixelShaderName, "");
	safe_string::concat3(NewVertexShaderName, sizeof(NewVertexShaderName), "gbuffer_stage_", VertexShaderName, "");
	C.begin_Pass(NewVertexShaderName, NewPixelShaderName, "main", "main", FALSE, TRUE, TRUE);

	C.set_Sampler("s_base", AlbedoTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
				  D3DTEXF_ANISOTROPIC, true);

	if (bUseOpacity)
		C.set_Sampler("s_custom_opacity", OpacityTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
					  D3DTEXF_ANISOTROPIC);

	if (bUseBakedAO)
		C.set_Sampler("s_baked_ao", BakedAOTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
					  D3DTEXF_ANISOTROPIC);

	if (bUseCustomNormal)
		C.set_Sampler("s_custom_normal", CustomNormalTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomRoughness)
		C.set_Sampler("s_custom_roughness", CustomRoughnessTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomMetallic)
		C.set_Sampler("s_custom_metallic", CustomMetallicTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomSubsurfacePower)
		C.set_Sampler("s_custom_subsurface_power", CustomSubsurfacePowerTexture, false, D3DTADDRESS_WRAP,
					  D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomEmission)
		C.set_Sampler("s_custom_emission", CustomEmissionTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomDisplacement)
		C.set_Sampler("s_custom_displacement", CustomDisplacementTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomCavity)
		C.set_Sampler("s_custom_cavity", CustomCavityTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomWeight)
		C.set_Sampler("s_custom_weight", CustomWeightTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					  D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	jitter(C);

	C.end_Pass();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
