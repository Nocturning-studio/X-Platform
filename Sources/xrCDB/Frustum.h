// Frustum.h: interface for the CFrustum class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FRUSTUM_H__E66ED755_F741_49CF_8B2A_404CCF7067F2__INCLUDED_)
#define AFX_FRUSTUM_H__E66ED755_F741_49CF_8B2A_404CCF7067F2__INCLUDED_
#pragma once

#include "../xrCore/FixedVector.h"

#pragma pack(push, 4)

enum EFC_Visible
{
	fcvNone = 0,
	fcvPartial,
	fcvFully,
	fcv_forcedword = u32(-1)
};

#define FRUSTUM_MAXPLANES 12
#define FRUSTUM_P_LEFT (1 << 0)
#define FRUSTUM_P_RIGHT (1 << 1)
#define FRUSTUM_P_TOP (1 << 2)
#define FRUSTUM_P_BOTTOM (1 << 3)
#define FRUSTUM_P_NEAR (1 << 4)
#define FRUSTUM_P_FAR (1 << 5)

#define FRUSTUM_P_LRTB (FRUSTUM_P_LEFT | FRUSTUM_P_RIGHT | FRUSTUM_P_TOP | FRUSTUM_P_BOTTOM)
#define FRUSTUM_P_ALL (FRUSTUM_P_LRTB | FRUSTUM_P_NEAR | FRUSTUM_P_FAR)

#define FRUSTUM_SAFE (FRUSTUM_MAXPLANES * 4)
typedef svector<float3, FRUSTUM_SAFE> sPoly;
ENGINE_API extern u32 frustum_aabb_remap[8][6];

class ENGINE_API CFrustum
{
  public:
	struct fplane : public Fplane
	{
		u32 aabb_overlap_id; // [0..7]
		void cache();
	};
	fplane planes[FRUSTUM_MAXPLANES];
	int p_count;

  public:
	ICF EFC_Visible AABB_OverlapPlane(const fplane& P, const float* mM) const
	{
		// calc extreme pts (neg,pos) along normal axis (pos in dir of norm, etc.)
		u32* id = frustum_aabb_remap[P.aabb_overlap_id];

		float3 Neg;
		Neg.set(mM[id[3]], mM[id[4]], mM[id[5]]);
		if (P.classify(Neg) > 0)
			return fcvNone;

		float3 Pos;
		Pos.set(mM[id[0]], mM[id[1]], mM[id[2]]);
		if (P.classify(Pos) <= 0)
			return fcvFully;

		return fcvPartial;
	}

  public:
	IC void _clear()
	{
		p_count = 0;
	}
	void _add(Fplane& P);
	void _add(float3& P1, float3& P2, float3& P3);

	void SimplifyPoly_AABB(sPoly* P, Fplane& plane);

	void CreateOccluder(float3* p, int count, float3& vBase, CFrustum& clip);
	BOOL CreateFromClipPoly(float3* p, int count, float3& vBase,
							CFrustum& clip); // returns 'false' if creation failed
	void CreateFromPoints(float3* p, int count, float3& vBase);
	void CreateFromMatrix(float4x4& M, u32 mask);
	void CreateFromPortal(sPoly* P, float3& vPN, float3& vBase, float4x4& mFullTransform);
	void CreateFromPlanes(Fplane* p, int count);

	sPoly* ClipPoly(sPoly& src, sPoly& dest) const;

	u32 getMask() const
	{
		return (1 << p_count) - 1;
	}

	EFC_Visible testSphere(float3& c, float r, u32& test_mask) const;
	BOOL testSphere_dirty(float3& c, float r) const;
	EFC_Visible testAABB(const float* mM, u32& test_mask) const;
	EFC_Visible testSAABB(float3& c, float r, const float* mM, u32& test_mask) const;
	BOOL testPolyInside_dirty(float3* p, int count) const;

	IC BOOL testPolyInside(sPoly& src) const
	{
		sPoly d;
		return !!ClipPoly(src, d);
	}
	IC BOOL testPolyInside(float3* p, int count) const
	{
		sPoly src(p, count);
		return testPolyInside(src);
	}
};
#pragma pack(pop)

#endif // !defined(AFX_FRUSTUM_H__E66ED755_F741_49CF_8B2A_404CCF7067F2__INCLUDED_)
