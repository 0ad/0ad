// Supporting data types for CEntity and related

#ifndef INCLUDED_ENTITYSUPPORT
#define INCLUDED_ENTITYSUPPORT

#include "ps/CStr.h"

struct SEntityAction
{
	int m_Id;
	float m_MaxRange;
	float m_MinRange;
	int m_Speed;
	CStr8 m_Animation;
	SEntityAction()
		: m_Id(0), m_MinRange(0), m_MaxRange(0), m_Speed(1000), m_Animation("walk") {}
	SEntityAction(int id, float minRange, float maxRange, int speed, CStr8& animation)
		: m_Id(id), m_MinRange(minRange), m_MaxRange(maxRange), m_Speed(speed), m_Animation(animation) {}
};

class CClassSet
{
	CClassSet* m_Parent;
	typedef std::vector<CStrW> Set;
	Set m_Members;
	Set m_AddedMembers;		// Members that this set adds to on top of those in its parent
	Set m_RemovedMembers;		// Members that this set removes even if its parent has them

public:
	CClassSet() : m_Parent(NULL) {}

	bool IsMember(const CStrW& Test);
	
	void Rebuild();

	inline void SetParent(CClassSet* Parent)
		{ m_Parent = Parent; Rebuild(); }

	CStrW GetMemberList();
	void SetFromMemberList(const CStrW& list);
};

#endif
