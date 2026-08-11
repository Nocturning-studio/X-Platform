#include "stdafx.h"
#pragma hdrstop

#include "../xrCDB/cl_intersect.h"
#include "SoundRender_Core.h"
#include "SoundRender_Emitter.h"
#include "SoundRender_Target.h"
#include "SoundRender_Source.h"

CSoundRender_Emitter* CSoundRender_Core::i_play(ref_sound* S, BOOL _loop, float delay)
{
	VERIFY(S->_p->feedback == 0);
	CSoundRender_Emitter* E = xr_new<CSoundRender_Emitter>();
	S->_p->feedback = E;
	E->start(S, _loop, delay);
	s_emitters.push_back(E);
	return E;
}

void CSoundRender_Core::update(const fvec3& P, const fvec3& D, const fvec3& N)
{
	u32 it;

	if (0 == bReady)
		return;
	bLocked = TRUE;

	Timer.time_factor(psTimeFactor);
	float new_tm = Timer.GetElapsed_sec();
	fTimer_Delta = new_tm - fTimer_Value;
	float dt_sec = fTimer_Delta;
	fTimer_Value = new_tm;

	s_emitters_u++;

	// Firstly update emitters, which are now being rendered
	for (it = 0; it < s_targets.size(); it++)
	{
		CSoundRender_Target* T = s_targets[it];
		CSoundRender_Emitter* E = T->get_emitter();
		if (E)
		{
			E->update(dt_sec);
			E->marker = s_emitters_u;
			E = T->get_emitter(); // update can stop itself
			if (E)
				T->priority = E->priority();
			else
				T->priority = -1;
		}
		else
		{
			T->priority = -1;
		}
	}

	// Update emmitters
	for (it = 0; it < s_emitters.size(); it++)
	{
		CSoundRender_Emitter* pEmitter = s_emitters[it];
		if (pEmitter->marker != s_emitters_u)
		{
			pEmitter->update(dt_sec);
			pEmitter->marker = s_emitters_u;
		}
		if (!pEmitter->isPlaying())
		{
			xr_delete(pEmitter);
			s_emitters.erase(s_emitters.begin() + it);
			it--;
		}
	}

	// Get currently rendering emitters
	s_targets_defer.clear();
	s_targets_pu++;

	for (it = 0; it < s_targets.size(); it++)
	{
		CSoundRender_Target* T = s_targets[it];
		if (T->get_emitter())
		{
			// Has emmitter, maybe just not started rendering
			if (T->get_Rendering())
			{
				T->fill_parameters();
				T->update();
			}
			else
				s_targets_defer.push_back(T);
		}
	}

	// Commit parameters from pending targets
	if (!s_targets_defer.empty())
	{
		s_targets_defer.erase(std::unique(s_targets_defer.begin(), s_targets_defer.end()), s_targets_defer.end());
		for (it = 0; it < s_targets_defer.size(); it++)
			s_targets_defer[it]->fill_parameters();
	}

	// update listener
	update_listener(P, D, N, dt_sec);

	// Start rendering of pending targets
	if (!s_targets_defer.empty())
	{
		for (it = 0; it < s_targets_defer.size(); it++)
			s_targets_defer[it]->render();
	}

	// Events
	update_events();

	bLocked = FALSE;
}

static u32 g_saved_event_count = 0;
void CSoundRender_Core::update_events()
{
	g_saved_event_count = s_events.size();
	for (u32 it = 0; it < s_events.size(); it++)
	{
		event& E = s_events[it];
		Handler(E.first, E.second);
	}
	s_events.clear_not_free();
}

void CSoundRender_Core::statistic(CSound_stats* dest, CSound_stats_ext* ext)
{
	if (dest)
	{
		dest->_rendered = 0;
		for (u32 it = 0; it < s_targets.size(); it++)
		{
			CSoundRender_Target* T = s_targets[it];
			if (T->get_emitter() && T->get_Rendering())
				dest->_rendered++;
		}
		dest->_simulated = s_emitters.size();
		dest->_cache_hits = cache._stat_hit;
		dest->_cache_misses = cache._stat_miss;
		dest->_events = g_saved_event_count;
		cache.stats_clear();
	}
	if (ext)
	{
		for (u32 it = 0; it < s_emitters.size(); it++)
		{
			CSoundRender_Emitter* _E = s_emitters[it];
			CSound_stats_ext::SItem _I;
			_I._3D = !_E->b2D;
			_I._rendered = !!_E->target;
			_I.name = _E->source()->fname;
			_I.params = _E->p_source;
			_I.volume = _E->smooth_volume;
			if (_E->owner_data)
			{
				_I.game_object = _E->owner_data->g_object;
				_I.game_type = _E->owner_data->g_type;
				_I.type = _E->owner_data->s_type;
			}
			else
			{
				_I.game_object = 0;
				_I.game_type = 0;
				_I.type = st_Effect;
			}
			ext->append(_I);
		}
	}
}

