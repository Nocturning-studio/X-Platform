////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_movement_manager.cpp
//	Created 	: 27.12.2003
//  Modified 	: 27.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Stalker movement manager
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "stalker_movement_manager.h"
#include "stalker_movement_manager_space.h"
#include "script_entity_action.h"
#include "ai/stalker/ai_stalker.h"
#include "sight_manager.h"
#include "detail_path_manager.h"
#include "level_location_selector.h"
#include "level_path_manager.h"
#include "movement_manager_space.h"
#include "detail_path_manager_space.h"
#include "entitycondition.h"
#include "ai_object_location.h"
#include "stalker_velocity_holder.h"
#include "stalker_velocity_collection.h"

#ifdef DEBUG
#include "stalker_decision_space.h"
#include "stalker_planner.h"
#include "script_game_object.h"
#include "stalker_combat_planner.h"
#endif // DEBUG

using namespace StalkerMovement;

extern bool show_restrictions(CRestrictedObject* object);

const float BAD_PATH_ANGLE = PI_DIV_2 - PI_DIV_8;
const float BAD_PATH_DISTANCE_CHECK = 2.f;

// Функция расчета точки на сплайне Катмулла-Рома
// p0, p1, p2, p3 - 4 последовательные точки пути
// t - параметр от 0.0 до 1.0 (прогресс между p1 и p2)
Fvector SplineCatmullRom(const Fvector& p0, const Fvector& p1, const Fvector& p2, const Fvector& p3, float t)
{
	float t2 = t * t;
	float t3 = t2 * t;

	Fvector res;
	res.x = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
					(-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);

	res.y = p1.y; // Y (высоту) лучше интерполировать линейно или оставить как есть, чтобы не проваливаться под террейн

	res.z = 0.5f * ((2.0f * p1.z) + (-p0.z + p2.z) * t + (2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * t2 +
					(-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * t3);

	return res;
}

void CStalkerMovementManager::predict_smooth_direction()
{
	// [ИЗМЕНЕНИЕ] Разрешаем сглаживание для любого движения (кроме стояния)
	if (path().empty() || movement_type() == eMovementTypeStand)
		return;

	u32 idx = detail().curr_travel_point_index();
	u32 count = path().size();

	// Нам нужно минимум 4 точки для полноценного сплайна
	if (idx + 2 >= count)
		return;

	Fvector p0, p1, p2, p3;

	// P1 - текущая позиция
	p1 = object().Position();

	// P2 - следующая точка
	p2 = path()[idx + 1].position;

	// P0 - предыдущая точка
	if (idx > 0)
		p0 = path()[idx - 1].position;
	else
		p0 = p1;

	// P3 - точка после следующей
	if (idx + 2 < count)
		p3 = path()[idx + 2].position;
	else
		p3 = p2;

	// [ИЗМЕНЕНИЕ] Динамический коэффициент упреждения (t)
	// Для бега смотрим дальше (0.4), для ходьбы чуть ближе (0.3), чтобы точнее входить в повороты
	float look_ahead_t = (movement_type() == eMovementTypeWalk) ? 0.3f : 0.45f;

	Fvector smooth_target = SplineCatmullRom(p0, p1, p2, p3, look_ahead_t);

	// Вычисляем направление к этой "мягкой" точке
	Fvector dir;
	dir.sub(smooth_target, object().Position());

	// Защита от нулевого вектора (если мы стоим точно в точке)
	if (dir.square_magnitude() < EPS_L)
		return;

	dir.normalize();

	float y, p;
	dir.getHP(y, p);

	float target_yaw = -y;

	// Применяем новый угол к корпусу и голове
	m_body.target.yaw = target_yaw;
	m_head.target.yaw = target_yaw;
}

IC void CStalkerMovementManager::setup_head_speed()
{
	if (mental_state() == eMentalStateFree)
	{
		if (object().sight().enabled())
			m_head.speed = PI_DIV_2;
	}
	else
		m_head.speed = m_danger_head_speed;
}

IC void CStalkerMovementManager::add_velocity(int mask, float linear, float compute_angular, float angular)
{
	detail().add_velocity(mask, CDetailPathManager::STravelParams(linear, compute_angular, angular));
}

IC float CStalkerMovementManager::path_direction_angle()
{
	if (!path().empty() && (path().size() > detail().curr_travel_point_index() + 1))
	{
		Fvector t;
		t.sub(path()[detail().curr_travel_point_index() + 1].position,
			  path()[detail().curr_travel_point_index()].position);
		float y, p;
		t.getHP(y, p);
		return (angle_difference(-y, m_body.current.yaw));
	}
	return (0.f);
}

void CStalkerMovementManager::initialize()
{
	set_path_type(MovementManager::ePathTypeLevelPath);
	set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
	set_body_state(eBodyStateStand);
	set_movement_type(eMovementTypeStand);
	set_mental_state(eMentalStateDanger);
	set_desired_direction(0);

#ifdef DEBUG
	restrictions().initialize();
#endif

	restrictions().remove_all_restrictions();
	set_nearest_accessible_position();
}

void CStalkerMovementManager::set_desired_position(const Fvector* desired_position)
{
	if (desired_position)
	{
		m_target.m_use_desired_position = true;
		VERIFY2(object().movement().accessible(*desired_position) || show_restrictions(&restrictions()),
				*object().cName());
		m_target.m_desired_position = *desired_position;
	}
	else
	{
		m_target.m_use_desired_position = false;
#ifdef DEBUG
		m_target.m_desired_position = Fvector().set(flt_max, flt_max, flt_max);
#endif
	}
}

IC void CStalkerMovementManager::setup_body_orientation()
{
	if (path().empty())
		return;

	if (path().size() <= detail().curr_travel_point_index() + 1)
		return;

	Fvector temp;
	temp.sub(path()[detail().curr_travel_point_index() + 1].position,
			 path()[detail().curr_travel_point_index()].position);
	float y, p;
	temp.getHP(y, p);
	m_body.target.yaw = -y;
	m_head.target.yaw = -y;
	m_head.speed = m_body.speed;
}

//////////////////////////////////////////////////////////////////////////

CStalkerMovementManager::CStalkerMovementManager(CAI_Stalker* object) : inherited(object)
{
	VERIFY(object);
	m_object = object;
	m_velocities = 0;
	m_last_query_object = 0;
	m_last_query_position = Fvector().set(flt_max, flt_max, flt_max);
	m_last_query_object_position = Fvector().set(flt_max, flt_max, flt_max);
	m_last_query_result = false;
	m_last_query_distance = flt_max;
	m_force_update = false;
}

CStalkerMovementManager::~CStalkerMovementManager()
{
}

void CStalkerMovementManager::Load(LPCSTR section)
{
	inherited::Load(section);
}

void CStalkerMovementManager::reload(LPCSTR section)
{
	inherited::reload(section);
	m_velocities = &stalker_velocity_holder().collection(pSettings->r_string(section, "movement_speeds"));
	init_velocity_masks();
}

void CStalkerMovementManager::init_velocity_masks()
{
	float cf = 2.f;

	// [IMPROVEMENT] More reactive combat turning
	float combat_turn_mult = 2.5f;

	add_velocity(eVelocityStandingFreeStand, 0.f, PI_DIV_4, PI_MUL_2);
	add_velocity(eVelocityStandingPanicStand, 0.f, PI_MUL_2);
	add_velocity(eVelocityStandingDangerStand, 0.f, PI_MUL_2);

	add_velocity(eVelocityStandingFreeCrouch, 0.f, PI_MUL_2);
	add_velocity(eVelocityStandingPanicCrouch, 0.f, PI_MUL_2);
	add_velocity(eVelocityStandingDangerCrouch, 0.f, PI_MUL_2);

	add_velocity(
		eVelocityWalkFreePositive,
		m_velocities->velocity(eMentalStateFree, eBodyStateStand, eMovementTypeWalk, eMovementDirectionForward),
		PI_DIV_8 / 1, cf * PI_DIV_8 / 1);
	add_velocity(eVelocityRunFreePositive,
				 m_velocities->velocity(eMentalStateFree, eBodyStateStand, eMovementTypeRun, eMovementDirectionForward),
				 PI_DIV_8 / 2, cf * PI_DIV_8 / 2);
	add_velocity(
		eVelocityWalkDangerStandPositive,
		m_velocities->velocity(eMentalStateDanger, eBodyStateStand, eMovementTypeWalk, eMovementDirectionForward),
		100 * PI, cf * PI);
	add_velocity(
		eVelocityWalkDangerCrouchPositive,
		m_velocities->velocity(eMentalStateDanger, eBodyStateCrouch, eMovementTypeWalk, eMovementDirectionForward),
		100 * PI, cf * PI_DIV_2);
	// Для бега в опасности (Danger Run) увеличиваем скорость поворота
	add_velocity(
		eVelocityRunDangerStandPositive,
		m_velocities->velocity(eMentalStateDanger, eBodyStateStand, eMovementTypeRun, eMovementDirectionForward),
		100 * PI,
		cf * PI * combat_turn_mult); // Ускоряем поворот
	// То же самое для Crouch Run (перебежки в присяде)
	add_velocity(
		eVelocityRunDangerCrouchPositive,
		m_velocities->velocity(eMentalStateDanger, eBodyStateCrouch, eMovementTypeRun, eMovementDirectionForward),
		100 * PI, cf * PI * combat_turn_mult);
	add_velocity(
		eVelocityRunPanicStandPositive,
		m_velocities->velocity(eMentalStatePanic, eBodyStateStand, eMovementTypeRun, eMovementDirectionForward),
		PI_DIV_8 / 2, cf * PI_DIV_8 / 2);

	add_velocity(
		eVelocityWalkDangerStandNegative,
		m_velocities->velocity(eMentalStateDanger, eBodyStateStand, eMovementTypeWalk, eMovementDirectionForward),
		100 * PI, cf * PI);
	add_velocity(
		eVelocityWalkDangerCrouchNegative,
		m_velocities->velocity(eMentalStateDanger, eBodyStateCrouch, eMovementTypeWalk, eMovementDirectionForward),
		100 * PI, cf * PI_DIV_2);
	add_velocity(
		eVelocityRunDangerStandNegative,
		m_velocities->velocity(eMentalStateDanger, eBodyStateStand, eMovementTypeRun, eMovementDirectionForward),
		100 * PI, cf * PI);
	add_velocity(
		eVelocityRunDangerCrouchNegative,
		m_velocities->velocity(eMentalStateDanger, eBodyStateCrouch, eMovementTypeRun, eMovementDirectionForward),
		100 * PI, cf * PI);
}

void CStalkerMovementManager::reinit()
{
	inherited::reinit();
	m_body.speed = PI_MUL_2;
	m_danger_head_speed = 3 * PI_DIV_2;
	m_head.speed = m_danger_head_speed;

	m_current.m_use_desired_position = false;
	m_current.m_use_desired_direction = false;
	m_current.m_desired_position = Fvector().set(flt_max, flt_max, flt_max);
	m_current.m_desired_direction = Fvector().set(flt_max, flt_max, flt_max);
	m_current.m_body_state = eBodyStateStand;
	m_current.m_movement_type = eMovementTypeStand;
	m_current.m_mental_state = eMentalStateDanger;
	m_current.m_path_type = MovementManager::ePathTypeNoPath;
	m_current.m_detail_path_type = DetailPathManager::eDetailPathTypeSmooth;

	m_target = m_current;

	m_last_turn_index = u32(-1);
}

bool CStalkerMovementManager::script_control()
{
	if (!object().GetScriptControl())
		return (false);

	if (!object().GetCurrentAction())
		return (false);

	if (fis_zero(object().GetCurrentAction()->m_tMovementAction.m_fSpeed))
		return (false);

	object().m_fCurSpeed = object().GetCurrentAction()->m_tMovementAction.m_fSpeed;
	set_desirable_speed(object().m_fCurSpeed);
	return (true);
}

void CStalkerMovementManager::setup_movement_params()
{
	inherited::set_path_type(path_type());
	switch (path_type())
	{
	case MovementManager::ePathTypeGamePath:
	case MovementManager::ePathTypePatrolPath: {
		set_desired_position(0);
		break;
	}
	}

	detail().set_path_type(detail_path_type());
	level_path().set_evaluator(base_level_params());

	if (use_desired_position())
	{
		VERIFY(_valid(desired_position()));
		if (!restrictions().accessible(desired_position()))
		{
			Fvector temp;
			level_path().set_dest_vertex(restrictions().accessible_nearest(desired_position(), temp));
			detail().set_dest_position(temp);
		}
		else
			detail().set_dest_position(desired_position());
	}
	else
	{
		if ((path_type() != MovementManager::ePathTypePatrolPath) &&
			(path_type() != MovementManager::ePathTypeGamePath) && (path_type() != MovementManager::ePathTypeNoPath))
		{
			if (!restrictions().accessible(level_path().dest_vertex_id()))
			{
				Fvector temp;
				level_path().set_dest_vertex(restrictions().accessible_nearest(
					ai().level_graph().vertex_position(level_path().dest_vertex_id()), temp));
				detail().set_dest_position(temp);
			}
			else
			{
				u32 vertex_id = level_path().dest_vertex_id();
				Fvector vertex_position = ai().level_graph().vertex_position(level_path().dest_vertex_id());
				VERIFY2(restrictions().accessible(vertex_position) || show_restrictions(&restrictions()),
						make_string("vertex_id[%d],position[%f][%f][%f],object[%s]", vertex_id, VPUSH(vertex_position),
									*object().cName()));
				detail().set_dest_position(vertex_position);
			}
		}
	}

	if (use_desired_direction())
	{
		VERIFY(_valid(desired_direction()));
		detail().set_dest_direction(desired_direction());
		detail().set_use_dest_orientation(true);
	}
	else
		detail().set_use_dest_orientation(false);
}

void CStalkerMovementManager::setup_velocities()
{
	// setup desirable velocities mask
	// if we want to stand, do not setup velocity to prevent path rebuilding

	int velocity_mask = eVelocityPositiveVelocity;

	// setup body state
	switch (body_state())
	{
	case eBodyStateCrouch: {
		velocity_mask |= eVelocityCrouch;
		break;
	}
	case eBodyStateStand: {
		velocity_mask |= eVelocityStand;
		break;
	}
	default:
		NODEFAULT;
	}

	// setup mental state
	switch (mental_state())
	{
	case eMentalStateDanger: {
		velocity_mask |= eVelocityDanger;
		break;
	}
	case eMentalStateFree: {
		velocity_mask |= eVelocityFree;
		break;
	}
	case eMentalStatePanic: {
		velocity_mask |= eVelocityPanic;
		break;
	}
	}

	// setup_movement_type
	switch (movement_type())
	{
	case eMovementTypeWalk: {
		velocity_mask |= eVelocityWalk;
		break;
	}
	case eMovementTypeRun: {
		velocity_mask |= eVelocityRun;
		break;
	}
	default: {
		velocity_mask |= eVelocityStanding;
		velocity_mask &= u32(-1) ^ (eVelocityNegativeVelocity | eVelocityPositiveVelocity);
	}
	}

	// setup all the possible velocities
	if (velocity_mask & eVelocityDanger)
	{
		detail().set_desirable_mask(velocity_mask);
		detail().set_velocity_mask(velocity_mask | eVelocityStanding);
	}
	else
	{
		detail().set_try_min_time(true);
		detail().set_desirable_mask(velocity_mask | eVelocityStanding);
		detail().set_velocity_mask(velocity_mask | eVelocityWalk | eVelocityStanding);
	}
}

void CStalkerMovementManager::parse_velocity_mask()
{
	if (path().empty() || (detail().curr_travel_point_index() != m_last_turn_index))
		m_last_turn_index = u32(-1);

	object().sight().enable(true);

	if ((movement_type() == eMovementTypeStand) || path().empty() ||
		(path().size() <= detail().curr_travel_point_index()) || path_completed() || !actual())
	{
		object().m_fCurSpeed = 0;
		if (mental_state() != eMentalStateDanger)
			m_body.speed = 1 * PI_DIV_2;
		else
			m_body.speed = PI_MUL_2;
		set_desirable_speed(object().m_fCurSpeed);
		setup_head_speed();
		m_current.m_movement_type = eMovementTypeStand;
		return;
	}

	DetailPathManager::STravelPathPoint point = path()[detail().curr_travel_point_index()];
	CDetailPathManager::STravelParams current_velocity = detail().velocity(point.velocity);

	if (fis_zero(current_velocity.linear_velocity))
	{
		if (mental_state() == eMentalStateFree)
		{
			setup_body_orientation();
			object().sight().enable(false);
			//			Msg						("%d FALSE",Engine.TimeManager.GetGlobalTimeMs());
		}
		if ((mental_state() != eMentalStateFree) ||
			//				(object().sight().current_action().sight_type() != SightManager::eSightTypePathDirection) ||
			fis_zero(path_direction_angle(), EPS_L) || (m_last_turn_index == detail().curr_travel_point_index()))
		{
			m_last_turn_index = detail().curr_travel_point_index();
			object().sight().enable(true);
			//			Msg						("%d TRUE",Engine.TimeManager.GetGlobalTimeMs());
			if (detail().curr_travel_point_index() + 1 < path().size())
			{
				point = path()[detail().curr_travel_point_index() + 1];
				current_velocity = detail().velocity(point.velocity);
			}
		}
	}
	else
	{
		if (mental_state() != eMentalStateDanger)
		{
			if (mental_state() == eMentalStatePanic)
			{
				if (!fis_zero(path_direction_angle(), PI_DIV_8 * .5f))
				{
					u32 temp = u32(-1);
					temp ^= eVelocityFree;
					temp ^= eVelocityDanger;
					temp ^= eVelocityPanic;
					point.velocity &= temp;
					point.velocity |= eVelocityDanger;
					current_velocity = detail().velocity(point.velocity);
				}
			}
			else
			{
				if (!fis_zero(path_direction_angle(), PI_DIV_8 * .5f))
				{
					setup_body_orientation();
					object().sight().enable(false);
					current_velocity = detail().velocity(path()[detail().curr_travel_point_index()].velocity);
					current_velocity.linear_velocity = 0.f;
					current_velocity.real_angular_velocity = PI;
				}
				else
					object().sight().enable(true);
			}
		}
	}

	// [IMPROVEMENT] Look Into Turn
	// Если мы бежим и не целимся во врага (свободный бег или поиск укрытия)
	if (m_current.m_movement_type == eMovementTypeRun && !object().GetScriptControl())
	{
		// Проверяем, куда ведет путь через 1-2 метра
		float look_angle = path_direction_angle();

		// Если угол значительный (поворот)
		if (_abs(look_angle) > PI_DIV_8)
		{
			// Принудительно вращаем голову в сторону пути быстрее, чем тело
			// m_head.target.yaw берется из setup_body_orientation, но мы ускоряем поворот
			m_head.speed = m_body.speed * 1.5f; // Голова поворачивается в 1.5 раза быстрее тела
		}
	}

	object().m_fCurSpeed = current_velocity.linear_velocity;
	m_body.speed = current_velocity.real_angular_velocity;
	set_desirable_speed(object().m_fCurSpeed);

	switch (point.velocity & eVelocityBodyState)
	{
	case eVelocityStand: {
		m_current.m_body_state = eBodyStateStand;
		break;
	}
	case eVelocityCrouch: {
		m_current.m_body_state = eBodyStateCrouch;
		break;
	}
	default:
		NODEFAULT;
	}

	switch (point.velocity & eVelocityMentalState)
	{
	case eVelocityFree: {
#ifdef DEBUG
		if (m_object->brain().current_action_id() == StalkerDecisionSpace::eWorldOperatorCombatPlanner)
		{
			CStalkerCombatPlanner& planner = smart_cast<CStalkerCombatPlanner&>(m_object->brain().current_action());
			if (planner.current_action_id() != StalkerDecisionSpace::eWorldOperatorKillWoundedEnemy)
				Msg("~ stalker %s is doing bad thing (action %s)", *m_object->cName(),
					planner.current_action().m_action_name);
		}
#endif // DEBUG
		m_current.m_mental_state = eMentalStateFree;
		break;
	}
	case eVelocityDanger: {
		m_current.m_mental_state = eMentalStateDanger;
		break;
	}
	case eVelocityPanic: {
		m_current.m_mental_state = eMentalStatePanic;
		break;
	}
	default:
		NODEFAULT;
	}

	VERIFY2((m_current.m_mental_state != eMentalStateFree) || m_current.m_body_state != eBodyStateCrouch,
			*object().cName());

	switch (point.velocity & eVelocityMovementType)
	{
	case eVelocityStanding: {
		m_current.m_movement_type = eMovementTypeStand;
		break;
	}
	case eVelocityWalk: {
		m_current.m_movement_type = eMovementTypeWalk;
		break;
	}
	case eVelocityRun: {
		m_current.m_movement_type = eMovementTypeRun;
		break;
	}
	default:
		NODEFAULT;
	}

	// [IMPROVEMENT] Smooth Stop
	if (!path().empty() && movement_type() != eMovementTypeStand)
	{
		// Дистанция до финиша
		float dist_to_end = detail().distance_to_target(); // Метод есть в CDetailPathManager (или вычислить вручную)

		// Если осталось меньше 1.5 метра
		if (dist_to_end < 1.5f)
		{
			// Линейная интерполяция скорости к нулю
			// Минимальная скорость 0.5 м/с, чтобы он все-таки дошел
			float slow_factor = dist_to_end / 1.5f;
			clamp(slow_factor, 0.2f, 1.0f);

			float smoothed_speed = object().m_fCurSpeed * slow_factor;
			set_desirable_speed(smoothed_speed);
		}
	}

	setup_head_speed();
}

void CStalkerMovementManager::set_nearest_accessible_position()
{
	set_nearest_accessible_position(object().Position(), object().ai_location().level_vertex_id());
}

void CStalkerMovementManager::set_nearest_accessible_position(Fvector desired_position, u32 level_vertex_id)
{
	if (!ai().level_graph().inside(level_vertex_id, desired_position))
		desired_position = ai().level_graph().vertex_position(level_vertex_id);
	else
		desired_position.y = ai().level_graph().vertex_plane_y(level_vertex_id, desired_position.x, desired_position.z);

	if (!restrictions().accessible(desired_position))
	{
		level_vertex_id = restrictions().accessible_nearest(Fvector().set(desired_position), desired_position);
		VERIFY(restrictions().accessible(level_vertex_id));
		VERIFY(restrictions().accessible(desired_position));
	}
	else
	{
		if (!restrictions().accessible(level_vertex_id))
		{
			level_vertex_id = restrictions().accessible_nearest(ai().level_graph().vertex_position(level_vertex_id),
																desired_position);
			VERIFY(restrictions().accessible(level_vertex_id));
			VERIFY(restrictions().accessible(desired_position));
		}
	}

	VERIFY(ai().level_graph().inside(level_vertex_id, desired_position));

	VERIFY2(restrictions().accessible(level_vertex_id) || show_restrictions(&restrictions()), *object().cName());
	set_level_dest_vertex(level_vertex_id);

	VERIFY2(restrictions().accessible(desired_position) || show_restrictions(&restrictions()), *object().cName());
	set_desired_position(&desired_position);
}

void CStalkerMovementManager::update(u32 time_delta)
{
	//OPTICK_EVENT("CStalkerMovementManager::update");

	if (!enabled())
		return;

	VERIFY((m_target.m_mental_state != eMentalStateFree) || (m_target.m_body_state != eBodyStateCrouch));
	m_current = m_target;

	if (m_force_update || (movement_type() != eMovementTypeStand))
		setup_movement_params();

	if (script_control())
		return;

	if (m_force_update || (movement_type() != eMovementTypeStand))
		setup_velocities();

	if (m_force_update || (movement_type() != eMovementTypeStand))
		update_path();

	parse_velocity_mask();

	predict_smooth_direction(); 

	process_smart_turns();
}

void CStalkerMovementManager::on_travel_point_change(const u32& previous_travel_point_index)
{
	inherited::on_travel_point_change(previous_travel_point_index);
}

void CStalkerMovementManager::on_restrictions_change()
{
	inherited::on_restrictions_change();
	if (use_desired_position() && !restrictions().accessible(desired_position()))
		set_nearest_accessible_position();
}

float CStalkerMovementManager::speed(const EMovementDirection& movement_direction)
{
	VERIFY(movement_type() != eMovementTypeStand);

	return (m_velocities->velocity(mental_state(), body_state(), movement_type(), movement_direction));
}

void CStalkerMovementManager::setup_speed_from_animation(const float& speed)
{
	set_desirable_speed(object().m_fCurSpeed = speed);
}

void CStalkerMovementManager::on_build_path()
{
	m_last_query_object = 0;
	m_last_query_position = Fvector().set(flt_max, flt_max, flt_max);
	m_last_query_object_position = Fvector().set(flt_max, flt_max, flt_max);
	m_last_query_result = false;
	m_last_query_distance = flt_max;
}

bool CStalkerMovementManager::is_object_on_the_way(const CGameObject* object, const float& distance)
{
	update_object_on_the_way(object, distance);

	if (m_last_query_object != object)
	{
		update_object_on_the_way(object, distance);
		return (m_last_query_result);
	}

	if (distance - EPS_L > m_last_query_distance)
	{
		update_object_on_the_way(object, distance);
		return (m_last_query_result);
	}

	if (!m_last_query_position.similar(this->object().Position()))
	{
		update_object_on_the_way(object, distance);
		return (m_last_query_result);
	}

	if (!m_last_query_object_position.similar(object->Position()))
	{
		update_object_on_the_way(object, distance);
		return (m_last_query_result);
	}

	return (m_last_query_result);
}

IC float distance_to_line(const Fvector& p0, const Fvector& p1, const Fvector& p2)
{
	if (p0.similar(p2))
		return (0.f);

	if (p1.similar(p2))
		return (0.f);

	Fvector p0p2 = Fvector().sub(p2, p0);
	float p0p2_magnitude = p0p2.magnitude();
	if (p0.similar(p1))
		return (p0p2_magnitude);

	p0p2.normalize();

	Fvector p0p1 = Fvector().sub(p1, p0);
	p0p1.normalize();

	float cos_alpha = p0p2.dotproduct(p0p1);
	if (cos_alpha < 0.f)
		return (p0p2_magnitude);

	Fvector p1p2 = Fvector().sub(p2, p1);
	Fvector p1p0 = Fvector(p0p1).invert();
	if (p1p2.dotproduct(p1p0) < 0.f)
		return (p1p2.magnitude());

	float sin_alpha = _sqrt(1.f - _sqr(cos_alpha));
	return (p0p2_magnitude * sin_alpha);
}

void CStalkerMovementManager::update_object_on_the_way(const CGameObject* object, const float& distance)
{
	if (!actual())
		return;

	if (path().empty())
		return;

	if (detail().curr_travel_point_index() >= detail().path().size() - 1)
		return;

	m_last_query_object = object;
	m_last_query_position = this->object().Position();
	m_last_query_object_position = object->Position();
	m_last_query_result = false;
	m_last_query_distance = distance;

	Fvector position = object->Position();
	float current_distance = 0.f;
	xr_vector<STravelPathPoint>::const_iterator I = detail().path().begin() + detail().curr_travel_point_index() + 1;
	xr_vector<STravelPathPoint>::const_iterator E = detail().path().end();
	for (; I != E; ++I)
	{
		if (distance_to_line((*(I - 1)).position, (*I).position, position) < 1.f)
		{
			m_last_query_result = true;
			return;
		}

		current_distance += (*(I - 1)).position.distance_to((*I).position);
		if (current_distance > distance)
			return;
	}
}

void CStalkerMovementManager::force_update(const bool& force_update)
{
	m_force_update = force_update;
}

void CStalkerMovementManager::check_for_bad_path()
{
	if (m_current.m_movement_type != eMovementTypeRun)
		return;

	if (m_current.m_mental_state != eMentalStateDanger)
		return;

	if (detail().completed(object().Position(), !detail().state_patrol_path()))
		return;

	typedef xr_vector<STravelPathPoint> PATH;
	const PATH& path = detail().path();

	u32 point_count = path.size();
	u32 point_index = detail().curr_travel_point_index();
	if (point_index + 2 >= point_count)
		return;

	float distance = path[point_index + 1].position.distance_to(object().Position());
	Fvector current_direction = Fvector().sub(path[point_index + 1].position, path[point_index].position);
	Fvector next_direction;
	if (current_direction.magnitude() >= EPS_L)
		current_direction.normalize();
	else
		current_direction.set(0.f, 0.f, 1.f);

	PATH::const_iterator E = path.end();
	PATH::const_iterator I = path.begin() + point_index + 1;
	VERIFY(I != E);
	PATH::const_iterator J = I + 1;
	VERIFY(J != E);
	for (; J != E; ++I, ++J)
	{
		next_direction = Fvector().sub((*J).position, (*I).position);
		float magnitude = next_direction.magnitude();
		distance += magnitude;
		//. how can it be?
		if (magnitude < EPS_L)
			continue;

		next_direction.normalize();
		float cos_angle = current_direction.dotproduct(next_direction);
		float angle = acosf(cos_angle);
		if (angle > BAD_PATH_ANGLE)
		{
#ifdef DEBUG
			Msg("bad path check changed movement type from RUN to WALK");
#endif // DEBUG
			m_current.m_movement_type = eMovementTypeWalk;
			return;
		}

		if (distance >= BAD_PATH_DISTANCE_CHECK)
			return;
	}
}

void CStalkerMovementManager::process_smart_turns()
{
	// [ИЗМЕНЕНИЕ] Работаем для любого движения, кроме стояния
	// Убрали проверку eMentalStateDanger, чтобы патрульные тоже ходили плавно
	if (m_current.m_movement_type == eMovementTypeStand)
		return;

	if (detail().completed(object().Position(), !detail().state_patrol_path()))
		return;

	const auto& path = detail().path();
	u32 point_index = detail().curr_travel_point_index();

	if (point_index + 2 >= path.size())
		return;

	Fvector current_dir = Fvector().sub(path[point_index + 1].position, path[point_index].position);
	if (current_dir.magnitude() < EPS_L)
		return;
	current_dir.normalize();

	Fvector next_dir = Fvector().sub(path[point_index + 2].position, path[point_index + 1].position);
	if (next_dir.magnitude() < EPS_L)
		return;
	next_dir.normalize();

	float cos_angle = current_dir.dotproduct(next_dir);
	clamp(cos_angle, -1.f, 1.f);
	float angle = acosf(cos_angle);

	const float START_SLOW_ANGLE = PI_DIV_6; // 30 градусов
	const float MAX_SLOW_ANGLE = PI_DIV_2;	 // 90 градусов

	if (angle > START_SLOW_ANGLE)
	{
		float factor = (angle - START_SLOW_ANGLE) / (MAX_SLOW_ANGLE - START_SLOW_ANGLE);
		clamp(factor, 0.f, 1.f);

		// [ИЗМЕНЕНИЕ] Разная степень торможения для ходьбы и бега
		// При ходьбе (Walk) тормозим меньше, так как скорость и так низкая (коэфф 0.7)
		// При беге (Run) тормозим сильнее, чтобы не было заносов (коэфф 0.4)
		float min_speed_limit = (m_current.m_movement_type == eMovementTypeWalk) ? 0.7f : 0.4f;

		float final_factor = 1.0f - (factor * (1.0f - min_speed_limit));

		float new_speed = object().m_fCurSpeed * final_factor;
		set_desirable_speed(new_speed);
	}
}
