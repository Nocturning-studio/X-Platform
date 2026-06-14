#include "stdafx.h"
#pragma hdrstop

#pragma warning(push)
#pragma warning(disable : 4995)
#include <d3dx9.h>
#pragma warning(pop)

#include "ResourceManager.h"
#include "Blender_Recorder.h"
#include "Blender.h"

#include "../xrRender/r_color_converting.h"

#include "igame_persistent.h"
#include "environment.h"

// matrices
#define BIND_DECLARE(xf)                                                                                               \
	class cl_transform_##xf : public R_constant_setup                                                                      \
	{                                                                                                                  \
		virtual void setup(R_constant* C)                                                                              \
		{                                                                                                              \
			RenderBackendLegacy.transforms.set_c_##xf(C);                                                                               \
		}                                                                                                              \
	};                                                                                                                 \
	static cl_transform_##xf binder_##xf
BIND_DECLARE(World);
BIND_DECLARE(InvWorld);
BIND_DECLARE(View);
BIND_DECLARE(Project);
BIND_DECLARE(WorldView);
BIND_DECLARE(ViewProject);
BIND_DECLARE(WorldViewProject);

#define DECLARE_TREE_BIND(c)                                                                                           \
	class cl_tree_##c : public R_constant_setup                                                                        \
	{                                                                                                                  \
		virtual void setup(R_constant* C)                                                                              \
		{                                                                                                              \
			RenderBackendLegacy.tree.set_c_##c(C);                                                                                  \
		}                                                                                                              \
	};                                                                                                                 \
	static cl_tree_##c tree_binder_##c

DECLARE_TREE_BIND(m_transform_v);
DECLARE_TREE_BIND(m_transform);
DECLARE_TREE_BIND(consts);
DECLARE_TREE_BIND(c_scale);
DECLARE_TREE_BIND(c_bias);
DECLARE_TREE_BIND(c_sun);

class cl_InvView : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		fmat4x4 mInvV = fmat4x4().invert(RenderBackendLegacy.transforms.m_View);

		RenderBackendLegacy.set_Constant(C, mInvV);
	}
};
static cl_InvView binder_InvView;

class cl_texgen : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		fmat4x4 mTexgen;

		float _w = float(Device.dwWidth);
		float _h = float(Device.dwHeight);
		float o_w = (.5f / _w);
		float o_h = (.5f / _h);
		fmat4x4 mTexelAdjust = {0.5f, 0.0f, 0.0f, 0.0f, 0.0f,		-0.5f,		0.0f, 0.0f,
								0.0f, 0.0f, 1.0f, 0.0f, 0.5f + o_w, 0.5f + o_h, 0.0f, 1.0f};

		mTexgen.mul(mTexelAdjust, RenderBackendLegacy.transforms.m_WorldViewProject);

		RenderBackendLegacy.set_Constant(C, mTexgen);
	}
};
static cl_texgen binder_texgen;

class cl_VPtexgen : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		fmat4x4 mTexgen;

		float _w = float(Device.dwWidth);
		float _h = float(Device.dwHeight);
		float o_w = (.5f / _w);
		float o_h = (.5f / _h);
		fmat4x4 mTexelAdjust = {0.5f, 0.0f, 0.0f, 0.0f, 0.0f,		-0.5f,		0.0f, 0.0f,
								0.0f, 0.0f, 1.0f, 0.0f, 0.5f + o_w, 0.5f + o_h, 0.0f, 1.0f};

		mTexgen.mul(mTexelAdjust, RenderBackendLegacy.transforms.m_ViewProject);

		RenderBackendLegacy.set_Constant(C, mTexgen);
	}
};
static cl_VPtexgen binder_VPtexgen;

// fog-params
class cl_fog_params : public R_constant_setup
{
	u32 marker;
	fvec4 result;
	virtual void setup(R_constant* C)
	{
		if (marker != Engine.TimeManager.GetFrameCount())
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			result.set(sRgbToLinear(desc->fog_color.x), sRgbToLinear(desc->fog_color.y), sRgbToLinear(desc->fog_color.z),
					   desc->fog_density);
		}
		RenderBackendLegacy.set_Constant(C, result);
	}
};
static cl_fog_params binder_fog_params;

