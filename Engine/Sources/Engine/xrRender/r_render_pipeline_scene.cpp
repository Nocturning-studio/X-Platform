////////////////////////////////////////////////////////////////////////////////
//  X-Platform — расширение функциональности X-Ray Engine
//
//  Автор:        NSDeathman
//  Студия:       Nocturning Studio
//
//  SPDX-FileCopyrightText: 2026 Nocturning Studio и участники X-Platform
//  SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
//
//  Этот файл распространяется на условиях PolyForm Noncommercial 1.0.0.
//  Копирование, изменение и распространение разрешены для некоммерческих
//  целей. Публичное указание авторства в вашем проекте, документации или
//  рекламе не требуется — оно приветствуется, но не является условием
//  использования.
//
//  При распространении копий или производных работ лицензия обязывает
//  сохранять настоящие уведомления и передавать получателю текст лицензии
//  или ссылку на него. Пожалуйста, не удаляйте эту плашку — это помогает
//  сохранять историю проекта и корректно соблюдать лицензию.
//
//  Коммерческое использование без отдельного разрешения правообладателей
//  запрещено.
//
//  Полный текст лицензии:
//      https://polyformproject.org/licenses/noncommercial/1.0.0
//
//  Этот файл является source-available, а не open source.
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "r_render_pipeline.h"
////////////////////////////////////////////////////////////////////////////////
void CRender::RenderScene()
{
	PROFILE_FUNCTION();

	if(m_bFirstFrameAfterReset)
	{
		m_saved_viewproj.set(Engine.RenderView.ViewProjection);
		m_saved_invview.invert(Engine.RenderView.View);
		m_bFirstFrameAfterReset = false;
	}

	prepare_to_render();

	calculate_scene_culling();

	render_scene_to_gbuffer();

	// Directional light - sun
	render_sun();

	// Omni/Spot lights
	render_lights();

	render_ambient_occlusion();

	// render_bent_normals();

	if(ps_r_postprocess_flags.test(RFLAG_REFLECTIONS))
	{
		create_hi_z_mip_chain();

		precombine_scene();

		render_screen_space_reflections();
	}
	else
	{
		clear_reflections();
	}

	render_skybox();

	combine_scene_lighting();

	render_stage_forward();

	// if (ps_r_lighting_flags.test(RFLAG_SUN_SHAFTS))
	combine_sun_shafts();

	render_postprocess();

	if(g_pGamePersistent)
		g_pGamePersistent->OnRenderPPUI_main();

	if(!m_bFirstFrameAfterReset)
		output_frame_to_screen();

	m_saved_viewproj.set(Engine.RenderView.ViewProjection);
	m_saved_invview.invert(Engine.RenderView.View);
}
////////////////////////////////////////////////////////////////////////////////
