#include "StdAfx.h"
#include "light.h"
#include "../xrCDB/cl_intersect.h"

const u32 delay_small_min = 1;
const u32 delay_small_max = 3;
const u32 delay_large_min = 10;
const u32 delay_large_max = 20;
const u32 cullfragments = 4;

bool light::vis_prepare_async(u32 frame)
{
	PROFILE_FUNCTION();

	if (frame < VisibilityData.frame2test)
		return false; // не время тестировать

	// Быстрая проверка: камера внутри объёма?
	const float base_safe_area = 20;
	float safe_area = base_safe_area;
	{
		float a0 = deg2rad(Engine.RenderView.Fov * Engine.RenderView.Aspect / 2.f);
		float a1 = deg2rad(Engine.RenderView.Fov / 2.f);
		float x0 = base_safe_area / _cos(a0);
		float x1 = base_safe_area / _cos(a1);
		float c = _sqrt(x0 * x0 + x1 * x1);
		safe_area = _max(_max(base_safe_area, _max(x0, x1)), c);
	}

	bool skiptest = false;
	if (ps_r_lighting_flags.test(RFLAG_EXP_DONT_TEST_UNSHADOWED) && !LightFlags.bShadow)
		skiptest = true;

	if (skiptest || Engine.RenderView.Position.distance_to(spatial.sphere.P) <= (spatial.sphere.R * 1.01f + safe_area))
	{
		VisibilityData.visible = true;
		VisibilityData.pending = false;
		VisibilityData.frame2test = frame + ::Random.randI(delay_small_min, delay_small_max);
		return false; // не нужен occlusion query
	}

	// Нужен occlusion query – подготавливаем трансформацию
	VisibilityData.pending = true;
	transform_calc();
	return true; // нужен вызов occq_begin
}

void light::vis_prepare()
{
	PROFILE_FUNCTION();

	u32 frame = Engine.TimeManager.GetFrameCount();
	if (!vis_prepare_async(frame))
		return;

	// Только если нужен occlusion query – выполняем D3D-вызовы
	RenderBackendLegacy.set_transform_world(m_transform);
	const u32 order = RenderImplementation.occq_begin(VisibilityData.query_id);
	if (order == 0 || VisibilityData.query_id == 0xffffffff)
	{
		VisibilityData.visible = true;
		VisibilityData.pending = false;
		VisibilityData.frame2test = frame + ::Random.randI(delay_small_min, delay_small_max);
		return;
	}
	VisibilityData.query_order = order;
	RenderImplementation.draw_volume(this);
	RenderImplementation.occq_end(VisibilityData.query_id);
}

void light::vis_update()
{
	if (!VisibilityData.pending)
		return;

	const u32 frame = Engine.TimeManager.GetFrameCount();
	const u32 fragments = RenderImplementation.occq_get(VisibilityData.query_id, false);

	if (fragments == 0xfffffffe)
	{
		// Данные ещё не готовы, ждём следующего кадра
		return;
	}

	// Данные получены (или ошибка/таймаут)
	VisibilityData.visible = (fragments > cullfragments);
	VisibilityData.pending = false;

	if (VisibilityData.visible)
		VisibilityData.frame2test = frame + ::Random.randI(delay_large_min, delay_large_max);
	else
		VisibilityData.frame2test = frame + 1;
}
