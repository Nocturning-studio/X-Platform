///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "blender_autoexposure.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::save_scene_luminance()
{
	////OPTICK_EVENT("CRender::save_scene_luminance");

	RenderBackend.CopyViewportSurface(RenderTarget->rt_SceneLuminance, RenderTarget->rt_SceneLuminancePrevious);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::downsample_scene_luminance()
{
	////OPTICK_EVENT("CRender::downsample_scene_luminance");

	// Инициализируем цепь уровней - начинаем с 0, генерируем в него luminance из generic1
	ref_rt MipChain = RenderTarget->rt_LUM_Mip_Chain;
	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_AUTOEXPOSURE_GENERATE_MIP_CHAIN], 0);
	RenderBackend.RenderViewportSurface(MipChain);

	// Генерируем остальные mip-уровни
	for (u32 i = 1; i < MipChain->get_levels_count(); i++)
	{
		// Устанавливаем шейдер
		RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_AUTOEXPOSURE_GENERATE_MIP_CHAIN], 1);

		// Разрешение предыдущего mip уровня
		u32 prev_mip = i - 1, prev_mip_width, prev_mip_height;
		MipChain->get_level_desc(prev_mip, prev_mip_width, prev_mip_height);
		RenderBackend.set_Constant("mip_data", 0.0f, prev_mip, 1.0f / prev_mip_width, 1.0f / prev_mip_height);

		// Рендерим
		IDirect3DSurface9* mip_surface = MipChain->get_surface_level(i);
		RenderBackend.RenderViewportSurface(mip_surface);
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

	Fvector4 adaptation_params;
	adaptation_params.set(adaptation_speed, TimeDelta, 0.0f, 0.0f);

	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_AUTOEXPOSURE_PREPARE_LUMINANCE]);
	RenderBackend.set_Constant("adaptation_params", adaptation_params);
	RenderBackend.RenderViewportSurface(1.0f, 1.0f, RenderTarget->rt_SceneLuminance);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::apply_exposure()
{
	////OPTICK_EVENT("CRender::apply_exposure");

	// Параметры автоэкспозиции
	Fvector3 _none, _full, _result;
	_none.set(1, 0, 1);
	_full.set(ps_r_autoexposure_middlegray, 1.f, ps_r_autoexposure_low_lum);
	_result.lerp(_none, _full, ps_r_autoexposure_amount);

	// Применяем экспозицию
	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_AUTOEXPOSURE_APPLY_EXPOSURE]);
	RenderBackend.set_Constant("autoexposure_params", _result.x, _result.z);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic[1]);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_autoexposure()
{
	////OPTICK_EVENT("CRender::render_autoexposure");

	save_scene_luminance();
	downsample_scene_luminance();
	prepare_scene_luminance();
	apply_exposure();
}
///////////////////////////////////////////////////////////////////////////////////
