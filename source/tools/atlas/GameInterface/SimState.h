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

#ifndef SIMSTATE_INCLUDED
#define SIMSTATE_INCLUDED

#include <set>
#include <vector>
#include <sstream>

#include "ps/CStr.h"
#include "maths/Vector3D.h"

class CUnit;
class CEntity;

class SimState
{
public:
	class Entity
	{
	public:
		static Entity Freeze(CUnit* unit);
		CEntity* Thaw();
	private:
		CStrW templateName;
		size_t unitID;
		std::set<CStr> selections;
		size_t playerID;
		CVector3D position;
		float angle;
	};

	class Nonentity
	{
	public:
		static Nonentity Freeze(CUnit* unit);
		CUnit* Thaw();
	private:
		CStrW actorName;
		size_t unitID;
		std::set<CStr> selections;
		CVector3D position;
		float angle;
	};
	
	static SimState* Freeze(bool onlyEntities);
	void Thaw();

private:
	// For old simulation:
	bool onlyEntities;
	std::vector<Entity> entities;
	std::vector<Nonentity> nonentities;
	// For simulation2:
	std::stringstream stream;
};

#endif // SIMSTATE_INCLUDED