// fog-color
class cl_fog_color : public R_constant_setup
{
	u32 marker;
	fvec4 result;
	virtual void setup(R_constant* C)
	{
		if (marker != Engine.TimeManager.GetFrameCount())
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			result.set(sRgbToLinear(desc->fog_color.x), sRgbToLinear(desc->fog_color.y), sRgbToLinear(desc->fog_color.z), 0);
		}
		RenderBackendLegacy.set_Constant(C, result);
	}
};
static cl_fog_color binder_fog_color;

static class cl_fog_density final : public R_constant_setup
{
	u32 marker;
	fvec4 FogDensity;
	void setup(R_constant* C) override
	{
		if (marker != Engine.TimeManager.GetFrameCount())
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			FogDensity.set(desc->fog_density, 0, 0, 0);
		}
		RenderBackendLegacy.set_Constant(C, FogDensity);
	}
} binder_fog_density;

static class cl_fog_sky_influence final : public R_constant_setup
{
	u32 marker;
	fvec4 FogDensity;
	void setup(R_constant* C) override
	{
		if (marker != Engine.TimeManager.GetFrameCount())
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			FogDensity.set(desc->fog_sky_influence, 0, 0, 0);
		}
		RenderBackendLegacy.set_Constant(C, FogDensity);
	}
} binder_fog_sky_influence;

static class cl_vertical_fog_intensity final : public R_constant_setup
{
	u32 marker;
	fvec4 VerticalFogIntensity;
	void setup(R_constant* C) override
	{
		if (marker != Engine.TimeManager.GetFrameCount())
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			VerticalFogIntensity.set(desc->vertical_fog_intensity, 0, 0, 0);
		}
		RenderBackendLegacy.set_Constant(C, VerticalFogIntensity);
	}
} binder_vertical_fog_intensity;

static class cl_vertical_fog_density final : public R_constant_setup
{
	u32 marker;
	fvec4 VerticalFogDensity;
	void setup(R_constant* C) override
	{
		if (marker != Engine.TimeManager.GetFrameCount())
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			VerticalFogDensity.set(desc->vertical_fog_density, 0, 0, 0);
		}
		RenderBackendLegacy.set_Constant(C, VerticalFogDensity);
	}
} binder_vertical_fog_density;

static class cl_vertical_fog_height final : public R_constant_setup
{
	u32 marker;
	fvec4 VerticalFogHeight;
	void setup(R_constant* C) override
	{
		if (marker != Engine.TimeManager.GetFrameCount())
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			VerticalFogHeight.set(desc->vertical_fog_height, 0, 0, 0);
		}
		RenderBackendLegacy.set_Constant(C, VerticalFogHeight);
	}
} binder_vertical_fog_height;

static class cl_rain_density : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* E = g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E->rain_density;
		RenderBackendLegacy.set_Constant(C, fValue, fValue, fValue, 0);
	}
} binder_rain_density;

static class cl_far_plane : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* E = g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E->far_plane;
		RenderBackendLegacy.set_Constant(C, fValue, fValue, fValue, 0);
	}
} binder_far_plane;

static class cl_water_intensity : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* E = g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E->m_fWaterIntensity;
		RenderBackendLegacy.set_Constant(C, fValue, fValue, fValue, 0);
	}
} binder_water_intensity;

static class cl_pos_decompress_params : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		float VertTan = -1.0f * tanf(deg2rad(Engine.RenderView.Fov / 2.0f));
		float HorzTan = -VertTan / Engine.RenderView.Aspect;
		RenderBackendLegacy.set_Constant(C, HorzTan, VertTan, (2.0f * HorzTan) / (float)Device.dwWidth, (2.0f * VertTan) / (float)Device.dwHeight);
	}
} binder_pos_decompress_params;

