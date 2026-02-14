////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker_cover.cpp
//	Created 	: 25.04.2006
//  Modified 	: 25.04.2006
//	Author		: Dmitriy Iassenev
//	Description :
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ai_stalker.h"
#include "../../cover_point.h"
#include "../../cover_evaluators.h"
#include "../../ai_space.h"
#include "../../cover_manager.h"
#include "../../stalker_movement_restriction.h"
#include "../../level_graph.h"
#include "../../inventory_item.h" 
#include "../../weapon.h"
#include "../../inventory.h"

extern const float MIN_SUITABLE_ENEMY_DISTANCE = 3.f;

#ifdef _DEBUG
static int g_advance_search_count = 0;
static int g_near_cover_search_count = 0;
static int g_far_cover_search_count = 0;
#endif

void CAI_Stalker::subscribe_on_best_cover_changed(const on_best_cover_changed_delegate& delegate)
{
	VERIFY(m_cover_delegates.end() == std::find(m_cover_delegates.begin(), m_cover_delegates.end(), delegate));
	m_cover_delegates.push_back(delegate);
}

void CAI_Stalker::unsubscribe_on_best_cover_changed(const on_best_cover_changed_delegate& delegate)
{
	cover_delegates::iterator I = std::find(m_cover_delegates.begin(), m_cover_delegates.end(), delegate);
	VERIFY(I != m_cover_delegates.end());
	m_cover_delegates.erase(I);
}

void CAI_Stalker::on_best_cover_changed(const CCoverPoint* new_cover, const CCoverPoint* old_cover)
{
	cover_delegates::const_iterator I = m_cover_delegates.begin();
	cover_delegates::const_iterator E = m_cover_delegates.end();
	for (; I != E; ++I)
		(*I)(new_cover, old_cover);
}

const CCoverPoint* CAI_Stalker::find_best_cover(const float3& position_to_cover_from)
{
#ifdef _DEBUG
//	Msg									("* [%6d][%s] search for new cover performed",Engine.TimeManager.GetGlobalTimeMs(),*cName());
#endif
#ifdef _DEBUG
	++g_near_cover_search_count;
#endif
	m_ce_best->setup(position_to_cover_from, MIN_SUITABLE_ENEMY_DISTANCE, 170.f, MIN_SUITABLE_ENEMY_DISTANCE);
	const CCoverPoint* point =
		ai().cover_manager().best_cover(Position(), 10.f, *m_ce_best, CStalkerMovementRestrictor(this, true));
	if (point)
		return (point);

#ifdef _DEBUG
	++g_far_cover_search_count;
#endif
	m_ce_best->setup(position_to_cover_from, 10.f, 170.f, 10.f);

	float search_radius = 10.f;
	float min_enemy_dist = MIN_SUITABLE_ENEMY_DISTANCE;

	// Получаем ранг (0..100)
	int rank = Rank();

	// 1. Агрессия с дробовиком
	if (inventory().ActiveItem() && inventory().ActiveItem()->object().ef_weapon_type() >= 7)
	{
		if (conditions().health() > 0.7f)
			search_radius = 40.f;
	}
	// 2. Ветераны и Мастера ищут укрытия в более широком радиусе (для флангования)
	else if (rank >= 50)
	{
		search_radius = 30.f;
		// Если здоровье полное, поджимаем врага (разрешаем укрытия ближе к врагу)
		if (conditions().health() > 0.9f)
			min_enemy_dist = 5.0f;
	}

	m_ce_best->setup(position_to_cover_from, min_enemy_dist, 170.f, min_enemy_dist);
	point =
		ai().cover_manager().best_cover(Position(), search_radius, *m_ce_best, CStalkerMovementRestrictor(this, true));
	return (point);
}

float CAI_Stalker::best_cover_value(const float3& position_to_cover_from)
{
	m_ce_best->setup(position_to_cover_from, MIN_SUITABLE_ENEMY_DISTANCE, 170.f, MIN_SUITABLE_ENEMY_DISTANCE);
	m_ce_best->initialize(Position(), true);
	m_ce_best->evaluate(m_best_cover, CStalkerMovementRestrictor(this, true).weight(m_best_cover));
	return (m_ce_best->best_value());
}

void CAI_Stalker::best_cover_can_try_advance()
{
	if (!m_best_cover_actual)
		return;

	if (m_best_cover_advance_cover == m_best_cover)
		return;

	m_best_cover_can_try_advance = true;
}

