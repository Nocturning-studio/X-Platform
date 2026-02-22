///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Blender_depth_of_field.h"

// Константы для расчета оптики (35mm Full Frame сенсор)
constexpr double SENSOR_DIAGONAL = 43.266615300557;
constexpr float FOCAL_DEPTH_MUL = 1000.0f; // Перевод игровых единиц в метры/миллиметры

// Вспомогательная функция для расчета фокусного расстояния из FOV
double fov_to_length(double fov)
{
	// Защита от некорректных углов
	if (fov < 1.0 || fov > 179.0)
		return 35.0;

	return (SENSOR_DIAGONAL / (2.0 * tan(PI * fov / 360.0)));
}

void CRender::render_depth_of_field()
{
	////OPTICK_EVENT("CRender::render_depth_of_field");

	RenderBackendLegacy.CopyViewportSurface(RenderTarget->rt_Generic[1], RenderTarget->rt_Generic[0]);

	// Params
	float3 DofParams;
	g_pGamePersistent->GetCurrentDof(DofParams);
	float FocusDist = DofParams.x * FOCAL_DEPTH_MUL;
	float FocalLen = (float)fov_to_length(Engine.RenderView.Fov);

	if (FocalLen < 10.0f)
		FocalLen = 35.0f;
	if (FocusDist < FocalLen + 10.0f)
		FocusDist = FocalLen + 10.0f;

	float FStop = (DofParams.z < 0.1f) ? 1.4f : DofParams.z;
	float Aperture = FocalLen / FStop;
	float SensorHeight = 24.0f;
	float PPM = (float(Device.dwHeight) / SensorHeight);

	RenderBackendLegacy.set_CullMode(CULL_DISABLE);
	RenderBackendLegacy.set_Stencil(FALSE);

	// PHASE 1: Calc CoC
	RenderBackendLegacy.set_Element(RenderTarget->s_dof->E[SE_PASS_DOF_CALC_COC]);
	RenderBackendLegacy.set_Constant("dof_coc_params", FocusDist, FocalLen, Aperture, PPM);
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_dof_coc);

	// PHASE 2: Tile Dilation (Low Res)
	RenderBackendLegacy.set_Element(RenderTarget->s_dof->E[SE_PASS_DOF_TILE_DILATION]);
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_dof_dilation);

	// PHASE 3: Separate (Half Res)
	RenderBackendLegacy.set_Element(RenderTarget->s_dof->E[SE_PASS_DOF_SEPARATE]);
	RenderBackendLegacy.set_Constant("dof_layer_select", 0.0f, 0.0f, 0.0f, 0.0f);
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_dof_far);

	RenderBackendLegacy.set_Element(RenderTarget->s_dof->E[SE_PASS_DOF_SEPARATE]);
	RenderBackendLegacy.set_Constant("dof_layer_select", 1.0f, 0.0f, 0.0f, 0.0f);
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_dof_near);

	// PHASE 4: Blur Far
	RenderBackendLegacy.set_Element(RenderTarget->s_dof->E[SE_PASS_DOF_BLUR_FAR]);
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Generic[1]);
	RenderBackendLegacy.CopyViewportSurface(RenderTarget->rt_Generic[1], RenderTarget->rt_dof_far);

	// PHASE 5: Blur Near (с Dilation map)
	RenderBackendLegacy.set_Element(RenderTarget->s_dof->E[SE_PASS_DOF_BLUR_NEAR]);
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Generic[1]);
	RenderBackendLegacy.CopyViewportSurface(RenderTarget->rt_Generic[1], RenderTarget->rt_dof_near);

	// PHASE 6: Composite
	RenderBackendLegacy.set_Element(RenderTarget->s_dof->E[SE_PASS_DOF_COMPOSITE]);
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Generic[1]);
}
