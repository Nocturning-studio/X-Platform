////////////////////////////////////////////////////////////////////////////////
// Created: 19.03.2025
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_render_pipeline.h"
////////////////////////////////////////////////////////////////////////////////
void CRender::render_stage_main_geometry()
{
	OPTICK_EVENT("CRender::render_stage_main_geometry");

	RenderBackend.set_ColorWriteEnable(TRUE);

	r_pmask(true, false, true); // enable priority "0",+ capture wmarks

	//if (m_need_render_sun)
	//	set_Recorder(&main_coarse_structure);
	//else
		set_Recorder(NULL);

	set_active_phase(PHASE_NORMAL);
	render_main(Device.mFullTransform, true);
	set_Recorder(NULL);
	r_pmask(true, false); // disable priority "1"

	RenderBackend.enable_anisotropy_filtering();

	RenderBackend.set_Render_Target_Surface(RenderTarget->rt_Generic[0]);
	RenderBackend.set_Depth_Buffer(HW.pBaseZB);

	// Stencil - write 0x1 at pixel pos
	RenderBackend.set_Stencil(TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);

	// Misc	- draw only front-faces
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, FALSE));

	// Set backface culling
	RenderBackend.set_CullMode(CULL_BACKFACE);

	RenderBackend.set_ColorWriteEnable();
	//RenderTarget->set_gbuffer();

	//CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL));

	if (psDeviceFlags.test(rsWireframe))
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));

	r_dsgraph_render_graph(0);

	if (Details)
	{
		// Details->UpdateVisibleM();
		Details->Render();
	}

	if (psDeviceFlags.test(rsWireframe))
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID));

	RenderBackend.disable_anisotropy_filtering();
}

void CRender::RenderDebug()
{
	ViewBase.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);

	HOM.Enable();
	HOM.Render(ViewBase);

	render_stage_main_geometry();

	output_frame_to_screen();
}
////////////////////////////////////////////////////////////////////////////////
