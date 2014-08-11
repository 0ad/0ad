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

#include "ps/CLogger.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/MessageTypes.h"

#define TOJSVAL_SETUP() \
	JSContext* cx = scriptInterface.GetContext(); \
	JSAutoRequest rq(cx); \
	JS::RootedObject obj(cx, JS_NewObject(cx, NULL, NULL, NULL)); \
	if (!obj) \
		return JS::UndefinedValue();

#define SET_MSG_PROPERTY(name) \
	do { \
		JS::RootedValue prop(cx);\
		ScriptInterface::ToJSVal(cx, &prop, this->name); \
		if (! JS_SetProperty(cx, obj, #name, prop.address())) \
			return JS::UndefinedValue(); \
	} while (0);

#define FROMJSVAL_SETUP() \
	JSContext* cx = scriptInterface.GetContext(); \
	JSAutoRequest rq(cx); \
	if (val.isPrimitive()) \
		return NULL; \
	JS::RootedObject obj(cx, &val.toObject()); \
	JS::RootedValue prop(cx);

#define GET_MSG_PROPERTY(type, name) \
	type name; \
	{ \
	if (! JS_GetProperty(cx, obj, #name, prop.address())) \
		return NULL; \
	if (! ScriptInterface::FromJSVal(cx, prop, name)) \
		return NULL; \
	}

JS::Value CMessage::ToJSValCached(ScriptInterface& scriptInterface) const
{
	if (m_Cached.uninitialised())
		m_Cached = CScriptValRooted(scriptInterface.GetContext(), ToJSVal(scriptInterface));

	return m_Cached.get();
}

////////////////////////////////

JS::Value CMessageTurnStart::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	return JS::ObjectValue(*obj);
}

CMessage* CMessageTurnStart::FromJSVal(ScriptInterface& UNUSED(scriptInterface), JS::HandleValue UNUSED(val))
{
	return new CMessageTurnStart();
}

////////////////////////////////

#define MESSAGE_1(name, t0, a0) \
	JS::Value CMessage##name::ToJSVal(ScriptInterface& scriptInterface) const \
	{ \
		TOJSVAL_SETUP(); \
		SET_MSG_PROPERTY(a0); \
		return JS::ObjectValue(*obj); \
	} \
	CMessage* CMessage##name::FromJSVal(ScriptInterface& scriptInterface, JS::HandleValue val) \
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

JS::Value CMessageInterpolate::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(deltaSimTime);
	SET_MSG_PROPERTY(offset);
	SET_MSG_PROPERTY(deltaRealTime);
	return JS::ObjectValue(*obj);
}

CMessage* CMessageInterpolate::FromJSVal(ScriptInterface& scriptInterface, JS::HandleValue val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(float, deltaSimTime);
	GET_MSG_PROPERTY(float, offset);
	GET_MSG_PROPERTY(float, deltaRealTime);
	return new CMessageInterpolate(deltaSimTime, offset, deltaRealTime);
}

////////////////////////////////

JS::Value CMessageRenderSubmit::ToJSVal(ScriptInterface& UNUSED(scriptInterface)) const
{
	LOGWARNING(L"CMessageRenderSubmit::ToJSVal not implemented");
	return JS::UndefinedValue();
}

CMessage* CMessageRenderSubmit::FromJSVal(ScriptInterface& UNUSED(scriptInterface), JS::HandleValue UNUSED(val))
{
	LOGWARNING(L"CMessageRenderSubmit::FromJSVal not implemented");
	return NULL;
}

////////////////////////////////

JS::Value CMessageProgressiveLoad::ToJSVal(ScriptInterface& UNUSED(scriptInterface)) const
{
	LOGWARNING(L"CMessageProgressiveLoad::ToJSVal not implemented");
	return JS::UndefinedValue();
}

CMessage* CMessageProgressiveLoad::FromJSVal(ScriptInterface& UNUSED(scriptInterface), JS::HandleValue UNUSED(val))
{
	LOGWARNING(L"CMessageProgressiveLoad::FromJSVal not implemented");
	return NULL;
}

////////////////////////////////

JS::Value CMessageDeserialized::ToJSVal(ScriptInterface& UNUSED(scriptInterface)) const
{
	LOGWARNING(L"CMessageDeserialized::ToJSVal not implemented");
	return JS::UndefinedValue();
}

CMessage* CMessageDeserialized::FromJSVal(ScriptInterface& UNUSED(scriptInterface), JS::HandleValue UNUSED(val))
{
	LOGWARNING(L"CMessageDeserialized::FromJSVal not implemented");
	return NULL;
}

////////////////////////////////

JS::Value CMessageCreate::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(entity);
	return JS::ObjectValue(*obj);
}

CMessage* CMessageCreate::FromJSVal(ScriptInterface& scriptInterface, JS::HandleValue val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(entity_id_t, entity);
	return new CMessageCreate(entity);
}

////////////////////////////////

JS::Value CMessageDestroy::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(entity);
	return JS::ObjectValue(*obj);
}

