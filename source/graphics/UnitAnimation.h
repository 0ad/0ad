#ifndef INCLUDED_UNITANIMATION
#define INCLUDED_UNITANIMATION

#include "ps/CStr.h"

class CUnit;

class CUnitAnimation
{
	NONCOPYABLE(CUnitAnimation);
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

#endif // INCLUDED_UNITANIMATION
