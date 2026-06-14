#ifndef ObjectAnimatorH
#define ObjectAnimatorH
#pragma once

#include "motion.h"

// refs
class ENGINE_API CObjectAnimator
{
  private:
	DEFINE_VECTOR(COMotion*, MotionVec, MotionIt);

  protected:
	bool bLoop;

	shared_str m_Name;

	fmat4x4 m_Transform;
	SAnimParams m_MParam;
	MotionVec m_Motions;
	float m_Speed;

	COMotion* m_Current;
	void LoadMotions(LPCSTR fname);
	void SetActiveMotion(COMotion* mot);
	COMotion* FindMotionByName(LPCSTR name);

  public:
	CObjectAnimator();
	virtual ~CObjectAnimator();

	void Clear();
	void Load(LPCSTR name);
	IC LPCSTR Name()
	{
		return *m_Name;
	}
	float& Speed()
	{
		return m_Speed;
	}

	COMotion* Play(bool bLoop, LPCSTR name = 0);
	void Pause(bool val)
	{
		return m_MParam.Pause(val);
	}
	void Stop();
	IC BOOL IsPlaying()
	{
		return m_MParam.bPlay;
	}

	IC const fmat4x4& Transform()
	{
		return m_Transform;
	}
	float GetLength();
	// Update
	void Update(float dt);
	void DrawPath();
};

#endif // ObjectAnimatorH
