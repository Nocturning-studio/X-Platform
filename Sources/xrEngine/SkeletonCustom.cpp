//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#pragma warning(push)
#pragma warning(disable : 4995)
#include <ppl.h>
#pragma warning(pop)

#include "SkeletonCustom.h"
#include "SkeletonX.h"
#include "fmesh.h"
#ifndef _EDITOR
#include "Render.h"
#endif
int psSkeletonUpdate = 32;
xrCriticalSection UCalc_Mutex
#ifdef PROFILE_CRITICAL_SECTIONS
	(MUTEX_PROFILE_ID(UCalc_Mutex))
#endif // PROFILE_CRITICAL_SECTIONS
		;

//////////////////////////////////////////////////////////////////////////
// BoneInstance methods
void CBoneInstance::construct()
{
	ZeroMemory(this, sizeof(*this));
	mTransform.identity();

	mRenderTransform.identity();
	Callback_overwrite = FALSE;
}
void CBoneInstance::set_callback(u32 Type, BoneCallback C, void* Param, BOOL overwrite)
{
	Callback = C;
	Callback_Param = Param;
	Callback_overwrite = overwrite;
	Callback_type = Type;
}
void CBoneInstance::reset_callback()
{
	Callback = 0;
	Callback_Param = 0;
	Callback_overwrite = FALSE;
	Callback_type = 0;
}

void CBoneInstance::set_param(u32 idx, float data)
{
	VERIFY(idx < MAX_BONE_PARAMS);
	param[idx] = data;
}
float CBoneInstance::get_param(u32 idx)
{
	VERIFY(idx < MAX_BONE_PARAMS);
	return param[idx];
}

#ifdef DEBUG
void CBoneData::DebugQuery(BoneDebug& L)
{
	for (u32 i = 0; i < children.size(); i++)
	{
		L.push_back(SelfID);
		L.push_back(children[i]->SelfID);
		children[i]->DebugQuery(L);
	}
}
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
bool pred_N(const std::pair<shared_str, u32>& N, LPCSTR B)
{
	return xr_strcmp(*N.first, B) < 0;
}
u16 CKinematics::LL_BoneID(LPCSTR B)
{
	accel::iterator I = std::lower_bound(bone_map_N->begin(), bone_map_N->end(), B, pred_N);
	if (I == bone_map_N->end())
		return BI_NONE;
	if (0 != xr_strcmp(*(I->first), B))
		return BI_NONE;
	return u16(I->second);
}
bool pred_P(const std::pair<shared_str, u32>& N, const shared_str& B)
{
	return N.first._get() < B._get();
}
u16 CKinematics::LL_BoneID(const shared_str& B)
{
	accel::iterator I = std::lower_bound(bone_map_P->begin(), bone_map_P->end(), B, pred_P);
	if (I == bone_map_P->end())
		return BI_NONE;
	if (I->first._get() != B._get())
		return BI_NONE;
	return u16(I->second);
}

//
LPCSTR CKinematics::LL_BoneName_dbg(u16 ID)
{
	CKinematics::accel::iterator _I, _E = bone_map_N->end();
	for (_I = bone_map_N->begin(); _I != _E; ++_I)
		if (_I->second == ID)
			return *_I->first;
	return 0;
}

#ifdef DEBUG
void CKinematics::DebugRender(float4x4& Transform)
{
	CalculateBones();

	CBoneData::BoneDebug dbgLines;
	(*bones)[iRoot]->DebugQuery(dbgLines);

	float3 Z;
	Z.set(0, 0, 0);
	float3 H1;
	H1.set(0.01f, 0.01f, 0.01f);
	float3 H2;
	H2.mul(H1, 2);
	for (u32 i = 0; i < dbgLines.size(); i += 2)
	{
		float4x4& M1 = bone_instances[dbgLines[i]].mTransform;
		float4x4& M2 = bone_instances[dbgLines[i + 1]].mTransform;

		float3 P1, P2;
		M1.transform_tiny(P1, Z);
		M2.transform_tiny(P2, Z);
		RenderBackend.dbg_DrawLINE(Transform, P1, P2, D3DCOLOR_XRGB(0, 255, 0));

		float4x4 M;
		M.mul_43(Transform, M2);
		RenderBackend.dbg_DrawOBB(M, H1, D3DCOLOR_XRGB(255, 255, 255));
		RenderBackend.dbg_DrawOBB(M, H2, D3DCOLOR_XRGB(255, 255, 255));
	}

	for (u32 b = 0; b < bones->size(); b++)
	{
		Fobb& obb = (*bones)[b]->obb;
		float4x4& Mbone = bone_instances[b].mTransform;
		float4x4 Mbox;
		obb.transform_get(Mbox);
		float4x4 X;
		X.mul(Mbone, Mbox);
		float4x4 W;
		W.mul(Transform, X);
		RenderBackend.dbg_DrawOBB(W, obb.m_halfsize, D3DCOLOR_XRGB(0, 0, 255));
	}
}
#endif

