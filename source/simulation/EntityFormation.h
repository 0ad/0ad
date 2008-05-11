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

	size_t GetEntityCount() { return m_numEntities; }
	float GetSpeed() { return m_speed; }
	size_t GetSlotCount();

	CEntityList GetEntityList();
	CVector2D GetSlotPosition( size_t order );
	CVector2D GetPosition() { return m_position; }
	CFormation* GetBase() { return m_base; }
	void BaseToMovement();

	void SelectAllUnits();

	inline void SetDuplication( bool duplicate ) { m_duplication=duplicate; }
	inline bool IsDuplication() { return m_duplication; }
	inline void SetLock( bool lock ){ m_locked=lock; }
	inline bool IsLocked() { return m_locked; }
	inline bool IsValidOrder(size_t order) { return ( order < GetSlotCount() ); }

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
