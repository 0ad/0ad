// LOSManager.h
//
// Matei Zaharia matei@sprint.ca / matei@wildfiregames.com
// 
// Maintains and updates line of sight data (including Shroud of Darkness
// and Fog of War).
//
// Usage: Doesn't do anything useful right now.


#ifndef LOS_MANAGER_INCLUDED
#define LOS_MANAGER_INCLUDED

#include "Singleton.h"
#include "Game.h"
#include "World.h"
#include "Player.h"

class CUnit;
class CPlayer;

enum ELOSStatus 
{ 
	LOS_VISIBLE = 2,		// tile is currently in LOS of one of the player's units
	LOS_EXPLORED = 1,		// tile was explored before but is now in Fog of War
	LOS_UNEXPLORED = 0		// tile is unexplored and therefore in Shroud of Darkness
};

enum EUnitLOSStatus 
{
	UNIT_VISIBLE = 2,		// unit is in LOS of one of the player's units
	UNIT_REMEMBERED = 1,	// unit was seen before and is permanent but is no longer in LOS
	UNIT_HIDDEN = 0			// unit is either not permanent or was never seen before
};

class CLOSManager : public Singleton<CLOSManager>
{
	int** m_Explored;		// (m_Explored[x][z] & (1<<p) says whether player p has explored tile (x,z),
							// i.e. has removed Shroud of Darkness from it.
	int** m_Visible;		// (m_Visible[x][z] & (1<<p)) says whether player p currently sees tile (x,z).
	// NOTE: This will have to be changed to a 3D array where each element stores the number of units
	// of a certain player that can see a certain tile if we want to use incremental LOS.

public:
	bool m_MapRevealed;		// Set to true to ignore LOS

	CLOSManager();
	~CLOSManager();

	void Initialize();
	void Update();

	// Get LOS status for a tile (in tile coordinates)
	ELOSStatus GetStatus(int tx, int tz, CPlayer* player);

	// Get LOS status for a point (in game coordinates)
	ELOSStatus GetStatus(float fx, float fz, CPlayer* player);

	// Returns whether a given entity is visible to the given player
	EUnitLOSStatus GetUnitStatus(CUnit* unit, CPlayer* player);
};

#endif
