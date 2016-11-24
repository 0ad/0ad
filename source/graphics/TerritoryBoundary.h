/* Copyright (C) 2012 Wildfire Games.
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

#ifndef INCLUDED_TERRITORYBOUNDARY
#define INCLUDED_TERRITORYBOUNDARY

#include <vector>

#include "maths/Vector2D.h"
#include "simulation2/helpers/Grid.h"
#include "simulation2/helpers/Player.h"

/**
 * Describes an outline of a territory, where the latter are understood to mean the largest sets of mutually connected tiles
 * ('connected' as in the mathematical sense from graph theory) that are either all reachable or all unreachable from a root
 * influence entity.
 *
 * Note that the latter property is also called the 'connected' flag in the territory manager terminology, because for tiles
 * to be reachable from a root influence entity they must in fact be mathematically connected. Hence, you should not confuse
 * the 'connected' flag with the pure mathematical concept of connectedness, because in the former it is implicitly
 * understood that the connection is to a root influence entity.
 */
struct STerritoryBoundary
{
	/// Set if this boundary should blink
	bool blinking;
	player_id_t owner;
	/// The boundary points, in clockwise order for inner boundaries and counter-clockwise order for outer boundaries.
	/// Note: if you need a way to explicitly find out which winding order these are in, you can have
	/// CTerritoryBoundCalculator::ComputeBoundaries set it during computation -- see its implementation for details.
	std::vector<CVector2D> points;
};

/**
 * Responsible for calculating territory boundaries, given an input territory map. Factored out for testing.
 */
class CTerritoryBoundaryCalculator
{
private:
	CTerritoryBoundaryCalculator(){} // private ctor

public:
	/**
	 * Computes and returns all territory boundaries on the provided territory map (see STerritoryBoundary for a definition).
	 * The result includes both inner and outer territory boundaries. Outer boundaries have their points in CCW order, inner
	 * boundaries have them in CW order (because this matches the winding orders needed by the renderer to offset them
	 * inwards/outwards appropriately).
	 */
	static std::vector<STerritoryBoundary> ComputeBoundaries(const Grid<u8>* territories);
};

#endif // INCLUDED_TERRITORYBOUNDARY