extern ENGINE_API float psHUD_FOV;
static class cl_pos_decompress_params_hud : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		float VertTan = -1.0f * tanf(deg2rad(psHUD_FOV / 2.0f));
		float HorzTan = -VertTan / Engine.RenderView.Aspect;

		RenderBackendLegacy.set_Constant(C, HorzTan, VertTan, (2.0f * HorzTan) / Device.dwWidth, (2.0f * VertTan) / Device.dwHeight);
	}
} binder_pos_decompress_params_hud;

static class cl_fov : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RenderBackendLegacy.set_Constant(C, Engine.RenderView.Fov, 0, 0, 0);
	}
} binder_fov;

static class cl_sepia_params : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* E = g_pGamePersistent->Environment().CurrentEnv;
		fvec3 SepiaColor = E->m_SepiaColor;
		float SepiaPower = E->m_SepiaPower;
		RenderBackendLegacy.set_Constant(C, sRgbToLinear(SepiaColor.x), sRgbToLinear(SepiaColor.y), sRgbToLinear(SepiaColor.z), SepiaPower);
	}
} binder_sepia_params;

static class cl_vignette_power : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* E = g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E->m_VignettePower;
		RenderBackendLegacy.set_Constant(C, fValue, fValue, fValue, 0);
	}
} binder_vignette_power;

// times
class cl_times : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		float t = Engine.TimeManager.GetGlobalTime();
		RenderBackendLegacy.set_Constant(C, t, t * 10, t / 10, _sin(t));
	}
};
static cl_times binder_times;

// eye-params
class cl_eye_P : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		fvec3& V = Engine.RenderView.Position;
		RenderBackendLegacy.set_Constant(C, V.x, V.y, V.z, 1);
	}
};
static cl_eye_P binder_eye_P;

// eye-params
class cl_eye_D : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		fvec3& V = Engine.RenderView.Direction;
		RenderBackendLegacy.set_Constant(C, V.x, V.y, V.z, 0);
	}
};
static cl_eye_D binder_eye_D;

// eye-params
class cl_eye_N : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		fvec3& V = Engine.RenderView.Top;
		RenderBackendLegacy.set_Constant(C, V.x, V.y, V.z, 0);
	}
};
static cl_eye_N binder_eye_N;

// D-Light0
class cl_sun0_color : public R_constant_setup
{
	u32 marker;
	fvec4 result;
	virtual void setup(R_constant* C)
	{
		if (marker != Engine.TimeManager.GetFrameCount())
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			result.set(sRgbToLinear(desc->sun_color.x), sRgbToLinear(desc->sun_color.y), sRgbToLinear(desc->sun_color.z), 0);
		}
		RenderBackendLegacy.set_Constant(C, result);
	}
};
static cl_sun0_color binder_sun0_color;

static class cl_env_color : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptorMixer* envdesc = g_pGamePersistent->Environment().CurrentEnv;
		fvec4 envclr = {sRgbToLinear(envdesc->hemi_color.x) * 2 + EPS,sRgbToLinear( envdesc->hemi_color.y) * 2 + EPS,
						   sRgbToLinear(envdesc->hemi_color.z) * 2 + EPS, envdesc->weight};
		RenderBackendLegacy.set_Constant(C, envclr);
	}
} binder_env_color;

class cl_sun0_dir_w : public R_constant_setup
{
	u32 marker;
	fvec4 result;
	virtual void setup(R_constant* C)
	{
		if (marker != Engine.TimeManager.GetFrameCount())
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			result.set(desc->sun_dir.x, desc->sun_dir.y, desc->sun_dir.z, 0);
		}
		RenderBackendLegacy.set_Constant(C, result);
	}
};
static cl_sun0_dir_w binder_sun0_dir_w;
class cl_sun0_dir_e : public R_constant_setup
{
	u32 marker;
	fvec4 result;
	virtual void setup(R_constant* C)
	{
		if (marker != Engine.TimeManager.GetFrameCount())
		{
			fvec3 D;
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			Engine.RenderView.View.transform_dir(D, desc->sun_dir);
			D.normalize();
			result.set(D.x, D.y, D.z, 0);
		}
		RenderBackendLegacy.set_Constant(C, result);
	}
};
static cl_sun0_dir_e binder_sun0_dir_e;

