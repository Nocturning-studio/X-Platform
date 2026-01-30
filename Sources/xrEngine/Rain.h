#pragma once
#ifndef RAIN_EFFECT_H_INCLUDED
#define RAIN_EFFECT_H_INCLUDED

#include "xr_collide_defs.h"

// Forward declarations
class ENGINE_API IRender_DetailModel;

class ENGINE_API CEffect_Rain
{
  private:
	// -------------------------------------------------------------------------
	// Constants & Settings
	// -------------------------------------------------------------------------
	static constexpr int MAX_DESIRED_DROPS = 2500;
	static constexpr float SOURCE_RADIUS = 20.0f;
	static constexpr float SOURCE_OFFSET = 30.0f;
	static constexpr float MAX_DROP_DISTANCE = SOURCE_OFFSET * 1.5f;
	static constexpr float SINK_OFFSET = -(MAX_DROP_DISTANCE - SOURCE_OFFSET);
	static constexpr float DROP_WIDTH = 0.30f;
	static constexpr float DROP_SPEED_MIN = 45.0f;
	static constexpr float DROP_SPEED_MAX = 90.0f;

	static constexpr int MAX_PARTICLES = 1000;
	static constexpr int PARTICLES_CACHE = 400;
	static constexpr float PARTICLE_TIME = 0.3f;

	// -------------------------------------------------------------------------
	// Internal Structures
	// -------------------------------------------------------------------------
	struct RainDrop
	{
		Fvector P;	  // Position
		Fvector Phit; // Hit position (end of life)
		Fvector D;	  // Direction
		float fSpeed;
		u32 dwTime_Life;
		u32 dwTime_Hit;
		u32 uv_set;

		void Invalidate()
		{
			dwTime_Life = 0;
		}
	};

	struct SplashParticle
	{
		SplashParticle *next, *prev;
		Fmatrix mTransform;
		Fsphere bounds;
		float time;
	};

	enum EState
	{
		stIdle = 0,
		stWorking
	};

  public:
	CEffect_Rain();
	~CEffect_Rain();

	void Render();
	void OnFrame();

	void InvalidateState()
	{
		m_state = stIdle;
	}

  private:
	// -------------------------------------------------------------------------
	// Core Logic
	// -------------------------------------------------------------------------
	void SpawnDrop(RainDrop& dest, float radius);
	void SpawnSplash(const Fvector& pos);
	void RenewDrop(RainDrop& dest, float height, BOOL bHit);

	// Physics Helpers
	BOOL RayTrace(const Fvector& s, const Fvector& d, float& range, collide::rq_target tgt);

	// Render Helpers
	void UpdateAndRenderDrops(u32 desired_items, u32 rain_color);
	void UpdateAndRenderSplashes(u32 rain_color);

	// -------------------------------------------------------------------------
	// Particle System (Manual Linked List Management)
	// -------------------------------------------------------------------------
	void InitParticlePool();
	void DestroyParticlePool();

	SplashParticle* AllocateParticle();
	void FreeParticle(SplashParticle* P);

	void ListRemove(SplashParticle* P, SplashParticle*& LST);
	void ListInsert(SplashParticle* P, SplashParticle*& LST);

  private:
	// -------------------------------------------------------------------------
	// Members
	// -------------------------------------------------------------------------

	// Resources
	ref_shader m_sh_rain;
	ref_geom m_geom_rain;
	ref_geom m_geom_drops;
	IRender_DetailModel* m_dm_drop;
	ref_sound m_snd_ambient;

	// Data
	xr_vector<RainDrop> m_drops;
	EState m_state;

	// Particles Data
	xr_vector<SplashParticle> m_particle_pool;
	SplashParticle* m_particle_active;
	SplashParticle* m_particle_idle;
};

#endif // RAIN_EFFECT_H_INCLUDED
