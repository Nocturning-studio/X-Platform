#pragma once

IC void CPHElement::InverceLocalForm(fmat4x4& m)
{
	m.identity();
	m.c.set(m_mass_center);
	m.invert();
}

IC void CPHElement::MulB43InverceLocalForm(fmat4x4& m)
{
	fvec3 ic;
	ic.set(m_mass_center);
	ic.invert();
	m.transform_dir(ic);
	m.c.add(ic);
}
