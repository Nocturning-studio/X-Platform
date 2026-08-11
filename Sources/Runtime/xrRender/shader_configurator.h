////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Modified: 14.10.2023
// Modifier: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef SHADER_CONFIGURATOR_INCLUDED
#define SHADER_CONFIGURATOR_INCLUDED
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern bool ConcatAndFindTexture(string_path& ResultPath, string_path AlbedoPath, LPCSTR TexturePrefix);
extern float GetFloatValueIfExist(LPCSTR section_name, LPCSTR line_name, float default_value, CInifile* config);
extern fvec3 GetRGBColorValueIfExist(LPCSTR section_name, LPCSTR line_name, fvec3 default_value, CInifile* config);
extern fvec4 GetRGBAColorValueIfExist(LPCSTR section_name, LPCSTR line_name, fvec4 default_value, CInifile* config);
extern LPCSTR GetStringValueIfExist(LPCSTR section_name, LPCSTR line_name, LPCSTR default_value, CInifile* config);
extern bool GetBoolValueIfExist(LPCSTR section_name, LPCSTR line_name, bool default_state, CInifile* config);
extern bool StringsIsSimilar(LPCSTR x, LPCSTR y);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum class ShaderStage
{
	gbuffer_stage = 0,
	shadow_depth_stage
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern void configure_shader(CBlender_Compile& C, bool bIsHightQualityGeometry, LPCSTR VertexShaderName, LPCSTR PixelShaderName, BOOL bUseAlpha, BOOL bUseDepthOnly = false, ShaderStage Stage = ShaderStage::gbuffer_stage);
extern void configure_shader_detail_object(CBlender_Compile& C, bool bIsHightQualityGeometry, LPCSTR VertexShaderName, LPCSTR PixelShaderName, BOOL bUseAlpha = false, ShaderStage Stage = ShaderStage::gbuffer_stage);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // SHADER_CONFIGURATOR_INCLUDED
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////