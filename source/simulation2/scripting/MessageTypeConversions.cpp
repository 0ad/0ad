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
	ScriptInterface::LocalRootScope scope(scriptInterface.GetContext()); \
	if (! scope.OK()) \
		return JSVAL_VOID; \
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

jsval CMessageUpdate::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(turnLength);
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageUpdate::FromJSVal(ScriptInterface& scriptInterface, jsval val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(CFixed_23_8, turnLength);
	return new CMessageUpdate(turnLength);
}

////////////////////////////////

jsval CMessageInterpolate::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(offset);
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageInterpolate::FromJSVal(ScriptInterface& scriptInterface, jsval val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(float, offset);
	return new CMessageInterpolate(offset);
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

////////////////////////////////////////////////////////////////

CMessage* CMessageFromJSVal(int mtid, ScriptInterface& scriptingInterface, jsval val)
{
	switch (mtid)
	{
#define MESSAGE(name) case MT_##name: return CMessage##name::FromJSVal(scriptingInterface, val);
#define INTERFACE(name)
#define COMPONENT(name)
#include "simulation2/TypeList.h"
	}

	return NULL;
}