CKinematics::CKinematics()
{
#ifdef DEBUG
	dbg_single_use_marker = FALSE;
#endif
}

CKinematics::~CKinematics()
{
	IBoneInstances_Destroy();
	// wallmarks
	ClearWallmarks();

	if (m_lod)
		::Render->model_Delete(m_lod);
}

void CKinematics::IBoneInstances_Create()
{
	// VERIFY2				(bones->size() < 64, "More than 64 bones is a crazy thing!");
	u32 size = bones->size();
	bone_instances = xr_alloc<CBoneInstance>(size);
	for (u32 i = 0; i < size; i++)
		bone_instances[i].construct();
}

void CKinematics::IBoneInstances_Destroy()
{
	if (bone_instances)
	{
		xr_free(bone_instances);
		bone_instances = NULL;
	}
}

bool pred_sort_N(const std::pair<shared_str, u32>& A, const std::pair<shared_str, u32>& B)
{
	return xr_strcmp(A.first, B.first) < 0;
}
bool pred_sort_P(const std::pair<shared_str, u32>& A, const std::pair<shared_str, u32>& B)
{
	return A.first._get() < B.first._get();
}

void CBoneData::CalculateM2B(const float4x4& parent)
{
	// Build matrix
	m2b_transform.mul_43(parent, bind_transform);

	// Calculate children
	for (xr_vector<CBoneData*>::iterator C = children.begin(); C != children.end(); C++)
		(*C)->CalculateM2B(m2b_transform);

	m2b_transform.invert();
}

CSkeletonX* CKinematics::LL_GetChild(u32 idx)
{
	IRender_Visual* V = children[idx];
	CSkeletonX* B = dynamic_cast<CSkeletonX*>(V);
	return B;
}

