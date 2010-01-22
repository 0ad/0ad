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

#include "ScriptComponent.h"

#include "simulation2/serialization/ISerializer.h"
#include "simulation2/serialization/IDeserializer.h"

CComponentTypeScript::CComponentTypeScript(ScriptInterface& scriptInterface, jsval instance) :
	m_ScriptInterface(scriptInterface), m_Instance(instance)
{
	debug_assert(instance);
	m_ScriptInterface.AddRoot(&m_Instance, "CComponentTypeScript.m_Instance");
}

CComponentTypeScript::~CComponentTypeScript()
{
	m_ScriptInterface.RemoveRoot(&m_Instance);
}

void CComponentTypeScript::Init(const CSimContext& UNUSED(context), const CParamNode& paramNode, entity_id_t ent)
{
	m_ScriptInterface.SetProperty(m_Instance, "entity", (int)ent, true);
	m_ScriptInterface.SetProperty(m_Instance, "template", paramNode, true);
	m_ScriptInterface.CallFunctionVoid(m_Instance, "Init");
}

void CComponentTypeScript::Deinit(const CSimContext& UNUSED(context))
{
	m_ScriptInterface.CallFunctionVoid(m_Instance, "Deinit");
}

void CComponentTypeScript::HandleMessage(const CSimContext& UNUSED(context), const CMessage& msg, bool global)
{
	const char* name = global ? msg.GetScriptGlobalHandlerName() : msg.GetScriptHandlerName();

	CScriptVal msgVal = msg.ToJSVal(m_ScriptInterface);
	// TODO: repeated conversions are exceedingly inefficient. Should
	// cache this once per message (if it's used by >= 1 scripted component)

	if (!m_ScriptInterface.CallFunctionVoid(m_Instance, name, msgVal))
		LOGERROR(L"Script message handler %hs failed", name);
}

void CComponentTypeScript::Serialize(ISerializer& serialize)
{
	serialize.ScriptVal("object", m_Instance);

	// TODO: it should be possible for scripts to provide their own (de)serialization
	// functions, for efficiency. (e.g. "function Serialize() { return a JS object containing
	// just the important state for this component }")
	// Or alternatively perhaps just have a way to mark certain fields as transient,
	// and a post-deserialize callback to reinitialise them.
}

void CComponentTypeScript::Deserialize(const CSimContext& UNUSED(context), const CParamNode& paramNode, IDeserializer& deserialize, entity_id_t ent)
{
	// Use ScriptObjectAppend so we don't lose the carefully-constructed
	// prototype/parent of this object
	deserialize.ScriptObjectAppend(m_Instance);

	m_ScriptInterface.SetProperty(m_Instance, "entity", (int)ent, true);
	m_ScriptInterface.SetProperty(m_Instance, "template", paramNode, true);
}
