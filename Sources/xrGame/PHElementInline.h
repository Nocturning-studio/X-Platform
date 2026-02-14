#pragma once

IC void CPHElement::InverceLocalForm(float4x4& m)
{
	m.identity();
	m.c.set(m_mass_center);
	m.invert();
}

IC void CPHElement::MulB43InverceLocalForm(float4x4& m)
{
	float3 ic;
	ic.set(m_mass_center);
	ic.invert();
	m.transform_dir(ic);
	m.c.add(ic);
}