//
class cl_amb_color : public R_constant_setup
{
	u32 marker;
	fvec4 result;
	virtual void setup(R_constant* C)
	{
		if (marker != Engine.TimeManager.GetFrameCount())
		{
			CEnvDescriptorMixer* desc = g_pGamePersistent->Environment().CurrentEnv;
			result.set(sRgbToLinear(desc->ambient.x), sRgbToLinear(desc->ambient.y), sRgbToLinear(desc->ambient.z), desc->weight);
		}
		RenderBackendLegacy.set_Constant(C, result);
	}
};
static cl_amb_color binder_amb_color;

class cl_ambient_brightness : public R_constant_setup
{
	u32 marker;
	fvec4 result;
	virtual void setup(R_constant* C)
	{
		if (marker != Engine.TimeManager.GetFrameCount())
		{
			CEnvDescriptorMixer* desc = g_pGamePersistent->Environment().CurrentEnv;
			result.set(desc->ambient_brightness, 0, 0, 0);
		}
		RenderBackendLegacy.set_Constant(C, result);
	}
};
static cl_ambient_brightness binder_ambient_brightness;

class cl_hemi_color : public R_constant_setup
{
	u32 marker;
	fvec4 result;
	virtual void setup(R_constant* C)
	{
		if (marker != Engine.TimeManager.GetFrameCount())
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			result.set(sRgbToLinear(desc->hemi_color.x), sRgbToLinear(desc->hemi_color.y), sRgbToLinear(desc->hemi_color.z), desc->hemi_color.w);
		}
		RenderBackendLegacy.set_Constant(C, result);
	}
};
static cl_hemi_color binder_hemi_color;

static class cl_screen_res : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RenderBackendLegacy.set_Constant(C, (float)Device.dwWidth, (float)Device.dwHeight, 1.0f / (float)Device.dwWidth, 1.0f / (float)Device.dwHeight);
	}
} binder_screen_res;

static class cl_InvProject final : public R_constant_setup
{
	void setup(R_constant* C) override
	{
		fmat4x4 m_invProject;
		m_invProject.invert(Engine.RenderView.Project);
		RenderBackendLegacy.set_Constant(C, m_invProject);
	}
} binder_InvProject;

class cl_wind_params : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;

		// Считаем 3D направление (Сферические координаты)
		// Yaw = wind_direction, Pitch = wind_tilt

		float yaw = desc->wind_direction;
		float pitch = desc->wind_tilt;

		// X-Ray использует Y-Up систему координат
		float dirX = _cos(yaw) * _cos(pitch); // X
		float dirY = _sin(pitch);			  // Y (Вертикаль)
		float dirZ = _sin(yaw) * _cos(pitch); // Z

		// W - передадим масштаб волны (Scale), если захотим, или оставим Strength
		// Но лучше Strength передавать отдельно, а тут вектор и Gusting

		RenderBackendLegacy.set_Constant(C, dirX, dirY, dirZ, desc->wind_gusting);
	}
};
static cl_wind_params binder_wind_params;

class cl_wind_turbulence : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;

		float intensity = desc->wind_turbulence;
		float velocity = desc->wind_velocity; // Берем скорость из конфига

		// Умножаем время на скорость из конфига
		// ВАЖНО: Просто умножать fTimeGlobal на velocity нельзя, если velocity меняется динамически (будут скачки).
		// Для идеальной плавности время нужно накапливать в CEnvironment::OnFrame:
		// fWindTime += Engine.TimeManager.GetDeltaTime() * current_velocity;
		// Но для простоты пока умножим, при плавном переходе погоды скачок будет сглажен интерполяцией.
		clamp(velocity, 0.0f, 0.5f);
		float anim_time = Engine.TimeManager.GetGlobalTime() * velocity;

		RenderBackendLegacy.set_Constant(C, intensity, desc->wind_turbulence, anim_time, desc->wind_strength);
	}
};
static cl_wind_turbulence binder_wind_turbulence;

