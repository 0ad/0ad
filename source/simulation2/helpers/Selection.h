/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_SELECTION
#define INCLUDED_SELECTION

/**
 * @file
 * Helper functions related to entity selection
 */

#include "simulation2/components/ICmpPosition.h"

#include <vector>

class CSimulation2;
class CCamera;

namespace EntitySelection
{

/**
 * Finds all selectable entities under the given screen coordinates.
 * Returns list ordered by closeness of picking, closest first.
 */
std::vector<entity_id_t> PickEntitiesAtPoint(CSimulation2& simulation, CCamera& camera, int screenX, int screenY);

} // namespace

#endif // INCLUDED_SELECTION
