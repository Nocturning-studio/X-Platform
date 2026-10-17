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
#include "RenderPipeline/DeferredLightingPipeline.h"
CDeferredLightingPipeline* scene_pipe = nullptr;
////////////////////////////////////////////////////////////////////////////////
void CRender::Render()
{
	PROFILE_FUNCTION();

	if(g_dedicated_server)
		return;

	Engine.Statistic->RenderCALC.Begin();

	bool b_need_render_menu = g_pGamePersistent ? g_pGamePersistent->OnRenderPPUI_query() : false;

	if(b_need_render_menu)
	{
		RenderBackendLegacy.Invalidate();
		Engine.RHI->InvalidateCache();
		RenderMenu();
		RenderBackendLegacy.Invalidate();
		Engine.RHI->InvalidateCache();
	}
	else
	{
		if(!(g_pGameLevel && g_pGameLevel->pHUD))
			return;

		if (scene_pipe == nullptr)
		{
			scene_pipe = xr_new<CDeferredLightingPipeline>();
			scene_pipe->Initialize();
		}

		prepare_to_render();
		calculate_scene_culling();

		scene_pipe->Execute();

		//RenderScene();
		// RenderDebug();
	}

	Engine.Statistic->RenderCALC.End();
}
////////////////////////////////////////////////////////////////////////////////
