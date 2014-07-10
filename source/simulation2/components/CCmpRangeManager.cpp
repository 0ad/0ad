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

#include "precompiled.h"

#include "simulation2/system/Component.h"
#include "ICmpRangeManager.h"

#include "ICmpTerrain.h"
#include "simulation2/system/EntityMap.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpTerritoryManager.h"
#include "simulation2/components/ICmpVision.h"
#include "simulation2/components/ICmpWaterManager.h"
#include "simulation2/helpers/Render.h"
#include "simulation2/helpers/Spatial.h"

#include "graphics/Overlay.h"
#include "graphics/Terrain.h"
#include "lib/timer.h"
#include "ps/CLogger.h"
#include "ps/Overlay.h"
#include "ps/Profile.h"
#include "renderer/Scene.h"
#include "lib/ps_stl.h"

#define LOS_TILES_RATIO 8
#define DEBUG_RANGE_MANAGER_BOUNDS 0

/**
 * Representation of a range query.
 */
struct Query
{
	bool enabled;
	bool parabolic;
	CEntityHandle source; // TODO: this could crash if an entity is destroyed while a Query is still referencing it
	entity_pos_t minRange;
	entity_pos_t maxRange;
	entity_pos_t elevationBonus;
	u32 ownersMask;
	i32 interface;
	std::vector<entity_id_t> lastMatch;
	u8 flagsMask;
};

/**
 * Convert an owner ID (-1 = unowned, 0 = gaia, 1..30 = players)
 * into a 32-bit mask for quick set-membership tests.
 */
static inline u32 CalcOwnerMask(player_id_t owner)
{
	if (owner >= -1 && owner < 31)
		return 1 << (1+owner);
	else
		return 0; // owner was invalid
}

/**
 * Returns LOS mask for given player.
 */
static inline u32 CalcPlayerLosMask(player_id_t player)
{
	if (player > 0 && player <= 16)
		return ICmpRangeManager::LOS_MASK << (2*(player-1));
	return 0;
}

/**
 * Returns shared LOS mask for given list of players.
 */
static u32 CalcSharedLosMask(std::vector<player_id_t> players)
{
	u32 playerMask = 0;
	for (size_t i = 0; i < players.size(); i++)
		playerMask |= CalcPlayerLosMask(players[i]);

	return playerMask;
}

/**
 * Checks whether v is in a parabolic range of (0,0,0)
 * The highest point of the paraboloid is (0,range/2,0)
 * and the circle of distance 'range' around (0,0,0) on height y=0 is part of the paraboloid
 *
 * Avoids sqrting and overflowing.
 */
static bool InParabolicRange(CFixedVector3D v, fixed range)
{
	i32 x = v.X.GetInternalValue(); // abs(x) <= 2^31
	i32 z = v.Z.GetInternalValue();
	u64 xx = (u64)FIXED_MUL_I64_I32_I32(x, x); // xx <= 2^62
	u64 zz = (u64)FIXED_MUL_I64_I32_I32(z, z);
	i64 d2 = (xx + zz) >> 1; // d2 <= 2^62 (no overflow)

	i32 y = v.Y.GetInternalValue();
	i32 c = range.GetInternalValue();
	i32 c_2 = c >> 1;

	i64 c2 = FIXED_MUL_I64_I32_I32(c_2 - y, c);

	if (d2 <= c2)
		return true;

	return false;
}

struct EntityParabolicRangeOutline
{
	entity_id_t source;
	CFixedVector3D position;
	entity_pos_t range;
	std::vector<entity_pos_t> outline;
};

static std::map<entity_id_t, EntityParabolicRangeOutline> ParabolicRangesOutlines;

/**
 * Representation of an entity, with the data needed for queries.
 */
struct EntityData
{
	EntityData() : retainInFog(0), owner(-1), inWorld(0), flags(1) { }
	entity_pos_t x, z;
	entity_pos_t visionRange;
	u32 visibilities; // 2-bit visibility, per player
	u8 retainInFog; // boolean
	i8 owner;
	u8 inWorld; // boolean
	u8 flags; // See GetEntityFlagMask
};

cassert(sizeof(EntityData) == 20);

/**
 * Serialization helper template for Query
 */
struct SerializeQuery
{
	template<typename S>
	void Common(S& serialize, const char* UNUSED(name), Query& value)
	{
		serialize.Bool("enabled", value.enabled);
		serialize.Bool("parabolic",value.parabolic);
		serialize.NumberFixed_Unbounded("min range", value.minRange);
		serialize.NumberFixed_Unbounded("max range", value.maxRange);
		serialize.NumberFixed_Unbounded("elevation bonus", value.elevationBonus);
		serialize.NumberU32_Unbounded("owners mask", value.ownersMask);
		serialize.NumberI32_Unbounded("interface", value.interface);
		SerializeVector<SerializeU32_Unbounded>()(serialize, "last match", value.lastMatch);
		serialize.NumberU8_Unbounded("flagsMask", value.flagsMask);
	}

	void operator()(ISerializer& serialize, const char* name, Query& value, const CSimContext& UNUSED(context))
	{
		Common(serialize, name, value);

		uint32_t id = value.source.GetId();
		serialize.NumberU32_Unbounded("source", id);
	}

