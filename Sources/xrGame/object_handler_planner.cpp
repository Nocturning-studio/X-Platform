////////////////////////////////////////////////////////////////////////////
//	Module 		: object_handler_planner.cpp
//	Created 	: 11.03.2004
//  Modified 	: 01.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Object handler action planner
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "object_handler_planner.h"
#include "object_property_evaluators.h"
#include "object_actions.h"
#include "ai_monster_space.h"
#include "object_handler_space.h"
#include "ai/stalker/ai_stalker.h"
#include "inventory.h"
#include "object_handler_planner_impl.h"
#include "weaponmagazined.h"
#include "missile.h"
#include "ai_monster_space.h"

using namespace ObjectHandlerSpace;

IC ObjectHandlerSpace::EWorldProperties CObjectHandlerPlanner::object_property(
	MonsterSpace::EObjectAction object_action) const
{
	switch (object_action)
	{
	case MonsterSpace::eObjectActionSwitch1:
		return (ObjectHandlerSpace::eWorldPropertySwitch1);
	case MonsterSpace::eObjectActionSwitch2:
		return (ObjectHandlerSpace::eWorldPropertySwitch2);
	case MonsterSpace::eObjectActionAim1:
		return (ObjectHandlerSpace::eWorldPropertyAimingReady1);
	case MonsterSpace::eObjectActionAim2:
		return (ObjectHandlerSpace::eWorldPropertyAiming2);
	case MonsterSpace::eObjectActionFire1:
		return (ObjectHandlerSpace::eWorldPropertyFiring1);
	case MonsterSpace::eObjectActionFire2:
		return (ObjectHandlerSpace::eWorldPropertyFiring2);
	case MonsterSpace::eObjectActionIdle:
		return (ObjectHandlerSpace::eWorldPropertyIdle);
	case MonsterSpace::eObjectActionStrapped:
		return (ObjectHandlerSpace::eWorldPropertyIdleStrap);
	case MonsterSpace::eObjectActionDrop:
		return (ObjectHandlerSpace::eWorldPropertyDropped);
	case MonsterSpace::eObjectActionActivate:
		return (ObjectHandlerSpace::eWorldPropertyIdle);
	case MonsterSpace::eObjectActionDeactivate:
		return (ObjectHandlerSpace::eWorldPropertyNoItemsIdle);
	case MonsterSpace::eObjectActionAimReady1:
		return (ObjectHandlerSpace::eWorldPropertyAimingReady1);
	case MonsterSpace::eObjectActionAimReady2:
		return (ObjectHandlerSpace::eWorldPropertyAimingReady2);
	case MonsterSpace::eObjectActionAimForceFull1:
		return (ObjectHandlerSpace::eWorldPropertyAimForceFull1);
	case MonsterSpace::eObjectActionAimForceFull2:
		return (ObjectHandlerSpace::eWorldPropertyAimForceFull2);
	case MonsterSpace::eObjectActionUse:
		return (ObjectHandlerSpace::eWorldPropertyUsed);
	default:
		NODEFAULT;
	}
#ifdef DEBUG
	return (ObjectHandlerSpace::eWorldPropertyDummy);
#endif
}

