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

#ifndef INCLUDED_MESSAGETYPES
#define INCLUDED_MESSAGETYPES

#include "simulation2/system/Components.h"
#include "simulation2/system/Entity.h"
#include "simulation2/system/Message.h"

#include "simulation2/helpers/Player.h"
#include "simulation2/helpers/Position.h"

#include "simulation2/components/ICmpPathfinder.h"

#include "maths/Vector3D.h"

#define DEFAULT_MESSAGE_IMPL(name) \
	virtual int GetType() const { return MT_##name; } \
	virtual const char* GetScriptHandlerName() const { return "On" #name; } \
	virtual const char* GetScriptGlobalHandlerName() const { return "OnGlobal" #name; } \
	virtual JS::Value ToJSVal(ScriptInterface& scriptInterface) const; \
	static CMessage* FromJSVal(ScriptInterface&, JS::HandleValue val);

class SceneCollector;
class CFrustum;

class CMessageTurnStart : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(TurnStart)

	CMessageTurnStart()
	{
	}
};

// The update process is split into a number of phases, in an attempt
// to cope with dependencies between components. Each phase is implemented
// as a separate message. Simulation2.cpp sends them in sequence.

/**
 * Generic per-turn update message, for things that don't care much about ordering.
 */
class CMessageUpdate : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(Update)

	CMessageUpdate(fixed turnLength) :
		turnLength(turnLength)
	{
	}

	fixed turnLength;
};

/**
 * Update phase for formation controller movement (must happen before individual
 * units move to follow their formation).
 */
class CMessageUpdate_MotionFormation : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(Update_MotionFormation)

	CMessageUpdate_MotionFormation(fixed turnLength) :
		turnLength(turnLength)
	{
	}

	fixed turnLength;
};

/**
 * Update phase for non-formation-controller unit movement.
 */
class CMessageUpdate_MotionUnit : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(Update_MotionUnit)

	CMessageUpdate_MotionUnit(fixed turnLength) :
		turnLength(turnLength)
	{
	}

	fixed turnLength;
};

/**
 * Final update phase, after all other updates.
 */
class CMessageUpdate_Final : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(Update_Final)

	CMessageUpdate_Final(fixed turnLength) :
		turnLength(turnLength)
	{
	}

	fixed turnLength;
};

/**
 * Prepare for rendering a new frame (set up model positions etc).
 */
class CMessageInterpolate : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(Interpolate)

	CMessageInterpolate(float deltaSimTime, float offset, float deltaRealTime) :
		deltaSimTime(deltaSimTime), offset(offset), deltaRealTime(deltaRealTime)
	{
	}

	/// Elapsed simulation time since previous interpolate, in seconds. This is similar to the elapsed real time, except
	/// it is scaled by the current simulation rate (and might indeed be zero).
	float deltaSimTime;
	/// Range [0, 1] (inclusive); fractional time of current frame between previous/next simulation turns.
	float offset;
	/// Elapsed real time since previous interpolate, in seconds.
	float deltaRealTime;
};

/**
 * Add renderable objects to the scene collector.
 * Called after CMessageInterpolate.
 */
class CMessageRenderSubmit : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(RenderSubmit)

	CMessageRenderSubmit(SceneCollector& collector, const CFrustum& frustum, bool culling) :
		collector(collector), frustum(frustum), culling(culling)
	{
	}

	SceneCollector& collector;
	const CFrustum& frustum;
	bool culling;
};

/**
 * Handle progressive loading of resources.
 * A component that listens to this message must do the following:
 *  - Increase *msg.total by the non-zero number of loading tasks this component can perform.
 *  - If *msg.progressed == true, return and do nothing.
 *  - If you've loaded everything, increase *msg.progress by the value you added to .total
 *  - Otherwise do some loading, set *msg.progressed = true, and increase *msg.progress by a
 *    value indicating how much progress you've made in total (0 <= p <= what you added to .total)
 * In some situations these messages will never be sent - components must ensure they
 * load all their data themselves before using it in that case.
 */
