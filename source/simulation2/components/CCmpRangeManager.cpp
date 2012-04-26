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

#include "precompiled.h"

#include "simulation2/system/Component.h"
#include "ICmpRangeManager.h"

#include "simulation2/MessageTypes.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpTerritoryManager.h"
#include "simulation2/components/ICmpVision.h"
#include "simulation2/helpers/Render.h"
#include "simulation2/helpers/Spatial.h"

#include "graphics/Overlay.h"
#include "graphics/Terrain.h"
#include "lib/timer.h"
#include "maths/FixedVector2D.h"
#include "ps/CLogger.h"
#include "ps/Overlay.h"
#include "ps/Profile.h"
#include "renderer/Scene.h"

#define DEBUG_RANGE_MANAGER_BOUNDS 0

/**
 * Representation of a range query.
 */
struct Query
{
	bool enabled;
	entity_id_t source;
	entity_pos_t minRange;
	entity_pos_t maxRange;
	u32 ownersMask;
	i32 interface;
	std::vector<entity_id_t> lastMatch;
	u8 flagsMask;
};

/**
 * Convert an owner ID (-1 = unowned, 0 = gaia, 1..30 = players)
 * into a 32-bit mask for quick set-membership tests.
 */
static u32 CalcOwnerMask(i32 owner)
{
	if (owner >= -1 && owner < 31)
		return 1 << (1+owner);
	else
		return 0; // owner was invalid
}

/**
 * Representation of an entity, with the data needed for queries.
 */
struct EntityData
{
	EntityData() : retainInFog(0), owner(-1), inWorld(0), flags(1) { }
	entity_pos_t x, z;
	entity_pos_t visionRange;
	u8 retainInFog; // boolean
	i8 owner;
	u8 inWorld; // boolean
	u8 flags; // See GetEntityFlagMask
};

cassert(sizeof(EntityData) == 16);


/**
 * Serialization helper template for Query
 */
struct SerializeQuery
{
	template<typename S>
	void operator()(S& serialize, const char* UNUSED(name), Query& value)
	{
		serialize.Bool("enabled", value.enabled);
		serialize.NumberU32_Unbounded("source", value.source);
		serialize.NumberFixed_Unbounded("min range", value.minRange);
		serialize.NumberFixed_Unbounded("max range", value.maxRange);
		serialize.NumberU32_Unbounded("owners mask", value.ownersMask);
		serialize.NumberI32_Unbounded("interface", value.interface);
		SerializeVector<SerializeU32_Unbounded>()(serialize, "last match", value.lastMatch);
		serialize.NumberU8_Unbounded("flagsMask", value.flagsMask);
	}
};

/**
 * Serialization helper template for EntityData
 */
struct SerializeEntityData
{
	template<typename S>
	void operator()(S& serialize, const char* UNUSED(name), EntityData& value)
	{
		serialize.NumberFixed_Unbounded("x", value.x);
		serialize.NumberFixed_Unbounded("z", value.z);
		serialize.NumberFixed_Unbounded("vision", value.visionRange);
		serialize.NumberU8("retain in fog", value.retainInFog, 0, 1);
		serialize.NumberI8_Unbounded("owner", value.owner);
		serialize.NumberU8("in world", value.inWorld, 0, 1);
		serialize.NumberU8_Unbounded("flags", value.flags);
	}
};


/**
 * Functor for sorting entities by distance from a source point.
 * It must only be passed entities that are in 'entities'
 * and are currently in the world.
 */
struct EntityDistanceOrdering
{
	EntityDistanceOrdering(const std::map<entity_id_t, EntityData>& entities, const CFixedVector2D& source) :
		m_EntityData(entities), m_Source(source)
	{
	}

	bool operator()(entity_id_t a, entity_id_t b)
	{
		const EntityData& da = m_EntityData.find(a)->second;
		const EntityData& db = m_EntityData.find(b)->second;
		CFixedVector2D vecA = CFixedVector2D(da.x, da.z) - m_Source;
		CFixedVector2D vecB = CFixedVector2D(db.x, db.z) - m_Source;
		return (vecA.CompareLength(vecB) < 0);
	}

	const std::map<entity_id_t, EntityData>& m_EntityData;
	CFixedVector2D m_Source;

private:
	EntityDistanceOrdering& operator=(const EntityDistanceOrdering&);
};

/**
 * Range manager implementation.
 * Maintains a list of all entities (and their positions and owners), which is used for
 * queries.
 *
 * LOS implementation is based on the model described in GPG2.
 * (TODO: would be nice to make it cleverer, so e.g. mountains and walls
 * can block vision)
 */
