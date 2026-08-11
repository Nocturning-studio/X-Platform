#include "pch_script.h"
#include "game_cl_base.h"

using namespace luabind;

typedef void (xr_vector<SZoneMapEntityData>::*zone_vector_push)(const xr_vector<SZoneMapEntityData>::value_type&);

#pragma optimize("s", on)
void SZoneMapEntityData::script_register(lua_State* L)
{
	module(L)[luabind::class_<SZoneMapEntityData>("SZoneMapEntityData")
				  .def(constructor<>())
				  .def_readwrite("pos", &SZoneMapEntityData::pos)
				  .def_readwrite("color", &SZoneMapEntityData::color),

			  luabind::class_<xr_vector<SZoneMapEntityData>>("ZoneMapEntities")
				  .def("push_back", (void(xr_vector<SZoneMapEntityData>::*)(SZoneMapEntityData&))(
										(zone_vector_push)&xr_vector<SZoneMapEntityData>::push_back))];
}

void RPoint::script_register(lua_State* L)
{
	module(L)[luabind::class_<RPoint>("RPoint")
				  .def(constructor<>())
				  .def_readwrite("P", &RPoint::P)
				  .def_readwrite("A", &RPoint::A)];
}
