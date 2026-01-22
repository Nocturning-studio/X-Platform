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
	// ѕолучаем текущее врем€ один раз
	u32 global_time = Engine.TimeManager.GetGlobalTimeMs();

	// -------------------------------------------------------------------------
	// 1. Ѕыстра€ проверка (Fast Path)
	// -------------------------------------------------------------------------
	// ≈сли кости уже обновлены в этом кадре - выходим без блокировок.
	// Ёто критично дл€ производительности основного потока.
	if (global_time == UCalc_Time && !bForceExact)
		return;

	// -------------------------------------------------------------------------
	// 2. «ахват блокировки
	// -------------------------------------------------------------------------
	// ≈сли мы здесь, значит, кто-то должен посчитать кости.
	// Ѕлокируем мьютекс, чтобы это сделал только один поток.
	UCalc_Mutex.Enter();

	// -------------------------------------------------------------------------
	// 3. ѕовторна€ проверка (Double Check)
	// -------------------------------------------------------------------------
	// ѕока текущий поток ждал освобождени€ UCalc_Mutex, другой поток (например, Shadow Cascade 0)
	// мог уже выполнить расчет и обновить UCalc_Time.
	// ≈сли мы не проверим это снова, мы сделаем работу дважды (и можем испортить данные).
	if (global_time == UCalc_Time && !bForceExact)
	{
		UCalc_Mutex.Leave();
		return;
	}

	// -------------------------------------------------------------------------
	// 4. Ћогика интервала обновлени€ (Slow Update Optimization)
	// -------------------------------------------------------------------------
	// ѕровер€ем, прошло ли достаточно времени дл€ "медленного" обновлени€
	if (!bForceExact && (global_time < (UCalc_Time + UCalc_Interval)))
	{
		UCalc_Mutex.Leave();
		return;
	}

	// -------------------------------------------------------------------------
	// 5. –асчет (State Mutation)
	// -------------------------------------------------------------------------
	// ¬се вызовы, мен€ющие состо€ние под замком
	
	if (Update_Visibility)
		Visibility_Update();

	OnCalculateBones();

	// ќбновл€ем врем€ только сейчас, когда мы уверены, что будем считать
	UCalc_Time = global_time;

#ifdef DEBUG
	Engine.Statistic->Animation.Begin();
#endif

	// —расчет иерархии костей
	Bone_Calculate(bones->at(iRoot), &Fidentity);

#ifdef DEBUG
	check_kinematics(this, dbg_name.c_str());
	Engine.Statistic->Animation.End();
#endif

	// -------------------------------------------------------------------------
	// 6. –асчет Bounding Box (Visibox)
	// -------------------------------------------------------------------------
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
			Fmatrix& Mbone = bone_instances[b].mTransform;
			Fmatrix Mbox;
			obb.transform_get(Mbox);
			Fmatrix X;
			X.mul_43(Mbone, Mbox);
			Fvector& S = obb.m_halfsize;

			Fvector P, A;
			// ... (код расчета 8 точек OBB) ...
			A.set(-S.x, -S.y, -S.z); X.transform_tiny(P, A); Box.modify(P);
			A.set(-S.x, -S.y,  S.z); X.transform_tiny(P, A); Box.modify(P);
			A.set( S.x, -S.y,  S.z); X.transform_tiny(P, A); Box.modify(P);
			A.set( S.x, -S.y, -S.z); X.transform_tiny(P, A); Box.modify(P);
			A.set(-S.x,  S.y, -S.z); X.transform_tiny(P, A); Box.modify(P);
			A.set(-S.x,  S.y,  S.z); X.transform_tiny(P, A); Box.modify(P);
			A.set( S.x,  S.y,  S.z); X.transform_tiny(P, A); Box.modify(P);
			A.set( S.x,  S.y, -S.z); X.transform_tiny(P, A); Box.modify(P);
		}
		
		if (bones->size())
		{
			vis.box.min = (Box.min);
			vis.box.max = (Box.max);
			vis.box.getsphere(vis.sphere.P, vis.sphere.R);
		}
		
#ifdef DEBUG
		// Validate
		VERIFY3(_valid(vis.box.min) && _valid(vis.box.max), "Invalid bones-transform in model", dbg_name.c_str());
		if (vis.sphere.R > 1000.f)
		{
			for (u16 ii = 0; ii < LL_BoneCount(); ++ii)
			{
				Fmatrix tr;
				tr = LL_GetTransform(ii);
				Log("bone ", LL_BoneName_dbg(ii));
				Log("bone_matrix", tr);
			}
			Log("end-------");
		}
		VERIFY3(vis.sphere.R < 1000.f, "Invalid bones-transform in model", dbg_name.c_str());
#endif
	}

	// Callback тоже лучше вызывать под замком, если он читает кости,
	// либо вынести наружу, если он потокобезопасен и т€жел (обычно он читает, так что оставл€ем внутри).
	if (Update_Callback)
		Update_Callback(this);

	// -------------------------------------------------------------------------
	// 7. ќсвобождение блокировки
	// -------------------------------------------------------------------------
	UCalc_Mutex.Leave();
}

#ifdef DEBUG
void check_kinematics(CKinematics* _k, LPCSTR s)
{
	CKinematics* K = _k;
	Fmatrix& MrootBone = K->LL_GetBoneInstance(K->LL_GetBoneRoot()).mTransform;
	if (MrootBone.c.y > 10000)
	{
		Msg("all bones transform:--------[%s]", s);

		for (u16 ii = 0; ii < K->LL_BoneCount(); ++ii)
		{
			Fmatrix tr;

			tr = K->LL_GetTransform(ii);
			Log("bone ", K->LL_BoneName_dbg(ii));
			Log("bone_matrix", tr);
		}
		Log("end-------");
		VERIFY3(0, "check_kinematics failed for ", s);
	}
}
#endif

void CKinematics::Bone_Calculate(CBoneData* bd, Fmatrix* parent)
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
