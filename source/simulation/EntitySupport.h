// Supporting data types for CEntity and related

#ifndef ENTITY_SUPPORT_INCLUDED
#define ENTITY_SUPPORT_INCLUDED

#include "EntityHandles.h"
#include "ScriptObject.h"

class CEntityManager;

struct SEntityAction
{
	float m_MaxRange;
	float m_MinRange;
	size_t m_Speed;
	CStr8 m_Animation;
	SEntityAction() { m_MaxRange = m_MinRange = 0.0f; m_Speed = 1000; m_Animation = "walk"; }
	SEntityAction(float minRange, float maxRange, size_t speed, CStr8& animation)
		: m_MinRange(minRange), m_MaxRange(maxRange), m_Speed(speed), m_Animation(animation) {}
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

	bool IsMember( const CStrW& Test );
	
	void Rebuild();

	inline void SetParent( CClassSet* Parent )
		{ m_Parent = Parent; Rebuild(); }

	CStrW getMemberList();
	void setFromMemberList(const CStrW& list);
};

#endif