class CMessageProgressiveLoad : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(ProgressiveLoad)

	CMessageProgressiveLoad(bool* progressed, int* total, int* progress) :
		progressed(progressed), total(total), progress(progress)
	{
	}

	bool* progressed;
	int* total;
	int* progress;
};

/**
 * Broadcast after the entire simulation state has been deserialized.
 * Components should do all their self-contained work in their Deserialize
 * function when possible. But any reinitialisation that depends on other
 * components or other entities, that might not be constructed until later
 * in the deserialization process, may be done in response to this message
 * instead.
 */
class CMessageDeserialized : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(Deserialized)

	CMessageDeserialized()
	{
	}
};


/**
 * This is sent immediately after a new entity's components have all been created
 * and initialised.
 */
class CMessageCreate : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(Create)

	CMessageCreate(entity_id_t entity) :
		entity(entity)
	{
	}

	entity_id_t entity;
};

/**
 * This is sent immediately before a destroyed entity is flushed and really destroyed.
 * (That is, after CComponentManager::DestroyComponentsSoon and inside FlushDestroyedComponents).
 * The entity will still exist at the time this message is sent.
 * It's possible for this message to be sent multiple times for one entity, but all its components
 * will have been deleted after the first time.
 */
class CMessageDestroy : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(Destroy)

	CMessageDestroy(entity_id_t entity) :
		entity(entity)
	{
	}

	entity_id_t entity;
};

class CMessageOwnershipChanged : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(OwnershipChanged)

	CMessageOwnershipChanged(entity_id_t entity, player_id_t from, player_id_t to) :
		entity(entity), from(from), to(to)
	{
	}

	entity_id_t entity;
	player_id_t from;
	player_id_t to;
};

/**
 * Sent by CCmpPosition whenever anything has changed that will affect the
 * return value of GetPosition2D() or GetRotation().Y
 *
 * If @c inWorld is false, then the other fields are invalid and meaningless.
 * Otherwise they represent the current position.
 */
class CMessagePositionChanged : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(PositionChanged)

	CMessagePositionChanged(entity_id_t entity, bool inWorld, entity_pos_t x, entity_pos_t z, entity_angle_t a) :
		entity(entity), inWorld(inWorld), x(x), z(z), a(a)
	{
	}

	entity_id_t entity;
	bool inWorld;
	entity_pos_t x, z;
	entity_angle_t a;
};

/**
 * Sent by CCmpPosition whenever anything has changed that will affect the
 * return value of GetInterpolatedTransform()
 */
class CMessageInterpolatedPositionChanged : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(InterpolatedPositionChanged)

	CMessageInterpolatedPositionChanged(entity_id_t entity, bool inWorld, const CVector3D& pos0, const CVector3D& pos1) :
		entity(entity), inWorld(inWorld), pos0(pos0), pos1(pos1)
	{
	}

	entity_id_t entity;
	bool inWorld;
	CVector3D pos0;
	CVector3D pos1;
};

/*Sent whenever the territory type (neutral,own,enemy) differs from the former type*/
class CMessageTerritoryPositionChanged : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(TerritoryPositionChanged)

	CMessageTerritoryPositionChanged(entity_id_t entity, player_id_t newTerritory) :
		entity(entity), newTerritory(newTerritory)
	{
	}

	entity_id_t entity;
	player_id_t newTerritory;
};

/**
 * Sent by CCmpUnitMotion during Update, whenever the motion status has changed
 * since the previous update.
 */
class CMessageMotionChanged : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(MotionChanged)

	CMessageMotionChanged(bool starting, bool error) :
		starting(starting), error(error)
	{
	}

	bool starting; // whether this is a start or end of movement
	bool error; // whether we failed to start moving (couldn't find any path)
};

/**
 * Sent when water height has been changed.
 */
class CMessageWaterChanged : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(WaterChanged)

	CMessageWaterChanged()
	{
	}
};

/**
 * Sent when terrain (texture or elevation) has been changed.
 */
class CMessageTerrainChanged : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(TerrainChanged)

	CMessageTerrainChanged(int32_t i0, int32_t j0, int32_t i1, int32_t j1) :
		i0(i0), j0(j0), i1(i1), j1(j1)
	{
	}

	int32_t i0, j0, i1, j1; // inclusive lower bound, exclusive upper bound, in tiles
};

