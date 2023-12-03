/* Copyright (C) 2021 Wildfire Games.
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

#include "IComponent.h"

#include "simulation2/system/ComponentManager.h"

#include <string>

IComponent::~IComponent()
{
}

std::string IComponent::GetSchema()
{
	// No schema specified -> allow only empty elements
	return "<empty/>";
}

void IComponent::RegisterComponentType(CComponentManager& mgr, EInterfaceId iid, EComponentTypeId cid, AllocFunc alloc, DeallocFunc dealloc, const char* name, const std::string& schema)
{
	mgr.RegisterComponentType(iid, cid, alloc, dealloc, name, schema);
}

void IComponent::RegisterComponentTypeScriptWrapper(CComponentManager& mgr, EInterfaceId iid, EComponentTypeId cid, AllocFunc alloc, DeallocFunc dealloc, const char* name, const std::string& schema)
{
	mgr.RegisterComponentTypeScriptWrapper(iid, cid, alloc, dealloc, name, schema);
}

void IComponent::HandleMessage(const CMessage& UNUSED(msg), bool UNUSED(global))
{
}

bool IComponent::NewJSObject(const ScriptInterface& UNUSED(scriptInterface), JS::MutableHandleObject UNUSED(out)) const
{
	return false;
}

JS::Value IComponent::GetJSInstance() const
{
	return JS::NullValue();
}
