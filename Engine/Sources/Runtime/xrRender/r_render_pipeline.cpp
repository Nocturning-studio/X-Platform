////////////////////////////////////////////////////////////////////////////////
// Created: 16.03.2025
// Author: NSDeathman
// Nocturning studio for X-Platform
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
		Engine.RHI->InvalidateStateCache();
		RenderMenu();
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

		RenderBackendLegacy.Invalidate();
		Engine.RHI->InvalidateStateCache();

		scene_pipe->Execute();

		//RenderScene();
		// RenderDebug();
	}

	Engine.Statistic->RenderCALC.End();
}
////////////////////////////////////////////////////////////////////////////////