void CObjectHandlerPlanner::set_goal(MonsterSpace::EObjectAction object_action, CGameObject* game_object,
									 u32 min_queue_size, u32 max_queue_size, u32 min_queue_interval,
									 u32 max_queue_interval)
{
	EWorldProperties goal = object_property(object_action);
	u32 condition_id = goal;

	if (game_object && (eWorldPropertyNoItemsIdle != goal))
	{
		CWeapon* weapon = smart_cast<CWeapon*>(game_object);
		if (weapon && (goal == eWorldPropertyIdleStrap) && !weapon->can_be_strapped())
			goal = eWorldPropertyIdle;
		condition_id = uid(game_object->ID(), goal);
	}
	else
		condition_id = u32(eWorldPropertyNoItemsIdle);

#ifdef DEBUG
	if (m_use_log)
	{
		Msg("%6d : Active item %s", Engine.TimeManager.GetGlobalTimeMs(),
			object().inventory().ActiveItem() ? *object().inventory().ActiveItem()->object().cName()
											  : "no active items");
		Msg("%6d : Goal %s", Engine.TimeManager.GetGlobalTimeMs(), property2string(condition_id));
	}
#endif
	CState condition;
	condition.add_condition(CWorldProperty(condition_id, true));
	set_target_state(condition);

	if (!game_object || (min_queue_size < 0))
		return;

	CWeaponMagazined* weapon = smart_cast<CWeaponMagazined*>(game_object);
	if (!weapon)
		return;

	if ((m_min_queue_size != min_queue_size) || (m_max_queue_size != max_queue_size) ||
		(m_min_queue_interval != min_queue_interval) || (m_max_queue_interval != max_queue_interval) ||
		(m_next_time_change <= Engine.TimeManager.GetGlobalTimeMs()))
	{
		m_min_queue_size = min_queue_size;
		m_max_queue_size = max_queue_size;
		m_min_queue_interval = min_queue_interval;
		m_max_queue_interval = max_queue_interval;

		if (m_max_queue_size == m_min_queue_size)
			m_queue_size = _max(1, m_min_queue_size);
		else
			m_queue_size = std::max(1, ::Random.randI(m_min_queue_size, m_max_queue_size));

		if (m_max_queue_interval == m_min_queue_interval)
			m_queue_interval = m_min_queue_interval;
		else
			m_queue_interval = ::Random.randI(m_min_queue_interval, m_max_queue_interval);

		m_next_time_change = Engine.TimeManager.GetGlobalTimeMs() + m_queue_interval;

		weapon->SetQueueSize(m_queue_size);
		this->action(uid(weapon->ID(), eWorldOperatorQueueWait1))
			.set_inertia_time(m_queue_interval ? m_queue_interval : 300);
		this->action(uid(weapon->ID(), eWorldOperatorQueueWait2))
			.set_inertia_time(m_queue_interval ? m_queue_interval : 300);
	}
}

#ifdef LOG_ACTION
// Helper для безопасной конкатенации
IC void safe_cat(LPSTR dest, LPCSTR src, size_t dest_size)
{
	if (xr_strlen(dest) + xr_strlen(src) < dest_size)
		strcat(dest, src);
}

