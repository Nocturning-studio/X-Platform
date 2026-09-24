#include "stdafx.h"

void CRender::set_light_accumulator()
{
	if(dwAccumulatorClearMark == Engine.TimeManager.GetFrameCount())
	{
		RenderBackend.SetRenderTarget(RenderTarget->rt_Light_Accumulator);
		RenderBackend.SetDepthBuffer(RenderBackend.GetBaseZB());
	}
	else
	{
		dwAccumulatorClearMark = Engine.TimeManager.GetFrameCount();

		RenderBackend.SetRenderTarget(RenderTarget->rt_Light_Accumulator);
		RenderBackend.SetDepthBuffer(RenderBackend.GetBaseZB());
		dwLightMarkerID = 5; // start from 5, increment in 2 units
		RenderBackend.Clear(0L, nullptr, D3DCLEAR_TARGET, 0x0, 1.0f, 0L);
	}
}
