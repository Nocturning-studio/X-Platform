////////////////////////////////////////////////////////////////////////////////
// Created: 16.03.2025
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_render_pipeline.h"
////////////////////////////////////////////////////////////////////////////////
void CRender::Render()
{
	PROFILE_FUNCTION();

	if (g_dedicated_server)
		return;

	Engine.Statistic->RenderCALC.Begin();

	bool b_need_render_menu = g_pGamePersistent ? g_pGamePersistent->OnRenderPPUI_query() : false;

	if (b_need_render_menu)
	{
		RenderMenu();
	}
	else
	{
		if (!(g_pGameLevel && g_pGameLevel->pHUD))
			return;

		RenderScene();
		//RenderDebug();
	}

	Engine.Statistic->RenderCALC.End();
}
////////////////////////////////////////////////////////////////////////////////