LPCSTR CObjectHandlerPlanner::action2string(const _action_id_type& id)
{
	LPSTR S = m_temp_string;
	S[0] = 0; // Clear string

	u16 obj_id = action_object_id(id);
	if (obj_id != 0xffff)
	{
		CObject* obj = Level().Objects.net_Find(obj_id);
		if (obj)
			xr_strcpy(S, sizeof(m_temp_string), *obj->cName());
		else
			xr_strcpy(S, sizeof(m_temp_string), "no_items");
	}
	else
	{
		xr_strcpy(S, sizeof(m_temp_string), "no_items");
	}

	safe_cat(S, ":", sizeof(m_temp_string));

	switch (action_state_id(id))
	{
	case ObjectHandlerSpace::eWorldOperatorShow:
		safe_cat(S, "Show", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorHide:
		safe_cat(S, "Hide", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorDrop:
		safe_cat(S, "Drop", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorStrapping:
		safe_cat(S, "Strapping", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorStrapping2Idle:
		safe_cat(S, "Strapping to idle", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorUnstrapping:
		safe_cat(S, "Unstrapping", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorUnstrapping2Idle:
		safe_cat(S, "Unstrapping to idle", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorStrapped:
		safe_cat(S, "StrappedIdle", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorIdle:
		safe_cat(S, "Idle", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorAim1:
		safe_cat(S, "Aim1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorAim2:
		safe_cat(S, "Aim2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorReload1:
		safe_cat(S, "Reload1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorReload2:
		safe_cat(S, "Reload2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorForceReload1:
		safe_cat(S, "Force Reload1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorForceReload2:
		safe_cat(S, "Force Reload2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorFire1:
		safe_cat(S, "Fire1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorFire2:
		safe_cat(S, "Fire2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorAimingReady1:
		safe_cat(S, "AimingReady1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorAimingReady2:
		safe_cat(S, "AimingReady2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorSwitch1:
		safe_cat(S, "Switch1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorSwitch2:
		safe_cat(S, "Switch2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorQueueWait1:
		safe_cat(S, "QueueWait1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorQueueWait2:
		safe_cat(S, "QueueWait2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorThrowStart:
		safe_cat(S, "ThrowStart", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorThrowIdle:
		safe_cat(S, "ThrowIdle", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorThrow:
		safe_cat(S, "Throwing", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorThreaten:
		safe_cat(S, "Threaten", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorPrepare:
		safe_cat(S, "Preparing", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorUse:
		safe_cat(S, "Using", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorGetAmmo1:
		safe_cat(S, "GetAmmo1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorGetAmmo2:
		safe_cat(S, "GetAmmo2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorAimForceFull1:
		safe_cat(S, "AimForceFull1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldOperatorAimForceFull2:
		safe_cat(S, "AimForceFull2", sizeof(m_temp_string));
		break;
	default:
		NODEFAULT;
	}
	return (S);
}

LPCSTR CObjectHandlerPlanner::property2string(const _condition_type& id)
{
	LPSTR S = m_temp_string;
	S[0] = 0;

	u16 obj_id = action_object_id(id);
	if (obj_id != 0xffff)
	{
		CObject* obj = Level().Objects.net_Find(obj_id);
		if (obj)
			xr_strcpy(S, sizeof(m_temp_string), *obj->cName());
		else
			xr_strcpy(S, sizeof(m_temp_string), "no_items");
	}
	else
	{
		xr_strcpy(S, sizeof(m_temp_string), "no_items");
	}

	safe_cat(S, ":", sizeof(m_temp_string));

	switch (action_state_id(id))
	{
	case ObjectHandlerSpace::eWorldPropertyHidden:
		safe_cat(S, "Hidden", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyStrapped:
		safe_cat(S, "Strapped", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyStrapped2Idle:
		safe_cat(S, "Strapped to idle", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertySwitch1:
		safe_cat(S, "Switch1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertySwitch2:
		safe_cat(S, "Switch2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyAimed1:
		safe_cat(S, "Aimed1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyAimed2:
		safe_cat(S, "Aimed2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyAimForceFull1:
		safe_cat(S, "AimedForceFull1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyAimForceFull2:
		safe_cat(S, "AimedForceFull2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyAiming1:
		safe_cat(S, "Aiming1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyAiming2:
		safe_cat(S, "Aiming2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyEmpty1:
		safe_cat(S, "Empty1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyEmpty2:
		safe_cat(S, "Empty2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyFull1:
		safe_cat(S, "Full1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyFull2:
		safe_cat(S, "Full2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyReady1:
		safe_cat(S, "Ready1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyReady2:
		safe_cat(S, "Ready2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyFiring1:
		safe_cat(S, "Firing1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyFiring2:
		safe_cat(S, "Firing2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyAimingReady1:
		safe_cat(S, "AimingReady1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyAimingReady2:
		safe_cat(S, "AimingReady2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyAmmo1:
		safe_cat(S, "Ammo1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyAmmo2:
		safe_cat(S, "Ammo2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyIdle:
		safe_cat(S, "Idle", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyIdleStrap:
		safe_cat(S, "IdleStrap", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyDropped:
		safe_cat(S, "Dropped", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyQueueWait1:
		safe_cat(S, "QueueWait1", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyQueueWait2:
		safe_cat(S, "QueueWait2", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyThrowStarted:
		safe_cat(S, "ThrowStarted", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyThrowIdle:
		safe_cat(S, "ThrowIdle", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyThrow:
		safe_cat(S, "Throwing", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyThreaten:
		safe_cat(S, "Threaten", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyPrepared:
		safe_cat(S, "Prepared", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyUsed:
		safe_cat(S, "Used", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyUseEnough:
		safe_cat(S, "UseEnough", sizeof(m_temp_string));
		break;
	case ObjectHandlerSpace::eWorldPropertyItemID: {
		if (xr_strlen(S) > 0)
			S[xr_strlen(S) - 1] = 0;
		break;
	}
	default:
		NODEFAULT;
	}
	return (S);
}
#endif

// OPTIMIZED: Удаление диапазоном вместо поштучного удаления
void CObjectHandlerPlanner::remove_evaluators(CObject* object)
{
	// UID конструируется так, что ID объекта находится в старших битах.
	// Поэтому все эвалуаторы одного объекта идут подряд в сортированном контейнере.

	u16 id = object->ID();
	// Нижняя граница (ID, тип 0)
	EVALUATORS::iterator I = m_evaluators.lower_bound(uid(id, 0));
	// Верхняя граница (ID+1, тип 0) - т.е. начало следующего ID
	EVALUATORS::iterator E = m_evaluators.lower_bound(uid(id + 1, 0));

	// Сначала очищаем память эвалуаторов
	for (auto it = I; it != E; ++it)
	{
		xr_delete(it->second);
	}

	// Затем удаляем диапазон из map (это быстро)
	m_evaluators.erase(I, E);
}

// OPTIMIZED: Удаление диапазоном
void CObjectHandlerPlanner::remove_operators(CObject* object)
{
	u16 id = object->ID();

	// Находим начало диапазона операторов для данного объекта
	OPERATOR_VECTOR::iterator I = std::lower_bound(m_operators.begin(), m_operators.end(), uid(id, 0));

	// Ищем конец диапазона (пока ID совпадает)
	OPERATOR_VECTOR::iterator E = I;
	for (; E != m_operators.end(); ++E)
	{
		if (action_object_id((*E).m_operator_id) != id)
			break;

		// ВАЖНО: Предполагается, что remove_operator делал xr_delete(op).
		// Если операторы владеют ресурсами, их нужно очистить здесь.
		// Обычно в GOAP X-Ray операторы хранятся по значению или смарт-поинтеру,
		// но если там raw pointer - добавить xr_delete(*E);
		xr_delete((*E).m_operator);
	}

	// Удаляем весь блок из вектора за раз
	if (I != E)
	{
		m_operators.erase(I, E);
	}
}

void CObjectHandlerPlanner::init_storage()
{
	m_storage.set_property(eWorldPropertyAimed1, false);
	m_storage.set_property(eWorldPropertyAimed2, false);
	m_storage.set_property(eWorldPropertyUseEnough, false);
	m_storage.set_property(eWorldPropertyStrapped, false);
	m_storage.set_property(eWorldPropertyStrapped2Idle, false);
}

void CObjectHandlerPlanner::setup(CAI_Stalker* object)
{
	inherited::setup(object);
	CActionBase<CAI_Stalker>* action;

	m_min_queue_size = 0;
	m_max_queue_size = 0;
	m_min_queue_interval = 0;
	m_max_queue_interval = 0;
	m_next_time_change = 0;

	clear();

	init_storage();

	add_evaluator(u32(eWorldPropertyNoItems), xr_new<CObjectPropertyEvaluatorNoItems>(m_object));
	add_evaluator(u32(eWorldPropertyNoItemsIdle), xr_new<CObjectPropertyEvaluatorConst>(false));
	action = xr_new<CSObjectActionBase>(m_object, m_object, &m_storage, "no items idle");
	add_condition(action, 0xffff, eWorldPropertyItemID, true);
	add_effect(action, 0xffff, eWorldPropertyIdle, true);
	add_operator(u32(eWorldOperatorNoItemsIdle), action);

	set_goal(MonsterSpace::eObjectActionIdle, 0, 0, 0, 0, 0);

#ifdef LOG_ACTION
	set_use_log(!!psAI_Flags.test(aiGOAPObject));
#endif
}

void CObjectHandlerPlanner::add_item(CInventoryItem* inventory_item)
{
	CWeapon* weapon = smart_cast<CWeapon*>(inventory_item);
	if (weapon)
	{
		add_evaluators(weapon);
		add_operators(weapon);
		return;
	}

	CMissile* missile = smart_cast<CMissile*>(inventory_item);
	if (missile)
	{
		add_evaluators(missile);
		add_operators(missile);
		return;
	}
}

void CObjectHandlerPlanner::remove_item(CInventoryItem* inventory_item)
{
	VERIFY(target_state().conditions().size() == 1);
	if (action_object_id(target_state().conditions().back().condition()) == inventory_item->object().ID())
	{
		init_storage();
		set_goal(MonsterSpace::eObjectActionIdle, 0, 0, 0, 0, 0);
	}

	remove_evaluators(&inventory_item->object());
	remove_operators(&inventory_item->object());
}

void CObjectHandlerPlanner::update()
{
	//OPTICK_EVENT("CObjectHandlerPlanner::update");

#ifdef LOG_ACTION
	if ((psAI_Flags.test(aiGOAPObject) && !m_use_log) || (!psAI_Flags.test(aiGOAPObject) && m_use_log))
		set_use_log(!!psAI_Flags.test(aiGOAPObject));
#endif
	inherited::update();
}
