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

// LOSManager.h
// 
// Maintains and updates line of sight data (including Shroud of Darkness
// and Fog of War).
//
// Usage: 
//    - Initialize() is called when the game is started to allocate the visibility arrays
//    - Update() is called each frame by CSimulation::Update() to update the visibility arrays
//    - m_MapRevealed can be set to true to reveal the entire map (remove both LOS and FOW)
//    - GetStatus can be used to obtain the LOS status of a tile or a world-space point
//    - GetUnitStatus returns the LOS status of an entity or actor


#ifndef INCLUDED_LOSMANAGER
#define INCLUDED_LOSMANAGER

class CUnit;
class CEntity;
class CPlayer;

#undef _2_los

enum ELOSStatus 
{ 
	LOS_VISIBLE = 2,		// tile is currently in LOS of one of the player's units
	LOS_EXPLORED = 1,		// tile was explored before but is now in Fog of War
	LOS_UNEXPLORED = 0		// tile is unexplored and therefore in Shroud of Darkness
};

// if changing these values, adapt ScriptGlue!RevealMap
enum ELOSSetting
{
	LOS_SETTING_NORMAL,
	LOS_SETTING_EXPLORED,
	LOS_SETTING_ALL_VISIBLE
};

enum EUnitLOSStatus 
{
	UNIT_VISIBLE = 2,		// unit is in LOS of one of the player's units
	UNIT_REMEMBERED = 1,	// unit was seen before and is permanent but is no longer in LOS
	UNIT_HIDDEN = 0			// unit is either not permanent or was never seen before
};

extern size_t LOS_GetTokenFor(size_t player_id);

class CLOSManager
{
#ifdef _2_los
	int** m_Explored;		// (m_Explored[x][z] & (1<<p) says whether player p has explored tile (x,z),
							// i.e. has removed Shroud of Darkness from it.
	int** m_Visible;		// (m_Visible[x][z] & (1<<p)) says whether player p currently sees tile (x,z).
	// NOTE: This will have to be changed to a 3D array where each element stores the number of units
	// of a certain player that can see a certain tile if we want to use incremental LOS.
#else
	u16** m_VisibilityMatrix;
#endif

	size_t m_TilesPerSide;
	size_t m_TilesPerSide_1;	// as above, -1

public:


	ELOSSetting m_LOSSetting;
	bool m_FogOfWar;

	CLOSManager();
	~CLOSManager();

	void Initialize(ELOSSetting losSetting, bool fogOfWar);
	void Update();

	// Get LOS status for a tile (in tile coordinates)
	ELOSStatus GetStatus(ssize_t tx, ssize_t tz, CPlayer* player);

	// Get LOS status for a point (in game coordinates)
	ELOSStatus GetStatus(float fx, float fz, CPlayer* player);

	// Returns whether a given actor is visible to the given player
	EUnitLOSStatus GetUnitStatus(CUnit* unit, CPlayer* player);
	
	// Returns whether a given entity is visible to the given player
	EUnitLOSStatus GetUnitStatus(CEntity* entity, CPlayer* player);
};

#endif
