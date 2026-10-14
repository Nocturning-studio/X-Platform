#include "pch.h"

void CRender::set_light_accumulator()
{
	if(dwAccumulatorClearMark == Engine.TimeManager.GetFrameCount())
	{
		RenderBackendLegacy.SetRenderTarget(RenderTarget->rt_Light_Accumulator);
		RenderBackendLegacy.SetDepthBuffer(RenderBackendLegacy.GetBaseZB());
	}
	else
	{
		dwAccumulatorClearMark = Engine.TimeManager.GetFrameCount();

		RenderBackendLegacy.SetRenderTarget(RenderTarget->rt_Light_Accumulator);
		RenderBackendLegacy.SetDepthBuffer(RenderBackendLegacy.GetBaseZB());
		dwLightMarkerID = 5; // start from 5, increment in 2 units
		RenderBackendLegacy.Clear(0L, nullptr, D3DCLEAR_TARGET, 0x0, 1.0f, 0L);
	}
}
