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
		L)[class_<fvec3>("vector")
			   .def_readwrite("x", &fvec3::x)
			   .def_readwrite("y", &fvec3::y)
			   .def_readwrite("z", &fvec3::z)
			   .def(constructor<>())
			   .def("set", (fvec3 & (fvec3::*)(float, float, float))(&fvec3::set), return_reference_to(_1))
			   .def("set", (fvec3 & (fvec3::*)(const fvec3&))(&fvec3::set), return_reference_to(_1))
			   .def("add", (fvec3 & (fvec3::*)(float))(&fvec3::add), return_reference_to(_1))
			   .def("add", (fvec3 & (fvec3::*)(const fvec3&))(&fvec3::add), return_reference_to(_1))
			   .def("add", (fvec3 & (fvec3::*)(const fvec3&, const fvec3&))(&fvec3::add),
					return_reference_to(_1))
			   .def("add", (fvec3 & (fvec3::*)(const fvec3&, float))(&fvec3::add), return_reference_to(_1))
			   .def("sub", (fvec3 & (fvec3::*)(float))(&fvec3::sub), return_reference_to(_1))
			   .def("sub", (fvec3 & (fvec3::*)(const fvec3&))(&fvec3::sub), return_reference_to(_1))
			   .def("sub", (fvec3 & (fvec3::*)(const fvec3&, const fvec3&))(&fvec3::sub),
					return_reference_to(_1))
			   .def("sub", (fvec3 & (fvec3::*)(const fvec3&, float))(&fvec3::sub), return_reference_to(_1))
			   .def("mul", (fvec3 & (fvec3::*)(float))(&fvec3::mul), return_reference_to(_1))
			   .def("mul", (fvec3 & (fvec3::*)(const fvec3&))(&fvec3::mul), return_reference_to(_1))
			   .def("mul", (fvec3 & (fvec3::*)(const fvec3&, const fvec3&))(&fvec3::mul),
					return_reference_to(_1))
			   .def("mul", (fvec3 & (fvec3::*)(const fvec3&, float))(&fvec3::mul), return_reference_to(_1))
			   .def("div", (fvec3 & (fvec3::*)(float))(&fvec3::div), return_reference_to(_1))
			   .def("div", (fvec3 & (fvec3::*)(const fvec3&))(&fvec3::div), return_reference_to(_1))
			   .def("div", (fvec3 & (fvec3::*)(const fvec3&, const fvec3&))(&fvec3::div),
					return_reference_to(_1))
			   .def("div", (fvec3 & (fvec3::*)(const fvec3&, float))(&fvec3::div), return_reference_to(_1))
			   .def("invert", (fvec3 & (fvec3::*)())(&fvec3::invert), return_reference_to(_1))
			   .def("invert", (fvec3 & (fvec3::*)(const fvec3&))(&fvec3::invert), return_reference_to(_1))
			   .def("min", (fvec3 & (fvec3::*)(const fvec3&))(&fvec3::min), return_reference_to(_1))
			   .def("min", (fvec3 & (fvec3::*)(const fvec3&, const fvec3&))(&fvec3::min),
					return_reference_to(_1))
			   .def("max", (fvec3 & (fvec3::*)(const fvec3&))(&fvec3::max), return_reference_to(_1))
			   .def("max", (fvec3 & (fvec3::*)(const fvec3&, const fvec3&))(&fvec3::max),
					return_reference_to(_1))
			   .def("abs", &fvec3::abs, return_reference_to(_1))
			   .def("similar", &fvec3::similar)
			   .def("set_length", &fvec3::set_length, return_reference_to(_1))
			   .def("align", &fvec3::align, return_reference_to(_1))
			   //			.def("squeeze",						&fvec3::squeeze,
			   //return_reference_to(_1))
			   .def("clamp", (fvec3 & (fvec3::*)(const fvec3&))(&fvec3::clamp), return_reference_to(_1))
			   .def("clamp", (fvec3 & (fvec3::*)(const fvec3&, const fvec3))(&fvec3::clamp),
					return_reference_to(_1))
			   .def("inertion", &fvec3::inertion, return_reference_to(_1))
			   .def("average", (fvec3 & (fvec3::*)(const fvec3&))(&fvec3::average), return_reference_to(_1))
			   .def("average", (fvec3 & (fvec3::*)(const fvec3&, const fvec3&))(&fvec3::average),
					return_reference_to(_1))
			   .def("lerp", &fvec3::lerp, return_reference_to(_1))
			   .def("mad", (fvec3 & (fvec3::*)(const fvec3&, float))(&fvec3::mad), return_reference_to(_1))
			   .def("mad", (fvec3 & (fvec3::*)(const fvec3&, const fvec3&, float))(&fvec3::mad),
					return_reference_to(_1))
			   .def("mad", (fvec3 & (fvec3::*)(const fvec3&, const fvec3&))(&fvec3::mad),
					return_reference_to(_1))
			   .def("mad", (fvec3 & (fvec3::*)(const fvec3&, const fvec3&, const fvec3&))(&fvec3::mad),
					return_reference_to(_1))
			   //			.def("square_magnitude",			&fvec3::square_magnitude)
			   .def("magnitude", &fvec3::magnitude)
			   //			.def("normalize_magnitude",			&fvec3::normalize_magn)
			   .def("normalize", (fvec3 & (fvec3::*)())(&fvec3::normalize_safe), return_reference_to(_1))
			   .def("normalize", (fvec3 & (fvec3::*)(const fvec3&))(&fvec3::normalize_safe),
					return_reference_to(_1))
			   .def("normalize_safe", (fvec3 & (fvec3::*)())(&fvec3::normalize_safe), return_reference_to(_1))
			   .def("normalize_safe", (fvec3 & (fvec3::*)(const fvec3&))(&fvec3::normalize_safe),
					return_reference_to(_1))
			   //			.def("random_dir",					(fvec3 & (fvec3::*)())(&fvec3::random_dir),
			   //return_reference_to(_1)) 			.def("random_dir",					(fvec3 & (fvec3::*)(const fvec3 &,
			   //float))(&fvec3::random_dir),													return_reference_to(_1))
			   //			.def("random_point",				(fvec3 & (fvec3::*)(const fvec3
			   //&))(&fvec3::random_point), return_reference_to(_1)) 			.def("random_point",				(fvec3 &
			   //(fvec3::*)(float))(&fvec3::random_point),
			   //return_reference_to(_1))
			   .def("dotproduct", &fvec3::dotproduct)
			   .def("crossproduct", &fvec3::crossproduct, return_reference_to(_1))
			   .def("distance_to_xz", &fvec3::distance_to_xz)
			   .def("distance_to_sqr", &fvec3::distance_to_sqr)
			   .def("distance_to", &fvec3::distance_to)
			   //			.def("from_bary",					(fvec3 & (fvec3::*)(const fvec3 &, const fvec3
			   //&, const fvec3 &, float, float, float))(&fvec3::from_bary),	return_reference_to(_1))
			   //			.def("from_bary",					(fvec3 & (fvec3::*)(const fvec3 &, const fvec3
			   //&, const fvec3 &, const fvec3 &))(&fvec3::from_bary),		return_reference_to(_1))
			   //			.def("from_bary4",					&fvec3::from_bary4,
			   //return_reference_to(_1)) 			.def("mknormal_non_normalized",		&fvec3::mknormal_non_normalized,
			   //return_reference_to(_1)) 			.def("mknormal",					&fvec3::mknormal,
			   //return_reference_to(_1))
			   .def("setHP", &fvec3::setHP, return_reference_to(_1))
			   //			.def("getHP",						&fvec3::getHP,
			   //out_value(_2) + out_value(_3))
			   .def("getH", &fvec3::getH)
			   .def("getP", &fvec3::getP)

			   .def("reflect", &fvec3::reflect, return_reference_to(_1))
			   .def("slide", &fvec3::slide, return_reference_to(_1)),
		   //			.def("generate_orthonormal_basis",	&fvec3::generate_orthonormal_basis),

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