class CCmpRangeManager : public ICmpRangeManager
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeGloballyToMessageType(MT_Create);
		componentManager.SubscribeGloballyToMessageType(MT_PositionChanged);
		componentManager.SubscribeGloballyToMessageType(MT_OwnershipChanged);
		componentManager.SubscribeGloballyToMessageType(MT_Destroy);

		componentManager.SubscribeToMessageType(MT_Update);

		componentManager.SubscribeToMessageType(MT_RenderSubmit); // for debug overlays
	}

	DEFAULT_COMPONENT_ALLOCATOR(RangeManager)

	bool m_DebugOverlayEnabled;
	bool m_DebugOverlayDirty;
	std::vector<SOverlayLine> m_DebugOverlayLines;

	// World bounds (entities are expected to be within this range)
	entity_pos_t m_WorldX0;
	entity_pos_t m_WorldZ0;
	entity_pos_t m_WorldX1;
	entity_pos_t m_WorldZ1;

	// Range query state:
	tag_t m_QueryNext; // next allocated id
	std::map<tag_t, Query> m_Queries;
	std::map<entity_id_t, EntityData> m_EntityData;
	SpatialSubdivision<entity_id_t> m_Subdivision; // spatial index of m_EntityData

	// LOS state:

	std::map<player_id_t, bool> m_LosRevealAll;
	bool m_LosCircular;
	i32 m_TerrainVerticesPerSide;
	size_t m_TerritoriesDirtyID;

	// Counts of units seeing vertex, per vertex, per player (starting with player 0).
	// Use u16 to avoid overflows when we have very large (but not infeasibly large) numbers
	// of units in a very small area.
	// (Note we use vertexes, not tiles, to better match the renderer.)
	// Lazily constructed when it's needed, to save memory in smaller games.
	std::vector<std::vector<u16> > m_LosPlayerCounts;

	// 2-bit ELosState per player, starting with player 1 (not 0!) up to player MAX_LOS_PLAYER_ID (inclusive)
	std::vector<u32> m_LosState;
	static const player_id_t MAX_LOS_PLAYER_ID = 16;

	// Special static visibility data for the "reveal whole map" mode
	// (TODO: this is usually a waste of memory)
	std::vector<u32> m_LosStateRevealed;

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
		m_QueryNext = 1;

		m_DebugOverlayEnabled = false;
		m_DebugOverlayDirty = true;

		m_WorldX0 = m_WorldZ0 = m_WorldX1 = m_WorldZ1 = entity_pos_t::Zero();

		// Initialise with bogus values (these will get replaced when
		// SetBounds is called)
		ResetSubdivisions(entity_pos_t::FromInt(1), entity_pos_t::FromInt(1));

		// The whole map should be visible to Gaia by default, else e.g. animals
		// will get confused when trying to run from enemies
		m_LosRevealAll[0] = true;

		m_LosCircular = false;
		m_TerrainVerticesPerSide = 0;

		m_TerritoriesDirtyID = 0;
	}

	virtual void Deinit()
	{
	}

	template<typename S>
	void SerializeCommon(S& serialize)
	{
		serialize.NumberFixed_Unbounded("world x0", m_WorldX0);
		serialize.NumberFixed_Unbounded("world z0", m_WorldZ0);
		serialize.NumberFixed_Unbounded("world x1", m_WorldX1);
		serialize.NumberFixed_Unbounded("world z1", m_WorldZ1);

		serialize.NumberU32_Unbounded("query next", m_QueryNext);
		SerializeMap<SerializeU32_Unbounded, SerializeQuery>()(serialize, "queries", m_Queries);
		SerializeMap<SerializeU32_Unbounded, SerializeEntityData>()(serialize, "entity data", m_EntityData);

		SerializeMap<SerializeI32_Unbounded, SerializeBool>()(serialize, "los reveal all", m_LosRevealAll);
		serialize.Bool("los circular", m_LosCircular);
		serialize.NumberI32_Unbounded("terrain verts per side", m_TerrainVerticesPerSide);

		// We don't serialize m_Subdivision or m_LosPlayerCounts
		// since they can be recomputed from the entity data when deserializing;
		// m_LosState must be serialized since it depends on the history of exploration

		SerializeVector<SerializeU32_Unbounded>()(serialize, "los state", m_LosState);
	}

	virtual void Serialize(ISerializer& serialize)
	{
		SerializeCommon(serialize);
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(paramNode);

		SerializeCommon(deserialize);

		// Reinitialise subdivisions and LOS data
		ResetDerivedData(true);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_Create:
		{
			const CMessageCreate& msgData = static_cast<const CMessageCreate&> (msg);
			entity_id_t ent = msgData.entity;

			// Ignore local entities - we shouldn't let them influence anything
			if (ENTITY_IS_LOCAL(ent))
				break;

			// Ignore non-positional entities
			CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), ent);
			if (!cmpPosition)
				break;

			// The newly-created entity will have owner -1 and position out-of-world
			// (any initialisation of those values will happen later), so we can just
			// use the default-constructed EntityData here
			EntityData entdata;

			// Store the LOS data, if any
			CmpPtr<ICmpVision> cmpVision(GetSimContext(), ent);
			if (cmpVision)
			{
				entdata.visionRange = cmpVision->GetRange();
				entdata.retainInFog = (cmpVision->GetRetainInFog() ? 1 : 0);
			}

			// Remember this entity
			m_EntityData.insert(std::make_pair(ent, entdata));

			break;
		}
		case MT_PositionChanged:
		{
			const CMessagePositionChanged& msgData = static_cast<const CMessagePositionChanged&> (msg);
			entity_id_t ent = msgData.entity;

			std::map<u32, EntityData>::iterator it = m_EntityData.find(ent);

			// Ignore if we're not already tracking this entity
			if (it == m_EntityData.end())
				break;

			if (msgData.inWorld)
			{
				if (it->second.inWorld)
				{
					CFixedVector2D from(it->second.x, it->second.z);
					CFixedVector2D to(msgData.x, msgData.z);
					m_Subdivision.Move(ent, from, to);
					LosMove(it->second.owner, it->second.visionRange, from, to);
				}
				else
				{
					CFixedVector2D to(msgData.x, msgData.z);
					m_Subdivision.Add(ent, to);
					LosAdd(it->second.owner, it->second.visionRange, to);
				}

				it->second.inWorld = 1;
				it->second.x = msgData.x;
				it->second.z = msgData.z;
			}
			else
			{
				if (it->second.inWorld)
				{
					CFixedVector2D from(it->second.x, it->second.z);
					m_Subdivision.Remove(ent, from);
					LosRemove(it->second.owner, it->second.visionRange, from);
				}

				it->second.inWorld = 0;
				it->second.x = entity_pos_t::Zero();
				it->second.z = entity_pos_t::Zero();
			}

			break;
		}
		case MT_OwnershipChanged:
		{
			const CMessageOwnershipChanged& msgData = static_cast<const CMessageOwnershipChanged&> (msg);
			entity_id_t ent = msgData.entity;

			std::map<u32, EntityData>::iterator it = m_EntityData.find(ent);

			// Ignore if we're not already tracking this entity
			if (it == m_EntityData.end())
				break;

			if (it->second.inWorld)
			{
				CFixedVector2D pos(it->second.x, it->second.z);
				LosRemove(it->second.owner, it->second.visionRange, pos);
				LosAdd(msgData.to, it->second.visionRange, pos);
			}

			ENSURE(-128 <= msgData.to && msgData.to <= 127);
			it->second.owner = (i8)msgData.to;

			break;
		}
		case MT_Destroy:
		{
			const CMessageDestroy& msgData = static_cast<const CMessageDestroy&> (msg);
			entity_id_t ent = msgData.entity;

			std::map<u32, EntityData>::iterator it = m_EntityData.find(ent);

			// Ignore if we're not already tracking this entity
			if (it == m_EntityData.end())
				break;

			if (it->second.inWorld)
				m_Subdivision.Remove(ent, CFixedVector2D(it->second.x, it->second.z));

			// This will be called after Ownership's OnDestroy, so ownership will be set
			// to -1 already and we don't have to do a LosRemove here
			ENSURE(it->second.owner == -1);

			m_EntityData.erase(it);

			break;
		}
		case MT_Update:
		{
			m_DebugOverlayDirty = true;
			UpdateTerritoriesLos();
			ExecuteActiveQueries();
			break;
		}
		case MT_RenderSubmit:
		{
			const CMessageRenderSubmit& msgData = static_cast<const CMessageRenderSubmit&> (msg);
			RenderSubmit(msgData.collector);
			break;
		}
		}
	}

	virtual void SetBounds(entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, ssize_t vertices)
	{
		m_WorldX0 = x0;
		m_WorldZ0 = z0;
		m_WorldX1 = x1;
		m_WorldZ1 = z1;
		m_TerrainVerticesPerSide = (i32)vertices;

		ResetDerivedData(false);
	}

	virtual void Verify()
	{
		// Ignore if map not initialised yet
		if (m_WorldX1.IsZero())
			return;

		// Check that calling ResetDerivedData (i.e. recomputing all the state from scratch)
		// does not affect the incrementally-computed state

		std::vector<std::vector<u16> > oldPlayerCounts = m_LosPlayerCounts;
		std::vector<u32> oldStateRevealed = m_LosStateRevealed;
		SpatialSubdivision<entity_id_t> oldSubdivision = m_Subdivision;

		ResetDerivedData(true);
		
		if (oldPlayerCounts != m_LosPlayerCounts)
		{
			for (size_t i = 0; i < oldPlayerCounts.size(); ++i)
			{
				debug_printf(L"%d: ", (int)i);
				for (size_t j = 0; j < oldPlayerCounts[i].size(); ++j)
					debug_printf(L"%d ", oldPlayerCounts[i][j]);
				debug_printf(L"\n");
			}
			for (size_t i = 0; i < m_LosPlayerCounts.size(); ++i)
			{
				debug_printf(L"%d: ", (int)i);
				for (size_t j = 0; j < m_LosPlayerCounts[i].size(); ++j)
					debug_printf(L"%d ", m_LosPlayerCounts[i][j]);
				debug_printf(L"\n");
			}
			debug_warn(L"inconsistent player counts");
		}
		if (oldStateRevealed != m_LosStateRevealed)
			debug_warn(L"inconsistent revealed");
		if (oldSubdivision != m_Subdivision)
			debug_warn(L"inconsistent subdivs");
	}

	// Reinitialise subdivisions and LOS data, based on entity data
	void ResetDerivedData(bool skipLosState)
	{
		ENSURE(m_WorldX0.IsZero() && m_WorldZ0.IsZero()); // don't bother implementing non-zero offsets yet
		ResetSubdivisions(m_WorldX1, m_WorldZ1);

		m_LosPlayerCounts.clear();
		m_LosPlayerCounts.resize(MAX_LOS_PLAYER_ID+1);
		if (!skipLosState)
		{
			m_LosState.clear();
			m_LosState.resize(m_TerrainVerticesPerSide*m_TerrainVerticesPerSide);
		}
		m_LosStateRevealed.clear();
		m_LosStateRevealed.resize(m_TerrainVerticesPerSide*m_TerrainVerticesPerSide);

		for (std::map<entity_id_t, EntityData>::const_iterator it = m_EntityData.begin(); it != m_EntityData.end(); ++it)
		{
			if (it->second.inWorld)
				LosAdd(it->second.owner, it->second.visionRange, CFixedVector2D(it->second.x, it->second.z));
		}

		for (ssize_t j = 0; j < m_TerrainVerticesPerSide; ++j)
			for (ssize_t i = 0; i < m_TerrainVerticesPerSide; ++i)
				m_LosStateRevealed[i + j*m_TerrainVerticesPerSide] = LosIsOffWorld(i, j) ? 0 : 0xFFFFFFFFu;
	}

	void ResetSubdivisions(entity_pos_t x1, entity_pos_t z1)
	{
		// Use 8x8 tile subdivisions
		// (TODO: find the optimal number instead of blindly guessing)
		m_Subdivision.Reset(x1, z1, entity_pos_t::FromInt(8*TERRAIN_TILE_SIZE));

		for (std::map<entity_id_t, EntityData>::const_iterator it = m_EntityData.begin(); it != m_EntityData.end(); ++it)
		{
			if (it->second.inWorld)
				m_Subdivision.Add(it->first, CFixedVector2D(it->second.x, it->second.z));
		}
	}

	virtual tag_t CreateActiveQuery(entity_id_t source,
		entity_pos_t minRange, entity_pos_t maxRange,
		std::vector<int> owners, int requiredInterface, u8 flags)
	{
		tag_t id = m_QueryNext++;
		m_Queries[id] = ConstructQuery(source, minRange, maxRange, owners, requiredInterface, flags);

		return id;
	}

	virtual void DestroyActiveQuery(tag_t tag)
	{
		if (m_Queries.find(tag) == m_Queries.end())
		{
			LOGERROR(L"CCmpRangeManager: DestroyActiveQuery called with invalid tag %u", tag);
			return;
		}

		m_Queries.erase(tag);
	}

	virtual void EnableActiveQuery(tag_t tag)
	{
		std::map<tag_t, Query>::iterator it = m_Queries.find(tag);
		if (it == m_Queries.end())
		{
			LOGERROR(L"CCmpRangeManager: EnableActiveQuery called with invalid tag %u", tag);
			return;
		}

		Query& q = it->second;
		q.enabled = true;
	}

	virtual void DisableActiveQuery(tag_t tag)
	{
		std::map<tag_t, Query>::iterator it = m_Queries.find(tag);
		if (it == m_Queries.end())
		{
			LOGERROR(L"CCmpRangeManager: DisableActiveQuery called with invalid tag %u", tag);
			return;
		}

		Query& q = it->second;
		q.enabled = false;
	}

	virtual std::vector<entity_id_t> ExecuteQuery(entity_id_t source,
		entity_pos_t minRange, entity_pos_t maxRange,
		std::vector<int> owners, int requiredInterface)
	{
		PROFILE("ExecuteQuery");

		Query q = ConstructQuery(source, minRange, maxRange, owners, requiredInterface, GetEntityFlagMask("normal"));

		std::vector<entity_id_t> r;

		CmpPtr<ICmpPosition> cmpSourcePosition(GetSimContext(), q.source);
		if (!cmpSourcePosition || !cmpSourcePosition->IsInWorld())
		{
			// If the source doesn't have a position, then the result is just the empty list
			return r;
		}

		PerformQuery(q, r);

		// Return the list sorted by distance from the entity
		CFixedVector2D pos = cmpSourcePosition->GetPosition2D();
		std::stable_sort(r.begin(), r.end(), EntityDistanceOrdering(m_EntityData, pos));

		return r;
	}

	virtual std::vector<entity_id_t> ResetActiveQuery(tag_t tag)
	{
		PROFILE("ResetActiveQuery");

		std::vector<entity_id_t> r;

		std::map<tag_t, Query>::iterator it = m_Queries.find(tag);
		if (it == m_Queries.end())
		{
			LOGERROR(L"CCmpRangeManager: ResetActiveQuery called with invalid tag %u", tag);
			return r;
		}

		Query& q = it->second;
		q.enabled = true;

		CmpPtr<ICmpPosition> cmpSourcePosition(GetSimContext(), q.source);
		if (!cmpSourcePosition || !cmpSourcePosition->IsInWorld())
		{
			// If the source doesn't have a position, then the result is just the empty list
			q.lastMatch = r;
			return r;
		}

		PerformQuery(q, r);

		q.lastMatch = r;

		// Return the list sorted by distance from the entity
		CFixedVector2D pos = cmpSourcePosition->GetPosition2D();
		std::stable_sort(r.begin(), r.end(), EntityDistanceOrdering(m_EntityData, pos));

		return r;
	}

	virtual std::vector<entity_id_t> GetEntitiesByPlayer(player_id_t player)
	{
		std::vector<entity_id_t> entities;

		u32 ownerMask = CalcOwnerMask(player);

		for (std::map<entity_id_t, EntityData>::const_iterator it = m_EntityData.begin(); it != m_EntityData.end(); ++it)
		{
			// Check owner and add to list if it matches
			if (CalcOwnerMask(it->second.owner) & ownerMask)
				entities.push_back(it->first);
		}

		return entities;
	}

	virtual void SetDebugOverlay(bool enabled)
	{
		m_DebugOverlayEnabled = enabled;
		m_DebugOverlayDirty = true;
		if (!enabled)
			m_DebugOverlayLines.clear();
	}

	/**
	 * Update all currently-enabled active queries.
	 */
	void ExecuteActiveQueries()
	{
		PROFILE3("ExecuteActiveQueries");

		// Store a queue of all messages before sending any, so we can assume
		// no entities will move until we've finished checking all the ranges
		std::vector<std::pair<entity_id_t, CMessageRangeUpdate> > messages;

		for (std::map<tag_t, Query>::iterator it = m_Queries.begin(); it != m_Queries.end(); ++it)
		{
			Query& q = it->second;

			if (!q.enabled)
				continue;

			CmpPtr<ICmpPosition> cmpSourcePosition(GetSimContext(), q.source);
			if (!cmpSourcePosition || !cmpSourcePosition->IsInWorld())
				continue;

			std::vector<entity_id_t> r;
			r.reserve(q.lastMatch.size());

			PerformQuery(q, r);

			// Compute the changes vs the last match
			std::vector<entity_id_t> added;
			std::vector<entity_id_t> removed;
			std::set_difference(r.begin(), r.end(), q.lastMatch.begin(), q.lastMatch.end(), std::back_inserter(added));
			std::set_difference(q.lastMatch.begin(), q.lastMatch.end(), r.begin(), r.end(), std::back_inserter(removed));

			if (added.empty() && removed.empty())
				continue;

			// Return the 'added' list sorted by distance from the entity
			// (Don't bother sorting 'removed' because they might not even have positions or exist any more)
			CFixedVector2D pos = cmpSourcePosition->GetPosition2D();
			std::stable_sort(added.begin(), added.end(), EntityDistanceOrdering(m_EntityData, pos));

			messages.push_back(std::make_pair(q.source, CMessageRangeUpdate(it->first)));
			messages.back().second.added.swap(added);
			messages.back().second.removed.swap(removed);

			it->second.lastMatch.swap(r);
		}

		for (size_t i = 0; i < messages.size(); ++i)
			GetSimContext().GetComponentManager().PostMessage(messages[i].first, messages[i].second);
	}

	/**
	 * Returns whether the given entity matches the given query (ignoring maxRange)
	 */
	bool TestEntityQuery(const Query& q, entity_id_t id, const EntityData& entity)
	{
		// Quick filter to ignore entities with the wrong owner
		if (!(CalcOwnerMask(entity.owner) & q.ownersMask))
			return false;

		// Ignore entities not present in the world
		if (!entity.inWorld)
			return false;

		// Ignore entities that don't match the current flags
		if (!(entity.flags & q.flagsMask))
			return false;

		// Ignore self
		if (id == q.source)
			return false;

		// Ignore if it's missing the required interface
		if (q.interface && !GetSimContext().GetComponentManager().QueryInterface(id, q.interface))
			return false;

		return true;
	}

	/**
	 * Returns a list of distinct entity IDs that match the given query, sorted by ID.
	 */
	void PerformQuery(const Query& q, std::vector<entity_id_t>& r)
	{
		CmpPtr<ICmpPosition> cmpSourcePosition(GetSimContext(), q.source);
		if (!cmpSourcePosition || !cmpSourcePosition->IsInWorld())
			return;
		CFixedVector2D pos = cmpSourcePosition->GetPosition2D();

		// Special case: range -1.0 means check all entities ignoring distance
		if (q.maxRange == entity_pos_t::FromInt(-1))
		{
			for (std::map<entity_id_t, EntityData>::const_iterator it = m_EntityData.begin(); it != m_EntityData.end(); ++it)
			{
				if (!TestEntityQuery(q, it->first, it->second))
					continue;

				r.push_back(it->first);
			}
		}
		else
		{
			// Get a quick list of entities that are potentially in range
			std::vector<entity_id_t> ents = m_Subdivision.GetNear(pos, q.maxRange);

			for (size_t i = 0; i < ents.size(); ++i)
			{
				std::map<entity_id_t, EntityData>::const_iterator it = m_EntityData.find(ents[i]);
				ENSURE(it != m_EntityData.end());

				if (!TestEntityQuery(q, it->first, it->second))
					continue;

				// Restrict based on precise distance
				int distVsMax = (CFixedVector2D(it->second.x, it->second.z) - pos).CompareLength(q.maxRange);
				if (distVsMax > 0)
					continue;

				if (!q.minRange.IsZero())
				{
					int distVsMin = (CFixedVector2D(it->second.x, it->second.z) - pos).CompareLength(q.minRange);
					if (distVsMin < 0)
						continue;
				}

				r.push_back(it->first);
			}
		}
	}

	Query ConstructQuery(entity_id_t source,
		entity_pos_t minRange, entity_pos_t maxRange,
		const std::vector<int>& owners, int requiredInterface, u8 flagsMask)
	{
		// Min range must be non-negative
		if (minRange < entity_pos_t::Zero())
			LOGWARNING(L"CCmpRangeManager: Invalid min range %f in query for entity %u", minRange.ToDouble(), source);

		// Max range must be non-negative, or else -1
		if (maxRange < entity_pos_t::Zero() && maxRange != entity_pos_t::FromInt(-1))
			LOGWARNING(L"CCmpRangeManager: Invalid max range %f in query for entity %u", maxRange.ToDouble(), source);

		Query q;
		q.enabled = false;
		q.source = source;
		q.minRange = minRange;
		q.maxRange = maxRange;

		q.ownersMask = 0;
		for (size_t i = 0; i < owners.size(); ++i)
			q.ownersMask |= CalcOwnerMask(owners[i]);

		q.interface = requiredInterface;
		q.flagsMask = flagsMask;

		return q;
	}

	void RenderSubmit(SceneCollector& collector)
	{
		if (!m_DebugOverlayEnabled)
			return;

		CColor enabledRingColour(0, 1, 0, 1);
		CColor disabledRingColour(1, 0, 0, 1);
		CColor rayColour(1, 1, 0, 0.2f);

		if (m_DebugOverlayDirty)
		{
			m_DebugOverlayLines.clear();

			for (std::map<tag_t, Query>::iterator it = m_Queries.begin(); it != m_Queries.end(); ++it)
			{
				Query& q = it->second;

				CmpPtr<ICmpPosition> cmpSourcePosition(GetSimContext(), q.source);
				if (!cmpSourcePosition || !cmpSourcePosition->IsInWorld())
					continue;
				CFixedVector2D pos = cmpSourcePosition->GetPosition2D();

				// Draw the max range circle
				m_DebugOverlayLines.push_back(SOverlayLine());
				m_DebugOverlayLines.back().m_Color = (q.enabled ? enabledRingColour : disabledRingColour);
				SimRender::ConstructCircleOnGround(GetSimContext(), pos.X.ToFloat(), pos.Y.ToFloat(), q.maxRange.ToFloat(), m_DebugOverlayLines.back(), true);

				// Draw the min range circle
				if (!q.minRange.IsZero())
				{
					m_DebugOverlayLines.push_back(SOverlayLine());
					m_DebugOverlayLines.back().m_Color = (q.enabled ? enabledRingColour : disabledRingColour);
					SimRender::ConstructCircleOnGround(GetSimContext(), pos.X.ToFloat(), pos.Y.ToFloat(), q.minRange.ToFloat(), m_DebugOverlayLines.back(), true);
				}

				// Draw a ray from the source to each matched entity
				for (size_t i = 0; i < q.lastMatch.size(); ++i)
				{
					CmpPtr<ICmpPosition> cmpTargetPosition(GetSimContext(), q.lastMatch[i]);
					if (!cmpTargetPosition || !cmpTargetPosition->IsInWorld())
						continue;
					CFixedVector2D targetPos = cmpTargetPosition->GetPosition2D();

					std::vector<float> coords;
					coords.push_back(pos.X.ToFloat());
					coords.push_back(pos.Y.ToFloat());
					coords.push_back(targetPos.X.ToFloat());
					coords.push_back(targetPos.Y.ToFloat());

					m_DebugOverlayLines.push_back(SOverlayLine());
					m_DebugOverlayLines.back().m_Color = rayColour;
					SimRender::ConstructLineOnGround(GetSimContext(), coords, m_DebugOverlayLines.back(), true);
				}
			}

			m_DebugOverlayDirty = false;
		}

		for (size_t i = 0; i < m_DebugOverlayLines.size(); ++i)
			collector.Submit(&m_DebugOverlayLines[i]);
	}
	
	virtual u8 GetEntityFlagMask(std::string identifier)
	{
		if (identifier == "normal")
			return 1;
		if (identifier == "injured")
			return 2;

		LOGWARNING(L"CCmpRangeManager: Invalid flag identifier %hs", identifier.c_str());
		return 0;
	}
	
	virtual void SetEntityFlag(entity_id_t ent, std::string identifier, bool value)
	{
		std::map<entity_id_t, EntityData>::iterator it = m_EntityData.find(ent);

		// We don't have this entity
		if (it == m_EntityData.end())
			return;

		u8 flag = GetEntityFlagMask(identifier);

		// We don't have a flag set
		if (flag == 0)
		{
			LOGWARNING(L"CCmpRangeManager: Invalid flag identifier %hs for entity %u", identifier.c_str(), ent);
			return;
		}

		if (value)
			it->second.flags |= flag;
		else
			it->second.flags &= ~flag;
	}

	// ****************************************************************

	// LOS implementation:

	virtual CLosQuerier GetLosQuerier(player_id_t player)
	{
		if (GetLosRevealAll(player))
			return CLosQuerier(player, m_LosStateRevealed, m_TerrainVerticesPerSide);
		else
			return CLosQuerier(player, m_LosState, m_TerrainVerticesPerSide);
	}

	virtual ELosVisibility GetLosVisibility(entity_id_t ent, player_id_t player, bool forceRetainInFog)
	{
		// (We can't use m_EntityData since this needs to handle LOCAL entities too)

		// Entities not with positions in the world are never visible
		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), ent);
		if (!cmpPosition || !cmpPosition->IsInWorld())
			return VIS_HIDDEN;

		CFixedVector2D pos = cmpPosition->GetPosition2D();

		int i = (pos.X / (int)TERRAIN_TILE_SIZE).ToInt_RoundToNearest();
		int j = (pos.Y / (int)TERRAIN_TILE_SIZE).ToInt_RoundToNearest();

		// Reveal flag makes all positioned entities visible
		if (GetLosRevealAll(player))
		{
			if (LosIsOffWorld(i, j))
				return VIS_HIDDEN;
			else
				return VIS_VISIBLE;
		}

		// Visible if within a visible region

		CLosQuerier los(player, m_LosState, m_TerrainVerticesPerSide);

		if (los.IsVisible(i, j))
			return VIS_VISIBLE;

		// Fogged if the 'retain in fog' flag is set, and in a non-visible explored region
		if (los.IsExplored(i, j))
		{
			CmpPtr<ICmpVision> cmpVision(GetSimContext(), ent);
			if (forceRetainInFog || (cmpVision && cmpVision->GetRetainInFog()))
				return VIS_FOGGED;
		}

		// Otherwise not visible
		return VIS_HIDDEN;
	}

	virtual void SetLosRevealAll(player_id_t player, bool enabled)
	{
		m_LosRevealAll[player] = enabled;
	}

	virtual bool GetLosRevealAll(player_id_t player)
	{
		std::map<player_id_t, bool>::const_iterator it;

		// Special player value can force reveal-all for every player
		it = m_LosRevealAll.find(-1);
		if (it != m_LosRevealAll.end() && it->second)
			return true;

		// Otherwise check the player-specific flag
		it = m_LosRevealAll.find(player);
		if (it != m_LosRevealAll.end() && it->second)
			return true;

		return false;
	}

	virtual void SetLosCircular(bool enabled)
	{
		m_LosCircular = enabled;

		ResetDerivedData(false);
	}

	virtual bool GetLosCircular()
	{
		return m_LosCircular;
	}

	void UpdateTerritoriesLos()
	{
		CmpPtr<ICmpTerritoryManager> cmpTerritoryManager(GetSimContext(), SYSTEM_ENTITY);
		if (!cmpTerritoryManager || !cmpTerritoryManager->NeedUpdate(&m_TerritoriesDirtyID))
			return;

		const Grid<u8>& grid = cmpTerritoryManager->GetTerritoryGrid();
		ENSURE(grid.m_W == m_TerrainVerticesPerSide-1 && grid.m_H == m_TerrainVerticesPerSide-1);

		// For each tile, if it is owned by a valid player then update the LOS
		// for every vertex around that tile, to mark them as explored

		for (u16 j = 0; j < grid.m_H; ++j)
		{
			for (u16 i = 0; i < grid.m_W; ++i)
			{
				u8 p = grid.get(i, j) & ICmpTerritoryManager::TERRITORY_PLAYER_MASK;
				if (p > 0 && p <= MAX_LOS_PLAYER_ID)
				{
					m_LosState[i + j*m_TerrainVerticesPerSide] |= (LOS_EXPLORED << (2*(p-1)));
					m_LosState[i+1 + j*m_TerrainVerticesPerSide] |= (LOS_EXPLORED << (2*(p-1)));
					m_LosState[i + (j+1)*m_TerrainVerticesPerSide] |= (LOS_EXPLORED << (2*(p-1)));
					m_LosState[i+1 + (j+1)*m_TerrainVerticesPerSide] |= (LOS_EXPLORED << (2*(p-1)));
				}
			}
		}
	}

	/**
	 * Returns whether the given vertex is outside the normal bounds of the world
	 * (i.e. outside the range of a circular map)
	 */
	inline bool LosIsOffWorld(ssize_t i, ssize_t j)
	{
		// WARNING: CCmpObstructionManager::Rasterise needs to be kept in sync with this
		const ssize_t edgeSize = 3; // number of vertexes around the edge that will be off-world

		if (m_LosCircular)
		{
			// With a circular map, vertex is off-world if hypot(i - size/2, j - size/2) >= size/2:

			ssize_t dist2 = (i - m_TerrainVerticesPerSide/2)*(i - m_TerrainVerticesPerSide/2)
					+ (j - m_TerrainVerticesPerSide/2)*(j - m_TerrainVerticesPerSide/2);

			ssize_t r = m_TerrainVerticesPerSide/2 - edgeSize + 1;
				// subtract a bit from the radius to ensure nice
				// SoD blurring around the edges of the map

			return (dist2 >= r*r);
		}
		else
		{
			// With a square map, the outermost edge of the map should be off-world,
			// so the SoD texture blends out nicely

			return (i < edgeSize || j < edgeSize || i >= m_TerrainVerticesPerSide-edgeSize || j >= m_TerrainVerticesPerSide-edgeSize);
		}
	}

	/**
	 * Update the LOS state of tiles within a given horizontal strip (i0,j) to (i1,j) (inclusive).
	 */
	inline void LosAddStripHelper(u8 owner, i32 i0, i32 i1, i32 j, u16* counts)
	{
		if (i1 < i0)
			return;

		i32 idx0 = j*m_TerrainVerticesPerSide + i0;
		i32 idx1 = j*m_TerrainVerticesPerSide + i1;

		for (i32 idx = idx0; idx <= idx1; ++idx)
		{
			// Increasing from zero to non-zero - move from unexplored/explored to visible+explored
			if (counts[idx] == 0)
			{
				i32 i = i0 + idx - idx0;
				if (!LosIsOffWorld(i, j))
					m_LosState[idx] |= ((LOS_VISIBLE | LOS_EXPLORED) << (2*(owner-1)));
			}

			ASSERT(counts[idx] < 65535);
			counts[idx] = (u16)(counts[idx] + 1); // ignore overflow; the player should never have 64K units
		}
	}

	/**
	 * Update the LOS state of tiles within a given horizontal strip (i0,j) to (i1,j) (inclusive).
	 */
	inline void LosRemoveStripHelper(u8 owner, i32 i0, i32 i1, i32 j, u16* counts)
	{
		if (i1 < i0)
			return;

		i32 idx0 = j*m_TerrainVerticesPerSide + i0;
		i32 idx1 = j*m_TerrainVerticesPerSide + i1;

		for (i32 idx = idx0; idx <= idx1; ++idx)
		{
			ASSERT(counts[idx] > 0);
			counts[idx] = (u16)(counts[idx] - 1);

			// Decreasing from non-zero to zero - move from visible+explored to explored
			if (counts[idx] == 0)
			{
				// (If LosIsOffWorld then this is a no-op, so don't bother doing the check)
				m_LosState[idx] &= ~(LOS_VISIBLE << (2*(owner-1)));
			}
		}
	}

	/**
	 * Update the LOS state of tiles within a given circular range,
	 * either adding or removing visibility depending on the template parameter.
	 * Assumes owner is in the valid range.
	 */
	template<bool adding>
	void LosUpdateHelper(u8 owner, entity_pos_t visionRange, CFixedVector2D pos)
	{
		if (m_TerrainVerticesPerSide == 0) // do nothing if not initialised yet
			return;

		PROFILE("LosUpdateHelper");

		std::vector<u16>& counts = m_LosPlayerCounts.at(owner);

		// Lazy initialisation of counts:
		if (counts.empty())
			counts.resize(m_TerrainVerticesPerSide*m_TerrainVerticesPerSide);

		u16* countsData = &counts[0];

		// Compute the circular region as a series of strips.
		// Rather than quantise pos to vertexes, we do more precise sub-tile computations
		// to get smoother behaviour as a unit moves rather than jumping a whole tile
		// at once.
		// To avoid the cost of sqrt when computing the outline of the circle,
		// we loop from the bottom to the top and estimate the width of the current
		// strip based on the previous strip, then adjust each end of the strip
		// inwards or outwards until it's the widest that still falls within the circle.

		// Compute top/bottom coordinates, and clamp to exclude the 1-tile border around the map
		// (so that we never render the sharp edge of the map)
		i32 j0 = ((pos.Y - visionRange)/(int)TERRAIN_TILE_SIZE).ToInt_RoundToInfinity();
		i32 j1 = ((pos.Y + visionRange)/(int)TERRAIN_TILE_SIZE).ToInt_RoundToNegInfinity();
		i32 j0clamp = std::max(j0, 1);
		i32 j1clamp = std::min(j1, m_TerrainVerticesPerSide-2);

		// Translate world coordinates into fractional tile-space coordinates
		entity_pos_t x = pos.X / (int)TERRAIN_TILE_SIZE;
		entity_pos_t y = pos.Y / (int)TERRAIN_TILE_SIZE;
		entity_pos_t r = visionRange / (int)TERRAIN_TILE_SIZE;
		entity_pos_t r2 = r.Square();

		// Compute the integers on either side of x
		i32 xfloor = (x - entity_pos_t::Epsilon()).ToInt_RoundToNegInfinity();
		i32 xceil = (x + entity_pos_t::Epsilon()).ToInt_RoundToInfinity();

		// Initialise the strip (i0, i1) to a rough guess
		i32 i0 = xfloor;
		i32 i1 = xceil;

		for (i32 j = j0clamp; j <= j1clamp; ++j)
		{
			// Adjust i0 and i1 to be the outermost values that don't exceed
			// the circle's radius (i.e. require dy^2 + dx^2 <= r^2).
			// When moving the points inwards, clamp them to xceil+1 or xfloor-1
			// so they don't accidentally shoot off in the wrong direction forever.

			entity_pos_t dy = entity_pos_t::FromInt(j) - y;
			entity_pos_t dy2 = dy.Square();
			while (dy2 + (entity_pos_t::FromInt(i0-1) - x).Square() <= r2)
				--i0;
			while (i0 < xceil && dy2 + (entity_pos_t::FromInt(i0) - x).Square() > r2)
				++i0;
			while (dy2 + (entity_pos_t::FromInt(i1+1) - x).Square() <= r2)
				++i1;
			while (i1 > xfloor && dy2 + (entity_pos_t::FromInt(i1) - x).Square() > r2)
				--i1;

#if DEBUG_RANGE_MANAGER_BOUNDS
			if (i0 <= i1)
			{
				ENSURE(dy2 + (entity_pos_t::FromInt(i0) - x).Square() <= r2);
				ENSURE(dy2 + (entity_pos_t::FromInt(i1) - x).Square() <= r2);
			}
			ENSURE(dy2 + (entity_pos_t::FromInt(i0 - 1) - x).Square() > r2);
			ENSURE(dy2 + (entity_pos_t::FromInt(i1 + 1) - x).Square() > r2);
#endif

			// Clamp the strip to exclude the 1-tile border,
			// then add or remove the strip as requested
			i32 i0clamp = std::max(i0, 1);
			i32 i1clamp = std::min(i1, m_TerrainVerticesPerSide-2);
			if (adding)
				LosAddStripHelper(owner, i0clamp, i1clamp, j, countsData);
			else
				LosRemoveStripHelper(owner, i0clamp, i1clamp, j, countsData);
		}
	}

	/**
	 * Update the LOS state of tiles within a given circular range,
	 * by removing visibility around the 'from' position
	 * and then adding visibility around the 'to' position.
	 */
	void LosUpdateHelperIncremental(u8 owner, entity_pos_t visionRange, CFixedVector2D from, CFixedVector2D to)
	{
		if (m_TerrainVerticesPerSide == 0) // do nothing if not initialised yet
			return;

		PROFILE("LosUpdateHelperIncremental");

		std::vector<u16>& counts = m_LosPlayerCounts.at(owner);

		// Lazy initialisation of counts:
		if (counts.empty())
			counts.resize(m_TerrainVerticesPerSide*m_TerrainVerticesPerSide);

		u16* countsData = &counts[0];

		// See comments in LosUpdateHelper.
		// This does exactly the same, except computing the strips for
		// both circles simultaneously.
		// (The idea is that the circles will be heavily overlapping,
		// so we can compute the difference between the removed/added strips
		// and only have to touch tiles that have a net change.)

		i32 j0_from = ((from.Y - visionRange)/(int)TERRAIN_TILE_SIZE).ToInt_RoundToInfinity();
		i32 j1_from = ((from.Y + visionRange)/(int)TERRAIN_TILE_SIZE).ToInt_RoundToNegInfinity();
		i32 j0_to = ((to.Y - visionRange)/(int)TERRAIN_TILE_SIZE).ToInt_RoundToInfinity();
		i32 j1_to = ((to.Y + visionRange)/(int)TERRAIN_TILE_SIZE).ToInt_RoundToNegInfinity();
		i32 j0clamp = std::max(std::min(j0_from, j0_to), 1);
		i32 j1clamp = std::min(std::max(j1_from, j1_to), m_TerrainVerticesPerSide-2);

		entity_pos_t x_from = from.X / (int)TERRAIN_TILE_SIZE;
		entity_pos_t y_from = from.Y / (int)TERRAIN_TILE_SIZE;
		entity_pos_t x_to = to.X / (int)TERRAIN_TILE_SIZE;
		entity_pos_t y_to = to.Y / (int)TERRAIN_TILE_SIZE;
		entity_pos_t r = visionRange / (int)TERRAIN_TILE_SIZE;
		entity_pos_t r2 = r.Square();

		i32 xfloor_from = (x_from - entity_pos_t::Epsilon()).ToInt_RoundToNegInfinity();
		i32 xceil_from = (x_from + entity_pos_t::Epsilon()).ToInt_RoundToInfinity();
		i32 xfloor_to = (x_to - entity_pos_t::Epsilon()).ToInt_RoundToNegInfinity();
		i32 xceil_to = (x_to + entity_pos_t::Epsilon()).ToInt_RoundToInfinity();

		i32 i0_from = xfloor_from;
		i32 i1_from = xceil_from;
		i32 i0_to = xfloor_to;
		i32 i1_to = xceil_to;

		for (i32 j = j0clamp; j <= j1clamp; ++j)
		{
			entity_pos_t dy_from = entity_pos_t::FromInt(j) - y_from;
			entity_pos_t dy2_from = dy_from.Square();
			while (dy2_from + (entity_pos_t::FromInt(i0_from-1) - x_from).Square() <= r2)
				--i0_from;
			while (i0_from < xceil_from && dy2_from + (entity_pos_t::FromInt(i0_from) - x_from).Square() > r2)
				++i0_from;
			while (dy2_from + (entity_pos_t::FromInt(i1_from+1) - x_from).Square() <= r2)
				++i1_from;
			while (i1_from > xfloor_from && dy2_from + (entity_pos_t::FromInt(i1_from) - x_from).Square() > r2)
				--i1_from;

			entity_pos_t dy_to = entity_pos_t::FromInt(j) - y_to;
			entity_pos_t dy2_to = dy_to.Square();
			while (dy2_to + (entity_pos_t::FromInt(i0_to-1) - x_to).Square() <= r2)
				--i0_to;
			while (i0_to < xceil_to && dy2_to + (entity_pos_t::FromInt(i0_to) - x_to).Square() > r2)
				++i0_to;
			while (dy2_to + (entity_pos_t::FromInt(i1_to+1) - x_to).Square() <= r2)
				++i1_to;
			while (i1_to > xfloor_to && dy2_to + (entity_pos_t::FromInt(i1_to) - x_to).Square() > r2)
				--i1_to;

#if DEBUG_RANGE_MANAGER_BOUNDS
			if (i0_from <= i1_from)
			{
				ENSURE(dy2_from + (entity_pos_t::FromInt(i0_from) - x_from).Square() <= r2);
				ENSURE(dy2_from + (entity_pos_t::FromInt(i1_from) - x_from).Square() <= r2);
			}
			ENSURE(dy2_from + (entity_pos_t::FromInt(i0_from - 1) - x_from).Square() > r2);
			ENSURE(dy2_from + (entity_pos_t::FromInt(i1_from + 1) - x_from).Square() > r2);
			if (i0_to <= i1_to)
			{
				ENSURE(dy2_to + (entity_pos_t::FromInt(i0_to) - x_to).Square() <= r2);
				ENSURE(dy2_to + (entity_pos_t::FromInt(i1_to) - x_to).Square() <= r2);
			}
			ENSURE(dy2_to + (entity_pos_t::FromInt(i0_to - 1) - x_to).Square() > r2);
			ENSURE(dy2_to + (entity_pos_t::FromInt(i1_to + 1) - x_to).Square() > r2);
#endif

			// Check whether this strip moved at all
			if (!(i0_to == i0_from && i1_to == i1_from))
			{
				i32 i0clamp_from = std::max(i0_from, 1);
				i32 i1clamp_from = std::min(i1_from, m_TerrainVerticesPerSide-2);
				i32 i0clamp_to = std::max(i0_to, 1);
				i32 i1clamp_to = std::min(i1_to, m_TerrainVerticesPerSide-2);

				// Check whether one strip is negative width,
				// and we can just add/remove the entire other strip
				if (i1clamp_from < i0clamp_from)
				{
					LosAddStripHelper(owner, i0clamp_to, i1clamp_to, j, countsData);
				}
				else if (i1clamp_to < i0clamp_to)
				{
					LosRemoveStripHelper(owner, i0clamp_from, i1clamp_from, j, countsData);
				}
				else
				{
					// There are four possible regions of overlap between the two strips
					// (remove before add, remove after add, add before remove, add after remove).
					// Process each of the regions as its own strip.
					// (If this produces negative-width strips then they'll just get ignored
					// which is fine.)
					// (If the strips don't actually overlap (which is very rare with normal unit
					// movement speeds), the region between them will be both added and removed,
					// so we have to do the add first to avoid overflowing to -1 and triggering
					// assertion failures.)
					LosAddStripHelper(owner, i0clamp_to, i0clamp_from-1, j, countsData);
					LosAddStripHelper(owner, i1clamp_from+1, i1clamp_to, j, countsData);
					LosRemoveStripHelper(owner, i0clamp_from, i0clamp_to-1, j, countsData);
					LosRemoveStripHelper(owner, i1clamp_to+1, i1clamp_from, j, countsData);
				}
			}
		}
	}

	void LosAdd(player_id_t owner, entity_pos_t visionRange, CFixedVector2D pos)
	{
		if (visionRange.IsZero() || owner <= 0 || owner > MAX_LOS_PLAYER_ID)
			return;

		LosUpdateHelper<true>((u8)owner, visionRange, pos);
	}

	void LosRemove(player_id_t owner, entity_pos_t visionRange, CFixedVector2D pos)
	{
		if (visionRange.IsZero() || owner <= 0 || owner > MAX_LOS_PLAYER_ID)
			return;

		LosUpdateHelper<false>((u8)owner, visionRange, pos);
	}

	void LosMove(player_id_t owner, entity_pos_t visionRange, CFixedVector2D from, CFixedVector2D to)
	{
		if (visionRange.IsZero() || owner <= 0 || owner > MAX_LOS_PLAYER_ID)
			return;

		if ((from - to).CompareLength(visionRange) > 0)
		{
			// If it's a very large move, then simply remove and add to the new position

			LosUpdateHelper<false>((u8)owner, visionRange, from);
			LosUpdateHelper<true>((u8)owner, visionRange, to);
		}
		else
		{
			// Otherwise use the version optimised for mostly-overlapping circles

			LosUpdateHelperIncremental((u8)owner, visionRange, from, to);
		}
	}

	virtual i32 GetPercentMapExplored(player_id_t player)
	{
		i32 exploredVertices = 0;
		i32 overallVisibleVertices = 0;
		CLosQuerier los(player, m_LosState, m_TerrainVerticesPerSide);

		for (i32 j = 0; j < m_TerrainVerticesPerSide; j++)
		{
			for (i32 i = 0; i < m_TerrainVerticesPerSide; i++)
			{
				if (!LosIsOffWorld(i, j))
				{
					overallVisibleVertices++;
					exploredVertices += (i32)los.IsExplored_UncheckedRange(i, j);
				}
			}
		}
		return exploredVertices * 100 / overallVisibleVertices;
	}
};

REGISTER_COMPONENT_TYPE(RangeManager)