void CKinematics::Load(const char* N, IReader* data, u32 dwFlags)
{
	// Msg				("skeleton: %s",N);
	inherited::Load(N, data, dwFlags);

	pUserData = NULL;
	m_lod = NULL;
	// loading lods

	IReader* LD = data->open_chunk(OGF_S_LODS);
	if (LD)
	{
		string_path short_name;
		strcpy_s(short_name, sizeof(short_name), N);

		if (strext(short_name))
			*strext(short_name) = 0;
		// From stream
		{
			string_path lod_name;
			LD->r_string(lod_name, sizeof(lod_name));
			//.         strconcat		(sizeof(name_load),name_load, short_name, ":lod:", lod_name.c_str());
			m_lod = ::Render->model_CreateChild(lod_name, NULL);
			VERIFY3(m_lod, "Cant create LOD model for", N);
			//.			VERIFY2			(m_lod->Type==MT_HIERRARHY || m_lod->Type==MT_PROGRESSIVE ||
			//m_lod->Type==MT_NORMAL,lod_name.c_str());
			/*
						strconcat		(name_load, short_name, ":lod:1");
						m_lod 			= ::Render->model_CreateChild(name_load,LD);
						VERIFY			(m_lod->Type==MT_SKELETON_GEOMDEF_PM || m_lod->Type==MT_SKELETON_GEOMDEF_ST);
			*/
		}
		LD->close();
	}

#ifndef _EDITOR
	// User data
	IReader* UD = data->open_chunk(OGF_S_USERDATA);
	pUserData = UD ? xr_new<CInifile>(UD, FS.get_path("$game_config$")->m_Path) : 0;
	if (UD)
		UD->close();
#endif

	// Globals
	bone_map_N = xr_new<accel>();
	bone_map_P = xr_new<accel>();
	bones = xr_new<vecBones>();
	bone_instances = NULL;

	// Load bones
	R_ASSERT(data->find_chunk(OGF_S_BONE_NAMES));

	visimask.zero();
	int dwCount = data->r_u32();

	VERIFY3(dwCount < 64, "More than 64 bones is a crazy thing!", N);

	xr_vector<shared_str> L_parents;
	L_parents.reserve(dwCount); 

	for (; dwCount; dwCount--)
	{
		string256 buf;

		// Bone
		u16 ID = u16(bones->size());
		data->r_stringZ(buf, sizeof(buf));
		xr_strlwr(buf);
		CBoneData* pBone = CreateBoneData(ID);
		pBone->name = shared_str(buf);
		pBone->child_faces.resize(children.size());
		bones->push_back(pBone);
		bone_map_N->push_back(mk_pair(pBone->name, ID));
		bone_map_P->push_back(mk_pair(pBone->name, ID));

		// It's parent
		data->r_stringZ(buf, sizeof(buf));
		xr_strlwr(buf);
		L_parents.push_back(buf);

		data->r(&pBone->obb, sizeof(Fobb));
		visimask.set(u64(1) << ID, TRUE);
	}
	concurrency::parallel_sort(bone_map_N->begin(), bone_map_N->end(), pred_sort_N);
	concurrency::parallel_sort(bone_map_P->begin(), bone_map_P->end(), pred_sort_P);

	// Attach bones to their parents
	iRoot = BI_NONE;
	for (u32 i = 0; i < bones->size(); i++)
	{
		shared_str P = L_parents[i];
		CBoneData* B = (*bones)[i];
		if (!P || !P[0])
		{
			// no parent - this is root bone
			R_ASSERT(BI_NONE == iRoot);
			iRoot = u16(i);
			B->SetParentID(BI_NONE);
			continue;
		}
		else
		{
			u16 ID = LL_BoneID(P);
			R_ASSERT(ID != BI_NONE);
			(*bones)[ID]->children.push_back(B);
			B->SetParentID(ID);
		}
	}
	R_ASSERT(BI_NONE != iRoot);

	// Free parents
	L_parents.clear();

	// IK data
	IReader* IKD = data->open_chunk(OGF_S_IKDATA);
	if (IKD)
	{
		for (u32 i = 0; i < bones->size(); i++)
		{
			CBoneData* B = (*bones)[i];
			u16 vers = (u16)IKD->r_u32();
			IKD->r_stringZ(B->game_mtl_name);
			IKD->r(&B->shape, sizeof(SBoneShape));
			B->IK_data.Import(*IKD, vers);
			float3 vXYZ, vT;
			IKD->r_fvector3(vXYZ);
			IKD->r_fvector3(vT);
			B->bind_transform.setXYZi(vXYZ);
			B->bind_transform.translate_over(vT);
			B->mass = IKD->r_float();
			IKD->r_fvector3(B->center_of_mass);
		}
		// calculate model to bone converting matrix
		(*bones)[LL_GetBoneRoot()]->CalculateM2B(Fidentity);
		IKD->close();
	}

	// after load process
	{
		for (u16 child_idx = 0; child_idx < (u16)children.size(); child_idx++)
			LL_GetChild(child_idx)->AfterLoad(this, child_idx);
	}

	// unique bone faces
	{
		for (u32 bone_idx = 0; bone_idx < bones->size(); bone_idx++)
		{
			CBoneData* B = (*bones)[bone_idx];
			for (u32 child_idx = 0; child_idx < children.size(); child_idx++)
			{
				CBoneData::FacesVec faces = B->child_faces[child_idx];
				concurrency::parallel_sort(faces.begin(), faces.end());
				CBoneData::FacesVecIt new_end = std::unique(faces.begin(), faces.end());
				faces.erase(new_end, faces.end());
				B->child_faces[child_idx].clear_and_free();
				B->child_faces[child_idx] = faces;
			}
		}
	}

	// reset update_callback
	Update_Callback = NULL;
	// reset update frame
	wm_frame = u32(-1);

	LL_Validate();
}

