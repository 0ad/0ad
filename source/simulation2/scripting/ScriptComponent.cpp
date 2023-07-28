/* Copyright (C) 2023 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "ScriptComponent.h"

#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/JSON.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/Object.h"
#include "simulation2/serialization/ISerializer.h"
#include "simulation2/serialization/IDeserializer.h"

CComponentTypeScript::CComponentTypeScript(const ScriptInterface& scriptInterface, JS::HandleValue instance) :
	m_ScriptInterface(scriptInterface)
{
	m_Instance.init(ScriptRequest(m_ScriptInterface).cx, instance);
}

void CComponentTypeScript::Init(const CParamNode& paramNode, entity_id_t ent)
{
	ScriptRequest rq(m_ScriptInterface);
	Script::SetProperty(rq, m_Instance, "entity", (int)ent, true, false);
	Script::SetProperty(rq, m_Instance, "template", paramNode, true, false);
	ScriptFunction::CallVoid(rq, m_Instance, "Init");
}

void CComponentTypeScript::Deinit()
{
	ScriptRequest rq(m_ScriptInterface);
	ScriptFunction::CallVoid(rq, m_Instance, "Deinit");
}

void CComponentTypeScript::HandleMessage(const CMessage& msg, bool global)
{
	ScriptRequest rq(m_ScriptInterface);

	const char* name = global ? msg.GetScriptGlobalHandlerName() : msg.GetScriptHandlerName();

	JS::RootedValue msgVal(rq.cx, msg.ToJSValCached(rq));

	if (!ScriptFunction::CallVoid(rq, m_Instance, name, msgVal))
		LOGERROR("Script message handler %s failed", name);
}

void CComponentTypeScript::Serialize(ISerializer& serialize)
{
	ScriptRequest rq(m_ScriptInterface);

	try
	{
		serialize.ScriptVal("comp", &m_Instance);
	}
	catch(PSERROR_Serialize& err)
	{
		int ent = INVALID_ENTITY;
		Script::GetProperty(rq, m_Instance, "entity", ent);
		std::string name = "(error)";
		Script::GetObjectClassName(rq, m_Instance, name);
		LOGERROR("Script component %s of entity %i failed to serialize: %s\nSerializing:\n%s", name, ent, err.what(), Script::ToString(rq, &m_Instance));
		// Rethrow now that we added more details
		throw;
	}
}

void CComponentTypeScript::Deserialize(const CParamNode& paramNode, IDeserializer& deserialize, entity_id_t ent)
{
	ScriptRequest rq(m_ScriptInterface);

	Script::SetProperty(rq, m_Instance, "entity", (int)ent, true, false);
	Script::SetProperty(rq, m_Instance, "template", paramNode, true, false);

	deserialize.ScriptObjectAssign("comp", m_Instance);
}
