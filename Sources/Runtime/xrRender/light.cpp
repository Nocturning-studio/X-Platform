#include "StdAfx.h"
#include "light.h"

light::light(void) : ISpatial(g_SpatialSpace)
{
	spatial.type = STYPE_LIGHTSOURCE;
	LightFlags.type = POINT;
	LightFlags.bStatic = false;
	LightFlags.bActive = false;
	LightFlags.bShadow = false;
	position.set(0, -1000, 0);
	direction.set(0, -1, 0);
	right.set(0, 0, 0);
	range = 8.f;
	cone = deg2rad(60.f);
	color.set(1, 1, 1, 1);

	frame_render = 0;

	ZeroMemory(omnipart, sizeof(omnipart));
	s_spot = NULL;
	s_point = NULL;
	VisibilityData.frame2test = 0; // xffffffff;
	VisibilityData.query_id = 0;
	VisibilityData.query_order = 0;
	VisibilityData.visible = true;
	VisibilityData.pending = false;
	m_sectors = {};
}

void light::TryToDeactivateLight()
{
	try
	{
		set_active(false);
	}
	catch (...)
	{
		Msg("! Failed to deactivate light!");
	}
}

light::~light()
{
	for (int f = 0; f < 6; f++)
		xr_delete(omnipart[f]);

	TryToDeactivateLight();

	// remove from Lights_LastFrame
	for (u32 it = 0; it < RenderImplementation.Lights_LastFrame.size(); it++)
		if (this == RenderImplementation.Lights_LastFrame[it])
			RenderImplementation.Lights_LastFrame[it] = 0;

	m_sectors.clear();
}

void light::set_texture(LPCSTR name)
{
	if ((0 == name) || (0 == name[0]))
	{
		// default shaders
		s_spot.destroy();
		s_point.destroy();
		return;
	}

#pragma todo("Only shadowed spot implements projective texture")
	string256 temp;
	s_spot.create(RenderImplementation.RenderTarget->b_accum_spot, strconcat(sizeof(temp), temp, "r\\accum_spot_", name), name);
	s_spot.create(RenderImplementation.RenderTarget->b_accum_spot, strconcat(sizeof(temp), temp, "r\\accum_spot_", name), name);
}

void light::set_shadow(bool b)
{
	LightFlags.bShadow = b;

	if (LightFlags.type == IRender_Light::POINT)
	{
		if (LightFlags.bShadow)
		{
			// tough: create 6 shadowed lights
			if (0 == omnipart[0])
			{
				for (int f = 0; f < 6; f++)
					omnipart[f] = xr_new<light>();
			}
		}
		else
		{
			// tough: delete 6 shadowed lights
			if (0 != omnipart[0])
			{
				for (int f = 0; f < 6; f++)
					xr_delete(omnipart[f]);
			}
		}
	}
}

void light::get_sectors()
{
	if (0 == spatial.sector)
		spatial_updatesector();

	CSector* sector = (CSector*)spatial.sector;
	if (0 == sector)
		return;

	if (LightFlags.type == IRender_Light::SPOT || LightFlags.type == IRender_Light::OMNIPART)
	{
		CFrustum temp = CFrustum();
		temp.CreateFromMatrix(TransformContext.ShadowContext.combine, FRUSTUM_P_ALL);

		//m_sectors = RenderImplementation.detectSectors_frustum(sector, &temp);
	}
	if (LightFlags.type == IRender_Light::POINT)
	{
		//m_sectors = RenderImplementation.detectSectors_sphere(sector, position, fvec3().set(range, range, range));
	}
}

void light::set_active(bool a)
{
	// Добавляем простую блокировку для потокобезопасности
	static std::mutex active_mutex;
	std::lock_guard<std::mutex> lock(active_mutex);

	if (a)
	{
		if (LightFlags.bActive)
			return;

		// Проверка валидности позиции
		fvec3 zero = {0, -1000, 0};
		if (position.similar(zero, EPS_L))
		{
			DbgMsg("! [Warning] Trying to activate light with uninitialized position.");
			//flags.bActive = false;
			//return;
		}

		LightFlags.bActive = true;

		// Проверяем, что свет правильно инициализирован
		if (spatial.sector == nullptr)
		{
			spatial_updatesector();
		}

		spatial_register();
		spatial_move();
	}
	else
	{
		if (!LightFlags.bActive)
			return;
		LightFlags.bActive = false;
		spatial_move();
		spatial_unregister();
	}
}

