#include "stdafx.h"
#include "r_portal.h"
#include "r_sector.h"

CPortal::CPortal() : m_front_sector(nullptr), m_back_sector(nullptr)
{
}
CPortal::~CPortal()
{
}

void CPortal::Setup(fvec3* v_ptr, int v_count, CSector* face, CSector* back)
{
	// Расчет Bounding Sphere
	Fbox BB;
	BB.invalidate();
	for (int i = 0; i < v_count; i++)
		BB.modify(v_ptr[i]);
	BB.getsphere(m_sphere.P, m_sphere.R);

	// Копируем вершины
	m_vertices.assign(v_ptr, v_count);
	m_front_sector = face;
	m_back_sector = back;

	// Расчет плоскости
	fvec3 N, T;
	N.set(0, 0, 0);

	// Robust normal calculation
	u32 valid_tris = 0;
	for (int i = 2; i < v_count; i++)
	{
		T.mknormal_non_normalized(m_vertices[0], m_vertices[i - 1], m_vertices[i]);
		float mag = T.magnitude();
		if (mag > EPS_S)
		{
			N.add(T.div(mag));
			valid_tris++;
		}
	}
	if (valid_tris)
		N.div(float(valid_tris));
	else
		Msg("! Invalid portal geometry detected");

	m_plane.build(m_vertices[0], N);
}
