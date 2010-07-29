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

#ifndef INCLUDED_MESSAGETYPES
#define INCLUDED_MESSAGETYPES

#include "simulation2/system/Components.h"
#include "simulation2/system/Entity.h"
#include "simulation2/system/Message.h"

#include "simulation2/helpers/Position.h"

#define DEFAULT_MESSAGE_IMPL(name) \
	virtual int GetType() const { return MT_##name; } \
	virtual const char* GetScriptHandlerName() const { return "On" #name; } \
	virtual const char* GetScriptGlobalHandlerName() const { return "OnGlobal" #name; } \
	virtual jsval ToJSVal(ScriptInterface& scriptInterface) const; \
	static CMessage* FromJSVal(ScriptInterface&, jsval val);

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

class CMessageInterpolate : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(Interpolate)

	CMessageInterpolate(float frameTime, float offset) :
		frameTime(frameTime), offset(offset)
	{
	}

	float frameTime; // time in seconds since previous interpolate
	float offset; // range [0, 1] (inclusive); fractional time of current frame between previous/next simulation turns
};

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

	CMessageOwnershipChanged(entity_id_t entity, int32_t from, int32_t to) :
		entity(entity), from(from), to(to)
	{
	}

	entity_id_t entity;
	int32_t from;
	int32_t to;
};

/**
 * Sent during TurnStart.
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
 * Sent by CCmpUnitMotion during Update, whenever the motion status has changed
 * since the previous update.
 */
class CMessageMotionChanged : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(MotionChanged)

	CMessageMotionChanged(fixed speed) :
		speed(speed)
	{
	}

	fixed speed; // metres per second, or 0 if not moving
};

/**
 * Sent when terrain (texture or elevation) has been changed.
 */
class CMessageTerrainChanged : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(TerrainChanged)

	CMessageTerrainChanged(ssize_t i0, ssize_t j0, ssize_t i1, ssize_t j1) :
		i0(i0), j0(j0), i1(i1), j1(j1)
	{
	}

	ssize_t i0, j0, i1, j1; // inclusive lower bound, exclusive upper bound, in tiles
};

/**
 * Sent by CCmpRangeManager at most once per turn, when an active range query
 * has had matching units enter/leave the range since the last RangeUpdate.
 */
class CMessageRangeUpdate : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(RangeUpdate)

	CMessageRangeUpdate(u32 tag, const std::vector<entity_id_t>& added, const std::vector<entity_id_t>& removed) :
		tag(tag), added(added), removed(removed)
	{
	}

	u32 tag;
	std::vector<entity_id_t> added;
	std::vector<entity_id_t> removed;

	// CCmpRangeManager wants to store a vector of messages and wants to
	// swap vectors instead of copying (to save on memory allocations),
	// so add some constructors for it:

	CMessageRangeUpdate(u32 tag) :
		tag(tag)
	{
	}

	CMessageRangeUpdate(const CMessageRangeUpdate& other) :
		CMessage(), tag(other.tag), added(other.added), removed(other.removed)
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

#endif // INCLUDED_MESSAGETYPES