void light::set_position(const fvec3& P)
{
	float eps = EPS_L;
	if (position.similar(P, eps))
	{
		DbgMsg("~ [Debug] set_position skipped - same position");
		return;
	}

	DbgMsg("~ [Debug] set_position called: from (%.1f,%.1f,%.1f) to (%.1f,%.1f,%.1f)", position.x, position.y,
		   position.z,
		P.x, P.y, P.z);

	position.set(P);

	spatial_move();
}

void light::set_range(float R)
{
	float eps = _max(range * 0.1f, EPS_L);
	if (fsimilar(range, R, eps))
		return;
	range = R;
	spatial_move();
};

void light::set_cone(float angle)
{
	if (fsimilar(cone, angle))
		return;
	VERIFY(cone < deg2rad(121.f)); // 120 is hard limit for lights
	cone = angle;
	spatial_move();
}
void light::set_rotation(const fvec3& D, const fvec3& R)
{
	fvec3 old_D = direction;
	direction.normalize(D);
	right.normalize(R);
	if (!fsimilar(1.f, old_D.dotproduct(D)))
		spatial_move();
}

void light::spatial_move()
{
	DbgMsg("~ [Debug] spatial_move called for light at (%.1f,%.1f,%.1f)", position.x, position.y, position.z);

	// Проверка валидности позиции перед обновлением
	fvec3 zero = {0, -1000, 0};
	if (position.similar(zero, EPS_L))
	{
		DbgMsg("! [Warning] light::spatial_move called with uninitialized position");
		//return;
	}

	if (RenderImplementation.Sectors.size() > 1)
		get_sectors();

	switch (LightFlags.type)
	{
	case IRender_Light::REFLECTED:
	case IRender_Light::POINT: {
		spatial.sphere.set(position, range);
	}
	break;
	case IRender_Light::SPOT: {
		// minimal enclosing sphere around cone
		VERIFY2(cone < deg2rad(121.f), "Too large light-cone angle. Maybe you have passed it in 'degrees'?");
		if (cone >= PI_DIV_2)
		{
			// obtused-angled
			spatial.sphere.P.mad(position, direction, range);
			spatial.sphere.R = range * tanf(cone / 2.f);
		}
		else
		{
			// acute-angled
			spatial.sphere.R = range / (2.f * _sqr(std::cos(cone / 2.f)));
			spatial.sphere.P.mad(position, direction, spatial.sphere.R);
		}
	}
	break;
	case IRender_Light::OMNIPART: {
		// is it optimal? seems to be...
		spatial.sphere.P.mad(position, direction, range);
		spatial.sphere.R = range;
	}
	break;
	}

	// update spatial DB
	ISpatial::spatial_move();

	svis.invalidate();

}

vis_data& light::get_homdata()
{
	// commit vis-data
	hom.sphere.set(spatial.sphere.P, spatial.sphere.R);
	hom.box.set(spatial.sphere.P, spatial.sphere.P);
	hom.box.grow(spatial.sphere.R);
	return hom;
};

fvec3 light::spatial_sector_point()
{
	return position;
}

