//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "SkeletonCustom.h"

extern int psSkeletonUpdate;

#ifdef DEBUG
void check_kinematics(CKinematics* _k, LPCSTR s);
#endif

void CKinematics::CalculateBones(BOOL bForceExact)
{
	u32 global_time = Engine.TimeManager.GetGlobalTimeMs();

	// 1. Быстрая проверка (Fast Path) - без локов
	if (global_time == UCalc_Time && !bForceExact)
		return;

	// 2. Захват блокировки
	UCalc_Mutex.Enter();

	// 3. Повторная проверка (Double Check) - под локом
	if (global_time == UCalc_Time && !bForceExact)
	{
		UCalc_Mutex.Leave();
		return;
	}

	// 4. Логика интервала
	if (!bForceExact && (global_time < (UCalc_Time + UCalc_Interval)))
	{
		UCalc_Mutex.Leave();
		return;
	}

	// 5. Расчет (State Mutation)
	if (Update_Visibility)
		Visibility_Update();

	OnCalculateBones();

#ifdef DEBUG
	Engine.Statistic->Animation.Begin();
#endif

	// Расчет иерархии костей (Тяжелая операция)
	Bone_Calculate(bones->at(iRoot), &Fidentity);

#ifdef DEBUG
	check_kinematics(this, dbg_name.c_str());
	Engine.Statistic->Animation.End();
#endif

	// 6. Расчет Bounding Box (Visibox)
	UCalc_Visibox++;
	if (UCalc_Visibox >= psSkeletonUpdate)
	{
		UCalc_Visibox = -(::Random.randI(psSkeletonUpdate - 1));

		Fbox Box;
		Box.invalidate();
		for (u32 b = 0; b < bones->size(); b++)
		{
			if (!LL_GetBoneVisible(u16(b)))
				continue;

			Fobb& obb = (*bones)[b]->obb;
			fmat4x4& Mbone = bone_instances[b].mTransform;
			fmat4x4 Mbox;
			obb.transform_get(Mbox);
			fmat4x4 X;
			X.mul_43(Mbone, Mbox);
			fvec3& S = obb.m_halfsize;

			fvec3 P, A;

			A.set(-S.x, -S.y, -S.z);
			X.transform_tiny(P, A);
			Box.modify(P);
			A.set(-S.x, -S.y, S.z);
			X.transform_tiny(P, A);
			Box.modify(P);
			A.set(S.x, -S.y, S.z);
			X.transform_tiny(P, A);
			Box.modify(P);
			A.set(S.x, -S.y, -S.z);
			X.transform_tiny(P, A);
			Box.modify(P);
			A.set(-S.x, S.y, -S.z);
			X.transform_tiny(P, A);
			Box.modify(P);
			A.set(-S.x, S.y, S.z);
			X.transform_tiny(P, A);
			Box.modify(P);
			A.set(S.x, S.y, S.z);
			X.transform_tiny(P, A);
			Box.modify(P);
			A.set(S.x, S.y, -S.z);
			X.transform_tiny(P, A);
			Box.modify(P);
		}

		if (bones->size())
		{
			vis.box.min = (Box.min);
			vis.box.max = (Box.max);
			vis.box.getsphere(vis.sphere.P, vis.sphere.R);
		}
	}

	if (Update_Callback)
		Update_Callback(this);

	// Обновляем время только когда ВСЕ данные (кости и AABB) полностью готовы.
	// Теперь другие потоки, проверяющие Fast Path, увидят новое время
	// только когда данные действительно безопасны для чтения.
	UCalc_Time = global_time;

	// 7. Освобождение блокировки
	UCalc_Mutex.Leave();
}

#ifdef DEBUG
void check_kinematics(CKinematics* _k, LPCSTR s)
{
	CKinematics* K = _k;
	fmat4x4& MrootBone = K->LL_GetBoneInstance(K->LL_GetBoneRoot()).mTransform;
	if (MrootBone.c.y > 10000)
	{
		Msg("all bones transform:--------[%s]", s);

		for (u16 ii = 0; ii < K->LL_BoneCount(); ++ii)
		{
			fmat4x4 tr;

			tr = K->LL_GetTransform(ii);
			Log("bone ", K->LL_BoneName_dbg(ii));
			Log("bone_matrix", tr);
		}
		Log("end-------");
		VERIFY3(0, "check_kinematics failed for ", s);
	}
}
#endif

void CKinematics::Bone_Calculate(CBoneData* bd, fmat4x4* parent)
{
	u16 SelfID = bd->GetSelfID();
	if (LL_GetBoneVisible(SelfID))
	{
		CBoneInstance& INST = LL_GetBoneInstance(SelfID);
		if (INST.Callback_overwrite)
		{
			if (INST.Callback)
				INST.Callback(&INST);
		}
		else
		{
			// Build matrix
			INST.mTransform.mul_43(*parent, bd->bind_transform);
			if (INST.Callback)
				INST.Callback(&INST);
		}
		INST.mRenderTransform.mul_43(INST.mTransform, bd->m2b_transform);

		// Calculate children
		for (xr_vector<CBoneData*>::iterator C = bd->children.begin(); C != bd->children.end(); C++)
			Bone_Calculate(*C, &INST.mTransform);
	}
}