IC void iBuildGroups(CBoneData* B, U16Vec& tgt, u16 id, u16& last_id)
{
	if (B->IK_data.ik_flags.is(SJointIKData::flBreakable))
		id = ++last_id;
	tgt[B->GetSelfID()] = id;
	for (xr_vector<CBoneData*>::iterator bone_it = B->children.begin(); bone_it != B->children.end(); bone_it++)
		iBuildGroups(*bone_it, tgt, id, last_id);
}

void CKinematics::LL_Validate()
{
	// check breakable
	BOOL bCheckBreakable = FALSE;
	for (u16 k = 0; k < LL_BoneCount(); k++)
	{
		if (LL_GetData(k).IK_data.ik_flags.is(SJointIKData::flBreakable) && (LL_GetData(k).IK_data.type != jtNone))
		{
			bCheckBreakable = TRUE;
			break;
		}
	}

	if (bCheckBreakable)
	{
		BOOL bValidBreakable = TRUE;

		xr_vector<xr_vector<u16>> groups;
		LL_GetBoneGroups(groups);

		xr_vector<u16> b_parts(LL_BoneCount(), BI_NONE);

		CBoneData* root = &LL_GetData(LL_GetBoneRoot());
		u16 last_id = 0;
		iBuildGroups(root, b_parts, 0, last_id);

		for (u16 g = 0; g < (u16)groups.size(); ++g)
		{
			xr_vector<u16>& group = groups[g];
			if (group.empty())
				continue; // На всякий случай защита от пустых групп

			u16 bp_id = b_parts[group[0]];
			for (u32 b = 1; b < group.size(); b++)
			{
				if (bp_id != b_parts[group[b]])
				{
					bValidBreakable = FALSE;
					break;
				}
			}
		}

		if (bValidBreakable == FALSE)
		{
			for (u16 k = 0; k < LL_BoneCount(); k++)
			{
				CBoneData& BD = LL_GetData(k);
				if (BD.IK_data.ik_flags.is(SJointIKData::flBreakable))
					BD.IK_data.ik_flags.set(SJointIKData::flBreakable, FALSE);
			}
#ifdef DEBUG
			Msg("! ERROR: Invalid breakable object: '%s'", *dbg_name);
#endif
		}
	}
}

#define PCOPY(a) a = pFrom->a
void CKinematics::Copy(IRender_Visual* P)
{
	inherited::Copy(P);

	CKinematics* pFrom = (CKinematics*)P;
	PCOPY(pUserData);
	PCOPY(bones);
	PCOPY(iRoot);
	PCOPY(bone_map_N);
	PCOPY(bone_map_P);
	PCOPY(visimask);

	IBoneInstances_Create();

	for (u32 i = 0; i < children.size(); i++)
		LL_GetChild(i)->SetParent(this);

	CalculateBones_Invalidate();

	m_lod = (pFrom->m_lod) ? ::Render->model_Duplicate(pFrom->m_lod) : 0;
}

void CKinematics::CalculateBones_Invalidate()
{
	UCalc_Time = 0x0;
	UCalc_Visibox = psSkeletonUpdate;
}

void CKinematics::Spawn()
{
	inherited::Spawn();
	// bones
	for (u32 i = 0; i < bones->size(); i++)
		bone_instances[i].construct();
	Update_Callback = NULL;
	CalculateBones_Invalidate();
	// wallmarks
	ClearWallmarks();
	Visibility_Invalidate();
}

void CKinematics::Depart()
{
	inherited::Depart();
	// wallmarks
	ClearWallmarks();

	// unmask all bones
	visimask.zero();
	if (bones)
	{
		for (u32 b = 0; b < bones->size(); b++)
			visimask.set((u64(1) << b), TRUE);
	}
	// visibility
	children.insert(children.end(), children_invisible.begin(), children_invisible.end());
	children_invisible.clear();
}

void CKinematics::Release()
{
	// xr_free bones
	for (u32 i = 0; i < bones->size(); i++)
	{
		CBoneData*& B = (*bones)[i];
		xr_delete(B);
	}

	// destroy shared data
	xr_delete(pUserData);
	xr_delete(bones);
	xr_delete(bone_map_N);
	xr_delete(bone_map_P);

	if (m_lod)
		m_lod->Release();
	inherited::Release();
}

