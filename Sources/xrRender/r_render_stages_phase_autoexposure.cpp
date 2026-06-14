///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "blender_autoexposure.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::swap_luminance()
{
	RenderBackendLegacy.CopyViewportSurface(RenderTarget->rt_SceneLuminance, RenderTarget->rt_SceneLuminancePrevious);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::downsample_scene_luminance()
{
	// Инициализируем цепь уровней - начинаем с 0, генерируем в него luminance из generic1
	ref_rt MipChain = RenderTarget->rt_LUM_Mip_Chain;
	RenderBackendLegacy.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_AUTOEXPOSURE_GENERATE_MIP_CHAIN], 0);
	RenderBackendLegacy.RenderViewportSurface(MipChain);

	// Генерируем остальные mip-уровни
	for (u32 i = 1; i < MipChain->get_levels_count(); i++)
	{
		// Устанавливаем шейдер
		RenderBackendLegacy.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_AUTOEXPOSURE_GENERATE_MIP_CHAIN], 1);

		// Разрешение предыдущего mip уровня
		u32 prev_mip = i - 1, prev_mip_width, prev_mip_height;
		MipChain->get_level_desc(prev_mip, prev_mip_width, prev_mip_height);
		RenderBackendLegacy.set_Constant("mip_data", 0.0f, prev_mip, 1.0f / prev_mip_width, 1.0f / prev_mip_height);

		// Рендерим
		IDirect3DSurface9* mip_surface = MipChain->get_surface_level(i);
		RenderBackendLegacy.RenderViewportSurface(mip_surface);
	}
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::prepare_scene_luminance()
{
	// Параметры автоэкспозиции
	float TimeDelta = Engine.TimeManager.GetDeltaTime();
	float adaptation_speed = ps_r_autoexposure_adaptation;

	// Гарантируем разумные пределы
	adaptation_speed = std::max(0.01f, std::min(adaptation_speed, 10.0f));
	TimeDelta = std::min(TimeDelta, 0.033f);

	fvec4 adaptation_params{adaptation_speed, TimeDelta, 0.0f, 0.0f};

	RenderBackendLegacy.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_AUTOEXPOSURE_PREPARE_LUMINANCE]);
	RenderBackendLegacy.set_Constant("adaptation_params", adaptation_params);
	RenderBackendLegacy.RenderViewportSurface(1.0f, 1.0f, RenderTarget->rt_SceneLuminance);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::apply_exposure()
{
	// Параметры автоэкспозиции
	fvec3 none, full, result;
	none.set(1, 0, 1);
	full.set(ps_r_autoexposure_middlegray, 1.f, ps_r_autoexposure_low_lum);
	result.lerp(none, full, ps_r_autoexposure_amount);

	// Применяем экспозицию
	RenderBackendLegacy.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_AUTOEXPOSURE_APPLY_EXPOSURE]);
	RenderBackendLegacy.set_Constant("autoexposure_params", result.x, result.z);
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Generic[1]);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::dummy_exposure()
{
	// Параметры автоэкспозиции
	fvec3 none, full, result;
	none.set(1, 0, 1);
	full.set(ps_r_autoexposure_middlegray, 1.f, ps_r_autoexposure_low_lum);
	result.lerp(none, full, ps_r_autoexposure_amount);

	// Применяем экспозицию
	RenderBackendLegacy.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_AUTOEXPOSURE_APPLY_EXPOSURE], 1);
	RenderBackendLegacy.set_Constant("autoexposure_params", result.x, result.z);
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Generic[1]);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_autoexposure()
{
	swap_luminance();
	downsample_scene_luminance();
	prepare_scene_luminance();
	apply_exposure();
}
///////////////////////////////////////////////////////////////////////////////////
