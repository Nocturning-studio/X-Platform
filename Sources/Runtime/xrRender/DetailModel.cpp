#include "stdafx.h"
#pragma hdrstop
#include "detailmodel.h"

CDetail::~CDetail()
{
}

void CDetail::Unload()
{
	if (vertices)
	{
		xr_free(vertices);
		vertices = 0;
	}
	if (indices)
	{
		xr_free(indices);
		indices = 0;
	}
	shader.destroy();
}

void CDetail::transfer(fmat4x4& mTransform, fvfVertexOut* vDest, u32 C, u16* iDest, u32 iOffset)
{
	// Transfer vertices
	{
		CDetail::fvfVertexIn *srcIt = vertices, *srcEnd = vertices + number_vertices;
		CDetail::fvfVertexOut* dstIt = vDest;
		for (; srcIt != srcEnd; srcIt++, dstIt++)
		{
			mTransform.transform_tiny(dstIt->P, srcIt->P);
			dstIt->C = C;
			dstIt->u = srcIt->u;
			dstIt->v = srcIt->v;
		}
	}

	// Transfer indices (in 32bit lines)
	VERIFY(iOffset < 65535);
	{
		u32 item = (iOffset << 16) | iOffset;
		u32 count = number_indices / 2;
		LPDWORD sit = LPDWORD(indices);
		LPDWORD send = sit + count;
		LPDWORD dit = LPDWORD(iDest);
		for (; sit != send; dit++, sit++)
			*dit = *sit + item;
		if (number_indices & 1)
			iDest[number_indices - 1] = u16(indices[number_indices - 1] + u16(iOffset));
	}
}

void CDetail::transfer(fmat4x4& mTransform, fvfVertexOut* vDest, u32 C, u16* iDest, u32 iOffset, float du, float dv)
{
	// Transfer vertices
	{
		CDetail::fvfVertexIn *srcIt = vertices, *srcEnd = vertices + number_vertices;
		CDetail::fvfVertexOut* dstIt = vDest;
		for (; srcIt != srcEnd; srcIt++, dstIt++)
		{
			mTransform.transform_tiny(dstIt->P, srcIt->P);
			dstIt->C = C;
			dstIt->u = srcIt->u + du;
			dstIt->v = srcIt->v + dv;
		}
	}

	// Transfer indices (in 32bit lines)
	VERIFY(iOffset < 65535);
	{
		u32 item = (iOffset << 16) | iOffset;
		u32 count = number_indices / 2;
		LPDWORD sit = LPDWORD(indices);
		LPDWORD send = sit + count;
		LPDWORD dit = LPDWORD(iDest);
		for (; sit != send; dit++, sit++)
			*dit = *sit + item;
		if (number_indices & 1)
			iDest[number_indices - 1] = u16(indices[number_indices - 1] + u16(iOffset));
	}
}

void CDetail::Load(IReader* S)
{
	// Shader
	string256 fnT, fnS;
	S->r_stringZ(fnS, sizeof(fnS));
	S->r_stringZ(fnT, sizeof(fnT));
	shader.create(fnS, fnT);

	// Params
	m_Flags.assign(S->r_u32());
	m_fMinScale = S->r_float();
	m_fMaxScale = S->r_float();
	number_vertices = S->r_u32();
	number_indices = S->r_u32();
	R_ASSERT(0 == (number_indices % 3));

	// Vertices
	u32 size_vertices = number_vertices * sizeof(fvfVertexIn);
	vertices = xr_alloc<CDetail::fvfVertexIn>(number_vertices);
	S->r(vertices, size_vertices);

	// Indices
	u32 size_indices = number_indices * sizeof(u16);
	indices = xr_alloc<u16>(number_indices);
	S->r(indices, size_indices);

	// Validate indices
#ifdef DEBUG
	for (u32 idx = 0; idx < number_indices; idx++)
		R_ASSERT(indices[idx] < (u16)number_vertices);
#endif

	// Calc BB & SphereRadius
	bv_bb.invalidate();
	for (u32 i = 0; i < number_vertices; i++)
		bv_bb.modify(vertices[i].P);
	bv_bb.getsphere(bv_sphere.P, bv_sphere.R);
}
