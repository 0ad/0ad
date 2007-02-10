#ifndef UNITANIMATION_H__
#define UNITANIMATION_H__

#include "ps/CStr.h"

class CUnit;

class CUnitAnimation : boost::noncopyable
{
public:
	CUnitAnimation(CUnit& unit);

	// (All times are measured in seconds)

	void SetAnimationState(const CStr& name, bool once, float speed, bool keepSelection);
	void SetAnimationSync(float timeUntilActionPos);
	void Update(float time);

private:
	CUnit& m_Unit;
	CStr m_State;
	bool m_Looping;
	float m_Speed;
	float m_OriginalSpeed;
	float m_TimeToNextSync;
};

#endif // UNITANIMATION_H__