	void operator()(IDeserializer& deserialize, const char* name, Query& value, const CSimContext& context)
	{
		Common(deserialize, name, value);

		uint32_t id;
		deserialize.NumberU32_Unbounded("source", id);
		value.source = context.GetComponentManager().LookupEntityHandle(id, true);
			// the referenced entity might not have been deserialized yet,
			// so tell LookupEntityHandle to allocate the handle if necessary
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
		serialize.NumberU32_Unbounded("visibilities", value.visibilities);
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
	EntityDistanceOrdering(const EntityMap<EntityData>& entities, const CFixedVector2D& source) :
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

	const EntityMap<EntityData>& m_EntityData;
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
		componentManager.SubscribeGloballyToMessageType(MT_VisionRangeChanged);

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
	EntityMap<EntityData> m_EntityData;

	SpatialSubdivision m_Subdivision; // spatial index of m_EntityData

	// LOS state:
	static const player_id_t MAX_LOS_PLAYER_ID = 16;

	std::vector<bool> m_LosRevealAll;
	bool m_LosCircular;
	i32 m_TerrainVerticesPerSide;
	size_t m_TerritoriesDirtyID;
	
	// Cache for visibility tracking (not serialized)
	i32 m_LosTilesPerSide;
	bool* m_DirtyVisibility;
	std::vector<std::set<entity_id_t> > m_LosTiles;
	// List of entities that must be updated, regardless of the status of their tile
	std::vector<entity_id_t> m_ModifiedEntities;

	// Counts of units seeing vertex, per vertex, per player (starting with player 0).
	// Use u16 to avoid overflows when we have very large (but not infeasibly large) numbers
	// of units in a very small area.
	// (Note we use vertexes, not tiles, to better match the renderer.)
	// Lazily constructed when it's needed, to save memory in smaller games.
	std::vector<std::vector<u16> > m_LosPlayerCounts;

	// 2-bit ELosState per player, starting with player 1 (not 0!) up to player MAX_LOS_PLAYER_ID (inclusive)
	std::vector<u32> m_LosState;

	// Special static visibility data for the "reveal whole map" mode
	// (TODO: this is usually a waste of memory)
	std::vector<u32> m_LosStateRevealed;

	// Shared LOS masks, one per player.
	std::vector<u32> m_SharedLosMasks;

	// Cache explored vertices per player (not serialized)
	u32 m_TotalInworldVertices;
	std::vector<u32> m_ExploredVertices;

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
		ResetSubdivisions(entity_pos_t::FromInt(1024), entity_pos_t::FromInt(1024));

		// The whole map should be visible to Gaia by default, else e.g. animals
		// will get confused when trying to run from enemies
		m_LosRevealAll.resize(MAX_LOS_PLAYER_ID+2,false);
		m_LosRevealAll[0] = true;
		m_SharedLosMasks.resize(MAX_LOS_PLAYER_ID+2,0);

		m_LosCircular = false;
		m_TerrainVerticesPerSide = 0;

		m_DirtyVisibility = NULL;

		m_TerritoriesDirtyID = 0;
	}

	virtual void Deinit()
	{
		delete[] m_DirtyVisibility;
	}

	template<typename S>
	void SerializeCommon(S& serialize)
	{
		serialize.NumberFixed_Unbounded("world x0", m_WorldX0);
		serialize.NumberFixed_Unbounded("world z0", m_WorldZ0);
		serialize.NumberFixed_Unbounded("world x1", m_WorldX1);
		serialize.NumberFixed_Unbounded("world z1", m_WorldZ1);

		serialize.NumberU32_Unbounded("query next", m_QueryNext);
		SerializeMap<SerializeU32_Unbounded, SerializeQuery>()(serialize, "queries", m_Queries, GetSimContext());
		SerializeEntityMap<SerializeEntityData>()(serialize, "entity data", m_EntityData);

		SerializeVector<SerializeBool>()(serialize, "los reveal all", m_LosRevealAll);
		serialize.Bool("los circular", m_LosCircular);
		serialize.NumberI32_Unbounded("terrain verts per side", m_TerrainVerticesPerSide);

		SerializeVector<SerializeU32_Unbounded>()(serialize, "modified entities", m_ModifiedEntities);

		// We don't serialize m_Subdivision, m_LosPlayerCounts or m_LosTiles
		// since they can be recomputed from the entity data when deserializing;
		// m_LosState must be serialized since it depends on the history of exploration

		SerializeVector<SerializeU32_Unbounded>()(serialize, "los state", m_LosState);
		SerializeVector<SerializeU32_Unbounded>()(serialize, "shared los masks", m_SharedLosMasks);
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
			m_EntityData.insert(ent, entdata);
			break;
		}
		case MT_PositionChanged:
		{
			const CMessagePositionChanged& msgData = static_cast<const CMessagePositionChanged&> (msg);
			entity_id_t ent = msgData.entity;

			EntityMap<EntityData>::iterator it = m_EntityData.find(ent);

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
					i32 oldLosTile = PosToLosTilesHelper(it->second.x, it->second.z);
					i32 newLosTile = PosToLosTilesHelper(msgData.x, msgData.z);
					if (oldLosTile != newLosTile)
					{
						RemoveFromTile(oldLosTile, ent);
						AddToTile(newLosTile, ent);
					}
				}
				else
				{
					CFixedVector2D to(msgData.x, msgData.z);
					m_Subdivision.Add(ent, to);
					LosAdd(it->second.owner, it->second.visionRange, to);
					AddToTile(PosToLosTilesHelper(msgData.x, msgData.z), ent);
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
					RemoveFromTile(PosToLosTilesHelper(it->second.x, it->second.z), ent);
				}