// Standart constant-binding
void CBlender_Compile::SetMapping()
{
	// matrices
	set_Constant("m_World", &binder_World);
	set_Constant("m_invWorld", &binder_InvWorld);
	set_Constant("m_View", &binder_View);
	set_Constant("m_invView", &binder_InvView);
	set_Constant("m_Project", &binder_Project);
	set_Constant("m_invProject", &binder_InvProject);
	set_Constant("m_WorldView", &binder_WorldView);
	set_Constant("m_ViewProject", &binder_ViewProject);
	set_Constant("m_WorldViewProject", &binder_WorldViewProject);

	set_Constant("m_transform_v", &tree_binder_m_transform_v);
	set_Constant("m_transform", &tree_binder_m_transform);
	set_Constant("consts", &tree_binder_consts);
	set_Constant("c_scale", &tree_binder_c_scale);
	set_Constant("c_bias", &tree_binder_c_bias);
	set_Constant("c_sun", &tree_binder_c_sun);
	set_Constant("wind_params", &binder_wind_params);
	set_Constant("wind_turbulence", &binder_wind_turbulence);

	//	Igor	temp solution for the texgen functionality in the shader
	set_Constant("m_texgen", &binder_texgen);
	set_Constant("mVPTexgen", &binder_VPtexgen);

#ifndef _EDITOR
	// fog-params
	set_Constant("fog_params", &binder_fog_params);
	set_Constant("fog_color", &binder_fog_color);
	set_Constant("fog_density", &binder_fog_density);
	set_Constant("fog_sky_influence", &binder_fog_sky_influence);
	set_Constant("vertical_fog_intensity", &binder_vertical_fog_intensity);
	set_Constant("vertical_fog_density", &binder_vertical_fog_density);
	set_Constant("vertical_fog_height", &binder_vertical_fog_height);
#endif

	set_Constant("water_intensity", &binder_water_intensity);
	set_Constant("rain_density", &binder_rain_density);

	set_Constant("sepia_params", &binder_sepia_params);
	set_Constant("vignette_power", &binder_vignette_power);

	set_Constant("far_plane", &binder_far_plane);

	set_Constant("pos_decompression_params", &binder_pos_decompress_params);
	set_Constant("pos_decompression_params_hud", &binder_pos_decompress_params_hud);

	set_Constant("fov", &binder_fov);
	
	// env-params
	set_Constant("env_color", &binder_env_color);

	// time
	set_Constant("timers", &binder_times);

	// eye-params
	set_Constant("eye_position", &binder_eye_P);
	set_Constant("eye_direction", &binder_eye_D);
	set_Constant("eye_normal", &binder_eye_N);

#ifndef _EDITOR
	// global-lighting (env params)
	set_Constant("L_sun_color", &binder_sun0_color);
	set_Constant("L_sun_dir_w", &binder_sun0_dir_w);
	set_Constant("L_sun_dir_e", &binder_sun0_dir_e);
	set_Constant("L_hemi_color", &binder_hemi_color);
	set_Constant("L_ambient", &binder_amb_color);
	set_Constant("ambient_brightness", &binder_ambient_brightness);
#endif

	set_Constant("screen_res", &binder_screen_res);

	if (bDetail && bDetail_Diffuse && detail_scaler)
		set_Constant("dt_params", detail_scaler);

	// other common
	for (u32 it = 0; it < Engine.ResourceManager->v_constant_setup.size(); it++)
	{
		std::pair<shared_str, R_constant_setup*> cs = Engine.ResourceManager->v_constant_setup[it];
		set_Constant(*cs.first, cs.second);
	}
}
