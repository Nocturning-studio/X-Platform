#include "stdafx.h"

void CRender::set_light_accumulator()
{
	////OPTICK_EVENT("CRenderTarget::set_light_accumulator");

	if (dwAccumulatorClearMark == Engine.TimeManager.GetFrameCount())
	{
		RenderBackend.set_Render_Target_Surface(RenderTarget->rt_Light_Accumulator);

		RenderBackend.set_Depth_Buffer(RenderBackend.GetBaseZB());
	}
	else
	{
		dwAccumulatorClearMark = Engine.TimeManager.GetFrameCount();

		RenderBackend.set_Render_Target_Surface(RenderTarget->rt_Light_Accumulator);

		RenderBackend.set_Depth_Buffer(RenderBackend.GetBaseZB());
		dwLightMarkerID = 5; // start from 5, increment in 2 units
		CHK_DX(RenderBackend.GetDevice()->Clear(0L, NULL, D3DCLEAR_TARGET, color_rgba(0, 0, 0, 0), 1.0f, 0L));
	}
}

