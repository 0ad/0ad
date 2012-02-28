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

#ifndef INCLUDED_SELECTION
#define INCLUDED_SELECTION

/**
 * @file
 * Helper functions related to entity selection
 */

#include "simulation2/system/Entity.h"
#include "Player.h"

#include <vector>

class CSimulation2;
class CCamera;

namespace EntitySelection
{

/**
 * Finds all selectable entities under the given screen coordinates.
 * Returns list ordered by closeness of picking, closest first.
 * Restricted to entities in the LOS of @p player, but with any owner.
 * (For Atlas selections this value is ignored as the whole map is revealed).
 * If @p allowEditorSelectables then all entities with the IID_Selectable interface
 * will be selected, else only selectable entities without the EditorOnly flag set.
 */
std::vector<entity_id_t> PickEntitiesAtPoint(CSimulation2& simulation, const CCamera& camera, int screenX, int screenY, player_id_t player, bool allowEditorSelectables);

/**
 * Finds all selectable entities within the given screen coordinate rectangle,
 * that belong to player @p owner.
 * If @p owner is INVALID_PLAYER then ownership is ignored.
 * If @p allowEditorSelectables then all entities with the IID_Selectable interface
 * will be selected, else only selectable entities without the EditorOnly flag set.
 * Returns unordered list.
 */
std::vector<entity_id_t> PickEntitiesInRect(CSimulation2& simulation, const CCamera& camera, int sx0, int sy0, int sx1, int sy1, player_id_t owner, bool allowEditorSelectables);

/**
 * Finds all entities with the given entity template name, that belong to player @p owner.
 * If @p owner is INVALID_PLAYER then ownership is ignored.
 * If @p includeOffScreen then all entities visible in the world will be selected,
 * else only entities visible on the screen will be selected.
 * If @p matchRank then only entities that exactly match @p templateName will be selected,
 * else entities with matching SelectionGroupName will be selected.
 * If @p allowEditorSelectables then all entities with the IID_Selectable interface
 * will be selected, else only selectable entities without the EditorOnly flag set.
 */
std::vector<entity_id_t> PickSimilarEntities(CSimulation2& simulation, const CCamera& camera, const std::string& templateName, player_id_t owner, bool includeOffScreen, bool matchRank, bool allowEditorSelectables);

} // namespace

#endif // INCLUDED_SELECTION
