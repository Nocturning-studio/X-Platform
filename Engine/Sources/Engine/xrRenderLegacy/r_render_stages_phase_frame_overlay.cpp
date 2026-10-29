///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-Platform
///////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "Blender_frame_overlay.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_screen_overlays()
{
	////OPTICK_EVENT("CRenderTarget::render_screen_overlays");

	int GridEnabled = 0;
	int CinemaBordersEnabled = 0;
	int WatermarkEnabled = 0;

	if(ps_r_overlay_flags.test(RFLAG_PHOTO_GRID))
		GridEnabled = 1;

	if(ps_r_overlay_flags.test(RFLAG_CINEMA_BORDERS))
		CinemaBordersEnabled = 1;

	RenderBackendLegacy.SetCullMode(CULL_DISABLE);
	RenderBackendLegacy.SetStencil(FALSE);

	RenderBackendLegacy.SetShaderElement(RenderTarget->s_frame_overlay->E[SE_OVERLAYS_MAIN]);
	RenderBackendLegacy.SetConstant("enabled_overlays", (float)GridEnabled, (float)CinemaBordersEnabled, 0, 0);
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Generic[0]);

	if(ps_r_overlay_flags.test(RFLAG_WATERMARK))
	{
		RenderBackendLegacy.SetShaderElement(RenderTarget->s_frame_overlay->E[SE_OVERLAYS_WATERMARK]);
		RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Generic[0]);
	}
}
///////////////////////////////////////////////////////////////////////////////////
