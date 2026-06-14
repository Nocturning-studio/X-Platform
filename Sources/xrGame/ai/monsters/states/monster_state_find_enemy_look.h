#pragma once

#include "../state.h"

template <typename _Object> class CStateMonsterFindEnemyLook : public CState<_Object>
{
	typedef CState<_Object> inherited;
	typedef CState<_Object>* state_ptr;

	bool look_right_side;
	u8 current_stage;
	fvec3 target_point;

	fvec3 current_dir;
	fvec3 start_position;

	enum
	{
		eMoveToPoint = u32(0),
		eLookAround,
		eTurnToPoint
	};

  public:
	CStateMonsterFindEnemyLook(_Object* obj);
	virtual ~CStateMonsterFindEnemyLook();

	virtual void initialize();
	virtual void reselect_state();
	virtual bool check_completion();

	virtual void setup_substates();
};

#include "monster_state_find_enemy_look_inline.h"
