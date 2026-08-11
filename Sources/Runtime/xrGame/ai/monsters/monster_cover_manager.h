#pragma once

class CCoverPoint;
class CCoverEvaluator;
class CBaseMonster;

class CMonsterCoverManager
{
	CBaseMonster* m_object;
	CCoverEvaluator* m_ce_best;

  public:
	CMonsterCoverManager(CBaseMonster* monster);
	~CMonsterCoverManager();

	void load();
	// найти лучший ковер относительно "position"
	const CCoverPoint* find_cover(const fvec3& position, float min_pos_distance, float max_pos_distance,
								  float deviation = 0.f);

	// найти лучший ковер относительно "src_pos" и "dest_pos"
	const CCoverPoint* find_cover(const fvec3& src_pos, const fvec3& dest_pos, float min_pos_distance,
								  float max_pos_distance, float deviation = 0.f);

	void less_cover_direction(fvec3& dir);
};
