/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

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
