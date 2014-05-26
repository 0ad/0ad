/* Copyright (C) 2013 Wildfire Games.
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

#include "ps/CLogger.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/MessageTypes.h"

#define TOJSVAL_SETUP() \
	JSObject* obj; \
	{\
	JSAutoRequest rq(scriptInterface.GetContext()); \
	obj = JS_NewObject(scriptInterface.GetContext(), NULL, NULL, NULL); \
	if (!obj) \
		return JSVAL_VOID; \
	}

#define SET_MSG_PROPERTY(name) \
	do { \
		JSAutoRequest rq(scriptInterface.GetContext()); \
		JSContext* cx = scriptInterface.GetContext(); \
		JS::RootedValue prop(cx);\
		ScriptInterface::ToJSVal(cx, prop.get(), this->name); \
		if (! JS_SetProperty(cx, obj, #name, prop.address())) \
			return JSVAL_VOID; \
	} while (0);

#define FROMJSVAL_SETUP() \
	if ( JSVAL_IS_PRIMITIVE(val)) \
		return NULL; \
	JSObject* obj = JSVAL_TO_OBJECT(val); \
	JS::RootedValue prop(scriptInterface.GetContext());

#define GET_MSG_PROPERTY(type, name) \
	type name; \
	{ \
	JSAutoRequest rq(scriptInterface.GetContext()); \
	if (! JS_GetProperty(scriptInterface.GetContext(), obj, #name, prop.address())) \
		return NULL; \
	if (! ScriptInterface::FromJSVal(scriptInterface.GetContext(), prop.get(), name)) \
		return NULL; \
	}

jsval CMessage::ToJSValCached(ScriptInterface& scriptInterface) const
{
	if (m_Cached.uninitialised())
		m_Cached = CScriptValRooted(scriptInterface.GetContext(), ToJSVal(scriptInterface));

	return m_Cached.get();
}

////////////////////////////////

jsval CMessageTurnStart::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	return OBJECT_TO_JSVAL(obj);
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
	SET_MSG_PROPERTY(deltaSimTime);
	SET_MSG_PROPERTY(offset);
	SET_MSG_PROPERTY(deltaRealTime);
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageInterpolate::FromJSVal(ScriptInterface& scriptInterface, jsval val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(float, deltaSimTime);
	GET_MSG_PROPERTY(float, offset);
	GET_MSG_PROPERTY(float, deltaRealTime);
	return new CMessageInterpolate(deltaSimTime, offset, deltaRealTime);
}

////////////////////////////////

jsval CMessageRenderSubmit::ToJSVal(ScriptInterface& UNUSED(scriptInterface)) const
{
	LOGWARNING(L"CMessageRenderSubmit::ToJSVal not implemented");
	return JSVAL_VOID;
}

CMessage* CMessageRenderSubmit::FromJSVal(ScriptInterface& UNUSED(scriptInterface), jsval UNUSED(val))
{
	LOGWARNING(L"CMessageRenderSubmit::FromJSVal not implemented");
	return NULL;
}

////////////////////////////////

jsval CMessageProgressiveLoad::ToJSVal(ScriptInterface& UNUSED(scriptInterface)) const
{
	LOGWARNING(L"CMessageProgressiveLoad::ToJSVal not implemented");
	return JSVAL_VOID;
}

CMessage* CMessageProgressiveLoad::FromJSVal(ScriptInterface& UNUSED(scriptInterface), jsval UNUSED(val))
{
	LOGWARNING(L"CMessageProgressiveLoad::FromJSVal not implemented");
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
	GET_MSG_PROPERTY(player_id_t, from);
	GET_MSG_PROPERTY(player_id_t, to);
	return new CMessageOwnershipChanged(entity, from, to);
}

////////////////////////////////

jsval CMessagePositionChanged::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(entity);
	SET_MSG_PROPERTY(inWorld);
	SET_MSG_PROPERTY(x);
	SET_MSG_PROPERTY(z);
	SET_MSG_PROPERTY(a);
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessagePositionChanged::FromJSVal(ScriptInterface& scriptInterface, jsval val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(entity_id_t, entity);
	GET_MSG_PROPERTY(bool, inWorld);
	GET_MSG_PROPERTY(entity_pos_t, x);
	GET_MSG_PROPERTY(entity_pos_t, z);
	GET_MSG_PROPERTY(entity_angle_t, a);
	return new CMessagePositionChanged(entity, inWorld, x, z, a);
}

////////////////////////////////

jsval CMessageTerritoryPositionChanged::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(entity);
	SET_MSG_PROPERTY(newTerritory);
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageTerritoryPositionChanged::FromJSVal(ScriptInterface& scriptInterface, jsval val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(entity_id_t, entity);
	GET_MSG_PROPERTY(player_id_t, newTerritory);
	return new CMessageTerritoryPositionChanged(entity, newTerritory);
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

jsval CMessageTerrainChanged::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(i0);
	SET_MSG_PROPERTY(j0);
	SET_MSG_PROPERTY(i1);
	SET_MSG_PROPERTY(j1);
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageTerrainChanged::FromJSVal(ScriptInterface& scriptInterface, jsval val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(int32_t, i0);
	GET_MSG_PROPERTY(int32_t, j0);
	GET_MSG_PROPERTY(int32_t, i1);
	GET_MSG_PROPERTY(int32_t, j1);
	return new CMessageTerrainChanged(i0, i1, j0, j1);
}

////////////////////////////////

jsval CMessageWaterChanged::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageWaterChanged::FromJSVal(ScriptInterface& UNUSED(scriptInterface), jsval UNUSED(val))
{
	return new CMessageWaterChanged();
}

////////////////////////////////

jsval CMessageTerritoriesChanged::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageTerritoriesChanged::FromJSVal(ScriptInterface& UNUSED(scriptInterface), jsval UNUSED(val))
{
	return new CMessageTerritoriesChanged();
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
	LOGWARNING(L"CMessageRangeUpdate::FromJSVal not implemented");
	return NULL;
}

////////////////////////////////

jsval CMessagePathResult::ToJSVal(ScriptInterface& UNUSED(scriptInterface)) const
{
	LOGWARNING(L"CMessagePathResult::ToJSVal not implemented");
	return JSVAL_VOID;
}

CMessage* CMessagePathResult::FromJSVal(ScriptInterface& UNUSED(scriptInterface), jsval UNUSED(val))
{
	LOGWARNING(L"CMessagePathResult::FromJSVal not implemented");
	return NULL;
}

////////////////////////////////

jsval CMessageValueModification::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(entities);
	SET_MSG_PROPERTY(component);
	SET_MSG_PROPERTY(valueNames);
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageValueModification::FromJSVal(ScriptInterface& scriptInterface, jsval val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(std::vector<entity_id_t>, entities);
	GET_MSG_PROPERTY(std::wstring, component);
	GET_MSG_PROPERTY(std::vector<std::wstring>, valueNames);
	return new CMessageValueModification(entities, component, valueNames);
}

////////////////////////////////

jsval CMessageTemplateModification::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(player);
	SET_MSG_PROPERTY(component);
	SET_MSG_PROPERTY(valueNames);
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageTemplateModification::FromJSVal(ScriptInterface& scriptInterface, jsval val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(player_id_t, player);
	GET_MSG_PROPERTY(std::wstring, component);
	GET_MSG_PROPERTY(std::vector<std::wstring>, valueNames);
	return new CMessageTemplateModification(player, component, valueNames);
}

////////////////////////////////

jsval CMessageVisionRangeChanged::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(entity);
	SET_MSG_PROPERTY(oldRange);
	SET_MSG_PROPERTY(newRange);
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageVisionRangeChanged::FromJSVal(ScriptInterface& scriptInterface, jsval val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(entity_id_t, entity);
	GET_MSG_PROPERTY(entity_pos_t, oldRange);
	GET_MSG_PROPERTY(entity_pos_t, newRange);
	return new CMessageVisionRangeChanged(entity, oldRange, newRange);
}

////////////////////////////////

jsval CMessageMinimapPing::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageMinimapPing::FromJSVal(ScriptInterface& UNUSED(scriptInterface), jsval UNUSED(val))
{
	return new CMessageMinimapPing();
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
