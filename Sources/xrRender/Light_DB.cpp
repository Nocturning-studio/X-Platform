#include "StdAfx.h"
#include "..\xrEngine\xrLevel.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
#include "light_db.h"

CLight_DB::CLight_DB()
{
}

CLight_DB::~CLight_DB()
{
}

void CLight_DB::Load(IReader* fs)
{
	IReader* F = 0;

	// Lights itself
	sun_original = NULL;
	sun_adapted = NULL;
	{
		F = fs->open_chunk(fsL_LIGHT_DYNAMIC);

		u32 size = F->length();
		u32 element = sizeof(Flight) + 4;
		u32 count = size / element;
		VERIFY(count * element == size);
		v_static.reserve(count);
		for (u32 i = 0; i < count; i++)
		{
			Flight Ldata;
			light* L = Create();
			L->m_LightFlags.bStatic = true;
			L->set_type(IRender_Light::POINT);
			L->set_shadow(true);

			u32 controller = 0;
			F->r(&controller, 4);
			F->r(&Ldata, sizeof(Flight));
			if (Ldata.type == D3DLIGHT_DIRECTIONAL)
			{
				Fvector tmp_R;
				tmp_R.set(1, 0, 0);

				// directional (base)
				sun_original = L;
				L->set_type(IRender_Light::DIRECT);
				L->set_shadow(true);
				L->set_rotation(Ldata.direction, tmp_R);

				// copy to env-sun
				sun_adapted = L = Create();
				L->m_LightFlags.bStatic = true;
				L->set_type(IRender_Light::DIRECT);
				L->set_shadow(true);
				L->set_rotation(Ldata.direction, tmp_R);
			}
			else
			{
				Fvector tmp_D, tmp_R;
				tmp_D.set(0, 0, -1); // forward
				tmp_R.set(1, 0, 0);	 // right

				// point
				v_static.push_back(L);
				L->set_position(Ldata.position);
				L->set_rotation(tmp_D, tmp_R);
				L->set_range(Ldata.range);
				L->set_color(Ldata.diffuse);
				L->set_active(true);
				//				R_ASSERT			(L->spatial.sector	);
			}
		}

		F->close();
	}
	if(!sun_original && !sun_adapted)
		Msg("Where is sun?");
}

void CLight_DB::Unload()
{
	v_static.clear();
	sun_original.destroy();
	sun_adapted.destroy();
}

light* CLight_DB::Create()
{
	light* L = xr_new<light>();
	L->m_LightFlags.bStatic = false;
	L->m_LightFlags.bActive = false;
	L->m_LightFlags.bShadow = true;
	return L;
}

void CLight_DB::add_light(light* L)
{
	if (Engine.TimeManager.GetFrameCount() == L->get_frame_render())
		return;
	L->set_frame_render(Engine.TimeManager.GetFrameCount());
	if (RenderImplementation.o.noshadows)
		L->m_LightFlags.bShadow = FALSE;
	if (L->m_LightFlags.bStatic && !ps_r_lighting_flags.test(RFLAG_R1LIGHTS))
		return;
	if (Engine.RenderView.Position.distance_to(L->spatial.sphere.P) > ps_r_ls_far)
		return;
	L->_export(package);
}

void CLight_DB::Update()
{
	// set sun params
	if (sun_original && sun_adapted)
	{
		light* _sun_original = (light*)sun_original._get();
		light* _sun_adapted = (light*)sun_adapted._get();
		CEnvDescriptor* E = g_pGamePersistent->Environment().CurrentEnv;
//		VERIFY(_valid(E.sun_dir));
//#ifdef DEBUG
//		if (E.sun_dir.y >= 0)
//		{
//			Log("sect_name", E->sect_name.c_str());
//			Log("E.sun_dir", E->sun_dir);
//			Log("E.wind_direction", E->wind_direction);
//			Log("E.wind_strength", E->wind_strength);
//			Log("E.sun_color", E->sun_color);
//			Log("E.rain_color", E->rain_color);
//			Log("E.rain_density", E->rain_density);
//			Log("E.fog_density", E->fog_density);
//			Log("E.fog_color", E->fog_color);
//			Log("E.far_plane", E->far_plane);
//			Log("E.sky_rotation", E->sky_rotation);
//			Log("E.sky_color", E->sky_color);
//		}
//#endif
		VERIFY2(E->sun_dir.y < 0, "Invalid sun direction settings in evironment-config");
		Fvector OD, OP, AD, AP;
		OD.set(E->sun_dir).normalize();
		OP.mad(Engine.RenderView.Position, OD, -500.f);
		AD.set(0, -.75f, 0).add(E->sun_dir);

		// for some reason E.sun_dir can point-up
		int counter = 0;
		while (AD.magnitude() < 0.001 && counter < 10)
		{
			AD.add(E->sun_dir);
			counter++;
		}
		AD.normalize();
		AP.mad(Engine.RenderView.Position, AD, -500.f);
		sun_original->set_rotation(OD, _sun_original->get_right());
		sun_original->set_position(OP);
		sun_original->set_color(E->sun_color.x, E->sun_color.y, E->sun_color.z);
		sun_original->set_range(600.f);
		sun_adapted->set_rotation(OD, _sun_adapted->get_right());
		sun_adapted->set_position(OP);
		sun_adapted->set_color(E->sun_color.x * ps_r_sun_lumscale, E->sun_color.y * ps_r_sun_lumscale,
							   E->sun_color.z * ps_r_sun_lumscale);
		sun_adapted->set_range(600.f);
	}

	// Clear selection
	package.clear();
}
