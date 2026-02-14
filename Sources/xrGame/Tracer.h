// Tracer.h: interface for the CTracer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TRACER_H__AC263474_060A_489E_B84A_E879CD630A1E__INCLUDED_)
#define AFX_TRACER_H__AC263474_060A_489E_B84A_E879CD630A1E__INCLUDED_
#pragma once

class CBulletManager;

class CTracer
{
	friend CBulletManager;

  protected:
	ref_shader sh_Tracer;
	ref_geom sh_Geom;
	xr_vector<u32> m_aColors;

  public:
	CTracer();
	~CTracer();

	void Render(FVF::LIT*& verts, const float3& pos, const float3& center, const float3& dir, float length,
				float width, u8 colorID);
};

#endif // !defined(AFX_TRACER_H__AC263474_060A_489E_B84A_E879CD630A1E__INCLUDED_)
