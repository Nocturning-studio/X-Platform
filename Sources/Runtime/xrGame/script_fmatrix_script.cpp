////////////////////////////////////////////////////////////////////////////
//	Module 		: script_fmatrix_script.cpp
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script float matrix script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_fmatrix.h"

using namespace luabind;
void get_matrix_hpb(fmat4x4* self, float* h, float* p, float* b)
{
	self->getHPB(*h, *p, *b);
}
void matrix_transform(fmat4x4* self, fvec3* v)
{
	self->transform(*v);
}

#pragma optimize("s", on)
void CScriptFmatrix::script_register(lua_State* L)
{
	module(L)
		[class_<fmat4x4>("matrix")
			 .def_readwrite("i", &fmat4x4::i)
			 .def_readwrite("_14_", &fmat4x4::_14_)
			 .def_readwrite("j", &fmat4x4::j)
			 .def_readwrite("_24_", &fmat4x4::_24_)
			 .def_readwrite("k", &fmat4x4::k)
			 .def_readwrite("_34_", &fmat4x4::_34_)
			 .def_readwrite("c", &fmat4x4::c)
			 .def_readwrite("_44_", &fmat4x4::_44_)
			 .def(constructor<>())
			 .def("set", (fmat4x4 & (fmat4x4::*)(const fmat4x4&))(&fmat4x4::set), return_reference_to(_1))
			 .def("set",
				  (fmat4x4 &
				   (fmat4x4::*)(const fvec3&, const fvec3&, const fvec3&, const fvec3&))(&fmat4x4::set),
				  return_reference_to(_1))
			 .def("identity", &fmat4x4::identity, return_reference_to(_1))
			 .def("mk_xform", &fmat4x4::mk_transform, return_reference_to(_1))
			 .def("mul", (fmat4x4 & (fmat4x4::*)(const fmat4x4&, const fmat4x4&))(&fmat4x4::mul),
				  return_reference_to(_1))
			 .def("mul", (fmat4x4 & (fmat4x4::*)(const fmat4x4&, float))(&fmat4x4::mul), return_reference_to(_1))
			 .def("mul", (fmat4x4 & (fmat4x4::*)(float))(&fmat4x4::mul), return_reference_to(_1))
			 .def("div", (fmat4x4 & (fmat4x4::*)(const fmat4x4&, float))(&fmat4x4::div), return_reference_to(_1))
			 .def("div", (fmat4x4 & (fmat4x4::*)(float))(&fmat4x4::div), return_reference_to(_1))
			 //			.def("invert",						(fmat4x4 & (fmat4x4::*)())(&fmat4x4::invert),
			 //return_reference_to(_1)) 			.def("invert",						(fmat4x4 & (fmat4x4::*)(const fmat4x4
			 //&))(&fmat4x4::invert), return_reference_to(_1)) 			.def("transpose",					(fmat4x4 &
			 //(fmat4x4::*)())(&fmat4x4::transpose),
			 //return_reference_to(_1)) 			.def("transpose",					(fmat4x4 & (fmat4x4::*)(const fmat4x4
			 //&))(&fmat4x4::transpose), return_reference_to(_1)) 			.def("translate",					(fmat4x4 &
			 //(fmat4x4::*)(const fvec3 &))(&fmat4x4::translate),
			 //return_reference_to(_1)) 			.def("translate",					(fmat4x4 & (fmat4x4::*)(float, float,
			 //float))(&fmat4x4::translate), return_reference_to(_1)) 			.def("translate_over",				(fmat4x4 &
			 //(fmat4x4::*)(const fvec3 &))(&fmat4x4::translate_over),
			 //return_reference_to(_1)) 			.def("translate_over",				(fmat4x4 & (fmat4x4::*)(float, float,
			 //float))(&fmat4x4::translate_over), return_reference_to(_1)) 			.def("translate_add",
			 //&fmat4x4::translate_add,
			 //return_reference_to(_1)) 			.def("scale",						(fmat4x4 & (fmat4x4::*)(const fvec3
			 //&))(&fmat4x4::scale), return_reference_to(_1)) 			.def("scale",						(fmat4x4 &
			 //(fmat4x4::*)(float, float, float))(&fmat4x4::scale),
			 //return_reference_to(_1)) 			.def("rotateX",						&fmat4x4::rotateX,
			 //return_reference_to(_1)) 			.def("rotateY",						&fmat4x4::rotateY,
			 //return_reference_to(_1)) 			.def("rotateZ",						&fmat4x4::rotateZ,
			 //return_reference_to(_1)) 			.def("rotation",					(fmat4x4 & (fmat4x4::*)(const fvec3 &,
			 //const fvec3 &))(&fmat4x4::rotation),											return_reference_to(_1))
			 //			.def("rotation",					(fmat4x4 & (fmat4x4::*)(const fvec3 &,
			 //float))(&fmat4x4::rotation),													return_reference_to(_1))
			 //			.def("rotation",					&fmat4x4::rotation,
			 //return_reference_to(_1))
			 /*
						 .def("mapXYZ",						&fmat4x4::mapXYZ,
				return_reference_to(_1)) .def("mapXZY",						&fmat4x4::mapXZY,
				return_reference_to(_1)) .def("mapYXZ",						&fmat4x4::mapYXZ,
				return_reference_to(_1)) .def("mapYZX",						&fmat4x4::mapYZX,
				return_reference_to(_1)) .def("mapZXY",						&fmat4x4::mapZXY,
				return_reference_to(_1)) .def("mapZYX",						&fmat4x4::mapZYX,
				return_reference_to(_1)) .def("mirrorX",						&fmat4x4::mirrorX,
				return_reference_to(_1)) .def("mirrorX_over",				&fmat4x4::mirrorX_over,
				return_reference_to(_1)) .def("mirrorX_add ",				&fmat4x4::mirrorX_add,
				return_reference_to(_1)) .def("mirrorY",						&fmat4x4::mirrorY,
				return_reference_to(_1)) .def("mirrorY_over",				&fmat4x4::mirrorY_over,
				return_reference_to(_1)) .def("mirrorY_add ",				&fmat4x4::mirrorY_add,
				return_reference_to(_1)) .def("mirrorZ",						&fmat4x4::mirrorZ,
				return_reference_to(_1)) .def("mirrorZ_over",				&fmat4x4::mirrorZ_over,
				return_reference_to(_1)) .def("mirrorZ_add ",				&fmat4x4::mirrorZ_add,
				return_reference_to(_1))
			 */
			 //			.def("build_projection",			&fmat4x4::build_projection,
			 //return_reference_to(_1)) 			.def("build_projection_HAT",		&fmat4x4::build_projection_HAT,
			 //return_reference_to(_1)) 			.def("build_projection_ortho",		&fmat4x4::build_projection_ortho,
			 //return_reference_to(_1)) 			.def("build_camera",				&fmat4x4::build_camera,
			 //return_reference_to(_1)) 			.def("build_camera_dir",			&fmat4x4::build_camera_dir,
			 //return_reference_to(_1)) 			.def("inertion",					&fmat4x4::inertion,
			 //return_reference_to(_1)) 			.def("transform_tiny32",			&fmat4x4::transform_tiny32)
			 //			.def("transform_tiny23",			&fmat4x4::transform_tiny23)
			 //			.def("transform_tiny",				(void	   (fmat4x4::*)(fvec3 &)
			 //const)(&fmat4x4::transform_tiny),
			 //out_value(_2)) 			.def("transform_tiny",				(void	   (fmat4x4::*)(fvec3 &, const fvec3 &)
			 //const)(&fmat4x4::transform_tiny), out_value(_2)) 			.def("transform_dir",				(void
			 //(fmat4x4::*)(fvec3 &) const)(&fmat4x4::transform_dir),
			 //out_value(_2)) 			.def("transform_dir",				(void	   (fmat4x4::*)(fvec3 &, const fvec3 &)
			 //const)(&fmat4x4::transform_dir), out_value(_2)) 			.def("transform",					(void
			 //(fmat4x4::*)(fvec3 &) const)(&fmat4x4::transform),
			 //out_value(_2)) 			.def("transform",					&matrix_transform)
			 .def("setHPB", &fmat4x4::setHPB, return_reference_to(_1))
			 //			.def("setXYZ",						(fmat4x4 & (fmat4x4::*)(fvec3 &))(&fmat4x4::setXYZ),
			 //return_reference_to(_1)	+	out_value(_2))
			 .def("setXYZ", (fmat4x4 & (fmat4x4::*)(float, float, float))(&fmat4x4::setXYZ), return_reference_to(_1))
			 //			.def("setXYZi",						(fmat4x4 & (fmat4x4::*)(fvec3 &))(&fmat4x4::setXYZi),
			 //return_reference_to(_1) +	out_value(_2))
			 .def("setXYZi", (fmat4x4 & (fmat4x4::*)(float, float, float))(&fmat4x4::setXYZi), return_reference_to(_1))
			 //			.def("getHPB",						(void	   (fmat4x4::*)(fvec3 &) const)(&fmat4x4::getHPB),
			 //out_value(_2))
			 .def("getHPB", &get_matrix_hpb)
		 //			.def("getXYZ",						(void	   (fmat4x4::*)(fvec3 &) const)(&fmat4x4::getXYZ),
		 //out_value(_2)) 			.def("getXYZ",						(void	   (fmat4x4::*)(float &, float &, float &)
		 //const)(&fmat4x4::getXYZ)) 			.def("getXYZi",						(void	   (fmat4x4::*)(fvec3 &)
		 //const)(&fmat4x4::getXYZi), out_value(_2)) 			.def("getXYZi",						(void
		 //(fmat4x4::*)(float &, float &, float &) const)(&fmat4x4::getXYZi))
	];
}