void CAI_Stalker::update_best_cover_actuality(const float3& position_to_cover_from)
{
	if (!m_best_cover_actual)
		return;

	if (!m_best_cover)
		return;

	// [IMPROVEMENT] Anti-Bodyblock: Если мы долго блокируем линию огня союзникам
	// Сбрасиваем актуальность укрытия, чтобы найти новое
	if (m_body_block_time > 1000)
	{ // Если блокируем > 1 сек
		m_best_cover_actual = false;
		return;
	}

	if (m_best_cover->position().distance_to_sqr(position_to_cover_from) < _sqr(MIN_SUITABLE_ENEMY_DISTANCE))
	{
		m_best_cover_actual = false;
#if 0 // def _DEBUG
		Msg								("* [%6d][%s] enemy too close",Engine.TimeManager.GetGlobalTimeMs(),*cName());
#endif
		return;
	}

	float cover_value = best_cover_value(position_to_cover_from);
	if (cover_value >= m_best_cover_value + 1.f)
	{
		m_best_cover_actual = false;
#if 0 // def _DEBUG
		Msg								("* [%6d][%s] cover became too bad",Engine.TimeManager.GetGlobalTimeMs(),*cName());
#endif
		return;
	}

	//	if (cover_value >= 1.5f*m_best_cover_value) {
	//		m_best_cover_actual				= false;
	//		Msg								("* [%6d][%s] cover became too bad2",Engine.TimeManager.GetGlobalTimeMs(),*cName());
	//		return;
	//	}

	if (!m_best_cover_can_try_advance)
		return;

	if (m_best_cover_advance_cover == m_best_cover)
		return;

	m_best_cover_advance_cover = m_best_cover;
	m_best_cover_can_try_advance = false;

#ifdef _DEBUG
//	Msg									("* [%6d][%s] advance search performed",Engine.TimeManager.GetGlobalTimeMs(),*cName());
#endif
#ifdef _DEBUG
	++g_advance_search_count;
#endif
	m_ce_best->setup(position_to_cover_from, MIN_SUITABLE_ENEMY_DISTANCE, 170.f, MIN_SUITABLE_ENEMY_DISTANCE);
	m_best_cover =
		ai().cover_manager().best_cover(Position(), 10.f, *m_ce_best, CStalkerMovementRestrictor(this, true));
}

const CCoverPoint* CAI_Stalker::best_cover(const float3& position_to_cover_from)
{
	update_best_cover_actuality(position_to_cover_from);

	if (m_best_cover_actual)
		return (m_best_cover);

	m_best_cover_actual = true;

	const CCoverPoint* best_cover = find_best_cover(position_to_cover_from);
	if (best_cover != m_best_cover)
	{
		on_best_cover_changed(best_cover, m_best_cover);
		m_best_cover = best_cover;
		m_best_cover_advance_cover = 0;
		m_best_cover_can_try_advance = false;
	}
	m_best_cover_value = m_best_cover ? best_cover_value(position_to_cover_from) : flt_max;

	return (m_best_cover);
}

void CAI_Stalker::on_restrictions_change()
{
	inherited::on_restrictions_change();
	m_best_cover_actual = false;
#ifdef _DEBUG
	Msg("* [%6d][%s] on_restrictions_change", Engine.TimeManager.GetGlobalTimeMs(), *cName());
#endif
}

void CAI_Stalker::on_enemy_change(const CEntityAlive* enemy)
{
	inherited::on_enemy_change(enemy);
	m_item_actuality = false;
	m_best_cover_actual = false;
#ifdef _DEBUG
//	Msg									("* [%6d][%s] on_enemy_change",Engine.TimeManager.GetGlobalTimeMs(),*cName());
#endif
}

void CAI_Stalker::on_danger_location_add(const CDangerLocation& location)
{
	if (!m_best_cover)
		return;

	if (m_best_cover->position().distance_to_sqr(location.position()) <= _sqr(location.m_radius))
	{
#ifdef _DEBUG
//		Msg								("* [%6d][%s] on_danger_add",Engine.TimeManager.GetGlobalTimeMs(),*cName());
#endif
		m_best_cover_actual = false;
	}
}

void CAI_Stalker::on_danger_location_remove(const CDangerLocation& location)
{
	if (!m_best_cover)
	{
		if (Position().distance_to_sqr(location.position()) <= _sqr(location.m_radius))
		{
#ifdef _DEBUG
//			Msg							("* [%6d][%s] on_danger_remove",Engine.TimeManager.GetGlobalTimeMs(),*cName());
#endif
			m_best_cover_actual = false;
		}

		return;
	}

	if (m_best_cover->position().distance_to_sqr(location.position()) <= _sqr(location.m_radius))
	{
#ifdef _DEBUG
//		Msg								("* [%6d][%s] on_danger_remove",Engine.TimeManager.GetGlobalTimeMs(),*cName());
#endif
		m_best_cover_actual = false;
	}
}

void CAI_Stalker::on_cover_blocked(const CCoverPoint* cover)
{
#ifdef _DEBUG
//	Msg									("* [%6d][%s] cover is blocked",Engine.TimeManager.GetGlobalTimeMs(),*cName());
#endif
	m_best_cover_actual = false;
}
