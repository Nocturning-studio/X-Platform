#pragma once

#include "../xrEngine/SkeletonAnimated.h"
class CHudItem;

struct weapon_hud_value : public shared_value
{
	CKinematicsAnimated* m_animations;

  public:
	int m_fire_bone;
	float3 m_fp_offset;
	float3 m_fp2_offset;
	float3 m_sp_offset;

	float4x4 m_offset;

  public:
	virtual ~weapon_hud_value();
	BOOL load(const shared_str& section, CHudItem* owner);
};

typedef shared_container<weapon_hud_value> weapon_hud_container;
extern weapon_hud_container* g_pWeaponHUDContainer;

class shared_weapon_hud : public shared_item<weapon_hud_value>
{
  protected:
	struct on_new_pred
	{
		CHudItem* owner;
		on_new_pred(CHudItem* _owner) : owner(_owner)
		{
		}
		BOOL operator()(const shared_str& key, weapon_hud_value* val) const
		{
			return val->load(key, owner);
		}
	};

  public:
	void create(shared_str key, CHudItem* owner)
	{
		shared_item<weapon_hud_value>::create(key, g_pWeaponHUDContainer, on_new_pred(owner));
	}
	CKinematicsAnimated* animations()
	{
		return p_->m_animations;
	}
	u32 motion_length(MotionID M);
	MotionID motion_id(LPCSTR name);
};
//---------------------------------------------------------------------------

class CWeaponHUD
{
	// родительский объект HUD
	CHudItem* m_pParentWeapon;
	// флаг, если hud спрятан не показывается
	bool m_bHidden;
	bool m_bVisible;

	float4x4 m_Transform;

	// shared HUD data
	shared_weapon_hud m_shared_data;

	// таймеры для проигрывания анимаций
	u32 m_dwAnimTime;
	u32 m_dwAnimEndTime;
	bool m_bStopAtEndAnimIsRunning;
	u32 m_startedAnimState;
	//	CInventoryItem*		m_pCallbackItem;
	CHudItem* m_pCallbackItem;

	// остановление таймера текущей анимации, и вызов callback
	void StopCurrentAnim();

	// поворот и смещение для режима приближения
	float m_fZoomRotateX;
	float m_fZoomRotateY;
	float3 m_fZoomOffset;

  public:
	CWeaponHUD(CHudItem* pHudItem);
	~CWeaponHUD();

	// misc
	void Load(LPCSTR section);
	void net_DestroyHud();
	void Init();

	IC IRender_Visual* Visual()
	{
		return m_shared_data.animations();
	}
	IC float4x4& Transform()
	{
		return m_Transform;
	}

	int FireBone()
	{
		return m_shared_data.get_value()->m_fire_bone;
	}
	const float3& FirePoint()
	{
		return m_shared_data.get_value()->m_fp_offset;
	}
	const float3& FirePoint2()
	{
		return m_shared_data.get_value()->m_fp2_offset;
	}
	const float3& ShellPoint()
	{
		return m_shared_data.get_value()->m_sp_offset;
	}

	const float3& ZoomOffset() const
	{
		return m_fZoomOffset;
	}
	float ZoomRotateX() const
	{
		return m_fZoomRotateX;
	}
	float ZoomRotateY() const
	{
		return m_fZoomRotateY;
	}
	void SetZoomOffset(const float3& zoom_offset)
	{
		m_fZoomOffset = zoom_offset;
	}
	void SetZoomRotateX(float zoom_rotate_x)
	{
		m_fZoomRotateX = zoom_rotate_x;
	}
	void SetZoomRotateY(float zoom_rotate_y)
	{
		m_fZoomRotateY = zoom_rotate_y;
	}

	// Animations
	void animPlay(MotionID M, BOOL bMixIn /*=TRUE*/, CHudItem* _W /*=0*/, u32 state);
	void animDisplay(MotionID M, BOOL bMixIn);
	MotionID animGet(LPCSTR name);

	void UpdatePosition(const float4x4& transform);

	bool IsHidden()
	{
		return m_bHidden;
	}
	void Hide()
	{
		m_bHidden = true;
	}
	void Show()
	{
		m_bHidden = false;
	}

	void Visible(bool val)
	{
		m_bVisible = val;
	}

	// обновление HUD должно вызываться на каждом кадре
	void Update();

	void StopCurrentAnimWithoutCallback();

  public:
	static void CreateSharedContainer();
	static void DestroySharedContainer();
	static void CleanSharedContainer();
#ifdef DEBUG
  public:
	void dbg_SetFirePoint(const float3& fp)
	{
		((weapon_hud_value*)m_shared_data.get_value())->m_fp_offset.set(fp);
	}
	void dbg_SetFirePoint2(const float3& fp)
	{
		((weapon_hud_value*)m_shared_data.get_value())->m_fp2_offset.set(fp);
	}
	void dbg_SetShellPoint(const float3& sp)
	{
		((weapon_hud_value*)m_shared_data.get_value())->m_sp_offset.set(sp);
	}
#endif
};

#define MAX_ANIM_COUNT_WEAPON_HUD 8
typedef svector<MotionID, MAX_ANIM_COUNT_WEAPON_HUD> MotionSVec;
MotionID random_anim(MotionSVec& v);
