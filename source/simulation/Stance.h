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

#ifndef INCLUDED_STANCE
#define INCLUDED_STANCE

#include "ps/Vector2D.h"

class CEntity;
class CPlayer;

/**
 * A combat stance. This object is given various events by an entity and can choose what the
 * entity will do when it does not have any player orders.
 **/
class CStance
{
protected:
	CEntity* m_Entity;
public:
	CStance(CEntity* ent): m_Entity(ent) {}
	virtual ~CStance() {}

	// Called each tick if the unit is idle.
	virtual void OnIdle() = 0;

	// Called when the unit is damaged. Source might be NULL for damage from "acts of Gaia"
	// (but we get notified anyway in case we want to run around in fear). Most stances will
	// probably want to retaliate here.
	virtual void OnDamaged(CEntity* source) = 0;

	// Does this stance allow movement at all during AI control?
	virtual bool AllowsMovement() = 0;

	// Called when the unit is under AI control and wishes to move to the given position;
	// the stance may choose to cancel if it is too far.
	virtual bool CheckMovement(CVector2D proposedPos) = 0;
};

/**
 * Hold Fire stance: Unit never attacks, not even to fight back.
 **/
class CHoldStance : public CStance
{
public:
	CHoldStance(CEntity* ent): CStance(ent) {};
	virtual ~CHoldStance() {};
	virtual void OnIdle() {};
	virtual void OnDamaged(CEntity* UNUSED(source)) {};
	virtual bool AllowsMovement() { return false; };
	virtual bool CheckMovement(CVector2D UNUSED(proposedPos)) { return false; }
};

/**
 * Aggressive stance: The unit will attack any enemy it sees and pursue it indefinitely.
 **/
class CAggressStance : public CStance
{
public:
	CAggressStance(CEntity* ent): CStance(ent) {};
	virtual ~CAggressStance() {};
	virtual void OnIdle();
	virtual void OnDamaged(CEntity* source);
	virtual bool AllowsMovement() { return true; };
	virtual bool CheckMovement(CVector2D UNUSED(proposedPos)) { return true; }
};

/**
 * Stand Ground stance: The unit will attack enemies in LOS but never move.
 **/
class CStandStance : public CStance
{
public:
	CStandStance(CEntity* ent): CStance(ent) {};
	virtual ~CStandStance() {};
	virtual void OnIdle();
	virtual void OnDamaged(CEntity* source);
	virtual bool AllowsMovement() { return false; };
	virtual bool CheckMovement(CVector2D UNUSED(proposedPos)) { return false; }
};

/**
 * Defensive stance: The unit will attack enemies but will never pursue them further
 * than its LOS away from its original position (the point where it last became idle).
 **/
class CDefendStance : public CStance
{
	CVector2D idlePos;
public:
	CDefendStance(CEntity* ent): CStance(ent) {};
	virtual ~CDefendStance() {};
	virtual void OnIdle();
	virtual void OnDamaged(CEntity* source);
	virtual bool AllowsMovement() { return true; };
	virtual bool CheckMovement(CVector2D proposedPos);
};

/**
 * Utility functions used by the various stances.
 **/
class CStanceUtils
{
private:
	CStanceUtils() {};
public:
	// Attacks the given target using the appropriate attack action (obtained from JavaScript).
	static void Attack(CEntity* entity, CEntity* target);

	// Picks a visible entity to attack.
	static CEntity* ChooseTarget(CEntity* entity);

	// Picks a visible entity within the given circle to attack
	static CEntity* ChooseTarget(float x, float y, float radius, CPlayer* player);
};

#endif
