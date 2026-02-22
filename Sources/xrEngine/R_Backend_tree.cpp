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

void R_tree::set_m_transform_v(float4x4& mat)
{
	if (c_m_transform_v)
		RenderBackendLegacy.set_Constant(c_m_transform_v, mat);
}

void R_tree::set_m_transform(float4x4& mat)
{
	if (c_m_transform)
		RenderBackendLegacy.set_Constant(c_m_transform, mat);
}

void R_tree::set_consts(float x, float y, float z, float w)
{
	if (c_consts)
		RenderBackendLegacy.set_Constant(c_consts, x, y, z, w);
}

void R_tree::set_c_scale(float x, float y, float z, float w)
{
	if (c_c_scale)
		RenderBackendLegacy.set_Constant(c_c_scale, x, y, z, w);
}

void R_tree::set_c_bias(float x, float y, float z, float w)
{
	if (c_c_bias)
		RenderBackendLegacy.set_Constant(c_c_bias, x, y, z, w);
}

void R_tree::set_c_sun(float x, float y, float z, float w)
{
	if (c_c_sun)
		RenderBackendLegacy.set_Constant(c_c_sun, x, y, z, w);
}
