#include "stdafx.h"
#include "LightTrack.h"
#include "..\xrEngine\xr_object.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"

// Оптимизированные направления гемисферы из X-Ray 1.6
const float hdir[lt_hemisamples][3] = {
	{-0.26287f, 0.52573f, 0.80902f},  {0.27639f, 0.44721f, 0.85065f},	{-0.95106f, 0.00000f, 0.30902f},
	{-0.95106f, 0.00000f, -0.30902f}, {0.58779f, 0.00000f, -0.80902f},	{0.58779f, 0.00000f, 0.80902f},
	{-0.00000f, 0.00000f, 1.00000f},  {0.52573f, 0.85065f, 0.00000f},	{-0.26287f, 0.52573f, -0.80902f},
	{-0.42533f, 0.85065f, 0.30902f},  {0.95106f, 0.00000f, 0.30902f},	{0.95106f, 0.00000f, -0.30902f},
	{0.00000f, 1.00000f, 0.00000f},	  {-0.58779f, 0.00000f, 0.80902f},	{-0.72361f, 0.44721f, 0.52573f},
	{-0.72361f, 0.44721f, -0.52573f}, {-0.58779f, 0.00000f, -0.80902f}, {0.16246f, 0.85065f, -0.50000f},
	{0.89443f, 0.44721f, 0.00000f},	  {-0.85065f, 0.52573f, -0.00000f}, {0.16246f, 0.85065f, 0.50000f},
	{0.68819f, 0.52573f, -0.50000f},  {0.27639f, 0.44721f, -0.85065f},	{0.00000f, 0.00000f, -1.00000f},
	{-0.42533f, 0.85065f, -0.30902f}, {0.68819f, 0.52573f, 0.50000f}};

CROS_impl::CROS_impl()
{
	// Инициализация значений HEMI
	hemi_value = 0.5f;
	hemi_smooth = 0.5f;

	// Инициализация массивов
	for (int i = 0; i < lt_hemisamples; i++)
	{
		result[i] = false;
	}
	for (size_t i = 0; i < NUM_FACES; i++)
	{
		hemi_cube[i] = 0.0f;
		hemi_cube_smooth[i] = 0.0f;
	}

	// Инициализация управления обновлением
	dwFrame = u32(-1);
	dwFrameSmooth = u32(-1);
	result_count = 0;
	result_iterator = 0;
	result_frame = u32(-1);
	ticks_to_update = 0;
	sky_rays_uptodate = 0;
	last_position.set(0, 0, 0);

	MODE = IRender_ObjectSpecific::TRACE_HEMI;
}

inline void CROS_impl::accum_hemi(float* hemi_cube, fvec3& dir, float scale)
{
	if (dir.x > 0)
		hemi_cube[CUBE_FACE_POS_X] += dir.x * scale;
	else
		hemi_cube[CUBE_FACE_NEG_X] -= dir.x * scale;

	if (dir.y > 0)
		hemi_cube[CUBE_FACE_POS_Y] += dir.y * scale;
	else
		hemi_cube[CUBE_FACE_NEG_Y] -= dir.y * scale;

	if (dir.z > 0)
		hemi_cube[CUBE_FACE_POS_Z] += dir.z * scale;
	else
		hemi_cube[CUBE_FACE_NEG_Z] -= dir.z * scale;
}

void CROS_impl::smart_update(IRenderable* O)
{
	PROFILE_FUNCTION();

	if (!O)
		return;
	if (0 == O->renderable.visual)
		return;

	--ticks_to_update;

	// Получение текущей позиции
	fvec3 position;
	O->renderable.transform.transform_tiny(position, O->renderable.visual->vis.sphere.P);

	if (ticks_to_update <= 0)
	{
		update(O);
		last_position = position;

		if (result_count < lt_hemisamples)
			ticks_to_update = ::Random.randI(1, 2);
		else if (sky_rays_uptodate < lt_hemisamples)
			ticks_to_update = ::Random.randI(3, 7);
		else
			ticks_to_update = ::Random.randI(1000, 2001);
	}
	else
	{
		if (!last_position.similar(position, 0.15f))
		{
			sky_rays_uptodate = 0;
			update(O);
			last_position = position;

			if (result_count < lt_hemisamples)
				ticks_to_update = ::Random.randI(1, 2);
			else
				ticks_to_update = ::Random.randI(3, 7);
		}
	}
}

