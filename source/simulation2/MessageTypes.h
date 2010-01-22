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

#include "maths/Fixed.h"

#define DEFAULT_MESSAGE_IMPL(name) \
	virtual EMessageTypeId GetType() const { return MT_##name; } \
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

	CMessageUpdate(CFixed_23_8 turnLength) :
		turnLength(turnLength)
	{
	}

	CFixed_23_8 turnLength;
};

class CMessageInterpolate : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(Interpolate)

	CMessageInterpolate(float offset) :
		offset(offset)
	{
	}

	float offset;
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

	CMessageDestroy()
	{
	}
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


#endif // INCLUDED_MESSAGETYPES