void CKinematics::LL_SetBoneVisible(u16 bone_id, BOOL val, BOOL bRecursive)
{
	VERIFY(bone_id < LL_BoneCount());
	u64 mask = u64(1) << bone_id;
	visimask.set(mask, val);
	if (!visimask.is(mask))
	{
		bone_instances[bone_id].mTransform.scale(0.f, 0.f, 0.f);
	}
	else
	{
		CalculateBones_Invalidate();
	}
	bone_instances[bone_id].mRenderTransform.mul_43(bone_instances[bone_id].mTransform,
													(*bones)[bone_id]->m2b_transform);
	if (bRecursive)
	{
		for (xr_vector<CBoneData*>::iterator C = (*bones)[bone_id]->children.begin();
			 C != (*bones)[bone_id]->children.end(); C++)
			LL_SetBoneVisible((*C)->GetSelfID(), val, bRecursive);
	}
	Visibility_Invalidate();
}

void CKinematics::LL_SetBonesVisible(u64 mask)
{
	visimask.assign(0);
	for (u32 b = 0; b < bones->size(); b++)
	{
		u64 bm = u64(1) << b;
		if (mask & bm)
		{
			visimask.set(bm, TRUE);
		}
		else
		{
			float4x4& A = bone_instances[b].mTransform;
			float4x4& B = bone_instances[b].mRenderTransform;
			A.scale(0.f, 0.f, 0.f);
			B.mul_43(A, (*bones)[b]->m2b_transform);
		}
	}
	CalculateBones_Invalidate();
	Visibility_Invalidate();
}

void CKinematics::Visibility_Update()
{
	Update_Visibility = FALSE;
	// check visible
	for (u32 c_it = 0; c_it < children.size(); c_it++)
	{
		CSkeletonX* _c = dynamic_cast<CSkeletonX*>(children[c_it]);
		VERIFY(_c);
		if (!_c->has_visible_bones())
		{
			// move into invisible list
			children_invisible.push_back(children[c_it]);
			std::swap(children[c_it], children.back());
			children.pop_back();
		}
	}

	// check invisible
	for (u32 _it = 0; _it < children_invisible.size(); _it++)
	{
		CSkeletonX* _c = dynamic_cast<CSkeletonX*>(children_invisible[_it]);
		VERIFY(_c);
		if (_c->has_visible_bones())
		{
			// move into visible list
			children.push_back(children_invisible[_it]);
			std::swap(children_invisible[_it], children_invisible.back());
			children_invisible.pop_back();
		}
	}
}

IC static void RecursiveBindTransform(CKinematics* K, xr_vector<float4x4>& matrices, u16 bone_id, const float4x4& parent)
{
	CBoneData& BD = K->LL_GetData(bone_id);
	float4x4& BM = matrices[bone_id];
	// Build matrix
	BM.mul_43(parent, BD.bind_transform);
	for (xr_vector<CBoneData*>::iterator C = BD.children.begin(); C != BD.children.end(); C++)
		RecursiveBindTransform(K, matrices, (*C)->GetSelfID(), BM);
}

void CKinematics::LL_GetBindTransform(xr_vector<float4x4>& matrices)
{
	matrices.resize(LL_BoneCount());
	RecursiveBindTransform(this, matrices, iRoot, Fidentity);
}

void BuildMatrix(float4x4& mView, float invsz, const float3 norm, const float3& from)
{
	// build projection
	float4x4 mScale;
	float3 at, up, right, y;
	at.sub(from, norm);
	y.set(0, 1, 0);
	if (_abs(norm.y) > .99f)
		y.set(1, 0, 0);
	right.crossproduct(y, norm);
	up.crossproduct(norm, right);
	mView.build_camera(from, at, up);
	mScale.scale(invsz, invsz, invsz);
	mView.mulA_43(mScale);
}
void CKinematics::EnumBoneVertices(SEnumVerticesCallback& C, u16 bone_id)
{
	for (u32 i = 0; i < children.size(); i++)
		LL_GetChild(i)->EnumBoneVertices(C, bone_id);
}
#include "../xrCDB/cl_intersect.h"

DEFINE_VECTOR(Fobb, OBBVec, OBBVecIt);

