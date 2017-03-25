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
 *
 * @param camera use this view to convert screen to world coordinates.
 * @param screenX,screenY 2D screen coordinates.
 * @param player player whose LOS will be used when selecting entities. In Atlas
 *	this value is ignored as the whole map is revealed.
 * @param allowEditorSelectables if true, all entities with the ICmpSelectable interface
 *	will be selected (including decorative actors), else only those selectable ingame.
 * @param range Approximate range to check for entity in.
 *
 * @return ordered list of selected entities with the closest first.
 */
entity_id_t PickEntityAtPoint(CSimulation2& simulation, const CCamera& camera, int screenX, int screenY, player_id_t player, bool allowEditorSelectables);

/**
 * Finds all selectable entities within the given screen coordinate rectangle,
 * belonging to the given player. Used for bandboxing.
 *
 * @param camera use this view to convert screen to world coordinates.
 * @param sx0,sy0,sx1,sy1 diagonally opposite corners of the rectangle in 2D screen coordinates.
 * @param owner player whose entities we are selecting. Ownership is ignored if
 *	INVALID_PLAYER is used.
 * @param allowEditorSelectables if true, all entities with the ICmpSelectable interface
 *	will be selected (including decorative actors), else only those selectable ingame.
 *
 * @return unordered list of selected entities.
 */
std::vector<entity_id_t> PickEntitiesInRect(CSimulation2& simulation, const CCamera& camera, int sx0, int sy0, int sx1, int sy1, player_id_t owner, bool allowEditorSelectables);

/**
 * Finds all selectable entities within the given screen coordinate rectangle,
 * belonging to any given player (excluding Gaia). Used for status bars.
 */
std::vector<entity_id_t> PickNonGaiaEntitiesInRect(CSimulation2& simulation, const CCamera& camera, int sx0, int sy0, int sx1, int sy1, bool allowEditorSelectables);

/**
 * Finds all entities with the given entity template name, belonging to the given player.
 *
 * @param camera use this view to convert screen to world coordinates.
 * @param templateName the name of the template to match, or the selection group name
 *	for similar matching.
 * @param owner player whose entities we are selecting. Ownership is ignored if
 *	INVALID_PLAYER is used.
 * @param includeOffScreen if true, then all entities visible in the world will be selected,
 *	else only entities visible to the camera will be selected.
 * @param matchRank if true, only entities that exactly match templateName will be selected,
 *	else entities with matching SelectionGroupName will be selected.
 * @param allowEditorSelectables if true, all entities with the ICmpSelectable interface
 *	will be selected (including decorative actors), else only those selectable in-game.
 * @param allowFoundations if true, foundations are also included in the results. Only takes
 *  effect when matchRank = true.
 *
 * @return unordered list of selected entities.
 * @see ICmpIdentity
 */
std::vector<entity_id_t> PickSimilarEntities(CSimulation2& simulation, const CCamera& camera, const std::string& templateName,
	player_id_t owner, bool includeOffScreen, bool matchRank, bool allowEditorSelectables, bool allowFoundations);

} // namespace

#endif // INCLUDED_SELECTION
