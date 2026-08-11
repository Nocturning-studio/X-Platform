#ifndef r_backend_treeH
#define r_backend_treeH
#pragma once

class R_tree
{
public:
	R_constant* c_m_transform_v;
	R_constant* c_m_transform;
	R_constant* c_consts;
	R_constant* c_c_scale;
	R_constant* c_c_bias;
	R_constant* c_c_sun;

public:
	R_tree();
	void unmap();

	void set_c_m_transform_v(R_constant* C)
	{
		c_m_transform_v = C;
	}
	void set_c_m_transform(R_constant* C)
	{
		c_m_transform = C;
	}
	void set_c_consts(R_constant* C)
	{
		c_consts = C;
	}
	void set_c_c_scale(R_constant* C)
	{
		c_c_scale = C;
	}
	void set_c_c_bias(R_constant* C)
	{
		c_c_bias = C;
	}
	void set_c_c_sun(R_constant* C)
	{
		c_c_sun = C;
	}

	void set_m_transform_v(fmat4x4& mat);
	void set_m_transform(fmat4x4& mat);
	void set_consts(float x, float y, float z, float w);
	void set_c_scale(float x, float y, float z, float w);
	void set_c_bias(float x, float y, float z, float w);
	void set_c_sun(float x, float y, float z, float w);
};
#endif
