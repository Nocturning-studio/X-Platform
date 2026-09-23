#include "stdafx.h"
#pragma hdrstop

#include "r_backend_tree.h"

R_tree::R_tree()
{
	unmap();
}

void R_tree::unmap()
{
	c_m_transform_v = 0;
	c_m_transform = 0;
	c_consts = 0;
	c_c_scale = 0;
	c_c_bias = 0;
	c_c_sun = 0;
}

void R_tree::set_m_transform_v(fmat4x4& mat)
{
	if(c_m_transform_v)
		RenderBackend.SetConstant(c_m_transform_v, mat);
}

void R_tree::set_m_transform(fmat4x4& mat)
{
	if(c_m_transform)
		RenderBackend.SetConstant(c_m_transform, mat);
}

void R_tree::set_consts(float x, float y, float z, float w)
{
	if(c_consts)
		RenderBackend.SetConstant(c_consts, x, y, z, w);
}

void R_tree::set_c_scale(float x, float y, float z, float w)
{
	if(c_c_scale)
		RenderBackend.SetConstant(c_c_scale, x, y, z, w);
}

void R_tree::set_c_bias(float x, float y, float z, float w)
{
	if(c_c_bias)
		RenderBackend.SetConstant(c_c_bias, x, y, z, w);
}

void R_tree::set_c_sun(float x, float y, float z, float w)
{
	if(c_c_sun)
		RenderBackend.SetConstant(c_c_sun, x, y, z, w);
}