/**
 * Sent, at most once per turn, when the visibility of an entity changed
 */
class CMessageVisibilityChanged : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(VisibilityChanged)

	CMessageVisibilityChanged(player_id_t player, entity_id_t ent, int oldVisibility, int newVisibility) :
		player(player), ent(ent), oldVisibility(oldVisibility), newVisibility(newVisibility)
	{
	}

	player_id_t player;
	entity_id_t ent;
	int oldVisibility;
	int newVisibility;
};

/**
 * Sent when ObstructionManager's view of the shape of the world has changed
 * (changing the TILE_OUTOFBOUNDS tiles returned by Rasterise).
 */
class CMessageObstructionMapShapeChanged : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(ObstructionMapShapeChanged)

	CMessageObstructionMapShapeChanged()
	{
	}
};

/**
 * Sent when territory assignments have changed.
 */
class CMessageTerritoriesChanged : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(TerritoriesChanged)

	CMessageTerritoriesChanged()
	{
	}
};

/**
 * Sent by CCmpRangeManager at most once per turn, when an active range query
 * has had matching units enter/leave the range since the last RangeUpdate.
 */
class CMessageRangeUpdate : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(RangeUpdate)



	u32 tag;
	std::vector<entity_id_t> added;
	std::vector<entity_id_t> removed;

	// CCmpRangeManager wants to store a vector of messages and wants to
	// swap vectors instead of copying (to save on memory allocations),
	// so add some constructors for it:

	// don't init tag in empty ctor
	CMessageRangeUpdate() 
	{
	}
	CMessageRangeUpdate(u32 tag) : tag(tag)
	{
	}
	CMessageRangeUpdate(u32 tag, const std::vector<entity_id_t>& added, const std::vector<entity_id_t>& removed)
		: tag(tag), added(added), removed(removed)
	{
	}
	CMessageRangeUpdate(const CMessageRangeUpdate& other) 
		: CMessage(), tag(other.tag), added(other.added), removed(other.removed)
	{
	}
	CMessageRangeUpdate& operator=(const CMessageRangeUpdate& other)
	{
		tag = other.tag;
		added = other.added;
		removed = other.removed;
		return *this;
	}
};

/**
 * Sent by CCmpPathfinder after async path requests.
 */
class CMessagePathResult : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(PathResult)

	CMessagePathResult(u32 ticket, const ICmpPathfinder::Path& path) :
		ticket(ticket), path(path)
	{
	}

	u32 ticket;
	ICmpPathfinder::Path path;
};

/**
 * Sent by aura manager when a value of a certain entity's component is changed
 */
class CMessageValueModification : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(ValueModification)

	CMessageValueModification(const std::vector<entity_id_t>& entities, std::wstring component, const std::vector<std::wstring>& valueNames) :
	entities(entities),
	component(component),
	valueNames(valueNames)
	{
	}
	
	std::vector<entity_id_t> entities;
	std::wstring component;
	std::vector<std::wstring> valueNames;
};

/**
 * Sent by aura and tech managers when a value of a certain template's component is changed
 */
class CMessageTemplateModification : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(TemplateModification)
	
	CMessageTemplateModification(player_id_t player, std::wstring component, const std::vector<std::wstring>& valueNames) :
	player(player),
	component(component),
	valueNames(valueNames)
	{
	}
	
	player_id_t player;
	std::wstring component;
	std::vector<std::wstring> valueNames;
};

/**
 * Sent by CCmpVision when an entity's vision range changes.
 */
class CMessageVisionRangeChanged : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(VisionRangeChanged)

	CMessageVisionRangeChanged(entity_id_t entity, entity_pos_t oldRange, entity_pos_t newRange) :
	entity(entity), oldRange(oldRange), newRange(newRange)
	{
	}

	entity_id_t entity;
	entity_pos_t oldRange;
	entity_pos_t newRange;
};

/**
 * Sent when an entity pings the minimap
 */
class CMessageMinimapPing : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(MinimapPing)

	CMessageMinimapPing() 
	{
	}
};

#endif // INCLUDED_MESSAGETYPES
