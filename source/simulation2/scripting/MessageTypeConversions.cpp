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

#include "precompiled.h"

#include "scriptinterface/ScriptInterface.h"
#include "simulation2/MessageTypes.h"

#include "js/jsapi.h"

#define TOJSVAL_SETUP() \
	JSObject* obj = JS_NewObject(scriptInterface.GetContext(), NULL, NULL, NULL); \
	if (! obj) \
		return JSVAL_VOID

#define SET_MSG_PROPERTY(name) \
	do { \
		jsval prop = ScriptInterface::ToJSVal(scriptInterface.GetContext(), this->name); \
		if (! JS_SetProperty(scriptInterface.GetContext(), obj, #name, &prop)) \
			return JSVAL_VOID; \
	} while (0)

#define FROMJSVAL_SETUP() \
	if (! JSVAL_IS_OBJECT(val)) \
		return NULL; \
	JSObject* obj = JSVAL_TO_OBJECT(val)
jsval prop;

#define GET_MSG_PROPERTY(type, name) \
	if (! JS_GetProperty(scriptInterface.GetContext(), obj, #name, &prop)) \
		return NULL; \
	type name; \
	if (! ScriptInterface::FromJSVal(scriptInterface.GetContext(), prop, name)) \
		return NULL;

jsval CMessage::ToJSValCached(ScriptInterface& scriptInterface) const
{
	if (m_Cached.uninitialised())
		m_Cached = CScriptValRooted(scriptInterface.GetContext(), ToJSVal(scriptInterface));

	return m_Cached.get();
}

////////////////////////////////

jsval CMessageTurnStart::ToJSVal(ScriptInterface& UNUSED(scriptInterface)) const
{
	return JSVAL_VOID;
}

CMessage* CMessageTurnStart::FromJSVal(ScriptInterface& UNUSED(scriptInterface), jsval UNUSED(val))
{
	return new CMessageTurnStart();
}

////////////////////////////////

#define MESSAGE_1(name, t0, a0) \
	jsval CMessage##name::ToJSVal(ScriptInterface& scriptInterface) const \
	{ \
		TOJSVAL_SETUP(); \
		SET_MSG_PROPERTY(a0); \
		return OBJECT_TO_JSVAL(obj); \
	} \
	CMessage* CMessage##name::FromJSVal(ScriptInterface& scriptInterface, jsval val) \
	{ \
		FROMJSVAL_SETUP(); \
		GET_MSG_PROPERTY(t0, a0); \
		return new CMessage##name(a0); \
	}

MESSAGE_1(Update, fixed, turnLength)
MESSAGE_1(Update_MotionFormation, fixed, turnLength)
MESSAGE_1(Update_MotionUnit, fixed, turnLength)
MESSAGE_1(Update_Final, fixed, turnLength)

////////////////////////////////

jsval CMessageInterpolate::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(frameTime);
	SET_MSG_PROPERTY(offset);
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageInterpolate::FromJSVal(ScriptInterface& scriptInterface, jsval val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(float, frameTime);
	GET_MSG_PROPERTY(float, offset);
	return new CMessageInterpolate(frameTime, offset);
}

////////////////////////////////

jsval CMessageRenderSubmit::ToJSVal(ScriptInterface& UNUSED(scriptInterface)) const
{
	return JSVAL_VOID;
}

CMessage* CMessageRenderSubmit::FromJSVal(ScriptInterface& UNUSED(scriptInterface), jsval UNUSED(val))
{
	return NULL;
}

////////////////////////////////

jsval CMessageCreate::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(entity);
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageCreate::FromJSVal(ScriptInterface& scriptInterface, jsval val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(entity_id_t, entity);
	return new CMessageCreate(entity);
}

////////////////////////////////

jsval CMessageDestroy::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(entity);
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageDestroy::FromJSVal(ScriptInterface& scriptInterface, jsval val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(entity_id_t, entity);
	return new CMessageDestroy(entity);
}

////////////////////////////////

jsval CMessageOwnershipChanged::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(entity);
	SET_MSG_PROPERTY(from);
	SET_MSG_PROPERTY(to);
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageOwnershipChanged::FromJSVal(ScriptInterface& scriptInterface, jsval val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(entity_id_t, entity);
	GET_MSG_PROPERTY(int32_t, from);
	GET_MSG_PROPERTY(int32_t, to);
	return new CMessageOwnershipChanged(entity, from, to);
}

////////////////////////////////

jsval CMessagePositionChanged::ToJSVal(ScriptInterface& UNUSED(scriptInterface)) const
{
	return JSVAL_VOID;
}

CMessage* CMessagePositionChanged::FromJSVal(ScriptInterface& UNUSED(scriptInterface), jsval UNUSED(val))
{
	return NULL;
}

////////////////////////////////

jsval CMessageMotionChanged::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(starting);
	SET_MSG_PROPERTY(error);
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageMotionChanged::FromJSVal(ScriptInterface& scriptInterface, jsval val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(bool, starting);
	GET_MSG_PROPERTY(bool, error);
	return new CMessageMotionChanged(starting, error);
}

////////////////////////////////

jsval CMessageTerrainChanged::ToJSVal(ScriptInterface& UNUSED(scriptInterface)) const
{
	return JSVAL_VOID;
}

CMessage* CMessageTerrainChanged::FromJSVal(ScriptInterface& UNUSED(scriptInterface), jsval UNUSED(val))
{
	return NULL;
}

////////////////////////////////

jsval CMessageRangeUpdate::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(tag);
	SET_MSG_PROPERTY(added);
	SET_MSG_PROPERTY(removed);
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageRangeUpdate::FromJSVal(ScriptInterface& UNUSED(scriptInterface), jsval UNUSED(val))
{
	return NULL;
}

////////////////////////////////

jsval CMessagePathResult::ToJSVal(ScriptInterface& UNUSED(scriptInterface)) const
{
	return JSVAL_VOID;
}

CMessage* CMessagePathResult::FromJSVal(ScriptInterface& UNUSED(scriptInterface), jsval UNUSED(val))
{
	return NULL;
}

////////////////////////////////////////////////////////////////

CMessage* CMessageFromJSVal(int mtid, ScriptInterface& scriptingInterface, jsval val)
{
	switch (mtid)
	{
#define MESSAGE(name) case MT_##name: return CMessage##name::FromJSVal(scriptingInterface, val);
#define INTERFACE(name)
#define COMPONENT(name)
#include "simulation2/TypeList.h"
#undef COMPONENT
#undef INTERFACE
#undef MESSAGE
	}

	return NULL;
}
