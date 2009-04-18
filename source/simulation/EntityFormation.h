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

//Instances of this class contain the actual information about in-game formations.
//It is based off of Formation.cpp and uses it as a reference as to what can and cannot
//be done in this formation.  This is represented as m_base.

#ifndef INCLUDED_ENTITYFORMATION
#define INCLUDED_ENTITYFORMATION

#include "ps/Vector2D.h"

class CVector2D;
class CEntity;
struct CEntityList;
class CClassSet;
class CFormation;

class CEntityFormation
{
	friend class CFormationManager;
public:
	CEntityFormation( CFormation*& base, size_t index );
	~CEntityFormation();

	size_t GetEntityCount() const
	{
		return m_numEntities;
	}
	float GetSpeed() const
	{
		return m_speed;
	}
	size_t GetSlotCount() const;

	CEntityList GetEntityList() const;
	CVector2D GetSlotPosition( size_t order ) const;
	CVector2D GetPosition() const
	{
		return m_position;
	}
	CFormation* GetBase()
	{
		return m_base;
	}
	void BaseToMovement();

	void SelectAllUnits() const;

	inline void SetDuplication( bool duplicate )
	{
		m_duplication=duplicate;
	}
	inline bool IsDuplication() const
	{
		return m_duplication;
	}
	inline void SetLock( bool lock )
	{
		m_locked=lock;
	}
	inline bool IsLocked() const
	{
		return m_locked;
	}
	inline bool IsValidOrder(size_t order) const
	{
		return ( order < GetSlotCount() );
	}

private:
	size_t m_numEntities;
	size_t m_index;
	float m_speed;	//speed of slowest unit
	float m_orientation;	//Our orientation angle. Used for rotation.
	CVector2D m_position;

	bool m_locked;
	//Prevents other selected units from reordering the formation after one has already done it.
	bool m_duplication;

	CFormation* m_base;
	CFormation* m_self;   //Keeps track of base (referred to during movement switching)

	std::vector<CEntity*> m_entities;	//number of units currently in this formation
	std::vector<bool> m_angleDivs;	//attack direction penalty-true=being attacked from sector
	std::vector<float> m_angleVals;

	bool AddUnit( CEntity* entity );
	void RemoveUnit( CEntity* entity );
	bool IsSlotAppropriate( size_t order, CEntity* entity );   //If empty, can we use this slot?
	bool IsBetterUnit( size_t order, CEntity* entity );

	void UpdateFormation();
	void SwitchBase( CFormation*& base );

	void ResetIndex( size_t index );
	void ResetAllEntities();	//Sets all handles to invalid
	void ResetAngleDivs();
};
#endif
