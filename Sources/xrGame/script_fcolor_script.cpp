////////////////////////////////////////////////////////////////////////////
//	Module 		: script_Fcolor_script.cpp
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script float vector script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_fcolor.h"

using namespace luabind;

#pragma optimize("s", on)
void CScriptFcolor::script_register(lua_State* L)
{
	module(L)[class_<fcolor>("fcolor")
				  .def_readwrite("r", &fcolor::r)
				  .def_readwrite("g", &fcolor::g)
				  .def_readwrite("b", &fcolor::b)
				  .def_readwrite("a", &fcolor::a)
				  .def(constructor<>())
				  .def("set", (fcolor & (fcolor::*)(float, float, float, float))(&fcolor::set), return_reference_to(_1))
				  .def("set", (fcolor & (fcolor::*)(const fcolor&))(&fcolor::set), return_reference_to(_1))
				  .def("set", (fcolor & (fcolor::*)(u32))(&fcolor::set), return_reference_to(_1))];
}
