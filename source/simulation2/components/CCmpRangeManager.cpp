/* Copyright (C) 2020 Wildfire Games.
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
#include "simulation2/components/ICmpFogging.h"
#include "simulation2/components/ICmpMirage.h"
#include "simulation2/components/ICmpOwnership.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/components/ICmpTerritoryManager.h"
#include "simulation2/components/ICmpVisibility.h"
#include "simulation2/components/ICmpVision.h"
#include "simulation2/components/ICmpWaterManager.h"
#include "simulation2/helpers/MapEdgeTiles.h"
#include "simulation2/helpers/Render.h"
#include "simulation2/helpers/Spatial.h"

#include "graphics/Overlay.h"
#include "graphics/Terrain.h"
#include "lib/timer.h"
#include "ps/CLogger.h"
#include "ps/Profile.h"
#include "renderer/Scene.h"

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
 * Add/remove a player to/from mask, which is a 1-bit mask representing a list of players.
 * Returns true if the mask is modified.
 */
static bool SetPlayerSharedDirtyVisibilityBit(u16& mask, player_id_t player, bool enable)
{
	if (player <= 0 || player > 16)
		return false;

	u16 oldMask = mask;

	if (enable)
		mask |= (0x1 << (player - 1));
	else
		mask &= ~(0x1 << (player - 1));

	return oldMask != mask;
}

/**
 * Computes the 2-bit visibility for one player, given the total 32-bit visibilities
 */
static inline u8 GetPlayerVisibility(u32 visibilities, player_id_t player)
{
	if (player > 0 && player <= 16)
		return (visibilities >> (2 *(player-1))) & 0x3;
	return 0;
}

/**
 * Test whether the visibility is dirty for a given LoS tile and a given player
 */
static inline bool IsVisibilityDirty(u16 dirty, player_id_t player)
{
	if (player > 0 && player <= 16)
		return (dirty >> (player - 1)) & 0x1;
	return false;
}

/**
 * Test whether a player share this vision
 */
static inline bool HasVisionSharing(u16 visionSharing, player_id_t player)
{
	return visionSharing & 1 << (player-1);
}

/**
 * Computes the shared vision mask for the player
 */
