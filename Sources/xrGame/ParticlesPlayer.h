///////////////////////////////////////////////////////////////
// ParticlesPlayer.h
// интерфейс для проигрывания партиклов на объекте
///////////////////////////////////////////////////////////////

#pragma once

#include "ParticlesObject.h"
#include "../xrEngine/bone.h"

DEFINE_VECTOR(CParticlesObject*, PARTICLES_PTR_VECTOR, PARTICLES_PTR_VECTOR_IT);

class CObject;

class CParticlesPlayer
{

  public:
	// структура с внутренней информацией о партикле
	struct SParticlesInfo
	{
		CParticlesObject* ps;
		// float3				dir;
		// float4x4				x_form;
		float3 angles;
		u16 sender_id; // id - объекта, который запустил партиклы
		u32 life_time; // время жизни партикла (-1) - бесконечно

		// int					cur_time;	//текущее время существования партикла
		// bool				auto_stop;	//автоматическая остановка партиклов, когда закончится время
	};
	DEFINE_VECTOR(SParticlesInfo, ParticlesInfoList, ParticlesInfoListIt);

	// структура для косточки с списком запущенных партиклов
	struct SBoneInfo
	{
		u16 index;
		float3 offset;
		ParticlesInfoList particles;
		SParticlesInfo* FindParticles(const shared_str& ps_name);

	  public:
		SBoneInfo(u16 idx, const float3& offs) : index(idx), offset(offs)
		{
			;
		}
		SParticlesInfo* AppendParticles(CObject* object, const shared_str& ps_name);
		void StopParticles(const shared_str& ps_name, bool bDestroy);
		void StopParticles(u16 sender_id, bool bDestroy);
	};
	DEFINE_VECTOR(SBoneInfo, BoneInfoVec, BoneInfoVecIt);

  private:
	// список костей
	u64 bone_mask; // используемые кости
	BoneInfoVec m_Bones;
	CObject* m_self_object;

  protected:
	bool m_bActiveBones; // есть ли косточки на которых играются партиклы

  public:
	IC SBoneInfo* get_bone_info(u16 bone_index)
	{
		if (BI_NONE == bone_index)
			return 0;
		for (BoneInfoVecIt it = m_Bones.begin(); it != m_Bones.end(); it++)
			if (it->index == bone_index)
				return &(*it);
		return 0;
	}
	SBoneInfo* get_nearest_bone_info(CKinematics* K, u16 bone_index);
	float3 parent_vel;

  public:
	CParticlesPlayer(void);
	virtual ~CParticlesPlayer(void);
	void LoadParticles(CKinematics* K);

	void net_DestroyParticles();
	void net_SpawnParticles();

	void UpdateParticles();

	void StartParticles(const shared_str& ps_name, u16 bone_num, const float3& dir, u16 sender_id, int life_time = -1,
						bool auto_stop = true);
	void StartParticles(const shared_str& ps_name, const float3& dir, u16 sender_id, int life_time = -1,
						bool auto_stop = true);

	void StartParticles(const shared_str& ps_name, u16 bone_num, const float4x4& dir, u16 sender_id, int life_time = -1,
						bool auto_stop = true);
	void StartParticles(const shared_str& ps_name, const float4x4& dir, u16 sender_id, int life_time = -1,
						bool auto_stop = true);

	void StopParticles(u16 sender_ID, u16 bone_id, bool bDestroy);
	void StopParticles(const shared_str& particles_name, u16 bone_id, bool bDestroy);
	void AutoStopParticles(const shared_str& ps_name, u16 bone_id, u32 life_time);

	static void MakeTransform(CObject* pObject, u16 bone_id, const float3& dir, const float3& offset, float4x4& result);
	static void GetBonePos(CObject* pObject, u16 bone_id, const float3& offset, float3& result);
	u16 GetNearestBone(CKinematics* K, u16 bone_id);
	IC u16 GetRandomBone()
	{
		u16 l_PBCount = u16(m_Bones.size());
		if (l_PBCount)
			return m_Bones[(u16)Random.randI(l_PBCount)].index;
		else
			return BI_NONE;
	}

	void SetParentVel(const float3& vel)
	{
		parent_vel = vel;
	}

	bool IsPlaying()
	{
		return m_bActiveBones;
	}
	virtual CParticlesPlayer* cast_particles_player()
	{
		return this;
	}
};
