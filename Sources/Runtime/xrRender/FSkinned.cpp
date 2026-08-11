// SkeletonX.cpp: implementation of the CSkeletonX class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop

#pragma warning(disable : 4995)
#include <d3dx9.h>
#pragma warning(default : 4995)

#include "../xrEngine/fmesh.h"
#include "../xrEngine/xrBind_PSGP.h"
#include "FSkinned.h"

#include "../xrEngine/EnnumerateVertices.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
static shared_str sbones_array;

static D3DVERTEXELEMENT9 dwDecl_1W[] = {{0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
										{0, 16, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
										{0, 28, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0},
										{0, 40, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0},
										{0, 52, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
										{0, 60, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
										D3DDECL_END()};

struct vertHW_1W
{
	float _P[4]; // позиция (x,y,z,1)
	float _N[3]; // нормаль
	float _T[3]; // тангент
	float _B[3]; // бинормаль
	fvec2 tc;	 // UV
	u32 index;	 // индекс кости (умножен на 3)

	void set(const fvec3& P, const fvec3& N, const fvec3& T, const fvec3& B, const fvec2& tc, int idx)
	{
		_P[0] = P.x;
		_P[1] = P.y;
		_P[2] = P.z;
		_P[3] = 1.0f;
		_N[0] = N.x;
		_N[1] = N.y;
		_N[2] = N.z;
		_T[0] = T.x;
		_T[1] = T.y;
		_T[2] = T.z;
		_B[0] = B.x;
		_B[1] = B.y;
		_B[2] = B.z;
		this->tc = tc;
		index = idx;
	}

	u16 get_bone() const
	{
		return u16(index / 3);
	}
	void get_pos(fvec3& p) const
	{
		p.set(_P[0], _P[1], _P[2]);
	}
};

static D3DVERTEXELEMENT9 dwDecl_2W[] = {{0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
										{0, 16, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
										{0, 28, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0},
										{0, 40, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0},
										{0, 52, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
										{0, 60, D3DDECLTYPE_SHORT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
										D3DDECL_END()};
struct vertHW_2W
{
	float _P[4];	// позиция (xyz, w=weight)
	float _N[3];	// нормаль
	float _T[3];	// тангент
	float _B[3];	// бинормаль
	fvec2 tc;		// UV
	s16 indices[2]; // индексы костей (умножены на 3)

	void set(const fvec3& P, const fvec3& N, const fvec3& T, const fvec3& B, const fvec2& tc, int idx0, int idx1,
			 float w)
	{
		_P[0] = P.x;
		_P[1] = P.y;
		_P[2] = P.z;
		_P[3] = w;
		_N[0] = N.x;
		_N[1] = N.y;
		_N[2] = N.z;
		_T[0] = T.x;
		_T[1] = T.y;
		_T[2] = T.z;
		_B[0] = B.x;
		_B[1] = B.y;
		_B[2] = B.z;
		this->tc = tc;
		indices[0] = s16(idx0);
		indices[1] = s16(idx1);
	}

	float get_weight() const
	{
		return _P[3];
	}
	u16 get_bone(int i) const
	{
		return u16(indices[i] / 3);
	}
	void get_pos(fvec3& p) const
	{
		p.set(_P[0], _P[1], _P[2]);
	}
};

#pragma pack(pop)

//////////////////////////////////////////////////////////////////////
// Body Part
//////////////////////////////////////////////////////////////////////
void CSkeletonX_PM::Copy(IRender_Visual* V)
{
	inherited1::Copy(V);
	CSkeletonX_PM* X = (CSkeletonX_PM*)(V);
	_Copy((CSkeletonX*)X);
}
void CSkeletonX_ST::Copy(IRender_Visual* P)
{
	inherited1::Copy(P);
	CSkeletonX_ST* X = (CSkeletonX_ST*)P;
	_Copy((CSkeletonX*)X);
}
//////////////////////////////////////////////////////////////////////
void CSkeletonX_PM::Render(float LOD)
{
	int lod_id = inherited1::last_lod;
	if (LOD >= 0.f)
	{
		clamp(LOD, 0.f, 1.f);
		lod_id = iFloor((1.f - LOD) * float(nSWI.count - 1) + 0.5f);
		inherited1::last_lod = lod_id;
	}
	VERIFY(lod_id >= 0 && lod_id < int(nSWI.count));
	FSlideWindow& SW = nSWI.sw[lod_id];
	_Render(rm_geom, SW.num_verts, SW.offset, SW.num_tris);
}
void CSkeletonX_ST::Render(float LOD)
{
	_Render(rm_geom, vCount, 0, dwPrimitives);
}

//////////////////////////////////////////////////////////////////////
void CSkeletonX_PM::Release()
{
	inherited1::Release();
}
void CSkeletonX_ST::Release()
{
	inherited1::Release();
}
//////////////////////////////////////////////////////////////////////
void CSkeletonX_PM::Load(const char* N, IReader* data, u32 dwFlags)
{
	_Load(N, data, vCount);
	void* _verts_ = data->pointer();
	inherited1::Load(N, data, dwFlags | VLOAD_NOVERTICES);
	::Render->shader_option_skinning(-1);
	vBase = 0;
	_Load_hw(*this, _verts_);
}
void CSkeletonX_ST::Load(const char* N, IReader* data, u32 dwFlags)
{
	_Load(N, data, vCount);
	void* _verts_ = data->pointer();
	inherited1::Load(N, data, dwFlags | VLOAD_NOVERTICES);
	::Render->shader_option_skinning(-1);
	vBase = 0;
	_Load_hw(*this, _verts_);
}

void CSkeletonX_ext::_Load_hw(Fvisual& V, void* _verts_)
{
	// Create HW VB in case this is possible
	BOOL bSoft = false;//HW.GetCaps().geometry.bSoftware;
	u32 dwUsage =
		/*D3DUSAGE_WRITEONLY |*/ (bSoft ? D3DUSAGE_SOFTWAREPROCESSING : 0); // VB may be read by wallmarks code
	switch (RenderMode)
	{
	case RM_SKINNING_SOFT:
		// Msg					("skinning: software");
		V.rm_geom.create(vertRenderFVF, RenderBackend.Vertex.Buffer(), V.p_rm_Indices);
		break;
	case RM_SINGLE:
	case RM_SKINNING_1B: {
		u32 vStride = D3DXGetDeclVertexSize(dwDecl_1W, 0);
		VERIFY(vStride == sizeof(vertHW_1W));
		BYTE* bytes = nullptr;
		VERIFY(!V.p_rm_Vertices);
		R_CHK(RenderBackend.GetDevice()->CreateVertexBuffer(V.vCount * vStride, dwUsage, 0, D3DPOOL_DEFAULT, &V.p_rm_Vertices, 0));
		R_CHK(V.p_rm_Vertices->Lock(0, 0, (void**)&bytes, 0));

		vertHW_1W* dst = (vertHW_1W*)bytes;
		vertBoned1W* src = (vertBoned1W*)_verts_;
		for (u32 i = 0; i < V.vCount; ++i)
		{
			fvec2 uv{src->u, src->v};
			dst->set(src->P, src->N, src->T, src->B, uv, src->matrix * 3);
			++dst;
			++src;
		}
		V.p_rm_Vertices->Unlock();
		V.rm_geom.create(dwDecl_1W, V.p_rm_Vertices, V.p_rm_Indices);
	}
	break;
	case RM_SKINNING_2B: {
		u32 vStride = D3DXGetDeclVertexSize(dwDecl_2W, 0);
		VERIFY(vStride == sizeof(vertHW_2W));
		BYTE* bytes = nullptr;
		VERIFY(!V.p_rm_Vertices);
		R_CHK(RenderBackend.GetDevice()->CreateVertexBuffer(V.vCount * vStride, dwUsage, 0, D3DPOOL_DEFAULT, &V.p_rm_Vertices, 0));
		R_CHK(V.p_rm_Vertices->Lock(0, 0, (void**)&bytes, 0));

		vertHW_2W* dst = (vertHW_2W*)bytes;
		vertBoned2W* src = (vertBoned2W*)_verts_;
		for (u32 i = 0; i < V.vCount; ++i)
		{
			fvec2 uv{src->u, src->v};
			dst->set(src->P, src->N, src->T, src->B, uv, int(src->matrix0) * 3, int(src->matrix1) * 3, src->w);
			++dst;
			++src;
		}
		V.p_rm_Vertices->Unlock();
		V.rm_geom.create(dwDecl_2W, V.p_rm_Vertices, V.p_rm_Indices);
	}
	break;
	}
}

//-----------------------------------------------------------------------------------------------------
// Wallmarks
//-----------------------------------------------------------------------------------------------------
#include "../xrCDB/cl_intersect.h"
void CSkeletonX_ext::_CollectBoneFaces(Fvisual* V, u32 iBase, u32 iCount)
{
	u16* indices = 0;
	//.	R_CHK			(V->pIndices->Lock(iBase,iCount,(void**)&indices,D3DLOCK_READONLY));
	R_CHK(V->p_rm_Indices->Lock(0, V->dwPrimitives * 3, (void**)&indices, D3DLOCK_READONLY));
	indices += iBase;
	switch (RenderMode)
	{
	case RM_SKINNING_SOFT: {
		if (*Vertices1W)
		{
			vertBoned1W* vertices = *Vertices1W;
			for (u32 idx = 0; idx < iCount; idx++)
			{
				vertBoned1W& v = vertices[V->vBase + indices[idx]];
				CBoneData& BD = Parent->LL_GetData((u16)v.matrix);
				BD.AppendFace(ChildIDX, (u16)(idx / 3));
			}
		}
		else
		{
			VERIFY(*Vertices2W);
			vertBoned2W* vertices = *Vertices2W;
			for (u32 idx = 0; idx < iCount; idx++)
			{
				vertBoned2W& v = vertices[V->vBase + indices[idx]];
				CBoneData& BD0 = Parent->LL_GetData((u16)v.matrix0);
				BD0.AppendFace(ChildIDX, (u16)(idx / 3));
				CBoneData& BD1 = Parent->LL_GetData((u16)v.matrix1);
				BD1.AppendFace(ChildIDX, (u16)(idx / 3));
			}
		}
	}
	break;
	case RM_SINGLE:
	case RM_SKINNING_1B: {
		vertHW_1W* vertices = 0;
		R_CHK(V->p_rm_Vertices->Lock(V->vBase, V->vCount, (void**)&vertices, D3DLOCK_READONLY));
		for (u32 idx = 0; idx < iCount; idx++)
		{
			vertHW_1W& v = vertices[indices[idx]];
			CBoneData& BD = Parent->LL_GetData(v.get_bone());
			BD.AppendFace(ChildIDX, (u16)(idx / 3));
		}
		V->p_rm_Vertices->Unlock();
	}
	break;
	case RM_SKINNING_2B: {
		vertHW_2W* vertices = 0;
		R_CHK(V->p_rm_Vertices->Lock(V->vBase, V->vCount, (void**)&vertices, D3DLOCK_READONLY));
		for (u32 idx = 0; idx < iCount; idx++)
		{
			vertHW_2W& v = vertices[indices[idx]];
			CBoneData& BD0 = Parent->LL_GetData(v.get_bone(0));
			BD0.AppendFace(ChildIDX, (u16)(idx / 3));
			CBoneData& BD1 = Parent->LL_GetData(v.get_bone(1));
			BD1.AppendFace(ChildIDX, (u16)(idx / 3));
		}
		V->p_rm_Vertices->Unlock();
	}
	break;
	}
	R_CHK(V->p_rm_Indices->Unlock());
}

void CSkeletonX_ST::AfterLoad(CKinematics* parent, u16 child_idx)
{
	inherited2::AfterLoad(parent, child_idx);
	inherited2::_CollectBoneFaces(this, iBase, iCount);
}

void CSkeletonX_PM::AfterLoad(CKinematics* parent, u16 child_idx)
{
	inherited2::AfterLoad(parent, child_idx);
	FSlideWindow& SW = nSWI.sw[0]; // max LOD
	inherited2::_CollectBoneFaces(this, iBase + SW.offset, SW.num_tris * 3);
}

BOOL CSkeletonX_ext::_PickBoneHW1W(fvec3& normal, float& dist, const fvec3& S, const fvec3& D, Fvisual* V,
								   u16* indices, CBoneData::FacesVec& faces)
{
	vertHW_1W* vertices;
	CHK_DX(V->p_rm_Vertices->Lock(V->vBase, V->vCount, (void**)&vertices, D3DLOCK_READONLY));
	bool intersect = FALSE;
	for (CBoneData::FacesVecIt it = faces.begin(); it != faces.end(); it++)
	{
		fvec3 p[3];
		u32 idx = (*it) * 3;
		for (u32 k = 0; k < 3; k++)
		{
			vertHW_1W& vert = vertices[indices[idx + k]];
			const fmat4x4& transform = Parent->LL_GetBoneInstance(vert.get_bone()).mRenderTransform;
			vert.get_pos(p[k]);
			transform.transform_tiny(p[k]);
		}
		float u, v, range = flt_max;
		if (CDB::TestRayTri(S, D, p, u, v, range, true) && (range < dist))
		{
			normal.mknormal(p[0], p[1], p[2]);
			dist = range;
			intersect = TRUE;
		}
	}
	CHK_DX(V->p_rm_Vertices->Unlock());
	return intersect;
}
BOOL CSkeletonX_ext::_PickBoneHW2W(fvec3& normal, float& dist, const fvec3& S, const fvec3& D, Fvisual* V,
								   u16* indices, CBoneData::FacesVec& faces)
{
	vertHW_2W* vertices;
	CHK_DX(V->p_rm_Vertices->Lock(V->vBase, V->vCount, (void**)&vertices, D3DLOCK_READONLY));
	bool intersect = FALSE;
	for (CBoneData::FacesVecIt it = faces.begin(); it != faces.end(); it++)
	{
		fvec3 p[3];
		u32 idx = (*it) * 3;
		for (u32 k = 0; k < 3; k++)
		{
			fvec3 P0, P1;
			vertHW_2W& vert = vertices[indices[idx + k]];
			fmat4x4& transform0 = Parent->LL_GetBoneInstance(vert.get_bone(0)).mRenderTransform;
			fmat4x4& transform1 = Parent->LL_GetBoneInstance(vert.get_bone(1)).mRenderTransform;
			vert.get_pos(P0);
			transform0.transform_tiny(P0);
			vert.get_pos(P1);
			transform1.transform_tiny(P1);
			p[k].lerp(P0, P1, vert.get_weight());
		}
		float u, v, range = flt_max;
		if (CDB::TestRayTri(S, D, p, u, v, range, true) && (range < dist))
		{
			normal.mknormal(p[0], p[1], p[2]);
			dist = range;
			intersect = TRUE;
		}
	}
	CHK_DX(V->p_rm_Vertices->Unlock());
	return intersect;
}

BOOL CSkeletonX_ext::_PickBone(fvec3& normal, float& dist, const fvec3& start, const fvec3& dir, Fvisual* V,
							   u16 bone_id, u32 iBase, u32 iCount)
{
	VERIFY(Parent && (ChildIDX != u16(-1)));
	CBoneData& BD = Parent->LL_GetData(bone_id);
	CBoneData::FacesVec* faces = &BD.child_faces[ChildIDX];
	u16* indices = 0;
	//.	R_CHK				(V->pIndices->Lock(iBase,iCount,		(void**)&indices,	D3DLOCK_READONLY));
	CHK_DX(V->p_rm_Indices->Lock(0, V->dwPrimitives * 3, (void**)&indices, D3DLOCK_READONLY));
	// fill vertices
	BOOL result = FALSE;
	switch (RenderMode)
	{
	case RM_SKINNING_SOFT:
		if (*Vertices1W)
			result = _PickBoneSoft1W(normal, dist, start, dir, indices + iBase, *faces);
		else
			result = _PickBoneSoft2W(normal, dist, start, dir, indices + iBase, *faces);
		break;
	case RM_SINGLE:
	case RM_SKINNING_1B:
		result = _PickBoneHW1W(normal, dist, start, dir, V, indices + iBase, *faces);
		break;
	case RM_SKINNING_2B:
		result = _PickBoneHW2W(normal, dist, start, dir, V, indices + iBase, *faces);
		break;
	default:
		NODEFAULT;
	}
	CHK_DX(V->p_rm_Indices->Unlock());
	return result;
}
BOOL CSkeletonX_ST::PickBone(fvec3& normal, float& dist, const fvec3& start, const fvec3& dir, u16 bone_id)
{
	return inherited2::_PickBone(normal, dist, start, dir, this, bone_id, iBase, iCount);
}
BOOL CSkeletonX_PM::PickBone(fvec3& normal, float& dist, const fvec3& start, const fvec3& dir, u16 bone_id)
{
	FSlideWindow& SW = nSWI.sw[0];
	return inherited2::_PickBone(normal, dist, start, dir, this, bone_id, iBase + SW.offset, SW.num_tris * 3);
}

void CSkeletonX_ST::EnumBoneVertices(SEnumVerticesCallback& C, u16 bone_id)
{
	inherited2::_EnumBoneVertices(C, this, bone_id, iBase, iCount);
}

void CSkeletonX_PM::EnumBoneVertices(SEnumVerticesCallback& C, u16 bone_id)
{
	FSlideWindow& SW = nSWI.sw[0];
	inherited2::_EnumBoneVertices(C, this, bone_id, iBase + SW.offset, SW.num_tris * 3);
}

void CSkeletonX_ext::_FillVerticesHW1W(const fmat4x4& view, CSkeletonWallmark& wm, const fvec3& normal, float size,
									   Fvisual* V, u16* indices, CBoneData::FacesVec& faces)
{
	vertHW_1W* vertices;
	CHK_DX(V->p_rm_Vertices->Lock(V->vBase, V->vCount, (void**)&vertices, D3DLOCK_READONLY));
	for (CBoneData::FacesVecIt it = faces.begin(); it != faces.end(); it++)
	{
		fvec3 p[3];
		u32 idx = (*it) * 3;
		CSkeletonWallmark::WMFace F;
		for (u32 k = 0; k < 3; k++)
		{
			vertHW_1W& vert = vertices[indices[idx + k]];
			F.bone_id[k][0] = vert.get_bone();
			F.bone_id[k][1] = F.bone_id[k][0];
			F.weight[k] = 0.f;
			const fmat4x4& transform = Parent->LL_GetBoneInstance(F.bone_id[k][0]).mRenderTransform;
			vert.get_pos(F.vert[k]);
			transform.transform_tiny(p[k], F.vert[k]);
		}
		fvec3 test_normal;
		test_normal.mknormal(p[0], p[1], p[2]);
		float cosa = test_normal.dotproduct(normal);
		if (cosa < EPS)
			continue;
		if (CDB::TestSphereTri(wm.ContactPoint(), size, p))
		{
			fvec3 UV;
			for (u32 k = 0; k < 3; k++)
			{
				fvec2& uv = F.uv[k];
				view.transform_tiny(UV, p[k]);
				uv.x = (1 + UV.x) * .5f;
				uv.y = (1 - UV.y) * .5f;
			}
			wm.m_Faces.push_back(F);
		}
	}
	CHK_DX(V->p_rm_Vertices->Unlock());
}
void CSkeletonX_ext::_FillVerticesHW2W(const fmat4x4& view, CSkeletonWallmark& wm, const fvec3& normal, float size,
									   Fvisual* V, u16* indices, CBoneData::FacesVec& faces)
{
	vertHW_2W* vertices;
	CHK_DX(V->p_rm_Vertices->Lock(V->vBase, V->vCount, (void**)&vertices, D3DLOCK_READONLY));
	for (CBoneData::FacesVecIt it = faces.begin(); it != faces.end(); it++)
	{
		fvec3 p[3]{};
		u32 idx = (*it) * 3;
		CSkeletonWallmark::WMFace F;
		for (u32 k = 0; k < 3; k++)
		{
			fvec3 P0, P1;
			vertHW_2W& vert = vertices[indices[idx + k]];
			F.bone_id[k][0] = vert.get_bone(0);
			F.bone_id[k][1] = vert.get_bone(1);
			F.weight[k] = vert.get_weight();
			fmat4x4& transform0 = Parent->LL_GetBoneInstance(F.bone_id[k][0]).mRenderTransform;
			fmat4x4& transform1 = Parent->LL_GetBoneInstance(F.bone_id[k][1]).mRenderTransform;
			vert.get_pos(F.vert[k]);
			transform0.transform_tiny(P0, F.vert[k]);
			transform1.transform_tiny(P1, F.vert[k]);
			p[k].lerp(P0, P1, F.weight[k]);
		}
		fvec3 test_normal;
		test_normal.mknormal(p[0], p[1], p[2]);
		float cosa = test_normal.dotproduct(normal);
		if (cosa < EPS)
			continue;
		if (CDB::TestSphereTri(wm.ContactPoint(), size, p))
		{
			fvec3 UV;
			for (u32 k = 0; k < 3; k++)
			{
				fvec2& uv = F.uv[k];
				view.transform_tiny(UV, p[k]);
				uv.x = (1 + UV.x) * .5f;
				uv.y = (1 - UV.y) * .5f;
			}
			wm.m_Faces.push_back(F);
		}
	}
	CHK_DX(V->p_rm_Vertices->Unlock());
}

void CSkeletonX_ext::_FillVertices(const fmat4x4& view, CSkeletonWallmark& wm, const fvec3& normal, float size,
								   Fvisual* V, u16 bone_id, u32 iBase, u32 iCount)
{
	VERIFY(Parent && (ChildIDX != u16(-1)));
	CBoneData& BD = Parent->LL_GetData(bone_id);
	CBoneData::FacesVec* faces = &BD.child_faces[ChildIDX];
	u16* indices = 0;
	//.	R_CHK				(V->pIndices->Lock(iBase,iCount,		(void**)&indices,	D3DLOCK_READONLY));
	CHK_DX(V->p_rm_Indices->Lock(0, V->dwPrimitives * 3, (void**)&indices, D3DLOCK_READONLY));
	// fill vertices
	switch (RenderMode)
	{
	case RM_SKINNING_SOFT:
		if (*Vertices1W)
			_FillVerticesSoft1W(view, wm, normal, size, indices + iBase, *faces);
		else
			_FillVerticesSoft2W(view, wm, normal, size, indices + iBase, *faces);
		break;
	case RM_SINGLE:
	case RM_SKINNING_1B:
		_FillVerticesHW1W(view, wm, normal, size, V, indices + iBase, *faces);
		break;
	case RM_SKINNING_2B:
		_FillVerticesHW2W(view, wm, normal, size, V, indices + iBase, *faces);
		break;
	}
	CHK_DX(V->p_rm_Indices->Unlock());
}

void CSkeletonX_ST::FillVertices(const fmat4x4& view, CSkeletonWallmark& wm, const fvec3& normal, float size,
								 u16 bone_id)
{
	inherited2::_FillVertices(view, wm, normal, size, this, bone_id, iBase, iCount);
}
void CSkeletonX_PM::FillVertices(const fmat4x4& view, CSkeletonWallmark& wm, const fvec3& normal, float size,
								 u16 bone_id)
{
	FSlideWindow& SW = nSWI.sw[0];
	inherited2::_FillVertices(view, wm, normal, size, this, bone_id, iBase + SW.offset, SW.num_tris * 3);
}

template <typename vertex_buffer_type>
IC void TEnumBoneVertices(vertex_buffer_type vertices, u16* indices, CBoneData::FacesVec& faces,
						  SEnumVerticesCallback& C)
{
	for (CBoneData::FacesVecIt it = faces.begin(); it != faces.end(); it++)
	{
		u32 idx = (*it) * 3;
		for (u32 k = 0; k < 3; k++)
		{
			fvec3 P;
			vertices[indices[idx + k]].get_pos(P);
			C(P);
		}
	}
}

void CSkeletonX_ext::_EnumBoneVertices(SEnumVerticesCallback& C, Fvisual* V, u16 bone_id, u32 iBase, u32 iCount) const
{

	VERIFY(Parent && (ChildIDX != u16(-1)));
	CBoneData& BD = Parent->LL_GetData(bone_id);
	CBoneData::FacesVec* faces = &BD.child_faces[ChildIDX];
	u16* indices = 0;
	//.	R_CHK				(V->pIndices->Lock(iBase,iCount,		(void**)&indices,	D3DLOCK_READONLY));
	CHK_DX(V->p_rm_Indices->Lock(0, V->dwPrimitives * 3, (void**)&indices, D3DLOCK_READONLY));
	// fill vertices
	void* vertices = 0;
	if (RenderMode != RM_SKINNING_SOFT)
		CHK_DX(V->p_rm_Vertices->Lock(V->vBase, V->vCount, (void**)&vertices, D3DLOCK_READONLY));
	switch (RenderMode)
	{
	case RM_SKINNING_SOFT:
		if (*Vertices1W)
			TEnumBoneVertices(Vertices1W, indices + iBase, *faces, C);
		else
			TEnumBoneVertices(Vertices2W, indices + iBase, *faces, C);
		break;
	case RM_SINGLE:
	case RM_SKINNING_1B:
		TEnumBoneVertices((vertHW_1W*)vertices, indices + iBase, *faces, C);
		break;
	case RM_SKINNING_2B:
		TEnumBoneVertices((vertHW_2W*)vertices, indices + iBase, *faces, C);
		break;
	default:
		NODEFAULT;
	}
	if (RenderMode != RM_SKINNING_SOFT)
		CHK_DX(V->p_rm_Vertices->Unlock());
	CHK_DX(V->p_rm_Indices->Unlock());
}
