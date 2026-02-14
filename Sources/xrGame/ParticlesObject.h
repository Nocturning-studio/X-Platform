#ifndef ParticlesObjectH
#define ParticlesObjectH

#include "../xrEngine/PS_instance.h"

extern const float3 zero_vel;

class CParticlesObject : public CPS_Instance
{
	typedef CPS_Instance inherited;

	u32 dwLastTime;
	void Init(LPCSTR p_name, IRender_Sector* S, BOOL bAutoRemove);
	void UpdateSpatial();

  protected:
	bool m_bLooped;	  // флаг, что система зациклена
	bool m_bStopping; // вызвана функция Stop()

  protected:
	u32 mt_dt;

  protected:
	virtual ~CParticlesObject();

  public:
	CParticlesObject(LPCSTR p_name, BOOL bAutoRemove, bool destroy_on_game_load);

	virtual bool shedule_Needed()
	{
		return true;
	};
	virtual float shedule_Scale();
	virtual void shedule_Update(u32 dt);
	virtual void renderable_Render();
	void PerformAllTheWork(u32 dt);
	void __stdcall PerformAllTheWork_mt();

	float3& Position();
	void SetTransform(const float4x4& m);
	IC float4x4& Transform()
	{
		return renderable.transform;
	}
	void UpdateParent(const float4x4& m, const float3& vel);

	void play_at_pos(const float3& pos, BOOL transform = FALSE);
	virtual void Play();
	void Stop(BOOL bDefferedStop = TRUE);
	virtual BOOL Locked()
	{
		return mt_dt;
	}

	bool IsLooped()
	{
		return m_bLooped;
	}
	bool IsAutoRemove();
	bool IsPlaying();
	void SetAutoRemove(bool auto_remove);

	const shared_str Name();

  public:
	static CParticlesObject* Create(LPCSTR p_name, BOOL bAutoRemove = TRUE, bool remove_on_game_load = true)
	{
		return xr_new<CParticlesObject>(p_name, bAutoRemove, remove_on_game_load);
	}
	static void Destroy(CParticlesObject*& p)
	{
		if (p)
		{
			p->PSI_destroy();
			p = 0;
		}
	}
};

#endif /*ParticlesObjectH*/