CMessage* CMessageDestroy::FromJSVal(ScriptInterface& scriptInterface, JS::HandleValue val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(entity_id_t, entity);
	return new CMessageDestroy(entity);
}

////////////////////////////////

JS::Value CMessageOwnershipChanged::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(entity);
	SET_MSG_PROPERTY(from);
	SET_MSG_PROPERTY(to);
	return JS::ObjectValue(*obj);
}

CMessage* CMessageOwnershipChanged::FromJSVal(ScriptInterface& scriptInterface, JS::HandleValue val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(entity_id_t, entity);
	GET_MSG_PROPERTY(player_id_t, from);
	GET_MSG_PROPERTY(player_id_t, to);
	return new CMessageOwnershipChanged(entity, from, to);
}

////////////////////////////////

JS::Value CMessagePositionChanged::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(entity);
	SET_MSG_PROPERTY(inWorld);
	SET_MSG_PROPERTY(x);
	SET_MSG_PROPERTY(z);
	SET_MSG_PROPERTY(a);
	return JS::ObjectValue(*obj);
}

CMessage* CMessagePositionChanged::FromJSVal(ScriptInterface& scriptInterface, JS::HandleValue val)
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

JS::Value CMessageInterpolatedPositionChanged::ToJSVal(ScriptInterface& UNUSED(scriptInterface)) const
{
	LOGWARNING(L"CMessageInterpolatedPositionChanged::ToJSVal not implemented");
	return JS::UndefinedValue();
}

CMessage* CMessageInterpolatedPositionChanged::FromJSVal(ScriptInterface& UNUSED(scriptInterface), JS::HandleValue UNUSED(val))
{
	LOGWARNING(L"CMessageInterpolatedPositionChanged::FromJSVal not implemented");
	return NULL;
}

////////////////////////////////

JS::Value CMessageTerritoryPositionChanged::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(entity);
	SET_MSG_PROPERTY(newTerritory);
	return JS::ObjectValue(*obj);
}

CMessage* CMessageTerritoryPositionChanged::FromJSVal(ScriptInterface& scriptInterface, JS::HandleValue val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(entity_id_t, entity);
	GET_MSG_PROPERTY(player_id_t, newTerritory);
	return new CMessageTerritoryPositionChanged(entity, newTerritory);
}

////////////////////////////////

JS::Value CMessageMotionChanged::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(starting);
	SET_MSG_PROPERTY(error);
	return JS::ObjectValue(*obj);
}

CMessage* CMessageMotionChanged::FromJSVal(ScriptInterface& scriptInterface, JS::HandleValue val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(bool, starting);
	GET_MSG_PROPERTY(bool, error);
	return new CMessageMotionChanged(starting, error);
}

////////////////////////////////

JS::Value CMessageTerrainChanged::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(i0);
	SET_MSG_PROPERTY(j0);
	SET_MSG_PROPERTY(i1);
	SET_MSG_PROPERTY(j1);
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageTerrainChanged::FromJSVal(ScriptInterface& scriptInterface, JS::HandleValue val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(int32_t, i0);
	GET_MSG_PROPERTY(int32_t, j0);
	GET_MSG_PROPERTY(int32_t, i1);
	GET_MSG_PROPERTY(int32_t, j1);
	return new CMessageTerrainChanged(i0, i1, j0, j1);
}

////////////////////////////////

JS::Value CMessageVisibilityChanged::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(player);
	SET_MSG_PROPERTY(ent);
	SET_MSG_PROPERTY(oldVisibility);
	SET_MSG_PROPERTY(newVisibility);
	return JS::ObjectValue(*obj);
}

CMessage* CMessageVisibilityChanged::FromJSVal(ScriptInterface& scriptInterface, JS::HandleValue val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(player_id_t, player);
	GET_MSG_PROPERTY(entity_id_t, ent);
	GET_MSG_PROPERTY(int, oldVisibility);
	GET_MSG_PROPERTY(int, newVisibility);
	return new CMessageVisibilityChanged(player, ent, oldVisibility, newVisibility);
}