				it->second.inWorld = 0;
				it->second.x = entity_pos_t::Zero();
				it->second.z = entity_pos_t::Zero();
			}

			m_ModifiedEntities.push_back(ent);

			break;
		}
		case MT_OwnershipChanged:
		{
			const CMessageOwnershipChanged& msgData = static_cast<const CMessageOwnershipChanged&> (msg);
			entity_id_t ent = msgData.entity;

			EntityMap<EntityData>::iterator it = m_EntityData.find(ent);

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

			EntityMap<EntityData>::iterator it = m_EntityData.find(ent);

			// Ignore if we're not already tracking this entity
			if (it == m_EntityData.end())
				break;

			if (it->second.inWorld)
			{
				m_Subdivision.Remove(ent, CFixedVector2D(it->second.x, it->second.z));
				RemoveFromTile(PosToLosTilesHelper(it->second.x, it->second.z), ent);
			}

			// This will be called after Ownership's OnDestroy, so ownership will be set
			// to -1 already and we don't have to do a LosRemove here
			ENSURE(it->second.owner == -1);

			m_EntityData.erase(it);

			break;
		}
		case MT_VisionRangeChanged:
		{
			const CMessageVisionRangeChanged& msgData = static_cast<const CMessageVisionRangeChanged&> (msg);
			entity_id_t ent = msgData.entity;

			EntityMap<EntityData>::iterator it = m_EntityData.find(ent);

			// Ignore if we're not already tracking this entity
			if (it == m_EntityData.end())
				break;

			CmpPtr<ICmpVision> cmpVision(GetSimContext(), ent);
			if (!cmpVision)
				break;

			entity_pos_t oldRange = it->second.visionRange;
			entity_pos_t newRange = msgData.newRange;

			// If the range changed and the entity's in-world, we need to manually adjust it
			//	but if it's not in-world, we only need to set the new vision range
			CFixedVector2D pos(it->second.x, it->second.z);
			if (it->second.inWorld)
				LosRemove(it->second.owner, oldRange, pos);

			it->second.visionRange = newRange;

			if (it->second.inWorld)
				LosAdd(it->second.owner, newRange, pos);

			break;
		}
		case MT_Update:
		{
			m_DebugOverlayDirty = true;
			UpdateVisibilityData();
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
		
		m_LosTilesPerSide = (m_TerrainVerticesPerSide - 1)/LOS_TILES_RATIO;

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
		SpatialSubdivision oldSubdivision = m_Subdivision;

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

	SpatialSubdivision* GetSubdivision()
	{
		return & m_Subdivision;
	}

	// Reinitialise subdivisions and LOS data, based on entity data
	void ResetDerivedData(bool skipLosState)
	{
		ENSURE(m_WorldX0.IsZero() && m_WorldZ0.IsZero()); // don't bother implementing non-zero offsets yet
		ResetSubdivisions(m_WorldX1, m_WorldZ1);

		m_LosPlayerCounts.clear();
		m_LosPlayerCounts.resize(MAX_LOS_PLAYER_ID+1);
		m_ExploredVertices.clear();
		m_ExploredVertices.resize(MAX_LOS_PLAYER_ID+1, 0);
		if (skipLosState)
		{
			// recalc current exploration stats.
			for (i32 j = 0; j < m_TerrainVerticesPerSide; j++)
			{
				for (i32 i = 0; i < m_TerrainVerticesPerSide; i++)
				{
					if (!LosIsOffWorld(i, j))
					{
						for (u8 k = 1; k < MAX_LOS_PLAYER_ID+1; ++k)
							m_ExploredVertices.at(k) += ((m_LosState[j*m_TerrainVerticesPerSide + i] & (LOS_EXPLORED << (2*(k-1)))) > 0);
					}
				}
			}
		}
		else
		{
			m_LosState.clear();
			m_LosState.resize(m_TerrainVerticesPerSide*m_TerrainVerticesPerSide);
		}
		m_LosStateRevealed.clear();
		m_LosStateRevealed.resize(m_TerrainVerticesPerSide*m_TerrainVerticesPerSide);
		
		delete[] m_DirtyVisibility;
		m_DirtyVisibility = new bool[m_LosTilesPerSide*m_LosTilesPerSide]();
		m_LosTiles.clear();
		m_LosTiles.resize(m_LosTilesPerSide*m_LosTilesPerSide);

		for (EntityMap<EntityData>::const_iterator it = m_EntityData.begin(); it != m_EntityData.end(); ++it)
		{
			if (it->second.inWorld)
			{
				LosAdd(it->second.owner, it->second.visionRange, CFixedVector2D(it->second.x, it->second.z));
				AddToTile(PosToLosTilesHelper(it->second.x, it->second.z), it->first);
			}
		}

		m_TotalInworldVertices = 0;
		for (ssize_t j = 0; j < m_TerrainVerticesPerSide; ++j)
			for (ssize_t i = 0; i < m_TerrainVerticesPerSide; ++i)
			{
				if (LosIsOffWorld(i,j))
					m_LosStateRevealed[i + j*m_TerrainVerticesPerSide] = 0;
				else
				{
					m_LosStateRevealed[i + j*m_TerrainVerticesPerSide] = 0xFFFFFFFFu;
					m_TotalInworldVertices++;
				}
			}
	}

	void ResetSubdivisions(entity_pos_t x1, entity_pos_t z1)
	{
		// Use 8x8 tile subdivisions
		// (TODO: find the optimal number instead of blindly guessing)
		m_Subdivision.Reset(x1, z1, entity_pos_t::FromInt(8*TERRAIN_TILE_SIZE));

		for (EntityMap<EntityData>::const_iterator it = m_EntityData.begin(); it != m_EntityData.end(); ++it)
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

	virtual tag_t CreateActiveParabolicQuery(entity_id_t source,
		entity_pos_t minRange, entity_pos_t maxRange, entity_pos_t elevationBonus,
		std::vector<int> owners, int requiredInterface, u8 flags)
	{
		tag_t id = m_QueryNext++;
		m_Queries[id] = ConstructParabolicQuery(source, minRange, maxRange, elevationBonus, owners, requiredInterface, flags);

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

	virtual std::vector<entity_id_t> ExecuteQueryAroundPos(CFixedVector2D pos,
		entity_pos_t minRange, entity_pos_t maxRange,
		std::vector<int> owners, int requiredInterface)
	{
		Query q = ConstructQuery(INVALID_ENTITY, minRange, maxRange, owners, requiredInterface, GetEntityFlagMask("normal"));
		std::vector<entity_id_t> r;
		PerformQuery(q, r, pos);

		// Return the list sorted by distance from the entity
		std::stable_sort(r.begin(), r.end(), EntityDistanceOrdering(m_EntityData, pos));

		return r;
	}

	virtual std::vector<entity_id_t> ExecuteQuery(entity_id_t source,
		entity_pos_t minRange, entity_pos_t maxRange,
		std::vector<int> owners, int requiredInterface)
	{
		PROFILE("ExecuteQuery");

		Query q = ConstructQuery(source, minRange, maxRange, owners, requiredInterface, GetEntityFlagMask("normal"));

		std::vector<entity_id_t> r;

		CmpPtr<ICmpPosition> cmpSourcePosition(q.source);
		if (!cmpSourcePosition || !cmpSourcePosition->IsInWorld())
		{
			// If the source doesn't have a position, then the result is just the empty list
			return r;
		}

		CFixedVector2D pos = cmpSourcePosition->GetPosition2D();
		PerformQuery(q, r, pos);

		// Return the list sorted by distance from the entity
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

		CmpPtr<ICmpPosition> cmpSourcePosition(q.source);
		if (!cmpSourcePosition || !cmpSourcePosition->IsInWorld())
		{
			// If the source doesn't have a position, then the result is just the empty list
			q.lastMatch = r;
			return r;
		}

		CFixedVector2D pos = cmpSourcePosition->GetPosition2D();
		PerformQuery(q, r, pos);

		q.lastMatch = r;

		// Return the list sorted by distance from the entity
		std::stable_sort(r.begin(), r.end(), EntityDistanceOrdering(m_EntityData, pos));

		return r;
	}

	virtual std::vector<entity_id_t> GetEntitiesByPlayer(player_id_t player)
	{
		std::vector<entity_id_t> entities;

		u32 ownerMask = CalcOwnerMask(player);

		for (EntityMap<EntityData>::const_iterator it = m_EntityData.begin(); it != m_EntityData.end(); ++it)
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
		std::vector<entity_id_t> results;
		std::vector<entity_id_t> added;
		std::vector<entity_id_t> removed;

		for (std::map<tag_t, Query>::iterator it = m_Queries.begin(); it != m_Queries.end(); ++it)
		{
			Query& query = it->second;

			if (!query.enabled)
				continue;

			CmpPtr<ICmpPosition> cmpSourcePosition(query.source);
			if (!cmpSourcePosition || !cmpSourcePosition->IsInWorld())
				continue;

			results.clear();
			results.reserve(query.lastMatch.size());
			CFixedVector2D pos = cmpSourcePosition->GetPosition2D();
			PerformQuery(query, results, pos);

			// Compute the changes vs the last match
			added.clear();
			removed.clear();
			// Return the 'added' list sorted by distance from the entity
			// (Don't bother sorting 'removed' because they might not even have positions or exist any more)
			std::set_difference(results.begin(), results.end(), query.lastMatch.begin(), query.lastMatch.end(),
				std::back_inserter(added));
			std::set_difference(query.lastMatch.begin(), query.lastMatch.end(), results.begin(), results.end(),
				std::back_inserter(removed));
			if (added.empty() && removed.empty())
				continue;

			std::stable_sort(added.begin(), added.end(), EntityDistanceOrdering(m_EntityData, cmpSourcePosition->GetPosition2D()));

			messages.resize(messages.size() + 1);
			std::pair<entity_id_t, CMessageRangeUpdate>& back = messages.back();
			back.first = query.source.GetId();
			back.second.tag = it->first;
			back.second.added.swap(added);
			back.second.removed.swap(removed);
			it->second.lastMatch.swap(results);
		}

		CComponentManager& cmpMgr = GetSimContext().GetComponentManager();
		for (size_t i = 0; i < messages.size(); ++i)
			cmpMgr.PostMessage(messages[i].first, messages[i].second);
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
		if (id == q.source.GetId())
			return false;

		// Ignore if it's missing the required interface
		if (q.interface && !GetSimContext().GetComponentManager().QueryInterface(id, q.interface))
			return false;

		return true;
	}

	/**
	 * Returns a list of distinct entity IDs that match the given query, sorted by ID.
	 */
	void PerformQuery(const Query& q, std::vector<entity_id_t>& r, CFixedVector2D pos)
	{

		// Special case: range -1.0 means check all entities ignoring distance
		if (q.maxRange == entity_pos_t::FromInt(-1))
		{
			for (EntityMap<EntityData>::const_iterator it = m_EntityData.begin(); it != m_EntityData.end(); ++it)
			{
				if (!TestEntityQuery(q, it->first, it->second))
					continue;

				r.push_back(it->first);
			}
		}
		// Not the entire world, so check a parabolic range, or a regular range
		else if (q.parabolic)
		{
			// elevationBonus is part of the 3D position, as the source is really that much heigher
			CmpPtr<ICmpPosition> cmpSourcePosition(q.source);
			CFixedVector3D pos3d = cmpSourcePosition->GetPosition()+
			    CFixedVector3D(entity_pos_t::Zero(), q.elevationBonus, entity_pos_t::Zero()) ;
			// Get a quick list of entities that are potentially in range, with a cutoff of 2*maxRange
			SpatialQueryArray ents;
			m_Subdivision.GetNear(ents, pos, q.maxRange*2);

			for (int i = 0; i < ents.size(); ++i)
			{
				EntityMap<EntityData>::const_iterator it = m_EntityData.find(ents[i]);
				ENSURE(it != m_EntityData.end());

				if (!TestEntityQuery(q, it->first, it->second))
					continue;

				CmpPtr<ICmpPosition> cmpSecondPosition(GetSimContext(), ents[i]);
				if (!cmpSecondPosition || !cmpSecondPosition->IsInWorld())
					continue;
				CFixedVector3D secondPosition = cmpSecondPosition->GetPosition();

				// Restrict based on precise distance
				if (!InParabolicRange(
						CFixedVector3D(it->second.x, secondPosition.Y, it->second.z)
							- pos3d,
						q.maxRange))
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
		// check a regular range (i.e. not the entire world, and not parabolic)
		else
		{
			// Get a quick list of entities that are potentially in range
			SpatialQueryArray ents;
			m_Subdivision.GetNear(ents, pos, q.maxRange);

			for (int i = 0; i < ents.size(); ++i)
			{
				EntityMap<EntityData>::const_iterator it = m_EntityData.find(ents[i]);
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

	virtual entity_pos_t GetElevationAdaptedRange(CFixedVector3D pos, CFixedVector3D rot, entity_pos_t range, entity_pos_t elevationBonus, entity_pos_t angle)
	{
		entity_pos_t r = entity_pos_t::Zero() ;

		pos.Y += elevationBonus;
		entity_pos_t orientation = rot.Y;

		entity_pos_t maxAngle = orientation + angle/2;
		entity_pos_t minAngle = orientation - angle/2;

		int numberOfSteps = 16;

		if (angle == entity_pos_t::Zero())
			numberOfSteps = 1;

		std::vector<entity_pos_t> coords = getParabolicRangeForm(pos, range, range*2, minAngle, maxAngle, numberOfSteps);

		entity_pos_t part =  entity_pos_t::FromInt(numberOfSteps);

		for (int i = 0; i < numberOfSteps; i++)
		{
			r = r + CFixedVector2D(coords[2*i],coords[2*i+1]).Length() / part;
		}

		return r;

	}

	virtual std::vector<entity_pos_t> getParabolicRangeForm(CFixedVector3D pos, entity_pos_t maxRange, entity_pos_t cutoff, entity_pos_t minAngle, entity_pos_t maxAngle, int numberOfSteps)
	{

		// angle = 0 goes in the positive Z direction
		entity_pos_t precision = entity_pos_t::FromInt((int)TERRAIN_TILE_SIZE)/8;

		std::vector<entity_pos_t> r;


		CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
		CmpPtr<ICmpWaterManager> cmpWaterManager(GetSystemEntity());
		entity_pos_t waterLevel = cmpWaterManager->GetWaterLevel(pos.X,pos.Z);
		entity_pos_t thisHeight = pos.Y > waterLevel ? pos.Y : waterLevel;

		if (cmpTerrain)
		{
			for (int i = 0; i < numberOfSteps; i++)
			{
				entity_pos_t angle = minAngle + (maxAngle - minAngle) / numberOfSteps * i;
				entity_pos_t sin;
				entity_pos_t cos;
				entity_pos_t minDistance = entity_pos_t::Zero();
				entity_pos_t maxDistance = cutoff;
				sincos_approx(angle,sin,cos);

				CFixedVector2D minVector = CFixedVector2D(entity_pos_t::Zero(),entity_pos_t::Zero());
				CFixedVector2D maxVector = CFixedVector2D(sin,cos).Multiply(cutoff);
				entity_pos_t targetHeight = cmpTerrain->GetGroundLevel(pos.X+maxVector.X,pos.Z+maxVector.Y);
				// use water level to display range on water
				targetHeight = targetHeight > waterLevel ? targetHeight : waterLevel;

				if (InParabolicRange(CFixedVector3D(maxVector.X,targetHeight-thisHeight,maxVector.Y),maxRange))
				{
					r.push_back(maxVector.X);
					r.push_back(maxVector.Y);
					continue;
				}

				// Loop until vectors come close enough
				while ((maxVector - minVector).CompareLength(precision) > 0)
				{
					// difference still bigger than precision, bisect to get smaller difference
					entity_pos_t newDistance = (minDistance+maxDistance)/entity_pos_t::FromInt(2);

					CFixedVector2D newVector = CFixedVector2D(sin,cos).Multiply(newDistance);

					// get the height of the ground
					targetHeight = cmpTerrain->GetGroundLevel(pos.X+newVector.X,pos.Z+newVector.Y);
					targetHeight = targetHeight > waterLevel ? targetHeight : waterLevel;

					if (InParabolicRange(CFixedVector3D(newVector.X,targetHeight-thisHeight,newVector.Y),maxRange))
					{
						// new vector is in parabolic range, so this is a new minVector
						minVector = newVector;
						minDistance = newDistance;
					}
					else
					{
						// new vector is out parabolic range, so this is a new maxVector
						maxVector = newVector;
						maxDistance = newDistance;
					}

				}
				r.push_back(maxVector.X);
				r.push_back(maxVector.Y);

			}
			r.push_back(r[0]);
			r.push_back(r[1]);

		}
		return r;

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
		q.parabolic = false;
		q.source = GetSimContext().GetComponentManager().LookupEntityHandle(source);
		q.minRange = minRange;
		q.maxRange = maxRange;
		q.elevationBonus = entity_pos_t::Zero();

		q.ownersMask = 0;
		for (size_t i = 0; i < owners.size(); ++i)
			q.ownersMask |= CalcOwnerMask(owners[i]);

		q.interface = requiredInterface;
		q.flagsMask = flagsMask;

		return q;
	}

	Query ConstructParabolicQuery(entity_id_t source,
		entity_pos_t minRange, entity_pos_t maxRange, entity_pos_t elevationBonus,
		const std::vector<int>& owners, int requiredInterface, u8 flagsMask)
	{
		Query q = ConstructQuery(source,minRange,maxRange,owners,requiredInterface,flagsMask);
		q.parabolic = true;
		q.elevationBonus = elevationBonus;
		return q;
	}


	void RenderSubmit(SceneCollector& collector)
	{
		if (!m_DebugOverlayEnabled)
			return;
		static CColor disabledRingColour(1, 0, 0, 1);	// red
		static CColor enabledRingColour(0, 1, 0, 1);	// green
		static CColor subdivColour(0, 0, 1, 1);			// blue
		static CColor rayColour(1, 1, 0, 0.2f);

		if (m_DebugOverlayDirty)
		{
			m_DebugOverlayLines.clear();

			for (std::map<tag_t, Query>::iterator it = m_Queries.begin(); it != m_Queries.end(); ++it)
			{
				Query& q = it->second;

				CmpPtr<ICmpPosition> cmpSourcePosition(q.source);
				if (!cmpSourcePosition || !cmpSourcePosition->IsInWorld())
					continue;
				CFixedVector2D pos = cmpSourcePosition->GetPosition2D();

				// Draw the max range circle
				if (!q.parabolic)
				{
					m_DebugOverlayLines.push_back(SOverlayLine());
					m_DebugOverlayLines.back().m_Color = (q.enabled ? enabledRingColour : disabledRingColour);
					SimRender::ConstructCircleOnGround(GetSimContext(), pos.X.ToFloat(), pos.Y.ToFloat(), q.maxRange.ToFloat(), m_DebugOverlayLines.back(), true);
				}
				else
				{
					// elevation bonus is part of the 3D position. As if the unit is really that much higher
					CFixedVector3D pos = cmpSourcePosition->GetPosition();
					pos.Y += q.elevationBonus;

					std::vector<entity_pos_t> coords;

					// Get the outline from cache if possible
					if (ParabolicRangesOutlines.find(q.source.GetId()) != ParabolicRangesOutlines.end())
					{
						EntityParabolicRangeOutline e = ParabolicRangesOutlines[q.source.GetId()];
						if (e.position == pos && e.range == q.maxRange)
						{
							// outline is cached correctly, use it
							coords = e.outline;
						}
						else
						{
							// outline was cached, but important parameters changed
							// (position, elevation, range)
							// update it
							coords = getParabolicRangeForm(pos,q.maxRange,q.maxRange*2, entity_pos_t::Zero(), entity_pos_t::FromFloat(2.0f*3.14f),70);
							e.outline = coords;
							e.range = q.maxRange;
							e.position = pos;
							ParabolicRangesOutlines[q.source.GetId()] = e;
						}
					}
					else
					{
						// outline wasn't cached (first time you enable the range overlay
						// or you created a new entiy)
						// cache a new outline
						coords = getParabolicRangeForm(pos,q.maxRange,q.maxRange*2, entity_pos_t::Zero(), entity_pos_t::FromFloat(2.0f*3.14f),70);
						EntityParabolicRangeOutline e;
						e.source = q.source.GetId();
						e.range = q.maxRange;
						e.position = pos;
						e.outline = coords;
						ParabolicRangesOutlines[q.source.GetId()] = e;
					}

					CColor thiscolor = q.enabled ? enabledRingColour : disabledRingColour;

					// draw the outline (piece by piece)
					for (size_t i = 3; i < coords.size(); i += 2)
					{
						std::vector<float> c;
						c.push_back((coords[i-3]+pos.X).ToFloat());
						c.push_back((coords[i-2]+pos.Z).ToFloat());
						c.push_back((coords[i-1]+pos.X).ToFloat());
						c.push_back((coords[i]+pos.Z).ToFloat());
						m_DebugOverlayLines.push_back(SOverlayLine());
						m_DebugOverlayLines.back().m_Color = thiscolor;
						SimRender::ConstructLineOnGround(GetSimContext(), c, m_DebugOverlayLines.back(), true);
					}
				}

				// Draw the min range circle
				if (!q.minRange.IsZero())
				{
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

			// render subdivision grid
			float divSize = m_Subdivision.GetDivisionSize().ToFloat();
			int width = m_Subdivision.GetWidth();
			int height = m_Subdivision.GetHeight();
			for (int x = 0; x < width; ++x)
			{
				for (int y = 0; y < height; ++y)
				{
					m_DebugOverlayLines.push_back(SOverlayLine());
					m_DebugOverlayLines.back().m_Color = subdivColour;

					float xpos = x*divSize + divSize/2;
					float zpos = y*divSize + divSize/2;
					SimRender::ConstructSquareOnGround(GetSimContext(), xpos, zpos, divSize, divSize, 0.0f,
						m_DebugOverlayLines.back(), false, 1.0f);
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
		EntityMap<EntityData>::iterator it = m_EntityData.find(ent);

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
			return CLosQuerier(0xFFFFFFFFu, m_LosStateRevealed, m_TerrainVerticesPerSide);
		else
			return CLosQuerier(GetSharedLosMask(player), m_LosState, m_TerrainVerticesPerSide);
	}

	virtual ELosVisibility GetLosVisibility(CEntityHandle ent, player_id_t player, bool forceRetainInFog)
	{
		// (We can't use m_EntityData since this needs to handle LOCAL entities too)

		// Entities not with positions in the world are never visible
		if (ent.GetId() == INVALID_ENTITY)
			return VIS_HIDDEN;
		CmpPtr<ICmpPosition> cmpPosition(ent);
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
		CLosQuerier los(GetSharedLosMask(player), m_LosState, m_TerrainVerticesPerSide);

		if (los.IsVisible(i, j))
			return VIS_VISIBLE;

		// Fogged if the 'retain in fog' flag is set, and in a non-visible explored region
		if (los.IsExplored(i, j))
		{
			CmpPtr<ICmpVision> cmpVision(ent);
			if (forceRetainInFog || (cmpVision && cmpVision->GetRetainInFog()))
				return VIS_FOGGED;
		}

		// Otherwise not visible
		return VIS_HIDDEN;
	}

	virtual ELosVisibility GetLosVisibility(entity_id_t ent, player_id_t player, bool forceRetainInFog)
	{
		CEntityHandle handle = GetSimContext().GetComponentManager().LookupEntityHandle(ent);
		return GetLosVisibility(handle, player, forceRetainInFog);
	}

	i32 PosToLosTilesHelper(entity_pos_t x, entity_pos_t z)
	{
		i32 i = Clamp(
			(x/(entity_pos_t::FromInt(TERRAIN_TILE_SIZE * LOS_TILES_RATIO))).ToInt_RoundToZero(),
			0,
			m_LosTilesPerSide - 1);
		i32 j = Clamp(
			(z/(entity_pos_t::FromInt(TERRAIN_TILE_SIZE * LOS_TILES_RATIO))).ToInt_RoundToZero(),
			0,
			m_LosTilesPerSide - 1);
		return j*m_LosTilesPerSide + i;
	}

	void AddToTile(i32 tile, entity_id_t ent)
	{
		m_LosTiles[tile].insert(ent);
	}

	void RemoveFromTile(i32 tile, entity_id_t ent)
	{
		for (std::set<entity_id_t>::iterator tileIt = m_LosTiles[tile].begin();
			tileIt != m_LosTiles[tile].end();
			++tileIt)
		{
			if (*tileIt == ent)
			{
				m_LosTiles[tile].erase(tileIt);
				return;
			}
		}
	}

	void UpdateVisibilityData()
	{
		PROFILE("UpdateVisibilityData");
		
		for (i32 n = 0; n < m_LosTilesPerSide*m_LosTilesPerSide; ++n)
		{
			if (m_DirtyVisibility[n])
			{
				for (std::set<entity_id_t>::iterator it = m_LosTiles[n].begin();
					it != m_LosTiles[n].end();
					++it)
				{
					UpdateVisibility(*it);
				}
				m_DirtyVisibility[n] = false;
			}
		}

		for (std::vector<entity_id_t>::iterator it = m_ModifiedEntities.begin(); it != m_ModifiedEntities.end(); ++it)
		{
			UpdateVisibility(*it);
		}
		m_ModifiedEntities.clear();
	}

	void UpdateVisibility(entity_id_t ent)
	{
		EntityMap<EntityData>::iterator itEnts = m_EntityData.find(ent);
		if (itEnts == m_EntityData.end())
			return;
		
		for (player_id_t player = 1; player <= MAX_LOS_PLAYER_ID; ++player)
		{
			u8 oldVis = (itEnts->second.visibilities >> (2*player)) & 0x3;
			u8 newVis = GetLosVisibility(itEnts->first, player, false);

			if (oldVis != newVis)
			{
				CMessageVisibilityChanged msg(player, ent, oldVis, newVis);
				GetSimContext().GetComponentManager().PostMessage(ent, msg);
				itEnts->second.visibilities = (itEnts->second.visibilities & ~(0x3 << 2*player)) | (newVis << 2*player);
			}
		}
	}

	virtual void SetLosRevealAll(player_id_t player, bool enabled)
	{
		if (player == -1)
			m_LosRevealAll[MAX_LOS_PLAYER_ID+1] = enabled;
		else
		{
			ENSURE(player >= 0 && player <= MAX_LOS_PLAYER_ID);
			m_LosRevealAll[player] = enabled;
		}
	}

	virtual bool GetLosRevealAll(player_id_t player)
	{
		// Special player value can force reveal-all for every player
		if (m_LosRevealAll[MAX_LOS_PLAYER_ID+1] || player == -1)
			return true;
		ENSURE(player >= 0 && player <= MAX_LOS_PLAYER_ID+1);
		// Otherwise check the player-specific flag
		if (m_LosRevealAll[player])
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

	virtual void SetSharedLos(player_id_t player, std::vector<player_id_t> players)
	{
		m_SharedLosMasks[player] = CalcSharedLosMask(players);
	}

	virtual u32 GetSharedLosMask(player_id_t player)
	{
		return m_SharedLosMasks[player];
	}

	void ExploreAllTiles(player_id_t p)
	{
		for (u16 j = 0; j < m_TerrainVerticesPerSide; ++j)
		{
			for (u16 i = 0; i < m_TerrainVerticesPerSide; ++i)
			{
				if (LosIsOffWorld(i,j))
					continue;
				u32 &explored = m_ExploredVertices.at(p);
				explored += !(m_LosState[i + j*m_TerrainVerticesPerSide] & (LOS_EXPLORED << (2*(p-1))));
				m_LosState[i + j*m_TerrainVerticesPerSide] |= (LOS_EXPLORED << (2*(p-1)));
			}
		}
	}

	void UpdateTerritoriesLos()
	{
		CmpPtr<ICmpTerritoryManager> cmpTerritoryManager(GetSystemEntity());
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
					u32 &explored = m_ExploredVertices.at(p);
					explored += !(m_LosState[i + j*m_TerrainVerticesPerSide] & (LOS_EXPLORED << (2*(p-1))));
					m_LosState[i + j*m_TerrainVerticesPerSide] |= (LOS_EXPLORED << (2*(p-1)));
					explored += !(m_LosState[i+1 + j*m_TerrainVerticesPerSide] & (LOS_EXPLORED << (2*(p-1))));
					m_LosState[i+1 + j*m_TerrainVerticesPerSide] |= (LOS_EXPLORED << (2*(p-1)));
					explored += !(m_LosState[i + (j+1)*m_TerrainVerticesPerSide] & (LOS_EXPLORED << (2*(p-1))));
					m_LosState[i + (j+1)*m_TerrainVerticesPerSide] |= (LOS_EXPLORED << (2*(p-1)));
					explored += !(m_LosState[i+1 + (j+1)*m_TerrainVerticesPerSide] & (LOS_EXPLORED << (2*(p-1))));
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
		u32 &explored = m_ExploredVertices.at(owner);
		for (i32 idx = idx0; idx <= idx1; ++idx)
		{
			// Increasing from zero to non-zero - move from unexplored/explored to visible+explored
			if (counts[idx] == 0)
			{
				i32 i = i0 + idx - idx0;
				if (!LosIsOffWorld(i, j))
				{
					explored += !(m_LosState[idx] & (LOS_EXPLORED << (2*(owner-1))));
					m_LosState[idx] |= ((LOS_VISIBLE | LOS_EXPLORED) << (2*(owner-1)));
				}
				m_DirtyVisibility[(j/LOS_TILES_RATIO)*m_LosTilesPerSide + i/LOS_TILES_RATIO] = true;
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

				i32 i = i0 + idx - idx0;
				m_DirtyVisibility[(j/LOS_TILES_RATIO)*m_LosTilesPerSide + i/LOS_TILES_RATIO] = true;
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

	virtual u8 GetPercentMapExplored(player_id_t player)
	{
		return m_ExploredVertices.at((u8)player) * 100 / m_TotalInworldVertices;
	}
};

REGISTER_COMPONENT_TYPE(RangeManager)
