#include "pch_script.h"
#include "script_render_device.h"
#include "../../xrEngine/Engine.h"	   // Обязательно: доступ к Engine
#include "../../xrEngine/RenderView.h" // Обязательно: доступ к структуре RenderView

using namespace luabind;

bool is_device_paused(CRenderDevice* d)
{
	return !!Device.Paused();
}

void set_device_paused(CRenderDevice* d, bool b)
{
	Device.Pause(b, TRUE, FALSE, "set_device_paused_script");
}

extern ENGINE_API BOOL g_appLoaded;
bool is_app_ready()
{
	return !!g_appLoaded;
}

// --- Helper Functions для TimeManager ---
u32 time_global(const CRenderDevice* self)
{
	return Engine.TimeManager.GetGlobalTimeMs();
}

u32 get_time_delta(const CRenderDevice* self)
{
	return Engine.TimeManager.GetDeltaTimeMs();
}

float get_f_time_delta(const CRenderDevice* self)
{
	return Engine.TimeManager.GetDeltaTime();
}

u32 get_frame(const CRenderDevice* self)
{
	return Engine.TimeManager.GetFrameCount();
}

// --- Helper Functions для RenderView (Камера) ---
// Принимаем CRenderDevice*, чтобы Luabind понял контекст "self",
// но данные берем из Engine.RenderView

const float3& get_cam_pos(const CRenderDevice* self)
{
	return Engine.RenderView.Position;
}

const float3& get_cam_dir(const CRenderDevice* self)
{
	return Engine.RenderView.Direction;
}

const float3& get_cam_top(const CRenderDevice* self)
{
	return Engine.RenderView.Top;
}

const float3& get_cam_right(const CRenderDevice* self)
{
	return Engine.RenderView.Right;
}

float get_fov(const CRenderDevice* self)
{
	return Engine.RenderView.Fov;
}

float get_aspect(const CRenderDevice* self)
{
	return Engine.RenderView.Aspect;
}
// ------------------------------------------------

#pragma optimize("s", on)
void CScriptRenderDevice::script_register(lua_State* L)
{
	module(L)[class_<CRenderDevice>("render_device")
				  .def_readonly("width", &CRenderDevice::dwWidth)
				  .def_readonly("height", &CRenderDevice::dwHeight)

				  // TimeManager getters
				  .property("time_delta", &get_time_delta)
				  .property("f_time_delta", &get_f_time_delta)
				  .property("frame", &get_frame)
				  .def("time_global", &time_global)

				  // RenderView (Camera) getters
				  // Используем .property(имя, геттер), так как данные лежат в другом месте
				  .property("cam_pos", &get_cam_pos)
				  .property("cam_dir", &get_cam_dir)
				  .property("cam_top", &get_cam_top)
				  .property("cam_right", &get_cam_right)
				  .property("fov", &get_fov)
				  .property("aspect_ratio", &get_aspect)

				  .def_readonly("precache_frame", &CRenderDevice::dwPrecacheFrame)

				  .def("is_paused", &is_device_paused)
				  .def("pause", &set_device_paused),
			  def("app_ready", &is_app_ready)];
}
