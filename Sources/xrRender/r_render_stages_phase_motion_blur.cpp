///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Blender_motion_blur.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::motion_blur_pass_prepare_dilation_map()
{
	////OPTICK_EVENT("CRenderTarget::motion_blur_pass_prepare_dilation_map");

	// (new-camera) -> (world) -> (old_viewproj)
	fmat4x4 m_previous, m_current, m_invview;
	m_invview.invert(Engine.RenderView.View);
	m_previous.mul(RenderImplementation.m_saved_viewproj, m_invview);
	m_current.set(Engine.RenderView.Project);

	RenderBackendLegacy.set_CullMode(CULL_DISABLE);
	RenderBackendLegacy.set_Stencil(FALSE);

	float w = float(Device.dwWidth * 0.5f);
	float h = float(Device.dwHeight * 0.5f);

	// 1. Создаем карту векторов
	RenderBackendLegacy.set_Element(RenderTarget->s_motion_blur->E[SE_PASS_PREPARE_DILATION_MAP]);
	// ВАЖНО: Уменьшил силу в коде, так как новая формула точнее. Подберите значение по вкусу (например 0.5 - 1.5)
	RenderBackendLegacy.set_Constant("m_blur_power", ps_r_mblur);
	RenderBackendLegacy.set_Constant("m_current", m_current);
	RenderBackendLegacy.set_Constant("m_previous", m_previous);
	RenderBackendLegacy.RenderViewportSurface(w, h, RenderTarget->rt_Motion_Blur_Dilation_Map_0);

	// 2. Сглаживаем карту векторов (убирает шум в векторах)
	// Можно оставить 1 итерацию
	RenderBackendLegacy.set_Element(RenderTarget->s_motion_blur->E[SE_PASS_BLUR_DILATION_MAP], 0);
	RenderBackendLegacy.set_Constant("image_resolution", w, h, 1.0f / w, 1.0f / h);
	RenderBackendLegacy.RenderViewportSurface(w, h, RenderTarget->rt_Motion_Blur_Dilation_Map_1);

	RenderBackendLegacy.set_Element(RenderTarget->s_motion_blur->E[SE_PASS_BLUR_DILATION_MAP], 1);
	RenderBackendLegacy.set_Constant("image_resolution", w, h, 1.0f / w, 1.0f / h);
	RenderBackendLegacy.RenderViewportSurface(w, h, RenderTarget->rt_Motion_Blur_Dilation_Map_0);
}

void CRender::motion_blur_pass_blur()
{
	////OPTICK_EVENT("CRenderTarget::motion_blur_pass_blur");

	RenderBackendLegacy.set_CullMode(CULL_DISABLE);
	RenderBackendLegacy.set_Stencil(FALSE);

	RenderBackendLegacy.CopyViewportSurface(RenderTarget->rt_Generic[1], RenderTarget->rt_Generic[0]);
	RenderBackendLegacy.set_Element(RenderTarget->s_motion_blur->E[SE_PASS_BLUR_FRAME]);
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Generic[1]);
}

void CRender::motion_blur_pass_save_depth()
{
	////OPTICK_EVENT("CRenderTarget::motion_blur_pass_save_depth");

	RenderBackendLegacy.set_CullMode(CULL_DISABLE);
	RenderBackendLegacy.set_Stencil(FALSE);

	RenderBackendLegacy.set_Element(RenderTarget->s_motion_blur->E[SE_PASS_SAVE_DEPTH_BUFFER]);
	RenderBackendLegacy.RenderViewportSurface(RenderTarget->rt_Motion_Blur_Previous_Frame_Depth);
}

void CRender::render_motion_blur()
{
	////OPTICK_EVENT("CRenderTarget::render_motion_blur");

	// Важно: Порядок вызовов.
	// 1. Сначала считаем векторы (нужен текущий depth и предыдущий depth)
	motion_blur_pass_prepare_dilation_map();

	// 2. Размываем картинку
	motion_blur_pass_blur();

	// 3. Сохраняем текущий depth для следующего кадра
	motion_blur_pass_save_depth();
}
///////////////////////////////////////////////////////////////////////////////////