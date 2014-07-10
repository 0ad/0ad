/* Copyright (C) 2014 Wildfire Games.
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

#include "maths/FixedVector3D.h"
#include "maths/FixedVector2D.h"

#include "simulation2/system/Interface.h"
#include "simulation2/helpers/Position.h"
#include "simulation2/helpers/Player.h"
#include "simulation2/helpers/Spatial.h"

#include "graphics/Terrain.h" // for TERRAIN_TILE_SIZE

/**
 * Provides efficient range-based queries of the game world,
 * and also LOS-based effects (fog of war).
 *
 * (These are somewhat distinct concepts but they share a lot of the implementation,
 * so for efficiency they're combined into this class.)
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
	 * Access the spatial subdivision kept by the range manager.
	 * @return pointer to spatial subdivision structure.
	 */
	virtual SpatialSubdivision* GetSubdivision() = 0;

	/**
	 * Set the bounds of the world.
	 * Entities should not be outside the bounds (else efficiency will suffer).
	 * @param x0,z0,x1,z1 Coordinates of the corners of the world
	 * @param vertices Number of terrain vertices per side
	 */
	virtual void SetBounds(entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, ssize_t vertices) = 0;

	/**
	 * Execute a passive query.
	 * @param source the entity around which the range will be computed.
	 * @param minRange non-negative minimum distance in metres (inclusive).
	 * @param maxRange non-negative maximum distance in metres (inclusive); or -1.0 to ignore distance.
	 * @param owners list of player IDs that matching entities may have; -1 matches entities with no owner.
	 * @param requiredInterface if non-zero, an interface ID that matching entities must implement.
	 * @return list of entities matching the query, ordered by increasing distance from the source entity.
	 */
	virtual std::vector<entity_id_t> ExecuteQuery(entity_id_t source,
		entity_pos_t minRange, entity_pos_t maxRange, std::vector<int> owners, int requiredInterface) = 0;

	/**
	 * Execute a passive query.
	 * @param pos the position around which the range will be computed.
	 * @param minRange non-negative minimum distance in metres (inclusive).
	 * @param maxRange non-negative maximum distance in metres (inclusive); or -1.0 to ignore distance.
	 * @param owners list of player IDs that matching entities may have; -1 matches entities with no owner.
	 * @param requiredInterface if non-zero, an interface ID that matching entities must implement.
	 * @return list of entities matching the query, ordered by increasing distance from the source entity.
	 */
	virtual std::vector<entity_id_t> ExecuteQueryAroundPos(CFixedVector2D pos,
		entity_pos_t minRange, entity_pos_t maxRange, std::vector<int> owners, int requiredInterface) = 0;

	/**
	 * Construct an active query. The query will be disabled by default.
	 * @param source the entity around which the range will be computed.
	 * @param minRange non-negative minimum distance in metres (inclusive).
	 * @param maxRange non-negative maximum distance in metres (inclusive); or -1.0 to ignore distance.
	 * @param owners list of player IDs that matching entities may have; -1 matches entities with no owner.
	 * @param requiredInterface if non-zero, an interface ID that matching entities must implement.
	 * @param flags if a entity in range has one of the flags set it will show up.
	 * @return unique non-zero identifier of query.
	 */
	virtual tag_t CreateActiveQuery(entity_id_t source,
		entity_pos_t minRange, entity_pos_t maxRange, std::vector<int> owners, int requiredInterface, u8 flags) = 0;

    /**
	 * Construct an active query of a paraboloic form around the unit.
	 * The query will be disabled by default.
	 * @param source the entity around which the range will be computed.
	 * @param minRange non-negative minimum horizontal distance in metres (inclusive). MinRange doesn't do parabolic checks.
	 * @param maxRange non-negative maximum distance in metres (inclusive) for units on the same elevation;
	 *      or -1.0 to ignore distance.
	 *      For units on a different elevation, a physical correct paraboloid with height=maxRange/2 above the unit is used to query them
	 * @param elevationBonus extra bonus so the source can be placed higher and shoot further
	 * @param owners list of player IDs that matching entities may have; -1 matches entities with no owner.
	 * @param requiredInterface if non-zero, an interface ID that matching entities must implement.
	 * @param flags if a entity in range has one of the flags set it will show up.
	 * @return unique non-zero identifier of query.
	 */
	virtual tag_t CreateActiveParabolicQuery(entity_id_t source,
		entity_pos_t minRange, entity_pos_t maxRange, entity_pos_t elevationBonus, std::vector<int> owners, int requiredInterface, u8 flags) = 0;


	/**
	 * Get the average elevation over 8 points on distance range around the entity
	 * @param id the entity id to look around
	 * @param range the distance to compare terrain height with
	 * @return a fixed number representing the average difference. It's positive when the entity is on average higher than the terrain surrounding it.
	 */
	virtual entity_pos_t GetElevationAdaptedRange(CFixedVector3D pos, CFixedVector3D rot, entity_pos_t range, entity_pos_t elevationBonus, entity_pos_t angle) = 0;

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
	 * Returns list of all entities for specific player.
	 * (This is on this interface because it shares a lot of the implementation.
	 * Maybe it should be extended to be more like ExecuteQuery without
	 * the range parameter.)
	 */
	virtual std::vector<entity_id_t> GetEntitiesByPlayer(player_id_t player) = 0;

	/**
	 * Toggle the rendering of debug info.
	 */
	virtual void SetDebugOverlay(bool enabled) = 0;

	/**
	 * Returns the mask for the specified identifier.
	 */
	virtual u8 GetEntityFlagMask(std::string identifier) = 0;

	/**
	 * Set the flag specified by the identifier to the supplied value for the entity
	 * @param ent the entity whose flags will be modified.
	 * @param identifier the flag to be modified.
	 * @param value to which the flag will be set.
	 */
	virtual void SetEntityFlag(entity_id_t ent, std::string identifier, bool value) = 0;

	// LOS interface:

	enum ELosState
	{
		LOS_UNEXPLORED = 0,
		LOS_EXPLORED = 1,
		LOS_VISIBLE = 2,
		LOS_MASK = 3
	};

	enum ELosVisibility
	{
		VIS_HIDDEN = 0,
		VIS_FOGGED = 1,
		VIS_VISIBLE = 2
	};

	/**
	 * Object providing efficient abstracted access to the LOS state.
	 * This depends on some implementation details of CCmpRangeManager.
	 *
	 * This *ignores* the GetLosRevealAll flag - callers should check that explicitly.
	 */
	class CLosQuerier
	{
	private:
		friend class CCmpRangeManager;
		friend class TestLOSTexture;

		CLosQuerier(u32 playerMask, const std::vector<u32>& data, ssize_t verticesPerSide) :
			m_Data(&data[0]), m_PlayerMask(playerMask), m_VerticesPerSide(verticesPerSide)
		{
		}

		const CLosQuerier& operator=(const CLosQuerier&); // not implemented

	public:
		/**
		 * Returns whether the given vertex is visible (i.e. is within a unit's LOS).
		 */
		inline bool IsVisible(ssize_t i, ssize_t j)
		{
			if (!(i >= 0 && j >= 0 && i < m_VerticesPerSide && j < m_VerticesPerSide))
				return false;

			// Check high bit of each bit-pair
			if ((m_Data[j*m_VerticesPerSide + i] & m_PlayerMask) & 0xAAAAAAAAu)
				return true;
			else
				return false;
		}

		/**
		 * Returns whether the given vertex is explored (i.e. was (or still is) within a unit's LOS).
		 */
		inline bool IsExplored(ssize_t i, ssize_t j)
		{
			if (!(i >= 0 && j >= 0 && i < m_VerticesPerSide && j < m_VerticesPerSide))
				return false;

			// Check low bit of each bit-pair
			if ((m_Data[j*m_VerticesPerSide + i] & m_PlayerMask) & 0x55555555u)
				return true;
			else
				return false;
		}

		/**
		 * Returns whether the given vertex is visible (i.e. is within a unit's LOS).
		 * i and j must be in the range [0, verticesPerSide), else behaviour is undefined.
		 */
		inline bool IsVisible_UncheckedRange(ssize_t i, ssize_t j)
		{
#ifndef NDEBUG
			ENSURE(i >= 0 && j >= 0 && i < m_VerticesPerSide && j < m_VerticesPerSide);
#endif
			// Check high bit of each bit-pair
			if ((m_Data[j*m_VerticesPerSide + i] & m_PlayerMask) & 0xAAAAAAAAu)
				return true;
			else
				return false;
		}

		/**
		 * Returns whether the given vertex is explored (i.e. was (or still is) within a unit's LOS).
		 * i and j must be in the range [0, verticesPerSide), else behaviour is undefined.
		 */
		inline bool IsExplored_UncheckedRange(ssize_t i, ssize_t j)
		{
#ifndef NDEBUG
			ENSURE(i >= 0 && j >= 0 && i < m_VerticesPerSide && j < m_VerticesPerSide);
#endif
			// Check low bit of each bit-pair
			if ((m_Data[j*m_VerticesPerSide + i] & m_PlayerMask) & 0x55555555u)
				return true;
			else
				return false;
		}

	private:
		u32 m_PlayerMask;
		const u32* m_Data;
		ssize_t m_VerticesPerSide;
	};

	/**
	 * Returns a CLosQuerier for checking whether vertex positions are visible to the given player
	 *	(or other players it shares LOS with).
	 */
	virtual CLosQuerier GetLosQuerier(player_id_t player) = 0;

	/**
	 * Returns the visibility status of the given entity, with respect to the given player.
	 * Returns VIS_HIDDEN if the entity doesn't exist or is not in the world.
	 * This respects the GetLosRevealAll flag.
	 * If forceRetainInFog is true, the visibility acts as if CCmpVision's RetainInFog flag were set.
	 * TODO: This is a hack to allow preview entities in FoW to return fogged instead of hidden,
	 *	see http://trac.wildfiregames.com/ticket/958
	 */
	virtual ELosVisibility GetLosVisibility(CEntityHandle ent, player_id_t player, bool forceRetainInFog = false) = 0;
	virtual ELosVisibility GetLosVisibility(entity_id_t ent, player_id_t player, bool forceRetainInFog = false) = 0;


	/**
	 * GetLosVisibility wrapped for script calls.
	 * Returns "hidden", "fogged" or "visible".
	 */
	std::string GetLosVisibility_wrapper(entity_id_t ent, player_id_t player, bool forceRetainInFog);

	/**
	 * Explore all tiles (but leave them in the FoW) for player p
	 */
	virtual void ExploreAllTiles(player_id_t p) = 0;

	/**
	 * Set whether the whole map should be made visible to the given player.
	 * If player is -1, the map will be made visible to all players.
	 */
	virtual void SetLosRevealAll(player_id_t player, bool enabled) = 0;

	/**
	 * Returns whether the whole map has been made visible to the given player.
	 */
	virtual bool GetLosRevealAll(player_id_t player) = 0;

	/**
	 * Set the LOS to be restricted to a circular map.
	 */
	virtual void SetLosCircular(bool enabled) = 0;

	/**
	 * Returns whether the LOS is restricted to a circular map.
	 */
	virtual bool GetLosCircular() = 0;

	/**
	 * Sets shared LOS data for player to the given list of players.
	 */
	virtual void SetSharedLos(player_id_t player, std::vector<player_id_t> players) = 0;

	/**
	 * Returns shared LOS mask for player.
	 */
	virtual u32 GetSharedLosMask(player_id_t player) = 0;

	/**
	 * Get percent map explored statistics for specified player.
	 */
	virtual u8 GetPercentMapExplored(player_id_t player) = 0;


	/**
	 * Perform some internal consistency checks for testing/debugging.
	 */
	virtual void Verify() = 0;

	DECLARE_INTERFACE_TYPE(RangeManager)
};

#endif // INCLUDED_ICMPRANGEMANAGER
