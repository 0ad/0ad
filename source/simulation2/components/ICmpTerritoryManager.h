/* Copyright (C) 2017 Wildfire Games.
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

#ifndef INCLUDED_ICMPTERRITORYMANAGER
#define INCLUDED_ICMPTERRITORYMANAGER

#include "simulation2/system/Interface.h"

#include "simulation2/helpers/Grid.h"
#include "simulation2/helpers/Player.h"
#include "simulation2/components/ICmpPosition.h"

class ICmpTerritoryManager : public IComponent
{
public:
	virtual bool NeedUpdate(size_t* dirtyID) const = 0;

	/**
	 * Number of pathfinder navcells per territory tile.
	 * Passability data is stored per navcell, but we probably don't need that much
	 * resolution, and a lower resolution can make the boundary lines look prettier
	 * and will take less memory, so we downsample the passability data.
	 */
	static const int NAVCELLS_PER_TERRITORY_TILE = 8;

	static const int TERRITORY_PLAYER_MASK = 0x1F;
	static const int TERRITORY_CONNECTED_MASK = 0x20;
	static const int TERRITORY_BLINKING_MASK = 0x40;
	static const int TERRITORY_PROCESSED_MASK = 0x80; //< For internal use; marks a tile as processed.

	/**
	 * For each tile, the TERRITORY_PLAYER_MASK bits are player ID;
	 * TERRITORY_CONNECTED_MASK is set if the tile is connected to a root object
	 * (civ center etc).
	 */
	virtual const Grid<u8>& GetTerritoryGrid() = 0;

	/**
	 * Get owner of territory at given position.
	 * @return player ID of owner; 0 if neutral territory
	 */
	virtual player_id_t GetOwner(entity_pos_t x, entity_pos_t z) = 0;

	/**
	 * get the number of neighbour tiles for per player for the selected position
	 * @return A list with the number of neighbour tiles per player
	 */
	virtual std::vector<u32> GetNeighbours(entity_pos_t x, entity_pos_t z, bool filterConnected) = 0;

	/**
	 * Get whether territory at given position is connected to a root object
	 * (civ center etc) owned by that territory's player.
	 */
	virtual bool IsConnected(entity_pos_t x, entity_pos_t z) = 0;

	/**
	 * Set a piece of territory to blinking. Must be updated on every territory calculation
	 */
	virtual void SetTerritoryBlinking(entity_pos_t x, entity_pos_t z, bool enable) = 0;

	/**
	 * Check if a piece of territory is blinking.
	 */
	virtual bool IsTerritoryBlinking(entity_pos_t x, entity_pos_t z) = 0;

	/**
	 * Returns the percentage of the world controlled by a given player as defined by
	 * the number of territory cells the given player owns
	 */
	 virtual u8 GetTerritoryPercentage(player_id_t player) = 0;

	/**
	 * Enables or disables rendering of an territory borders.
	 */
	virtual void SetVisibility(bool visible) = 0;

	DECLARE_INTERFACE_TYPE(TerritoryManager)
};

#endif // INCLUDED_ICMPTERRITORYMANAGER
