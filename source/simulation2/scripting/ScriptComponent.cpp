/* Copyright (C) 2015 Wildfire Games.
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

#include "ScriptComponent.h"

#include "simulation2/serialization/ISerializer.h"
#include "simulation2/serialization/IDeserializer.h"

CComponentTypeScript::CComponentTypeScript(ScriptInterface& scriptInterface, JS::HandleValue instance) :
	m_ScriptInterface(scriptInterface), m_Instance(scriptInterface.GetJSRuntime(), instance)
{
	// Cache the property detection for efficiency
	JSContext* cx = m_ScriptInterface.GetContext();
	JSAutoRequest rq(cx);

	m_HasCustomSerialize = m_ScriptInterface.HasProperty(m_Instance, "Serialize");
	m_HasCustomDeserialize = m_ScriptInterface.HasProperty(m_Instance, "Deserialize");

	m_HasNullSerialize = false;
	if (m_HasCustomSerialize)
	{
		JS::RootedValue val(cx);
		if (m_ScriptInterface.GetProperty(m_Instance, "Serialize", &val) && val.isNull())
			m_HasNullSerialize = true;
	}
}

void CComponentTypeScript::Init(const CParamNode& paramNode, entity_id_t ent)
{
	m_ScriptInterface.SetProperty(m_Instance, "entity", (int)ent, true, false);
	m_ScriptInterface.SetProperty(m_Instance, "template", paramNode, true, false);
	m_ScriptInterface.CallFunctionVoid(m_Instance, "Init");
}

void CComponentTypeScript::Deinit()
{
	m_ScriptInterface.CallFunctionVoid(m_Instance, "Deinit");
}

void CComponentTypeScript::HandleMessage(const CMessage& msg, bool global)
{
	JSContext* cx = m_ScriptInterface.GetContext();
	JSAutoRequest rq(cx);

	const char* name = global ? msg.GetScriptGlobalHandlerName() : msg.GetScriptHandlerName();

	JS::RootedValue msgVal(cx, msg.ToJSValCached(m_ScriptInterface));

	if (!m_ScriptInterface.CallFunctionVoid(m_Instance, name, msgVal))
		LOGERROR("Script message handler %s failed", name);
}

void CComponentTypeScript::Serialize(ISerializer& serialize)
{
	// If the component set Serialize = null, then do no work here
	if (m_HasNullSerialize)
		return;

	JSContext* cx = m_ScriptInterface.GetContext();
	JSAutoRequest rq(cx);

	// Support a custom "Serialize" function, which returns a new object that will be
	// serialized instead of the component itself
	if (m_HasCustomSerialize)
	{
		JS::RootedValue val(cx);
		if (!m_ScriptInterface.CallFunction(m_Instance, "Serialize", &val))
			LOGERROR("Script Serialize call failed");
		serialize.ScriptVal("object", &val);
	}
	else
	{
		serialize.ScriptVal("object", &m_Instance);
	}
}

void CComponentTypeScript::Deserialize(const CParamNode& paramNode, IDeserializer& deserialize, entity_id_t ent)
{
	JSContext* cx = m_ScriptInterface.GetContext();
	JSAutoRequest rq(cx);

	m_ScriptInterface.SetProperty(m_Instance, "entity", (int)ent, true, false);
	m_ScriptInterface.SetProperty(m_Instance, "template", paramNode, true, false);

	// Support a custom "Deserialize" function, to which we pass the deserialized data
	// instead of automatically adding the deserialized properties onto the object
	if (m_HasCustomDeserialize)
	{
		JS::RootedValue val(cx);

		// If Serialize = null, we'll still call Deserialize but with undefined argument
		if (!m_HasNullSerialize)
			deserialize.ScriptVal("object", &val);

		if (!m_ScriptInterface.CallFunctionVoid(m_Instance, "Deserialize", val))
			LOGERROR("Script Deserialize call failed");
	}
	else
	{
		if (!m_HasNullSerialize)
		{
			// Use ScriptObjectAppend so we don't lose the carefully-constructed
			// prototype/parent of this object
			deserialize.ScriptObjectAppend("object", m_Instance);
		}
	}
}