//////////////////////////////////////////////////////////////////////////
// Transforms
void light::transform_calc()
{
	if (Engine.TimeManager.GetFrameCount() == m_transform_frame)
		return;
	m_transform_frame = Engine.TimeManager.GetFrameCount();

	// build final rotation / translation
	fvec3 L_dir, L_up, L_right;

	// dir
	L_dir.set(direction);
	float l_dir_m = L_dir.magnitude();
	if (_valid(l_dir_m) && l_dir_m > EPS_S)
		L_dir.div(l_dir_m);
	else
		L_dir.set(0, 0, 1);

	// R&N
	if (right.square_magnitude() > EPS)
	{
		// use specified 'up' and 'right', just enshure ortho-normalization
		L_right.set(right);
		L_right.normalize();
		L_up.crossproduct(L_dir, L_right);
		L_up.normalize();
		L_right.crossproduct(L_up, L_dir);
		L_right.normalize();
	}
	else
	{
		// auto find 'up' and 'right' vectors
		L_up.set(0, 1, 0);
		if (_abs(L_up.dotproduct(L_dir)) > .99f)
			L_up.set(0, 0, 1);
		L_right.crossproduct(L_up, L_dir);
		L_right.normalize();
		L_up.crossproduct(L_dir, L_right);
		L_up.normalize();
	}

	// matrix
	fmat4x4 mR;
	mR.i = L_right;
	mR._14 = 0;
	mR.j = L_up;
	mR._24 = 0;
	mR.k = L_dir;
	mR._34 = 0;
	mR.c = position;
	mR._44 = 1;

	// switch
	switch (LightFlags.type)
	{
	case IRender_Light::REFLECTED:
	case IRender_Light::POINT: {
		// scale of identity sphere
		float L_R = range;
		fmat4x4 mScale;
		mScale.scale(L_R, L_R, L_R);
		m_transform.mul_43(mR, mScale);
	}
	break;
	case IRender_Light::SPOT: {
		// scale to account range and angle
		float s = 2.f * range * tanf(cone / 2.f);
		fmat4x4 mScale;
		mScale.scale(s, s, range); // make range and radius
		m_transform.mul_43(mR, mScale);
	}
	break;
	case IRender_Light::OMNIPART: {
		float L_R = 2 * range; // volume is half-radius
		fmat4x4 mScale;
		mScale.scale(L_R, L_R, L_R);
		m_transform.mul_43(mR, mScale);
	}
	break;
	default:
		m_transform.identity();
		break;
	}
}

bool light::camera_inside_volume() const
{
	const fvec3& cam_pos = Engine.RenderView.Position;

#ifdef DEBUG_LIGHTS_CULLING
	Msg("[CPU-OCC] camera pos: pos=(%.1f,%.1f,%.1f)", cam_pos.x, cam_pos.y, cam_pos.x);
#endif

	switch (LightFlags.type)
	{
	case IRender_Light::POINT:
	case IRender_Light::OMNIPART:
	case IRender_Light::REFLECTED:
	{
		// Для point используем реальную позицию и радиус
		float dist = cam_pos.distance_to(position);
		bool inside = dist <= range;
#ifdef DEBUG_LIGHTS_CULLING
		Msg("[CPU-OCC] camera_inside_volume (POINT): pos=(%.1f,%.1f,%.1f) cam_dist=%.1f range=%.1f inside=%d",
			position.x, position.y, position.z, dist, range, inside);
#endif
		return inside;
	}

	case IRender_Light::SPOT:
	{
		// Проверяем расстояние до источника (позиция, не сфера!)
		float dist_to_source = cam_pos.distance_to(position);
		if (dist_to_source > range)
		{
#ifdef DEBUG_LIGHTS_CULLING
			Msg("[CPU-OCC] camera_inside_volume (SPOT): out of range (dist %.1f > range %.1f)", dist_to_source, range);
#endif
			return false;
		}

		// Вектор от источника к камере
		fvec3 dir_to_cam;
		dir_to_cam.sub(cam_pos, position).normalize_safe();

		// Угол между направлением света и направлением на камеру
		float cos_angle = dir_to_cam.dotproduct(direction);
		float half_cone = cone * 0.5f; // cone – полный угол в радианах
		bool inside_cone = cos_angle >= std::cos(half_cone);

#ifdef DEBUG_LIGHTS_CULLING
		Msg("[CPU-OCC] camera_inside_volume (SPOT): pos=(%.1f,%.1f,%.1f) dist=%.1f range=%.1f "
			"dir_to_cam=(%.2f,%.2f,%.2f) cos_angle=%.3f half_cone=%.3f cos_half=%.3f inside_cone=%d",
			position.x, position.y, position.z,
			dist_to_source, range,
			dir_to_cam.x, dir_to_cam.y, dir_to_cam.z,
			cos_angle, half_cone, std::cos(half_cone), inside_cone);
#endif
		return inside_cone;
	}

	default:
		return false;
	}
}