bool CKinematics::PickBone(const float4x4& parent_transform, float3& normal, float& dist, const float3& start,
						   const float3& dir, u16 bone_id)
{
	float3 S, D; // normal		= {0,0,0}
	// transform ray from world to model
	float4x4 P;
	P.invert(parent_transform);
	P.transform_tiny(S, start);
	P.transform_dir(D, dir);
	for (u32 i = 0; i < children.size(); i++)
		if (LL_GetChild(i)->PickBone(normal, dist, S, D, bone_id))
		{
			parent_transform.transform_dir(normal);
			return true;
		}
	return false;
}

void CKinematics::AddWallmark(const float4x4* parent_transform, const float3& start, const float3& dir,
							  ref_shader _shader, float size)
{
	float3 S, D, normal = {0, 0, 0};
	// transform ray from world to model
	float4x4 P;
	P.invert(*parent_transform);
	P.transform_tiny(S, start);
	P.transform_dir(D, dir);
	// find pick point
	float dist = flt_max;
	BOOL picked = FALSE;

	DEFINE_VECTOR(Fobb, OBBVec, OBBVecIt);
	OBBVec cache_obb;
	cache_obb.resize(LL_BoneCount());

	for (u16 k = 0; k < LL_BoneCount(); k++)
	{
		CBoneData& BD = LL_GetData(k);
		if (LL_GetBoneVisible(k) && !BD.shape.flags.is(SBoneShape::sfNoPickable))
		{
			Fobb& obb = cache_obb[k];
			obb.transform(BD.obb, LL_GetBoneInstance(k).mTransform);
			if (CDB::TestRayOBB(S, D, obb))
				for (u32 i = 0; i < children.size(); i++)
					if (LL_GetChild(i)->PickBone(normal, dist, S, D, k))
						picked = TRUE;
		}
	}
	if (!picked)
		return;

	// calculate contact point
	float3 cp;
	cp.mad(S, D, dist);

	// collect collide boxes
	Fsphere test_sphere;
	test_sphere.set(cp, size);
	U16Vec test_bones;
	test_bones.reserve(LL_BoneCount());

	for (u16 k = 0; k < LL_BoneCount(); k++)
	{
		CBoneData& BD = LL_GetData(k);
		if (LL_GetBoneVisible(k) && !BD.shape.flags.is(SBoneShape::sfNoPickable))
		{
			Fobb& obb = cache_obb[k];
			if (CDB::TestSphereOBB(test_sphere, obb))
				test_bones.push_back(k);
		}
	}

	// find similar wm
	for (u32 wm_idx = 0; wm_idx < wallmarks.size(); wm_idx++)
	{
		intrusive_ptr<CSkeletonWallmark>& wm = wallmarks[wm_idx];
		if (wm->Similar(_shader, cp, 0.02f))
		{
			if (wm_idx < wallmarks.size() - 1)
				wm = wallmarks.back();
			wallmarks.pop_back();
			break;
		}
	}

	// ok. allocate wallmark
	intrusive_ptr<CSkeletonWallmark> wm = xr_new<CSkeletonWallmark>(this, parent_transform, _shader, cp, Engine.TimeManager.GetGlobalTime());
	wm->m_LocalBounds.set(cp, size * 2.f);
	wm->Transform()->transform_tiny(wm->m_Bounds.P, cp);
	wm->m_Bounds.R = wm->m_Bounds.R;

	float3 tmp;
	tmp.invert(D);
	normal.add(tmp).normalize();

	// build UV projection matrix
	float4x4 mView, mRot;
	BuildMatrix(mView, 1 / (0.9f * size), normal, cp);
	mRot.rotateZ(::Random.randF(deg2rad(-20.f), deg2rad(20.f)));
	mView.mulA_43(mRot);

	// fill vertices
	for (u32 i = 0; i < children.size(); i++)
	{
		CSkeletonX* Skeleton = LL_GetChild(i);
		for (U16It b_it = test_bones.begin(); b_it != test_bones.end(); b_it++)
			Skeleton->FillVertices(mView, *wm, normal, size, *b_it);
	}

	wallmarks.push_back(wm);
}

static const float LIFE_TIME = 30.f;
struct zero_wm_pred
{
	bool operator()(const intrusive_ptr<CSkeletonWallmark> x)
	{
		return x == 0;
	}
};

