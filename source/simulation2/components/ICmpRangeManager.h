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

#ifndef INCLUDED_ICMPRANGEMANAGER
#define INCLUDED_ICMPRANGEMANAGER

#include "simulation2/system/Interface.h"

#include "simulation2/helpers/Position.h"

/**
 * Provides efficient range-based queries of the game world.
 *
 * Possible use cases:
 * - combat units need to detect targetable enemies entering LOS, so they can choose
 *   to auto-attack.
 * - auras let a unit have some effect on all units (or those of the same player, or of enemies)
 *   within a certain range.
 * - capturable animals need to detect when a player-owned unit is nearby and no units of other
 *   players are in range.
 * - scenario triggers may want to detect when units enter a given area.
 * - units gathering from a resource that is exhausted need to find a new resource of the
 *   same type, near the old one and reachable.
 * - projectile weapons with splash damage need to find all units within some distance
 *   of the target point.
 * - ...
 *
 * In most cases the users are event-based and want notifications when something
 * has entered or left the range, and the query can be set up once and rarely changed.
 * These queries have to be fast. It's fine to approximate an entity as a point.
 *
 * Current design:
 *
 * This class handles just the most common parts of range queries:
 * distance, target interface, and player ownership.
 * The caller can then apply any more complex filtering that it needs.
 *
 * There are two types of query:
 * Passive queries are performed by ExecuteQuery and immediately return the matching entities.
 * Active queries are set up by CreateActiveQuery, and then a CMessageRangeUpdate message will be
 * sent to the entity once per turn if anybody has entered or left the range since the last RangeUpdate.
 * Queries can be disabled, in which case no message will be sent.
 */
class ICmpRangeManager : public IComponent
{
public:
	/**
	 * External identifiers for active queries.
	 */
	typedef u32 tag_t;

	/**
	 * Execute a passive query.
	 * @param source the entity around which the range will be computed.
	 * @param maxRange maximum distance in metres (inclusive).
	 * @param owners list of player IDs that matching entities may have; -1 matches entities with no owner.
	 * @param requiredInterface if non-zero, an interface ID that matching entities must implement.
	 * @return list of entities matching the query, ordered by increasing distance from the source entity.
	 */
	virtual std::vector<entity_id_t> ExecuteQuery(entity_id_t source, entity_pos_t maxRange, std::vector<int> owners, int requiredInterface) = 0;

	/**
	 * Construct an active query. The query will be disabled by default.
	 * @param source the entity around which the range will be computed.
	 * @param maxRange maximum distance in metres (inclusive).
	 * @param owners list of player IDs that matching entities may have; -1 matches entities with no owner.
	 * @param requiredInterface if non-zero, an interface ID that matching entities must implement.
	 * @return unique non-zero identifier of query.
	 */
	virtual tag_t CreateActiveQuery(entity_id_t source, entity_pos_t maxRange, std::vector<int> owners, int requiredInterface) = 0;

	/**
	 * Destroy a query and clean up resources. This must be called when an entity no longer needs its
	 * query (e.g. when the entity is destroyed).
	 * @param tag identifier of query.
	 */
	virtual void DestroyActiveQuery(tag_t tag) = 0;

	/**
	 * Re-enable the processing of a query.
	 * @param tag identifier of query.
	 */
	virtual void EnableActiveQuery(tag_t tag) = 0;

	/**
	 * Disable the processing of a query (no RangeUpdate messages will be sent).
	 * @param tag identifier of query.
	 */
	virtual void DisableActiveQuery(tag_t tag) = 0;

	/**
	 * Immediately execute a query, and re-enable it if disabled.
	 * The next RangeUpdate message will say who has entered/left since this call,
	 * so you won't miss any notifications.
	 * @param tag identifier of query.
	 * @return list of entities matching the query, ordered by increasing distance from the source entity.
	 */
	virtual std::vector<entity_id_t> ResetActiveQuery(tag_t tag) = 0;

	/**
	 * Toggle the rendering of debug info.
	 */
	virtual void SetDebugOverlay(bool enabled) = 0;

	DECLARE_INTERFACE_TYPE(RangeManager)
};

#endif // INCLUDED_ICMPRANGEMANAGER