bool light::vis_prepare(u32 frame)
{
	if (frame < VisibilityData.frame2test)
		return false;

	// 1. Камера внутри объёма — видим без запроса
	if (camera_inside_volume())
	{
		VisibilityData.visible = true;
		VisibilityData.pending = false;
		VisibilityData.frame2test = frame + ::Random.randI(delay_small_min, delay_small_max);
		return false;
	}

	// 2. Камера рядом с источником — тоже считаем видимым
	//    Используем bounding‑сферу с запасом NEAR_LIGHT_RADIUS_BIAS.
	const float NEAR_LIGHT_RADIUS_BIAS = 2.0f;
	float dist_to_sphere = Engine.RenderView.Position.distance_to(spatial.sphere.P);
	if (dist_to_sphere <= spatial.sphere.R + NEAR_LIGHT_RADIUS_BIAS)
	{
		VisibilityData.visible = true;
		VisibilityData.pending = false;
		VisibilityData.frame2test = frame + ::Random.randI(delay_small_min, delay_small_max);
		return false;
	}

	// 3. Пропуск не-теневых источников (если флаг установлен)
	if (ps_r_lighting_flags.test(RFLAG_EXP_DONT_TEST_UNSHADOWED) && !LightFlags.bShadow)
	{
		VisibilityData.visible = true;
		VisibilityData.pending = false;
		VisibilityData.frame2test = frame + ::Random.randI(delay_small_min, delay_small_max);
		return false;
	}

	// 4. Требуется occlusion query
	VisibilityData.pending = true;
	transform_calc();
	return true;
}

//								+X,				-X,				+Y,				-Y,			+Z,				-Z
static fvec3 cmNorm[6] = {{0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, -1.f},
							{0.f, 0.f, 1.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}};
static fvec3 cmDir[6] = {{1.f, 0.f, 0.f},	 {-1.f, 0.f, 0.f}, {0.f, 1.f, 0.f},
						   {0.f, -1.f, 0.f}, {0.f, 0.f, 1.f},  {0.f, 0.f, -1.f}};

void light::_export(light_Package& package)
{
	if (LightFlags.bShadow)
	{
		switch (LightFlags.type)
		{
		case IRender_Light::POINT: {
			// tough: create/update 6 shadowed lights
			if (0 == omnipart[0])
				for (int f = 0; f < 6; f++)
					omnipart[f] = xr_new<light>();
			for (int f = 0; f < 6; f++)
			{
				light* L = omnipart[f];
				fvec3 R;
				R.crossproduct(cmNorm[f], cmDir[f]);
				L->set_type(IRender_Light::OMNIPART);
				L->set_shadow(true);
				L->set_position(position);
				L->set_rotation(cmDir[f], R);
				L->set_cone(PI_DIV_2);
				L->set_range(range);
				L->set_color(color);
				L->spatial.sector = spatial.sector; //. dangerous?
				L->get_shader_spot() = s_spot;
				L->get_shader_point() = s_point;
				package.v_shadowed.push_back(L);
			}
		}
		break;
		case IRender_Light::SPOT:
			package.v_shadowed.push_back(this);
			break;
		}
	}
	else
	{
		switch (LightFlags.type)
		{
		case IRender_Light::POINT:
			package.v_point.push_back(this);
			break;
		case IRender_Light::SPOT:
			package.v_spot.push_back(this);
			break;
		}
	}
}

extern float r_ssaGLOD_start, r_ssaGLOD_end;
extern float ps_r_slight_fade;
float light::get_LOD()
{
	if (!LightFlags.bShadow)
		return 1;
	float distSQ = Engine.RenderView.Position.distance_to_sqr(spatial.sphere.P) + EPS;
	float ScreenSpaceArea = ps_r_slight_fade * spatial.sphere.R / distSQ;
	float lod = std::sqrt(clampr((ScreenSpaceArea - r_ssaGLOD_end) / (r_ssaGLOD_start - r_ssaGLOD_end), 0.f, 1.f));
	return lod;
}