void CROS_impl::calc_sky_hemi_value(fvec3& position, CObject* _object)
{
	// hemi-tracing
	sky_rays_uptodate += ps_r_dhemi_count;
	sky_rays_uptodate = _min(sky_rays_uptodate, lt_hemisamples);

	for (u32 it = 0; it < (u32)ps_r_dhemi_count; it++)
	{
		u32 sample = 0;
		if (result_count < lt_hemisamples)
		{
			sample = result_count;
			result_count++;
		}
		else
		{
			sample = (result_iterator % lt_hemisamples);
			result_iterator++;
		}

		// take sample
		fvec3 direction;
		direction.set(hdir[sample][0], hdir[sample][1], hdir[sample][2]).normalize();
		result[sample] =
			!g_pGameLevel->ObjectSpace.RayTest(position, direction, 50.f, collide::rqtStatic, &cache[sample], _object);
	}

	// Расчет значения гемисферы
	int _pass = 0;
	for (int it = 0; it < result_count; it++)
		if (result[it])
			_pass++;

	hemi_value = float(_pass) / float(result_count ? result_count : 1);
	hemi_value *= ps_r_dhemi_sky_scale;

	// Накопление в кубические грани для каждого успешного сэмпла
	for (int it = 0; it < result_count; it++)
	{
		if (result[it])
		{
			fvec3 dir;
			dir.set(hdir[it][0], hdir[it][1], hdir[it][2]);
			accum_hemi(hemi_cube, dir, ps_r_dhemi_sky_scale);
		}
	}
}

void CROS_impl::update(IRenderable* O)
{
	// clip & verify
	if (dwFrame == Engine.TimeManager.GetFrameCount())
		return;
	dwFrame = Engine.TimeManager.GetFrameCount();
	if (0 == O)
		return;
	if (0 == O->renderable.visual)
		return;

	CObject* _object = dynamic_cast<CObject*>(O);

	// select sample, randomize position inside object
	fvec3 position;
	O->renderable.transform.transform_tiny(position, O->renderable.visual->vis.sphere.P);
	float radius = O->renderable.visual->vis.sphere.R;
	position.y += .3f * radius;

	// Инициализация кубических граней
	for (size_t i = 0; i < NUM_FACES; ++i)
	{
		hemi_cube[i] = 0;
	}

	bool bFirstTime = (0 == result_count);

	// Основной расчет HEMI
	calc_sky_hemi_value(position, _object);

	// Сглаживание HEMI при первом обновлении
	if (bFirstTime)
	{
		hemi_smooth = hemi_value;
		CopyMemory(hemi_cube_smooth, hemi_cube, NUM_FACES * sizeof(float));
	}
}

extern float ps_r_lt_smooth;

void CROS_impl::update_smooth(IRenderable* O)
{
	PROFILE_FUNCTION();

	if (dwFrameSmooth == Engine.TimeManager.GetFrameCount())
		return;
	dwFrameSmooth = Engine.TimeManager.GetFrameCount();

	// Используем умное обновление для HEMI
	smart_update(O);

	float l_f = Engine.TimeManager.GetDeltaTime() * ps_r_lt_smooth;
	clamp(l_f, 0.f, 1.f);
	float l_i = 1.f - l_f;
	hemi_smooth = hemi_value * l_f + hemi_smooth * l_i;

	// Сглаживание кубических граней
	for (size_t i = 0; i < NUM_FACES; ++i)
	{
		hemi_cube_smooth[i] = hemi_cube[i] * l_f + hemi_cube_smooth[i] * l_i;
	}
}

float CROS_impl::get_hemi()
{
	if (dwFrameSmooth != Engine.TimeManager.GetFrameCount())
		update_smooth();
	return hemi_smooth;
}

const float* CROS_impl::get_hemi_cube()
{
	if (dwFrameSmooth != Engine.TimeManager.GetFrameCount())
		update_smooth();
	return hemi_cube_smooth;
}