static inline u16 CalcVisionSharingMask(player_id_t player)
{
	return 1 << (player-1);
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
	u64 xx = SQUARE_U64_FIXED(v.X); // xx <= 2^62
	u64 zz = SQUARE_U64_FIXED(v.Z);
	i64 d2 = (xx + zz) >> 1; // d2 <= 2^62 (no overflow)

	i32 y = v.Y.GetInternalValue();
	i32 c = range.GetInternalValue();
	i32 c_2 = c >> 1;

	i64 c2 = MUL_I64_I32_I32(c_2 - y, c);

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
enum FlagMasks
{
	// flags used for queries
	None = 0x00,
	Normal = 0x01,
	Injured = 0x02,
	AllQuery = Normal | Injured,

	// 0x04 reserved for future use

	// general flags
	InWorld = 0x08,
	RetainInFog = 0x10,
	RevealShore = 0x20,
	ScriptedVisibility = 0x40,
	SharedVision = 0x80
};

struct EntityData
{
	EntityData() :
		visibilities(0), size(0), visionSharing(0),
		owner(-1), flags(FlagMasks::Normal)
		{ }
	entity_pos_t x, z;
	entity_pos_t visionRange;
	u32 visibilities; // 2-bit visibility, per player
	u32 size;
	u16 visionSharing; // 1-bit per player
	i8 owner;
	u8 flags; // See the FlagMasks enum

	template<int mask>
	inline bool HasFlag() const { return (flags & mask) != 0; }

	template<int mask>
	inline void SetFlag(bool val) { flags = val ? (flags | mask) : (flags & ~mask); }

	inline void SetFlag(u8 mask, bool val) { flags = val ? (flags | mask) : (flags & ~mask); }
};

cassert(sizeof(EntityData) == 24);

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
		serialize.NumberU32_Unbounded("visibilities", value.visibilities);
		serialize.NumberU32_Unbounded("size", value.size);
		serialize.NumberU16_Unbounded("vision sharing", value.visionSharing);
		serialize.NumberI8_Unbounded("owner", value.owner);
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

	bool operator()(entity_id_t a, entity_id_t b) const
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
		componentManager.SubscribeGloballyToMessageType(MT_VisionSharingChanged);

		componentManager.SubscribeToMessageType(MT_Deserialized);
		componentManager.SubscribeToMessageType(MT_Update);
		componentManager.SubscribeToMessageType(MT_RenderSubmit); // for debug overlays
	}

	DEFAULT_COMPONENT_ALLOCATOR(RangeManager)

	bool m_DebugOverlayEnabled;
	bool m_DebugOverlayDirty;
	std::vector<SOverlayLine> m_DebugOverlayLines;

	// Deserialization flag. A lot of different functions are called by Deserialize()
	// and we don't want to pass isDeserializing bool arguments to all of them...
	bool m_Deserializing;

	// World bounds (entities are expected to be within this range)
	entity_pos_t m_WorldX0;
	entity_pos_t m_WorldZ0;
	entity_pos_t m_WorldX1;
	entity_pos_t m_WorldZ1;

	// Range query state:
	tag_t m_QueryNext; // next allocated id
	std::map<tag_t, Query> m_Queries;
	EntityMap<EntityData> m_EntityData;

	FastSpatialSubdivision m_Subdivision; // spatial index of m_EntityData
	std::vector<entity_id_t> m_SubdivisionResults;

	// LOS state:
	static const player_id_t MAX_LOS_PLAYER_ID = 16;

	std::vector<bool> m_LosRevealAll;
	bool m_LosCircular;
	i32 m_TerrainVerticesPerSide;

	// Cache for visibility tracking
	i32 m_LosTilesPerSide;
	bool m_GlobalVisibilityUpdate;
	std::vector<u8> m_GlobalPlayerVisibilityUpdate;
	std::vector<u16> m_DirtyVisibility;
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
	// Shared dirty visibility masks, one per player.
	std::vector<u16> m_SharedDirtyVisibilityMasks;

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

		m_Deserializing = false;
		m_WorldX0 = m_WorldZ0 = m_WorldX1 = m_WorldZ1 = entity_pos_t::Zero();

		// Initialise with bogus values (these will get replaced when
		// SetBounds is called)
		ResetSubdivisions(entity_pos_t::FromInt(1024), entity_pos_t::FromInt(1024));

		m_SubdivisionResults.reserve(4096);

		// The whole map should be visible to Gaia by default, else e.g. animals
		// will get confused when trying to run from enemies
		m_LosRevealAll.resize(MAX_LOS_PLAYER_ID+2,false);
		m_LosRevealAll[0] = true;
		m_SharedLosMasks.resize(MAX_LOS_PLAYER_ID+2,0);
		m_SharedDirtyVisibilityMasks.resize(MAX_LOS_PLAYER_ID + 2, 0);

		m_GlobalVisibilityUpdate = true;
		m_GlobalPlayerVisibilityUpdate.resize(MAX_LOS_PLAYER_ID);

		m_LosCircular = false;
		m_TerrainVerticesPerSide = 0;
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
		SerializeMap<SerializeU32_Unbounded, SerializeQuery>()(serialize, "queries", m_Queries, GetSimContext());
		SerializeEntityMap<SerializeEntityData>()(serialize, "entity data", m_EntityData);

		SerializeVector<SerializeBool>()(serialize, "los reveal all", m_LosRevealAll);
		serialize.Bool("los circular", m_LosCircular);
		serialize.NumberI32_Unbounded("terrain verts per side", m_TerrainVerticesPerSide);

		serialize.Bool("global visibility update", m_GlobalVisibilityUpdate);
		SerializeVector<SerializeU8_Unbounded>()(serialize, "global player visibility update", m_GlobalPlayerVisibilityUpdate);
		SerializeRepetitiveVector<SerializeU16_Unbounded>()(serialize, "dirty visibility", m_DirtyVisibility);
		SerializeVector<SerializeU32_Unbounded>()(serialize, "modified entities", m_ModifiedEntities);

		// We don't serialize m_Subdivision, m_LosPlayerCounts or m_LosTiles
		// since they can be recomputed from the entity data when deserializing;
		// m_LosState must be serialized since it depends on the history of exploration

		SerializeRepetitiveVector<SerializeU32_Unbounded>()(serialize, "los state", m_LosState);
		SerializeVector<SerializeU32_Unbounded>()(serialize, "shared los masks", m_SharedLosMasks);
		SerializeVector<SerializeU16_Unbounded>()(serialize, "shared dirty visibility masks", m_SharedDirtyVisibilityMasks);
	}

	virtual void Serialize(ISerializer& serialize)
	{
		SerializeCommon(serialize);
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(paramNode);

		SerializeCommon(deserialize);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_Deserialized:
		{
			// Reinitialize subdivisions and LOS data after all
			// other components have been deserialized.
			m_Deserializing = true;
			ResetDerivedData();
			m_Deserializing = false;
			break;
		}
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
				entdata.SetFlag<FlagMasks::RevealShore>(cmpVision->GetRevealShore());
			}
			CmpPtr<ICmpVisibility> cmpVisibility(GetSimContext(), ent);
			if (cmpVisibility)
				entdata.SetFlag<FlagMasks::RetainInFog>(cmpVisibility->GetRetainInFog());

			// Store the size
			CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), ent);
			if (cmpObstruction)
				entdata.size = cmpObstruction->GetSize().ToInt_RoundToInfinity();

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
				if (it->second.HasFlag<FlagMasks::InWorld>())
				{
					CFixedVector2D from(it->second.x, it->second.z);
					CFixedVector2D to(msgData.x, msgData.z);
					m_Subdivision.Move(ent, from, to, it->second.size);
					if (it->second.HasFlag<FlagMasks::SharedVision>())
						SharingLosMove(it->second.visionSharing, it->second.visionRange, from, to);
					else
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
					m_Subdivision.Add(ent, to, it->second.size);
					if (it->second.HasFlag<FlagMasks::SharedVision>())
						SharingLosAdd(it->second.visionSharing, it->second.visionRange, to);
					else
						LosAdd(it->second.owner, it->second.visionRange, to);
					AddToTile(PosToLosTilesHelper(msgData.x, msgData.z), ent);
				}

				it->second.SetFlag<FlagMasks::InWorld>(true);
				it->second.x = msgData.x;
				it->second.z = msgData.z;
			}
			else
			{
				if (it->second.HasFlag<FlagMasks::InWorld>())
				{
					CFixedVector2D from(it->second.x, it->second.z);
					m_Subdivision.Remove(ent, from, it->second.size);
					if (it->second.HasFlag<FlagMasks::SharedVision>())
						SharingLosRemove(it->second.visionSharing, it->second.visionRange, from);
					else
						LosRemove(it->second.owner, it->second.visionRange, from);
					RemoveFromTile(PosToLosTilesHelper(it->second.x, it->second.z), ent);
				}

				it->second.SetFlag<FlagMasks::InWorld>(false);
				it->second.x = entity_pos_t::Zero();
				it->second.z = entity_pos_t::Zero();
			}

			RequestVisibilityUpdate(ent);

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

			if (it->second.HasFlag<FlagMasks::InWorld>())
			{
				// Entity vision is taken into account in VisionSharingChanged
				// when sharing component activated
				if (!it->second.HasFlag<FlagMasks::SharedVision>())
				{
					CFixedVector2D pos(it->second.x, it->second.z);
					LosRemove(it->second.owner, it->second.visionRange, pos);
					LosAdd(msgData.to, it->second.visionRange, pos);
				}

				if (it->second.HasFlag<FlagMasks::RevealShore>())
				{
					RevealShore(it->second.owner, false);
					RevealShore(msgData.to, true);
				}
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

			if (it->second.HasFlag<FlagMasks::InWorld>())
			{
				m_Subdivision.Remove(ent, CFixedVector2D(it->second.x, it->second.z), it->second.size);
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

			it->second.visionRange = newRange;

			if (it->second.HasFlag<FlagMasks::InWorld>())
			{
				CFixedVector2D pos(it->second.x, it->second.z);
				if (it->second.HasFlag<FlagMasks::SharedVision>())
				{
					SharingLosRemove(it->second.visionSharing, oldRange, pos);
					SharingLosAdd(it->second.visionSharing, newRange, pos);
				}
				else
				{
					LosRemove(it->second.owner, oldRange, pos);
					LosAdd(it->second.owner, newRange, pos);
				}
			}

			break;
		}
		case MT_VisionSharingChanged:
		{
			const CMessageVisionSharingChanged& msgData = static_cast<const CMessageVisionSharingChanged&> (msg);
			entity_id_t ent = msgData.entity;

			EntityMap<EntityData>::iterator it = m_EntityData.find(ent);

			// Ignore if we're not already tracking this entity
			if (it == m_EntityData.end())
				break;

			ENSURE(msgData.player > 0 && msgData.player < MAX_LOS_PLAYER_ID+1);
			u16 visionChanged = CalcVisionSharingMask(msgData.player);

			if (!it->second.HasFlag<FlagMasks::SharedVision>())
			{
				// Activation of the Vision Sharing
				ENSURE(it->second.owner == (i8)msgData.player);
				it->second.visionSharing = visionChanged;
				it->second.SetFlag<FlagMasks::SharedVision>(true);
				break;
			}

			if (it->second.HasFlag<FlagMasks::InWorld>())
			{
				entity_pos_t range = it->second.visionRange;
				CFixedVector2D pos(it->second.x, it->second.z);
				if (msgData.add)
					LosAdd(msgData.player, range, pos);
				else
					LosRemove(msgData.player, range, pos);
			}

			if (msgData.add)
				it->second.visionSharing |= visionChanged;
			else
				it->second.visionSharing &= ~visionChanged;
			break;
		}
		case MT_Update:
		{
			m_DebugOverlayDirty = true;
			ExecuteActiveQueries();
			UpdateVisibilityData();
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

		ResetDerivedData();
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
		FastSpatialSubdivision oldSubdivision = m_Subdivision;
		std::vector<std::set<entity_id_t> > oldLosTiles = m_LosTiles;

		m_Deserializing = true;
		ResetDerivedData();
		m_Deserializing = false;

		if (oldPlayerCounts != m_LosPlayerCounts)
		{
			for (size_t i = 0; i < oldPlayerCounts.size(); ++i)
			{
				debug_printf("%d: ", (int)i);
				for (size_t j = 0; j < oldPlayerCounts[i].size(); ++j)
					debug_printf("%d ", oldPlayerCounts[i][j]);
				debug_printf("\n");
			}
			for (size_t i = 0; i < m_LosPlayerCounts.size(); ++i)
			{
				debug_printf("%d: ", (int)i);
				for (size_t j = 0; j < m_LosPlayerCounts[i].size(); ++j)
					debug_printf("%d ", m_LosPlayerCounts[i][j]);
				debug_printf("\n");
			}
			debug_warn(L"inconsistent player counts");
		}
		if (oldStateRevealed != m_LosStateRevealed)
			debug_warn(L"inconsistent revealed");
		if (oldSubdivision != m_Subdivision)
			debug_warn(L"inconsistent subdivs");
		if (oldLosTiles != m_LosTiles)
			debug_warn(L"inconsistent los tiles");
	}

	FastSpatialSubdivision* GetSubdivision()
	{
		return &m_Subdivision;
	}

	// Reinitialise subdivisions and LOS data, based on entity data
	void ResetDerivedData()
	{
		ENSURE(m_WorldX0.IsZero() && m_WorldZ0.IsZero()); // don't bother implementing non-zero offsets yet
		ResetSubdivisions(m_WorldX1, m_WorldZ1);

		m_LosTilesPerSide = (m_TerrainVerticesPerSide - 1)/LOS_TILES_RATIO;

		m_LosPlayerCounts.clear();
		m_LosPlayerCounts.resize(MAX_LOS_PLAYER_ID+1);
		m_ExploredVertices.clear();
		m_ExploredVertices.resize(MAX_LOS_PLAYER_ID+1, 0);
		if (m_Deserializing)
		{
			// recalc current exploration stats.
			for (i32 j = 0; j < m_TerrainVerticesPerSide; j++)
				for (i32 i = 0; i < m_TerrainVerticesPerSide; i++)
					if (!LosIsOffWorld(i, j))
						for (u8 k = 1; k < MAX_LOS_PLAYER_ID+1; ++k)
							m_ExploredVertices.at(k) += ((m_LosState[j*m_TerrainVerticesPerSide + i] & (LOS_EXPLORED << (2*(k-1)))) > 0);
		}
		else
		{
			m_LosState.clear();
			m_LosState.resize(m_TerrainVerticesPerSide*m_TerrainVerticesPerSide);
		}
		m_LosStateRevealed.clear();
		m_LosStateRevealed.resize(m_TerrainVerticesPerSide*m_TerrainVerticesPerSide);

		if (!m_Deserializing)
		{
			m_DirtyVisibility.clear();
			m_DirtyVisibility.resize(m_LosTilesPerSide*m_LosTilesPerSide);
		}
		ENSURE(m_DirtyVisibility.size() == (size_t)(m_LosTilesPerSide*m_LosTilesPerSide));

		m_LosTiles.clear();
		m_LosTiles.resize(m_LosTilesPerSide*m_LosTilesPerSide);

		for (EntityMap<EntityData>::const_iterator it = m_EntityData.begin(); it != m_EntityData.end(); ++it)
			if (it->second.HasFlag<FlagMasks::InWorld>())
			{
				if (it->second.HasFlag<FlagMasks::SharedVision>())
					SharingLosAdd(it->second.visionSharing, it->second.visionRange, CFixedVector2D(it->second.x, it->second.z));
				else
					LosAdd(it->second.owner, it->second.visionRange, CFixedVector2D(it->second.x, it->second.z));
				AddToTile(PosToLosTilesHelper(it->second.x, it->second.z), it->first);

				if (it->second.HasFlag<FlagMasks::RevealShore>())
					RevealShore(it->second.owner, true);
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
		m_Subdivision.Reset(x1, z1);

		for (EntityMap<EntityData>::const_iterator it = m_EntityData.begin(); it != m_EntityData.end(); ++it)
			if (it->second.HasFlag<FlagMasks::InWorld>())
				m_Subdivision.Add(it->first, CFixedVector2D(it->second.x, it->second.z), it->second.size);
	}

	virtual tag_t CreateActiveQuery(entity_id_t source,
		entity_pos_t minRange, entity_pos_t maxRange,
		const std::vector<int>& owners, int requiredInterface, u8 flags)
	{
		tag_t id = m_QueryNext++;
		m_Queries[id] = ConstructQuery(source, minRange, maxRange, owners, requiredInterface, flags);

		return id;
	}

	virtual tag_t CreateActiveParabolicQuery(entity_id_t source,
		entity_pos_t minRange, entity_pos_t maxRange, entity_pos_t elevationBonus,
		const std::vector<int>& owners, int requiredInterface, u8 flags)
	{
		tag_t id = m_QueryNext++;
		m_Queries[id] = ConstructParabolicQuery(source, minRange, maxRange, elevationBonus, owners, requiredInterface, flags);

		return id;
	}

	virtual void DestroyActiveQuery(tag_t tag)
	{
		if (m_Queries.find(tag) == m_Queries.end())
		{
			LOGERROR("CCmpRangeManager: DestroyActiveQuery called with invalid tag %u", tag);
			return;
		}

		m_Queries.erase(tag);
	}

	virtual void EnableActiveQuery(tag_t tag)
	{
		std::map<tag_t, Query>::iterator it = m_Queries.find(tag);
		if (it == m_Queries.end())
		{
			LOGERROR("CCmpRangeManager: EnableActiveQuery called with invalid tag %u", tag);
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
			LOGERROR("CCmpRangeManager: DisableActiveQuery called with invalid tag %u", tag);
			return;
		}

		Query& q = it->second;
		q.enabled = false;
	}

	virtual bool IsActiveQueryEnabled(tag_t tag) const
	{
		std::map<tag_t, Query>::const_iterator it = m_Queries.find(tag);
		if (it == m_Queries.end())
		{
			LOGERROR("CCmpRangeManager: IsActiveQueryEnabled called with invalid tag %u", tag);
			return false;
		}

		const Query& q = it->second;
		return q.enabled;
	}

	virtual std::vector<entity_id_t> ExecuteQueryAroundPos(const CFixedVector2D& pos,
		entity_pos_t minRange, entity_pos_t maxRange,
		const std::vector<int>& owners, int requiredInterface)
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
		const std::vector<int>& owners, int requiredInterface)
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
			LOGERROR("CCmpRangeManager: ResetActiveQuery called with invalid tag %u", tag);
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

	virtual std::vector<entity_id_t> GetEntitiesByPlayer(player_id_t player) const
	{
		return GetEntitiesByMask(CalcOwnerMask(player));
	}

	virtual std::vector<entity_id_t> GetNonGaiaEntities() const
	{
		return GetEntitiesByMask(~3); // bit 0 for owner=-1 and bit 1 for gaia
	}

	virtual std::vector<entity_id_t> GetGaiaAndNonGaiaEntities() const
	{
		return GetEntitiesByMask(~1); // bit 0 for owner=-1
	}

	std::vector<entity_id_t> GetEntitiesByMask(u32 ownerMask) const
	{
		std::vector<entity_id_t> entities;

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

			results.clear();
			CmpPtr<ICmpPosition> cmpSourcePosition(query.source);
			if (cmpSourcePosition && cmpSourcePosition->IsInWorld())
			{
				results.reserve(query.lastMatch.size());
				PerformQuery(query, results, cmpSourcePosition->GetPosition2D());
			}

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

			if (cmpSourcePosition && cmpSourcePosition->IsInWorld())
				std::stable_sort(added.begin(), added.end(), EntityDistanceOrdering(m_EntityData, cmpSourcePosition->GetPosition2D()));

			messages.resize(messages.size() + 1);
			std::pair<entity_id_t, CMessageRangeUpdate>& back = messages.back();
			back.first = query.source.GetId();
			back.second.tag = it->first;
			back.second.added.swap(added);
			back.second.removed.swap(removed);
			query.lastMatch.swap(results);
		}

		CComponentManager& cmpMgr = GetSimContext().GetComponentManager();
		for (size_t i = 0; i < messages.size(); ++i)
			cmpMgr.PostMessage(messages[i].first, messages[i].second);
	}

	/**
	 * Returns whether the given entity matches the given query (ignoring maxRange)
	 */
	bool TestEntityQuery(const Query& q, entity_id_t id, const EntityData& entity) const
	{
		// Quick filter to ignore entities with the wrong owner
		if (!(CalcOwnerMask(entity.owner) & q.ownersMask))
			return false;

		// Ignore entities not present in the world
		if (!entity.HasFlag<FlagMasks::InWorld>())
			return false;

		// Ignore entities that don't match the current flags
		if (!((entity.flags & FlagMasks::AllQuery) & q.flagsMask))
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
			m_SubdivisionResults.clear();
			m_Subdivision.GetNear(m_SubdivisionResults, pos, q.maxRange * 2);

			for (size_t i = 0; i < m_SubdivisionResults.size(); ++i)
			{
				EntityMap<EntityData>::const_iterator it = m_EntityData.find(m_SubdivisionResults[i]);
				ENSURE(it != m_EntityData.end());

				if (!TestEntityQuery(q, it->first, it->second))
					continue;

				CmpPtr<ICmpPosition> cmpSecondPosition(GetSimContext(), m_SubdivisionResults[i]);
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
			std::sort(r.begin(), r.end());
		}
		// check a regular range (i.e. not the entire world, and not parabolic)
		else
		{
			// Get a quick list of entities that are potentially in range
			m_SubdivisionResults.clear();
			m_Subdivision.GetNear(m_SubdivisionResults, pos, q.maxRange);

			for (size_t i = 0; i < m_SubdivisionResults.size(); ++i)
			{
				EntityMap<EntityData>::const_iterator it = m_EntityData.find(m_SubdivisionResults[i]);
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
			std::sort(r.begin(), r.end());
		}
	}

	virtual entity_pos_t GetElevationAdaptedRange(const CFixedVector3D& pos1, const CFixedVector3D& rot, entity_pos_t range, entity_pos_t elevationBonus, entity_pos_t angle) const
	{
		entity_pos_t r = entity_pos_t::Zero();
		CFixedVector3D pos(pos1);

		pos.Y += elevationBonus;
		entity_pos_t orientation = rot.Y;

		entity_pos_t maxAngle = orientation + angle/2;
		entity_pos_t minAngle = orientation - angle/2;

		int numberOfSteps = 16;

		if (angle == entity_pos_t::Zero())
			numberOfSteps = 1;

		std::vector<entity_pos_t> coords = getParabolicRangeForm(pos, range, range*2, minAngle, maxAngle, numberOfSteps);

		entity_pos_t part = entity_pos_t::FromInt(numberOfSteps);

		for (int i = 0; i < numberOfSteps; ++i)
			r = r + CFixedVector2D(coords[2*i],coords[2*i+1]).Length() / part;

		return r;

	}

	virtual std::vector<entity_pos_t> getParabolicRangeForm(CFixedVector3D pos, entity_pos_t maxRange, entity_pos_t cutoff, entity_pos_t minAngle, entity_pos_t maxAngle, int numberOfSteps) const
	{
		std::vector<entity_pos_t> r;

		CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
		if (!cmpTerrain)
			return r;

		// angle = 0 goes in the positive Z direction
		u64 precisionSquared = SQUARE_U64_FIXED(entity_pos_t::FromInt(static_cast<int>(TERRAIN_TILE_SIZE)) / 8);

		CmpPtr<ICmpWaterManager> cmpWaterManager(GetSystemEntity());
		entity_pos_t waterLevel = cmpWaterManager ? cmpWaterManager->GetWaterLevel(pos.X, pos.Z) : entity_pos_t::Zero();
		entity_pos_t thisHeight = pos.Y > waterLevel ? pos.Y : waterLevel;

		for (int i = 0; i < numberOfSteps; ++i)
		{
			entity_pos_t angle = minAngle + (maxAngle - minAngle) / numberOfSteps * i;
			entity_pos_t sin;
			entity_pos_t cos;
			entity_pos_t minDistance = entity_pos_t::Zero();
			entity_pos_t maxDistance = cutoff;
			sincos_approx(angle, sin, cos);

			CFixedVector2D minVector = CFixedVector2D(entity_pos_t::Zero(), entity_pos_t::Zero());
			CFixedVector2D maxVector = CFixedVector2D(sin, cos).Multiply(cutoff);
			entity_pos_t targetHeight = cmpTerrain->GetGroundLevel(pos.X+maxVector.X, pos.Z+maxVector.Y);
			// use water level to display range on water
			targetHeight = targetHeight > waterLevel ? targetHeight : waterLevel;

			if (InParabolicRange(CFixedVector3D(maxVector.X, targetHeight-thisHeight, maxVector.Y), maxRange))
			{
				r.push_back(maxVector.X);
				r.push_back(maxVector.Y);
				continue;
			}

			// Loop until vectors come close enough
			while ((maxVector - minVector).CompareLengthSquared(precisionSquared) > 0)
			{
				// difference still bigger than precision, bisect to get smaller difference
				entity_pos_t newDistance = (minDistance+maxDistance)/entity_pos_t::FromInt(2);

				CFixedVector2D newVector = CFixedVector2D(sin, cos).Multiply(newDistance);

				// get the height of the ground
				targetHeight = cmpTerrain->GetGroundLevel(pos.X+newVector.X, pos.Z+newVector.Y);
				targetHeight = targetHeight > waterLevel ? targetHeight : waterLevel;

				if (InParabolicRange(CFixedVector3D(newVector.X, targetHeight-thisHeight, newVector.Y), maxRange))
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

		return r;
	}

	Query ConstructQuery(entity_id_t source,
		entity_pos_t minRange, entity_pos_t maxRange,
		const std::vector<int>& owners, int requiredInterface, u8 flagsMask) const
	{
		// Min range must be non-negative
		if (minRange < entity_pos_t::Zero())
			LOGWARNING("CCmpRangeManager: Invalid min range %f in query for entity %u", minRange.ToDouble(), source);

		// Max range must be non-negative, or else -1
		if (maxRange < entity_pos_t::Zero() && maxRange != entity_pos_t::FromInt(-1))
			LOGWARNING("CCmpRangeManager: Invalid max range %f in query for entity %u", maxRange.ToDouble(), source);

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

		if (q.ownersMask == 0)
			LOGWARNING("CCmpRangeManager: No owners in query for entity %u", source);

		q.interface = requiredInterface;
		q.flagsMask = flagsMask;

		return q;
	}

	Query ConstructParabolicQuery(entity_id_t source,
		entity_pos_t minRange, entity_pos_t maxRange, entity_pos_t elevationBonus,
		const std::vector<int>& owners, int requiredInterface, u8 flagsMask) const
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
		static CColor disabledRingColor(1, 0, 0, 1);	// red
		static CColor enabledRingColor(0, 1, 0, 1);	// green
		static CColor subdivColor(0, 0, 1, 1);			// blue
		static CColor rayColor(1, 1, 0, 0.2f);

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
					m_DebugOverlayLines.back().m_Color = (q.enabled ? enabledRingColor : disabledRingColor);
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

					CColor thiscolor = q.enabled ? enabledRingColor : disabledRingColor;

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
					SimRender::ConstructCircleOnGround(GetSimContext(), pos.X.ToFloat(), pos.Y.ToFloat(), q.minRange.ToFloat(), m_DebugOverlayLines.back(), true);

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
					m_DebugOverlayLines.back().m_Color = rayColor;
					SimRender::ConstructLineOnGround(GetSimContext(), coords, m_DebugOverlayLines.back(), true);
				}
			}

			// render subdivision grid
			float divSize = m_Subdivision.GetDivisionSize();
			int size = m_Subdivision.GetWidth();
			for (int x = 0; x < size; ++x)
			{
				for (int y = 0; y < size; ++y)
				{
					m_DebugOverlayLines.push_back(SOverlayLine());
					m_DebugOverlayLines.back().m_Color = subdivColor;

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

	virtual u8 GetEntityFlagMask(const std::string& identifier) const
	{
		if (identifier == "normal")
			return FlagMasks::Normal;
		if (identifier == "injured")
			return FlagMasks::Injured;

		LOGWARNING("CCmpRangeManager: Invalid flag identifier %s", identifier.c_str());
		return FlagMasks::None;
	}

	virtual void SetEntityFlag(entity_id_t ent, const std::string& identifier, bool value)
	{
		EntityMap<EntityData>::iterator it = m_EntityData.find(ent);

		// We don't have this entity
		if (it == m_EntityData.end())
			return;

		u8 flag = GetEntityFlagMask(identifier);

		if (flag == FlagMasks::None)
			LOGWARNING("CCmpRangeManager: Invalid flag identifier %s for entity %u", identifier.c_str(), ent);
		else
			it->second.SetFlag(flag, value);
	}

	// ****************************************************************

	// LOS implementation:

	virtual CLosQuerier GetLosQuerier(player_id_t player) const
	{
		if (GetLosRevealAll(player))
			return CLosQuerier(0xFFFFFFFFu, m_LosStateRevealed, m_TerrainVerticesPerSide);
		else
			return CLosQuerier(GetSharedLosMask(player), m_LosState, m_TerrainVerticesPerSide);
	}

	virtual void ActivateScriptedVisibility(entity_id_t ent, bool status)
	{
		EntityMap<EntityData>::iterator it = m_EntityData.find(ent);
		if (it != m_EntityData.end())
			it->second.SetFlag<FlagMasks::ScriptedVisibility>(status);
	}

	ELosVisibility ComputeLosVisibility(CEntityHandle ent, player_id_t player) const
	{
		// Entities not with positions in the world are never visible
		if (ent.GetId() == INVALID_ENTITY)
			return VIS_HIDDEN;
		CmpPtr<ICmpPosition> cmpPosition(ent);
		if (!cmpPosition || !cmpPosition->IsInWorld())
			return VIS_HIDDEN;

		// Mirage entities, whatever the situation, are visible for one specific player
		CmpPtr<ICmpMirage> cmpMirage(ent);
		if (cmpMirage && cmpMirage->GetPlayer() != player)
			return VIS_HIDDEN;

		CFixedVector2D pos = cmpPosition->GetPosition2D();
		int i = (pos.X / (int)TERRAIN_TILE_SIZE).ToInt_RoundToNearest();
		int j = (pos.Y / (int)TERRAIN_TILE_SIZE).ToInt_RoundToNearest();

		// Reveal flag makes all positioned entities visible and all mirages useless
		if (GetLosRevealAll(player))
		{
			if (LosIsOffWorld(i, j) || cmpMirage)
				return VIS_HIDDEN;
			else
				return VIS_VISIBLE;
		}

		// Get visible regions
		CLosQuerier los(GetSharedLosMask(player), m_LosState, m_TerrainVerticesPerSide);

		CmpPtr<ICmpVisibility> cmpVisibility(ent);

		// Possibly ask the scripted Visibility component
		EntityMap<EntityData>::const_iterator it = m_EntityData.find(ent.GetId());
		if (it != m_EntityData.end())
		{
			if (it->second.HasFlag<FlagMasks::ScriptedVisibility>() && cmpVisibility)
				return cmpVisibility->GetVisibility(player, los.IsVisible(i, j), los.IsExplored(i, j));
		}
		else
		{
			if (cmpVisibility && cmpVisibility->IsActivated())
				return cmpVisibility->GetVisibility(player, los.IsVisible(i, j), los.IsExplored(i, j));
		}

		// Else, default behavior

		if (los.IsVisible(i, j))
		{
			if (cmpMirage)
				return VIS_HIDDEN;

			return VIS_VISIBLE;
		}

		if (!los.IsExplored(i, j))
			return VIS_HIDDEN;

		// Invisible if the 'retain in fog' flag is not set, and in a non-visible explored region
		// Try using the 'retainInFog' flag in m_EntityData to save a script call
		if (it != m_EntityData.end())
		{
			if (!it->second.HasFlag<FlagMasks::RetainInFog>())
				return VIS_HIDDEN;
		}
		else
		{
			if (!(cmpVisibility && cmpVisibility->GetRetainInFog()))
				return VIS_HIDDEN;
		}

		if (cmpMirage)
			return VIS_FOGGED;

		CmpPtr<ICmpOwnership> cmpOwnership(ent);
		if (!cmpOwnership)
			return VIS_FOGGED;

		if (cmpOwnership->GetOwner() == player)
		{
			CmpPtr<ICmpFogging> cmpFogging(ent);
			if (!(cmpFogging && cmpFogging->IsMiraged(player)))
				return VIS_FOGGED;

			return VIS_HIDDEN;
		}

		// Fogged entities are hidden in two cases:
		// - They were not scouted
		// - A mirage replaces them
		CmpPtr<ICmpFogging> cmpFogging(ent);
		if (cmpFogging && cmpFogging->IsActivated() &&
			(!cmpFogging->WasSeen(player) || cmpFogging->IsMiraged(player)))
			return VIS_HIDDEN;

		return VIS_FOGGED;
	}

	ELosVisibility ComputeLosVisibility(entity_id_t ent, player_id_t player) const
	{
		CEntityHandle handle = GetSimContext().GetComponentManager().LookupEntityHandle(ent);
		return ComputeLosVisibility(handle, player);
	}

	virtual ELosVisibility GetLosVisibility(CEntityHandle ent, player_id_t player) const
	{
		entity_id_t entId = ent.GetId();

		// Entities not with positions in the world are never visible
		if (entId == INVALID_ENTITY)
			return VIS_HIDDEN;

		CmpPtr<ICmpPosition> cmpPosition(ent);
		if (!cmpPosition || !cmpPosition->IsInWorld())
			return VIS_HIDDEN;

		// Gaia and observers do not have a visibility cache
		if (player <= 0)
			return ComputeLosVisibility(ent, player);

		CFixedVector2D pos = cmpPosition->GetPosition2D();
		i32 n = PosToLosTilesHelper(pos.X, pos.Y);

		if (IsVisibilityDirty(m_DirtyVisibility[n], player))
			return ComputeLosVisibility(ent, player);

		if (std::find(m_ModifiedEntities.begin(), m_ModifiedEntities.end(), entId) != m_ModifiedEntities.end())
			return ComputeLosVisibility(ent, player);

		EntityMap<EntityData>::const_iterator it = m_EntityData.find(entId);
		if (it == m_EntityData.end())
			return ComputeLosVisibility(ent, player);

		return static_cast<ELosVisibility>(GetPlayerVisibility(it->second.visibilities, player));
	}

	virtual ELosVisibility GetLosVisibility(entity_id_t ent, player_id_t player) const
	{
		CEntityHandle handle = GetSimContext().GetComponentManager().LookupEntityHandle(ent);
		return GetLosVisibility(handle, player);
	}

	virtual ELosVisibility GetLosVisibilityPosition(entity_pos_t x, entity_pos_t z, player_id_t player) const
	{
		int i = (x / (int)TERRAIN_TILE_SIZE).ToInt_RoundToNearest();
		int j = (z / (int)TERRAIN_TILE_SIZE).ToInt_RoundToNearest();

		// Reveal flag makes all positioned entities visible and all mirages useless
		if (GetLosRevealAll(player))
		{
			if (LosIsOffWorld(i, j))
				return VIS_HIDDEN;
			else
				return VIS_VISIBLE;
		}

		// Get visible regions
		CLosQuerier los(GetSharedLosMask(player), m_LosState, m_TerrainVerticesPerSide);

		if (los.IsVisible(i,j))
			return VIS_VISIBLE;
		if (los.IsExplored(i,j))
			return VIS_FOGGED;
		return VIS_HIDDEN;
	}

	i32 PosToLosTilesHelper(entity_pos_t x, entity_pos_t z) const
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
		std::set<entity_id_t>::const_iterator tileIt = m_LosTiles[tile].find(ent);
		if (tileIt != m_LosTiles[tile].end())
			m_LosTiles[tile].erase(tileIt);
	}

	void UpdateVisibilityData()
	{
		PROFILE("UpdateVisibilityData");

		for (i32 n = 0; n < m_LosTilesPerSide * m_LosTilesPerSide; ++n)
		{
			for (player_id_t player = 1; player < MAX_LOS_PLAYER_ID + 1; ++player)
				if (IsVisibilityDirty(m_DirtyVisibility[n], player) || m_GlobalPlayerVisibilityUpdate[player-1] == 1 || m_GlobalVisibilityUpdate)
					for (const entity_id_t& ent : m_LosTiles[n])
						UpdateVisibility(ent, player);

			m_DirtyVisibility[n] = 0;
		}

		std::fill(m_GlobalPlayerVisibilityUpdate.begin(), m_GlobalPlayerVisibilityUpdate.end(), 0);
		m_GlobalVisibilityUpdate = false;

		// Calling UpdateVisibility can modify m_ModifiedEntities, so be careful:
		// infinite loops could be triggered by feedback between entities and their mirages.
		std::map<entity_id_t, u8> attempts;
		while (!m_ModifiedEntities.empty())
		{
			entity_id_t ent = m_ModifiedEntities.back();
			m_ModifiedEntities.pop_back();

			++attempts[ent];
			ENSURE(attempts[ent] < 100 && "Infinite loop in UpdateVisibilityData");

			UpdateVisibility(ent);
		}
	}

	virtual void RequestVisibilityUpdate(entity_id_t ent)
	{
		if (std::find(m_ModifiedEntities.begin(), m_ModifiedEntities.end(), ent) == m_ModifiedEntities.end())
			m_ModifiedEntities.push_back(ent);
	}

	void UpdateVisibility(entity_id_t ent, player_id_t player)
	{
		EntityMap<EntityData>::iterator itEnts = m_EntityData.find(ent);
		if (itEnts == m_EntityData.end())
			return;

		u8 oldVis = GetPlayerVisibility(itEnts->second.visibilities, player);
		u8 newVis = ComputeLosVisibility(itEnts->first, player);

		if (oldVis == newVis)
			return;

		itEnts->second.visibilities = (itEnts->second.visibilities & ~(0x3 << 2 * (player - 1))) | (newVis << 2 * (player - 1));

		CMessageVisibilityChanged msg(player, ent, oldVis, newVis);
		GetSimContext().GetComponentManager().PostMessage(ent, msg);
	}

	void UpdateVisibility(entity_id_t ent)
	{
		for (player_id_t player = 1; player < MAX_LOS_PLAYER_ID + 1; ++player)
			UpdateVisibility(ent, player);
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

		// On next update, update the visibility of every entity in the world
		m_GlobalVisibilityUpdate = true;
	}

	virtual bool GetLosRevealAll(player_id_t player) const
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

		ResetDerivedData();
	}

	virtual bool GetLosCircular() const
	{
		return m_LosCircular;
	}

	virtual void SetSharedLos(player_id_t player, const std::vector<player_id_t>& players)
	{
		m_SharedLosMasks[player] = CalcSharedLosMask(players);

		// Units belonging to any of 'players' can now trigger visibility updates for 'player'.
		// If shared LOS partners have been removed, we disable visibility updates from them
		// in order to improve performance. That also allows us to properly determine whether
		// 'player' needs a global visibility update for this turn.
		bool modified = false;

		for (player_id_t p = 1; p < MAX_LOS_PLAYER_ID+1; ++p)
		{
			bool inList = std::find(players.begin(), players.end(), p) != players.end();

			if (SetPlayerSharedDirtyVisibilityBit(m_SharedDirtyVisibilityMasks[p], player, inList))
				modified = true;
		}

		if (modified && (size_t)player <= m_GlobalPlayerVisibilityUpdate.size())
			m_GlobalPlayerVisibilityUpdate[player-1] = 1;
	}

	virtual u32 GetSharedLosMask(player_id_t player) const
	{
		return m_SharedLosMasks[player];
	}

	void ExploreAllTiles(player_id_t p)
	{
		for (u16 j = 0; j < m_TerrainVerticesPerSide; ++j)
			for (u16 i = 0; i < m_TerrainVerticesPerSide; ++i)
			{
				if (LosIsOffWorld(i,j))
					continue;
				u32 &explored = m_ExploredVertices.at(p);
				explored += !(m_LosState[i + j*m_TerrainVerticesPerSide] & (LOS_EXPLORED << (2*(p-1))));
				m_LosState[i + j*m_TerrainVerticesPerSide] |= (LOS_EXPLORED << (2*(p-1)));
			}

		SeeExploredEntities(p);
	}

	virtual void ExploreTerritories()
	{
		PROFILE3("ExploreTerritories");

		CmpPtr<ICmpTerritoryManager> cmpTerritoryManager(GetSystemEntity());
		const Grid<u8>& grid = cmpTerritoryManager->GetTerritoryGrid();

		// Territory data is stored per territory-tile (typically a multiple of terrain-tiles).
		// LOS data is stored per terrain-tile vertex.

		// For each territory-tile, if it is owned by a valid player then update the LOS
		// for every vertex inside/around that tile, to mark them as explored.

		// Currently this code doesn't support territory-tiles smaller than terrain-tiles
		// (it will get scale==0 and break), or a non-integer multiple, so check that first
		cassert(ICmpTerritoryManager::NAVCELLS_PER_TERRITORY_TILE >= Pathfinding::NAVCELLS_PER_TILE);
		cassert(ICmpTerritoryManager::NAVCELLS_PER_TERRITORY_TILE % Pathfinding::NAVCELLS_PER_TILE == 0);

		int scale = ICmpTerritoryManager::NAVCELLS_PER_TERRITORY_TILE / Pathfinding::NAVCELLS_PER_TILE;

		ENSURE(grid.m_W*scale == m_TerrainVerticesPerSide-1 && grid.m_H*scale == m_TerrainVerticesPerSide-1);

		for (u16 j = 0; j < grid.m_H; ++j)
			for (u16 i = 0; i < grid.m_W; ++i)
			{
				u8 p = grid.get(i, j) & ICmpTerritoryManager::TERRITORY_PLAYER_MASK;
				if (p > 0 && p <= MAX_LOS_PLAYER_ID)
				{
					u32& explored = m_ExploredVertices.at(p);
					for (int tj = j * scale; tj <= (j+1) * scale; ++tj)
						for (int ti = i * scale; ti <= (i+1) * scale; ++ti)
						{
							if (LosIsOffWorld(ti, tj))
								continue;

							u32& losState = m_LosState[ti + tj * m_TerrainVerticesPerSide];
							if (!(losState & (LOS_EXPLORED << (2*(p-1)))))
							{
								++explored;
								losState |= (LOS_EXPLORED << (2*(p-1)));
							}
						}
				}
			}

		for (player_id_t p = 1; p < MAX_LOS_PLAYER_ID+1; ++p)
			SeeExploredEntities(p);
	}

	/**
	 * Force any entity in explored territory to appear for player p.
	 * This is useful for miraging entities inside the territory borders at the beginning of a game,
	 * or if the "Explore Map" option has been set.
	 */
	void SeeExploredEntities(player_id_t p) const
	{
		// Warning: Code related to fogging (like ForceMiraging) shouldn't be
		// invoked while iterating through m_EntityData.
		// Otherwise, by deleting mirage entities and so on, that code will
		// change the indexes in the map, leading to segfaults.
		// So we just remember what entities to mirage and do that later.
		std::vector<entity_id_t> miragableEntities;

		for (EntityMap<EntityData>::const_iterator it = m_EntityData.begin(); it != m_EntityData.end(); ++it)
		{
			CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), it->first);
			if (!cmpPosition || !cmpPosition->IsInWorld())
				continue;

			CFixedVector2D pos = cmpPosition->GetPosition2D();
			int i = (pos.X / (int)TERRAIN_TILE_SIZE).ToInt_RoundToNearest();
			int j = (pos.Y / (int)TERRAIN_TILE_SIZE).ToInt_RoundToNearest();

			CLosQuerier los(GetSharedLosMask(p), m_LosState, m_TerrainVerticesPerSide);
			if (!los.IsExplored(i,j) || los.IsVisible(i,j))
				continue;

			CmpPtr<ICmpFogging> cmpFogging(GetSimContext(), it->first);
			if (cmpFogging)
				miragableEntities.push_back(it->first);
		}

		for (std::vector<entity_id_t>::iterator it = miragableEntities.begin(); it != miragableEntities.end(); ++it)
		{
			CmpPtr<ICmpFogging> cmpFogging(GetSimContext(), *it);
			ENSURE(cmpFogging && "Impossible to retrieve Fogging component, previously achieved");
			cmpFogging->ForceMiraging(p);
		}
	}

	virtual void RevealShore(player_id_t p, bool enable)
	{
		if (p <= 0 || p > MAX_LOS_PLAYER_ID)
			return;

		// Maximum distance to the shore
		const u16 maxdist = 10;

		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
		const Grid<u16>& shoreGrid = cmpPathfinder->ComputeShoreGrid(true);
		ENSURE(shoreGrid.m_W == m_TerrainVerticesPerSide-1 && shoreGrid.m_H == m_TerrainVerticesPerSide-1);

		std::vector<u16>& counts = m_LosPlayerCounts.at(p);
		ENSURE(!counts.empty());
		u16* countsData = &counts[0];

		for (u16 j = 0; j < shoreGrid.m_H; ++j)
			for (u16 i = 0; i < shoreGrid.m_W; ++i)
			{
				u16 shoredist = shoreGrid.get(i, j);
				if (shoredist > maxdist)
					continue;

				// Maybe we could be more clever and don't add dummy strips of one tile
				if (enable)
					LosAddStripHelper(p, i, i, j, countsData);
				else
					LosRemoveStripHelper(p, i, i, j, countsData);
			}
	}

	/**
	 * Returns whether the given vertex is outside the normal bounds of the world
	 * (i.e. outside the range of a circular map)
	 */
	inline bool LosIsOffWorld(ssize_t i, ssize_t j) const
	{
		if (m_LosCircular)
		{
			// With a circular map, vertex is off-world if hypot(i - size/2, j - size/2) >= size/2:

			ssize_t dist2 = (i - m_TerrainVerticesPerSide/2)*(i - m_TerrainVerticesPerSide/2)
					+ (j - m_TerrainVerticesPerSide/2)*(j - m_TerrainVerticesPerSide/2);

			ssize_t r = m_TerrainVerticesPerSide / 2 - MAP_EDGE_TILES + 1;
				// subtract a bit from the radius to ensure nice
				// SoD blurring around the edges of the map

			return (dist2 >= r*r);
		}
		else
		{
			// With a square map, the outermost edge of the map should be off-world,
			// so the SoD texture blends out nicely
			return i < MAP_EDGE_TILES || j < MAP_EDGE_TILES ||
				i >= m_TerrainVerticesPerSide - MAP_EDGE_TILES ||
				j >= m_TerrainVerticesPerSide - MAP_EDGE_TILES;
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

				MarkVisibilityDirtyAroundTile(owner, i, j);
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
				MarkVisibilityDirtyAroundTile(owner, i, j);
			}
		}
	}

	inline void MarkVisibilityDirtyAroundTile(u8 owner, i32 i, i32 j)
	{
		// If we're still in the deserializing process, we must not modify m_DirtyVisibility
		if (m_Deserializing)
			return;

		// Mark the LoS tiles around the updated vertex
		// 1: left-up, 2: right-up, 3: left-down, 4: right-down
		int n1 = ((j-1)/LOS_TILES_RATIO)*m_LosTilesPerSide + (i-1)/LOS_TILES_RATIO;
		int n2 = ((j-1)/LOS_TILES_RATIO)*m_LosTilesPerSide + i/LOS_TILES_RATIO;
		int n3 = (j/LOS_TILES_RATIO)*m_LosTilesPerSide + (i-1)/LOS_TILES_RATIO;
		int n4 = (j/LOS_TILES_RATIO)*m_LosTilesPerSide + i/LOS_TILES_RATIO;

		u16 sharedDirtyVisibilityMask = m_SharedDirtyVisibilityMasks[owner];

		if (j > 0 && i > 0)
			m_DirtyVisibility[n1] |= sharedDirtyVisibilityMask;
		if (n2 != n1 && j > 0 && i < m_TerrainVerticesPerSide)
			m_DirtyVisibility[n2] |= sharedDirtyVisibilityMask;
		if (n3 != n1 && j < m_TerrainVerticesPerSide && i > 0)
			m_DirtyVisibility[n3] |= sharedDirtyVisibilityMask;
		if (n4 != n1 && j < m_TerrainVerticesPerSide && i < m_TerrainVerticesPerSide)
			m_DirtyVisibility[n4] |= sharedDirtyVisibilityMask;
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

	void SharingLosAdd(u16 visionSharing, entity_pos_t visionRange, CFixedVector2D pos)
	{
		if (visionRange.IsZero())
			return;

		for (player_id_t i = 1; i < MAX_LOS_PLAYER_ID+1; ++i)
			if (HasVisionSharing(visionSharing, i))
				LosAdd(i, visionRange, pos);
	}

	void LosRemove(player_id_t owner, entity_pos_t visionRange, CFixedVector2D pos)
	{
		if (visionRange.IsZero() || owner <= 0 || owner > MAX_LOS_PLAYER_ID)
			return;

		LosUpdateHelper<false>((u8)owner, visionRange, pos);
	}

	void SharingLosRemove(u16 visionSharing, entity_pos_t visionRange, CFixedVector2D pos)
	{
		if (visionRange.IsZero())
			return;

		for (player_id_t i = 1; i < MAX_LOS_PLAYER_ID+1; ++i)
			if (HasVisionSharing(visionSharing, i))
				LosRemove(i, visionRange, pos);
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
			// Otherwise use the version optimised for mostly-overlapping circles
			LosUpdateHelperIncremental((u8)owner, visionRange, from, to);
	}

	void SharingLosMove(u16 visionSharing, entity_pos_t visionRange, CFixedVector2D from, CFixedVector2D to)
	{
		if (visionRange.IsZero())
			return;

		for (player_id_t i = 1; i < MAX_LOS_PLAYER_ID+1; ++i)
			if (HasVisionSharing(visionSharing, i))
				LosMove(i, visionRange, from, to);
	}

	virtual u8 GetPercentMapExplored(player_id_t player) const
	{
		return m_ExploredVertices.at((u8)player) * 100 / m_TotalInworldVertices;
	}

	virtual u8 GetUnionPercentMapExplored(const std::vector<player_id_t>& players) const
	{
		u32 exploredVertices = 0;
		std::vector<player_id_t>::const_iterator playerIt;

		for (i32 j = 0; j < m_TerrainVerticesPerSide; j++)
			for (i32 i = 0; i < m_TerrainVerticesPerSide; i++)
			{
				if (LosIsOffWorld(i, j))
					continue;

				for (playerIt = players.begin(); playerIt != players.end(); ++playerIt)
					if (m_LosState[j*m_TerrainVerticesPerSide + i] & (LOS_EXPLORED << (2*((*playerIt)-1))))
					{
						exploredVertices += 1;
						break;
					}
			}

		return exploredVertices * 100 / m_TotalInworldVertices;
	}
};

REGISTER_COMPONENT_TYPE(RangeManager)

#undef LOS_TILES_RATIO
#undef DEBUG_RANGE_MANAGER_BOUNDS
