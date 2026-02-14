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
void get_matrix_hpb(float4x4* self, float* h, float* p, float* b)
{
	self->getHPB(*h, *p, *b);
}
void matrix_transform(float4x4* self, float3* v)
{
	self->transform(*v);
}

#pragma optimize("s", on)
void CScriptFmatrix::script_register(lua_State* L)
{
	module(L)
		[class_<float4x4>("matrix")
			 .def_readwrite("i", &float4x4::i)
			 .def_readwrite("_14_", &float4x4::_14_)
			 .def_readwrite("j", &float4x4::j)
			 .def_readwrite("_24_", &float4x4::_24_)
			 .def_readwrite("k", &float4x4::k)
			 .def_readwrite("_34_", &float4x4::_34_)
			 .def_readwrite("c", &float4x4::c)
			 .def_readwrite("_44_", &float4x4::_44_)
			 .def(constructor<>())
			 .def("set", (float4x4 & (float4x4::*)(const float4x4&))(&float4x4::set), return_reference_to(_1))
			 .def("set",
				  (float4x4 &
				   (float4x4::*)(const float3&, const float3&, const float3&, const float3&))(&float4x4::set),
				  return_reference_to(_1))
			 .def("identity", &float4x4::identity, return_reference_to(_1))
			 .def("mk_xform", &float4x4::mk_transform, return_reference_to(_1))
			 .def("mul", (float4x4 & (float4x4::*)(const float4x4&, const float4x4&))(&float4x4::mul),
				  return_reference_to(_1))
			 .def("mul", (float4x4 & (float4x4::*)(const float4x4&, float))(&float4x4::mul), return_reference_to(_1))
			 .def("mul", (float4x4 & (float4x4::*)(float))(&float4x4::mul), return_reference_to(_1))
			 .def("div", (float4x4 & (float4x4::*)(const float4x4&, float))(&float4x4::div), return_reference_to(_1))
			 .def("div", (float4x4 & (float4x4::*)(float))(&float4x4::div), return_reference_to(_1))
			 //			.def("invert",						(float4x4 & (float4x4::*)())(&float4x4::invert),
			 //return_reference_to(_1)) 			.def("invert",						(float4x4 & (float4x4::*)(const float4x4
			 //&))(&float4x4::invert), return_reference_to(_1)) 			.def("transpose",					(float4x4 &
			 //(float4x4::*)())(&float4x4::transpose),
			 //return_reference_to(_1)) 			.def("transpose",					(float4x4 & (float4x4::*)(const float4x4
			 //&))(&float4x4::transpose), return_reference_to(_1)) 			.def("translate",					(float4x4 &
			 //(float4x4::*)(const float3 &))(&float4x4::translate),
			 //return_reference_to(_1)) 			.def("translate",					(float4x4 & (float4x4::*)(float, float,
			 //float))(&float4x4::translate), return_reference_to(_1)) 			.def("translate_over",				(float4x4 &
			 //(float4x4::*)(const float3 &))(&float4x4::translate_over),
			 //return_reference_to(_1)) 			.def("translate_over",				(float4x4 & (float4x4::*)(float, float,
			 //float))(&float4x4::translate_over), return_reference_to(_1)) 			.def("translate_add",
			 //&float4x4::translate_add,
			 //return_reference_to(_1)) 			.def("scale",						(float4x4 & (float4x4::*)(const float3
			 //&))(&float4x4::scale), return_reference_to(_1)) 			.def("scale",						(float4x4 &
			 //(float4x4::*)(float, float, float))(&float4x4::scale),
			 //return_reference_to(_1)) 			.def("rotateX",						&float4x4::rotateX,
			 //return_reference_to(_1)) 			.def("rotateY",						&float4x4::rotateY,
			 //return_reference_to(_1)) 			.def("rotateZ",						&float4x4::rotateZ,
			 //return_reference_to(_1)) 			.def("rotation",					(float4x4 & (float4x4::*)(const float3 &,
			 //const float3 &))(&float4x4::rotation),											return_reference_to(_1))
			 //			.def("rotation",					(float4x4 & (float4x4::*)(const float3 &,
			 //float))(&float4x4::rotation),													return_reference_to(_1))
			 //			.def("rotation",					&float4x4::rotation,
			 //return_reference_to(_1))
			 /*
						 .def("mapXYZ",						&float4x4::mapXYZ,
				return_reference_to(_1)) .def("mapXZY",						&float4x4::mapXZY,
				return_reference_to(_1)) .def("mapYXZ",						&float4x4::mapYXZ,
				return_reference_to(_1)) .def("mapYZX",						&float4x4::mapYZX,
				return_reference_to(_1)) .def("mapZXY",						&float4x4::mapZXY,
				return_reference_to(_1)) .def("mapZYX",						&float4x4::mapZYX,
				return_reference_to(_1)) .def("mirrorX",						&float4x4::mirrorX,
				return_reference_to(_1)) .def("mirrorX_over",				&float4x4::mirrorX_over,
				return_reference_to(_1)) .def("mirrorX_add ",				&float4x4::mirrorX_add,
				return_reference_to(_1)) .def("mirrorY",						&float4x4::mirrorY,
				return_reference_to(_1)) .def("mirrorY_over",				&float4x4::mirrorY_over,
				return_reference_to(_1)) .def("mirrorY_add ",				&float4x4::mirrorY_add,
				return_reference_to(_1)) .def("mirrorZ",						&float4x4::mirrorZ,
				return_reference_to(_1)) .def("mirrorZ_over",				&float4x4::mirrorZ_over,
				return_reference_to(_1)) .def("mirrorZ_add ",				&float4x4::mirrorZ_add,
				return_reference_to(_1))
			 */
			 //			.def("build_projection",			&float4x4::build_projection,
			 //return_reference_to(_1)) 			.def("build_projection_HAT",		&float4x4::build_projection_HAT,
			 //return_reference_to(_1)) 			.def("build_projection_ortho",		&float4x4::build_projection_ortho,
			 //return_reference_to(_1)) 			.def("build_camera",				&float4x4::build_camera,
			 //return_reference_to(_1)) 			.def("build_camera_dir",			&float4x4::build_camera_dir,
			 //return_reference_to(_1)) 			.def("inertion",					&float4x4::inertion,
			 //return_reference_to(_1)) 			.def("transform_tiny32",			&float4x4::transform_tiny32)
			 //			.def("transform_tiny23",			&float4x4::transform_tiny23)
			 //			.def("transform_tiny",				(void	   (float4x4::*)(float3 &)
			 //const)(&float4x4::transform_tiny),
			 //out_value(_2)) 			.def("transform_tiny",				(void	   (float4x4::*)(float3 &, const float3 &)
			 //const)(&float4x4::transform_tiny), out_value(_2)) 			.def("transform_dir",				(void
			 //(float4x4::*)(float3 &) const)(&float4x4::transform_dir),
			 //out_value(_2)) 			.def("transform_dir",				(void	   (float4x4::*)(float3 &, const float3 &)
			 //const)(&float4x4::transform_dir), out_value(_2)) 			.def("transform",					(void
			 //(float4x4::*)(float3 &) const)(&float4x4::transform),
			 //out_value(_2)) 			.def("transform",					&matrix_transform)
			 .def("setHPB", &float4x4::setHPB, return_reference_to(_1))
			 //			.def("setXYZ",						(float4x4 & (float4x4::*)(float3 &))(&float4x4::setXYZ),
			 //return_reference_to(_1)	+	out_value(_2))
			 .def("setXYZ", (float4x4 & (float4x4::*)(float, float, float))(&float4x4::setXYZ), return_reference_to(_1))
			 //			.def("setXYZi",						(float4x4 & (float4x4::*)(float3 &))(&float4x4::setXYZi),
			 //return_reference_to(_1) +	out_value(_2))
			 .def("setXYZi", (float4x4 & (float4x4::*)(float, float, float))(&float4x4::setXYZi), return_reference_to(_1))
			 //			.def("getHPB",						(void	   (float4x4::*)(float3 &) const)(&float4x4::getHPB),
			 //out_value(_2))
			 .def("getHPB", &get_matrix_hpb)
		 //			.def("getXYZ",						(void	   (float4x4::*)(float3 &) const)(&float4x4::getXYZ),
		 //out_value(_2)) 			.def("getXYZ",						(void	   (float4x4::*)(float &, float &, float &)
		 //const)(&float4x4::getXYZ)) 			.def("getXYZi",						(void	   (float4x4::*)(float3 &)
		 //const)(&float4x4::getXYZi), out_value(_2)) 			.def("getXYZi",						(void
		 //(float4x4::*)(float &, float &, float &) const)(&float4x4::getXYZi))
	];
}