float CSoundRender_Core::get_occlusion_to(const fvec3& hear_pt, const fvec3& snd_pt, float dispersion)
{
	float occ_value = 1.f;

	if (0 != geom_SOM)
	{
		// Calculate RAY params
		fvec3 pos, dir;
		pos.random_dir();
		pos.mul(dispersion);
		pos.add(snd_pt);
		dir.sub(pos, hear_pt);
		float range = dir.magnitude();
		dir.div(range);

#ifdef _EDITOR
		ETOOLS::ray_options(CDB::OPT_CULL);
		ETOOLS::ray_query(geom_SOM, hear_pt, dir, range);
		u32 r_cnt = ETOOLS::r_count();
		CDB::RESULT* _B = ETOOLS::r_begin();
#else
		geom_DB.ray_options(CDB::OPT_CULL);
		geom_DB.ray_query(geom_SOM, hear_pt, dir, range);
		u32 r_cnt = geom_DB.r_count();
		CDB::RESULT* _B = geom_DB.r_begin();
#endif
		if (0 != r_cnt)
		{
			for (u32 k = 0; k < r_cnt; k++)
			{
				CDB::RESULT* R = _B + k;
				occ_value *= *(float*)&R->dummy;
			}
		}
	}
	return occ_value;
}

float CSoundRender_Core::get_occlusion(fvec3& P, float R, fvec3* occ)
{
	float occ_value = 1.f;

	// Calculate RAY params
	fvec3 base = listener_position();
	fvec3 pos, dir;
	float range;
	pos.random_dir();
	pos.mul(R);
	pos.add(P);
	dir.sub(pos, base);
	range = dir.magnitude();
	dir.div(range);

	if (0 != geom_MODEL)
	{
		bool bNeedFullTest = true;
		// 1. Check cached polygon
		float _u, _v, _range;
		if (CDB::TestRayTri(base, dir, occ, _u, _v, _range, true))
			if (_range > 0 && _range < range)
			{
				occ_value = psSoundOcclusionScale;
				bNeedFullTest = false;
			}
		// 2. Polygon doesn't picked up - real database query
		if (bNeedFullTest)
		{
#ifdef _EDITOR
			ETOOLS::ray_options(CDB::OPT_ONLYNEAREST);
			ETOOLS::ray_query(geom_MODEL, base, dir, range);
			if (0 != ETOOLS::r_count())
			{
				// cache polygon
				const CDB::RESULT* Result = ETOOLS::r_begin();
#else
			geom_DB.ray_options(CDB::OPT_ONLYNEAREST);
			geom_DB.ray_query(geom_MODEL, base, dir, range);
			if (0 != geom_DB.r_count())
			{
				// cache polygon
				const CDB::RESULT* Result = geom_DB.r_begin();
#endif
				const CDB::TRI& T = geom_MODEL->get_tris()[Result->id];
				const fvec3* V = geom_MODEL->get_verts();
				occ[0].set(V[T.verts[0]]);
				occ[1].set(V[T.verts[1]]);
				occ[2].set(V[T.verts[2]]);
				occ_value = psSoundOcclusionScale;
			}
		}
	}
	if (0 != geom_SOM)
	{
#ifdef _EDITOR
		ETOOLS::ray_options(CDB::OPT_CULL);
		ETOOLS::ray_query(geom_SOM, base, dir, range);
		u32 r_cnt = ETOOLS::r_count();
		CDB::RESULT* _B = ETOOLS::r_begin();
#else
		geom_DB.ray_options(CDB::OPT_CULL);
		geom_DB.ray_query(geom_SOM, base, dir, range);
		u32 r_cnt = geom_DB.r_count();
		CDB::RESULT* _B = geom_DB.r_begin();
#endif
		if (0 != r_cnt)
		{
			for (u32 k = 0; k < r_cnt; k++)
			{
				CDB::RESULT* Result = _B + k;
				occ_value *= *(float*)&Result->dummy;
			}
		}
	}
	return occ_value;
}
