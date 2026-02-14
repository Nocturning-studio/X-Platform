////////////////////////////////////////////////////////////////////////////
//	Module 		: script_fvector_script.cpp
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script float vector script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_fvector.h"

using namespace luabind;

#pragma optimize("s", on)
void CScriptFvector::script_register(lua_State* L)
{
	module(
		L)[class_<float3>("vector")
			   .def_readwrite("x", &float3::x)
			   .def_readwrite("y", &float3::y)
			   .def_readwrite("z", &float3::z)
			   .def(constructor<>())
			   .def("set", (float3 & (float3::*)(float, float, float))(&float3::set), return_reference_to(_1))
			   .def("set", (float3 & (float3::*)(const float3&))(&float3::set), return_reference_to(_1))
			   .def("add", (float3 & (float3::*)(float))(&float3::add), return_reference_to(_1))
			   .def("add", (float3 & (float3::*)(const float3&))(&float3::add), return_reference_to(_1))
			   .def("add", (float3 & (float3::*)(const float3&, const float3&))(&float3::add),
					return_reference_to(_1))
			   .def("add", (float3 & (float3::*)(const float3&, float))(&float3::add), return_reference_to(_1))
			   .def("sub", (float3 & (float3::*)(float))(&float3::sub), return_reference_to(_1))
			   .def("sub", (float3 & (float3::*)(const float3&))(&float3::sub), return_reference_to(_1))
			   .def("sub", (float3 & (float3::*)(const float3&, const float3&))(&float3::sub),
					return_reference_to(_1))
			   .def("sub", (float3 & (float3::*)(const float3&, float))(&float3::sub), return_reference_to(_1))
			   .def("mul", (float3 & (float3::*)(float))(&float3::mul), return_reference_to(_1))
			   .def("mul", (float3 & (float3::*)(const float3&))(&float3::mul), return_reference_to(_1))
			   .def("mul", (float3 & (float3::*)(const float3&, const float3&))(&float3::mul),
					return_reference_to(_1))
			   .def("mul", (float3 & (float3::*)(const float3&, float))(&float3::mul), return_reference_to(_1))
			   .def("div", (float3 & (float3::*)(float))(&float3::div), return_reference_to(_1))
			   .def("div", (float3 & (float3::*)(const float3&))(&float3::div), return_reference_to(_1))
			   .def("div", (float3 & (float3::*)(const float3&, const float3&))(&float3::div),
					return_reference_to(_1))
			   .def("div", (float3 & (float3::*)(const float3&, float))(&float3::div), return_reference_to(_1))
			   .def("invert", (float3 & (float3::*)())(&float3::invert), return_reference_to(_1))
			   .def("invert", (float3 & (float3::*)(const float3&))(&float3::invert), return_reference_to(_1))
			   .def("min", (float3 & (float3::*)(const float3&))(&float3::min), return_reference_to(_1))
			   .def("min", (float3 & (float3::*)(const float3&, const float3&))(&float3::min),
					return_reference_to(_1))
			   .def("max", (float3 & (float3::*)(const float3&))(&float3::max), return_reference_to(_1))
			   .def("max", (float3 & (float3::*)(const float3&, const float3&))(&float3::max),
					return_reference_to(_1))
			   .def("abs", &float3::abs, return_reference_to(_1))
			   .def("similar", &float3::similar)
			   .def("set_length", &float3::set_length, return_reference_to(_1))
			   .def("align", &float3::align, return_reference_to(_1))
			   //			.def("squeeze",						&float3::squeeze,
			   //return_reference_to(_1))
			   .def("clamp", (float3 & (float3::*)(const float3&))(&float3::clamp), return_reference_to(_1))
			   .def("clamp", (float3 & (float3::*)(const float3&, const float3))(&float3::clamp),
					return_reference_to(_1))
			   .def("inertion", &float3::inertion, return_reference_to(_1))
			   .def("average", (float3 & (float3::*)(const float3&))(&float3::average), return_reference_to(_1))
			   .def("average", (float3 & (float3::*)(const float3&, const float3&))(&float3::average),
					return_reference_to(_1))
			   .def("lerp", &float3::lerp, return_reference_to(_1))
			   .def("mad", (float3 & (float3::*)(const float3&, float))(&float3::mad), return_reference_to(_1))
			   .def("mad", (float3 & (float3::*)(const float3&, const float3&, float))(&float3::mad),
					return_reference_to(_1))
			   .def("mad", (float3 & (float3::*)(const float3&, const float3&))(&float3::mad),
					return_reference_to(_1))
			   .def("mad", (float3 & (float3::*)(const float3&, const float3&, const float3&))(&float3::mad),
					return_reference_to(_1))
			   //			.def("square_magnitude",			&float3::square_magnitude)
			   .def("magnitude", &float3::magnitude)
			   //			.def("normalize_magnitude",			&float3::normalize_magn)
			   .def("normalize", (float3 & (float3::*)())(&float3::normalize_safe), return_reference_to(_1))
			   .def("normalize", (float3 & (float3::*)(const float3&))(&float3::normalize_safe),
					return_reference_to(_1))
			   .def("normalize_safe", (float3 & (float3::*)())(&float3::normalize_safe), return_reference_to(_1))
			   .def("normalize_safe", (float3 & (float3::*)(const float3&))(&float3::normalize_safe),
					return_reference_to(_1))
			   //			.def("random_dir",					(float3 & (float3::*)())(&float3::random_dir),
			   //return_reference_to(_1)) 			.def("random_dir",					(float3 & (float3::*)(const float3 &,
			   //float))(&float3::random_dir),													return_reference_to(_1))
			   //			.def("random_point",				(float3 & (float3::*)(const float3
			   //&))(&float3::random_point), return_reference_to(_1)) 			.def("random_point",				(float3 &
			   //(float3::*)(float))(&float3::random_point),
			   //return_reference_to(_1))
			   .def("dotproduct", &float3::dotproduct)
			   .def("crossproduct", &float3::crossproduct, return_reference_to(_1))
			   .def("distance_to_xz", &float3::distance_to_xz)
			   .def("distance_to_sqr", &float3::distance_to_sqr)
			   .def("distance_to", &float3::distance_to)
			   //			.def("from_bary",					(float3 & (float3::*)(const float3 &, const float3
			   //&, const float3 &, float, float, float))(&float3::from_bary),	return_reference_to(_1))
			   //			.def("from_bary",					(float3 & (float3::*)(const float3 &, const float3
			   //&, const float3 &, const float3 &))(&float3::from_bary),		return_reference_to(_1))
			   //			.def("from_bary4",					&float3::from_bary4,
			   //return_reference_to(_1)) 			.def("mknormal_non_normalized",		&float3::mknormal_non_normalized,
			   //return_reference_to(_1)) 			.def("mknormal",					&float3::mknormal,
			   //return_reference_to(_1))
			   .def("setHP", &float3::setHP, return_reference_to(_1))
			   //			.def("getHP",						&float3::getHP,
			   //out_value(_2) + out_value(_3))
			   .def("getH", &float3::getH)
			   .def("getP", &float3::getP)

			   .def("reflect", &float3::reflect, return_reference_to(_1))
			   .def("slide", &float3::slide, return_reference_to(_1)),
		   //			.def("generate_orthonormal_basis",	&float3::generate_orthonormal_basis),

		   class_<Fbox>("Fbox").def_readwrite("min", &Fbox::min).def_readwrite("max", &Fbox::max).def(constructor<>()),

		   class_<Frect>("Frect")
			   .def(constructor<>())
			   .def("set", (Frect & (Frect::*)(float, float, float, float))(&Frect::set), return_reference_to(_1))
			   .def_readwrite("lt", &Frect::lt)
			   .def_readwrite("rb", &Frect::rb)
			   .def_readwrite("x1", &Frect::x1)
			   .def_readwrite("x2", &Frect::x2)
			   .def_readwrite("y1", &Frect::y1)
			   .def_readwrite("y2", &Frect::y2)

	];
}