void CKinematics::CalculateWallmarks()
{
	// Защита от одновременного доступа
	Wallmarks_Mutex.Enter();

	if (!wallmarks.empty() && (wm_frame != Engine.TimeManager.GetFrameCount()))
	{
		wm_frame = Engine.TimeManager.GetFrameCount();
		bool need_remove = false;
		for (SkeletonWMVecIt it = wallmarks.begin(); it != wallmarks.end(); it++)
		{
			intrusive_ptr<CSkeletonWallmark>& wm = *it;
			float w = (Engine.TimeManager.GetGlobalTime() - wm->TimeStart()) / LIFE_TIME;
			if (w < 1.f)
			{
				// append wm to WallmarkEngine
				if (::Render->ViewBase.testSphere_dirty(wm->m_Bounds.P, wm->m_Bounds.R))
					::Render->add_SkeletonWallmark(wm);
			}
			else
			{
				// remove wallmark
				need_remove = true;
			}
		}
		if (need_remove)
		{
			SkeletonWMVecIt new_end = std::remove_if(wallmarks.begin(), wallmarks.end(), zero_wm_pred());
			wallmarks.erase(new_end, wallmarks.end());
		}
	}

	Wallmarks_Mutex.Leave();
}

void CKinematics::RenderWallmark(intrusive_ptr<CSkeletonWallmark> wm, FVF::LIT*& V)
{
	VERIFY(wm);
	VERIFY(V);
	VERIFY2(bones, "Invalid visual. Bones already released.");
	VERIFY2(bone_instances, "Invalid visual. bone_instances already deleted.");

	if ((wm == 0) || (0 == bones) || (0 == bone_instances))
		return;

	// skin vertices
	for (u32 f_idx = 0; f_idx < wm->m_Faces.size(); f_idx++)
	{
		CSkeletonWallmark::WMFace F = wm->m_Faces[f_idx];
		float w = (Engine.TimeManager.GetGlobalTime() - wm->TimeStart()) / LIFE_TIME;
		for (u32 k = 0; k < 3; k++)
		{
			float3 P;
			if (F.bone_id[k][0] == F.bone_id[k][1])
			{
				// 1-link
				float4x4& transform0 = LL_GetBoneInstance(F.bone_id[k][0]).mRenderTransform;
				transform0.transform_tiny(P, F.vert[k]);
			}
			else
			{
				// 2-link
				float3 P0, P1;
				float4x4& transform0 = LL_GetBoneInstance(F.bone_id[k][0]).mRenderTransform;
				float4x4& transform1 = LL_GetBoneInstance(F.bone_id[k][1]).mRenderTransform;
				transform0.transform_tiny(P0, F.vert[k]);
				transform1.transform_tiny(P1, F.vert[k]);
				P.lerp(P0, P1, F.weight[k]);
			}
			wm->Transform()->transform_tiny(V->p, P);
			V->t.set(F.uv[k]);
			int aC = iFloor(w * 255.f);
			clamp(aC, 0, 255);
			V->color = color_rgba(128, 128, 128, aC);
			V++;
		}
	}
	wm->Transform()->transform_tiny(wm->m_Bounds.P, wm->m_LocalBounds.P);
}

void CKinematics::ClearWallmarks()
{
	//	for (SkeletonWMVecIt it=wallmarks.begin(); it!=wallmarks.end(); it++)
	//		xr_delete	(*it);
	wallmarks.clear();
}

int CKinematics::LL_GetBoneGroups(xr_vector<xr_vector<u16>>& groups)
{
	groups.resize(children.size());
	for (u16 bone_idx = 0; bone_idx < (u16)bones->size(); bone_idx++)
	{
		CBoneData* B = (*bones)[bone_idx];
		for (u32 child_idx = 0; child_idx < children.size(); child_idx++)
		{
			if (!B->child_faces[child_idx].empty())
			{
				groups[child_idx].push_back(bone_idx);
			}
		}
	}
	return groups.size();
}

#ifdef DEBUG
CSkeletonWallmark::~CSkeletonWallmark()
{
	if (used_in_render != u32(-1))
	{
		Msg("used_in_render=%d", used_in_render);
		VERIFY(used_in_render == u32(-1));
	}
}
#endif
