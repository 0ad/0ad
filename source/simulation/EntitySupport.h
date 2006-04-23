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

struct SClassSet
{
	SClassSet* m_Parent;

	typedef STL_HASH_SET<CStrW, CStrW_hash_compare> ClassSet;

	ClassSet m_Set;
	std::vector<CStrW> m_Added;
	std::vector<CStrW> m_Removed;

	inline SClassSet() { m_Parent = NULL; }

	inline bool IsMember( CStrW Test )
		{ return( m_Set.find( Test ) != m_Set.end() ); }

	inline void SetParent( SClassSet* Parent )
		{ m_Parent = Parent; Rebuild(); }

	void Rebuild()
	{
		if( m_Parent )
			m_Set = m_Parent->m_Set;
		else
			m_Set.clear();

		std::vector<CStrW>::iterator it;
		for( it = m_Removed.begin(); it != m_Removed.end(); it++ )
			m_Set.erase( *it );
		for( it = m_Added.begin(); it != m_Added.end(); it++ )
			m_Set.insert( *it );
	}
};

#endif
