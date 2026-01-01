///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Blender_frame_overlay.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_screen_overlays()
{
	OPTICK_EVENT("CRenderTarget::render_screen_overlays");

	int GridEnabled = 0;
	int CinemaBordersEnabled = 0;
	int WatermarkEnabled = 0;

	if (ps_r_overlay_flags.test(RFLAG_PHOTO_GRID))
		GridEnabled = 1;

	if (ps_r_overlay_flags.test(RFLAG_CINEMA_BORDERS))
		CinemaBordersEnabled = 1;

	RenderBackend.set_CullMode(CULL_DISABLE);
	RenderBackend.set_Stencil(FALSE);

	RenderBackend.set_Element(RenderTarget->s_frame_overlay->E[SE_OVERLAYS_MAIN]);
	RenderBackend.set_Constant("enabled_overlays", (float)GridEnabled, (float)CinemaBordersEnabled, 0, 0);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic[0]);

	if (ps_r_overlay_flags.test(RFLAG_WATERMARK))
	{
		RenderBackend.set_Element(RenderTarget->s_frame_overlay->E[SE_OVERLAYS_WATERMARK]);
		RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic[0]);
	}
}
///////////////////////////////////////////////////////////////////////////////////