////////////////////////////////

JS::Value CMessageWaterChanged::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageWaterChanged::FromJSVal(ScriptInterface& UNUSED(scriptInterface), JS::HandleValue UNUSED(val))
{
	return new CMessageWaterChanged();
}

////////////////////////////////

JS::Value CMessageObstructionMapShapeChanged::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	return JS::ObjectValue(*obj);
}

CMessage* CMessageObstructionMapShapeChanged::FromJSVal(ScriptInterface& UNUSED(scriptInterface), JS::HandleValue UNUSED(val))
{
	return new CMessageObstructionMapShapeChanged();
}

////////////////////////////////

JS::Value CMessageTerritoriesChanged::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	return JS::ObjectValue(*obj);
}

CMessage* CMessageTerritoriesChanged::FromJSVal(ScriptInterface& UNUSED(scriptInterface), JS::HandleValue UNUSED(val))
{
	return new CMessageTerritoriesChanged();
}

////////////////////////////////

JS::Value CMessageRangeUpdate::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(tag);
	SET_MSG_PROPERTY(added);
	SET_MSG_PROPERTY(removed);
	return JS::ObjectValue(*obj);
}

CMessage* CMessageRangeUpdate::FromJSVal(ScriptInterface& UNUSED(scriptInterface), JS::HandleValue UNUSED(val))
{
	LOGWARNING(L"CMessageRangeUpdate::FromJSVal not implemented");
	return NULL;
}

////////////////////////////////

JS::Value CMessagePathResult::ToJSVal(ScriptInterface& UNUSED(scriptInterface)) const
{
	LOGWARNING(L"CMessagePathResult::ToJSVal not implemented");
	return JS::UndefinedValue();
}

CMessage* CMessagePathResult::FromJSVal(ScriptInterface& UNUSED(scriptInterface), JS::HandleValue UNUSED(val))
{
	LOGWARNING(L"CMessagePathResult::FromJSVal not implemented");
	return NULL;
}

////////////////////////////////

JS::Value CMessageValueModification::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(entities);
	SET_MSG_PROPERTY(component);
	SET_MSG_PROPERTY(valueNames);
	return JS::ObjectValue(*obj);
}

CMessage* CMessageValueModification::FromJSVal(ScriptInterface& scriptInterface, JS::HandleValue val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(std::vector<entity_id_t>, entities);
	GET_MSG_PROPERTY(std::wstring, component);
	GET_MSG_PROPERTY(std::vector<std::wstring>, valueNames);
	return new CMessageValueModification(entities, component, valueNames);
}

////////////////////////////////

JS::Value CMessageTemplateModification::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(player);
	SET_MSG_PROPERTY(component);
	SET_MSG_PROPERTY(valueNames);
	return JS::ObjectValue(*obj);
}

CMessage* CMessageTemplateModification::FromJSVal(ScriptInterface& scriptInterface, JS::HandleValue val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(player_id_t, player);
	GET_MSG_PROPERTY(std::wstring, component);
	GET_MSG_PROPERTY(std::vector<std::wstring>, valueNames);
	return new CMessageTemplateModification(player, component, valueNames);
}

////////////////////////////////

JS::Value CMessageVisionRangeChanged::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(entity);
	SET_MSG_PROPERTY(oldRange);
	SET_MSG_PROPERTY(newRange);
	return JS::ObjectValue(*obj);
}

CMessage* CMessageVisionRangeChanged::FromJSVal(ScriptInterface& scriptInterface, JS::HandleValue val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(entity_id_t, entity);
	GET_MSG_PROPERTY(entity_pos_t, oldRange);
	GET_MSG_PROPERTY(entity_pos_t, newRange);
	return new CMessageVisionRangeChanged(entity, oldRange, newRange);
}

////////////////////////////////

JS::Value CMessageMinimapPing::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	return JS::ObjectValue(*obj);
}

CMessage* CMessageMinimapPing::FromJSVal(ScriptInterface& UNUSED(scriptInterface), JS::HandleValue UNUSED(val))
{
	return new CMessageMinimapPing();
}

////////////////////////////////////////////////////////////////

CMessage* CMessageFromJSVal(int mtid, ScriptInterface& scriptingInterface, JS::HandleValue val)
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
